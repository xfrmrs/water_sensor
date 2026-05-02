#include <Arduino.h>
#include <SimpleKalmanFilter.h>

// Error condition constants
#define WATER_MAX_DURATION 90
#define DEBUG false
#define DEBUG_M true

//pin assignments 
#define TRIG 4    // yellow
#define ECHO 5    // green
#define WATER 13  // white w/black stripe
#define ERRLED 16 // (on-board)

static const uint8_t MAX_CONFIGURABLE_PINGS = 12;

struct Config {
  unsigned long waterMaxDuration;
  unsigned long loopDelayMs;
  unsigned long waterDelayMs;
  unsigned long waterLowUs;
  unsigned long waterHighUs;
  unsigned long waterErrUs;
  unsigned long pulseTimeoutUs;
  uint8_t nPings;
  uint8_t minValidPings;
  unsigned long pingGapMs;
  unsigned long minValidEchoUs;
  unsigned long shortJumpUs;
  unsigned long shortConfirmDeltaUs;
  uint8_t shortConfirmCount;
  uint8_t maxHeldInvalidBursts;
};

const Config DEFAULT_CONFIG = {
  90,
  1000,
  1500,
  1200UL,
  500UL,
  2500UL,
  5000UL,
  5,
  3,
  60,
  232UL,
  250UL,
  120UL,
  2,
  3
};

// Global variables
Config config = DEFAULT_CONFIG;

int count = 0;
bool filling = false;

unsigned long lastAcceptedUs = 0;
unsigned long pendingShortUs = 0;
uint8_t pendingShortCount = 0;
uint8_t invalidBurstCount = 0;

/*
 SimpleKalmanFilter(e_mea, e_est, q);
 e_mea: Measurement Uncertainty
 e_est: Estimation Uncertainty
 q: Process Noise
*/
SimpleKalmanFilter simpleKalmanFilter(5, 2, 0.01);

// Function declarations
unsigned long WaterLevel();
bool WaterLow(unsigned long waterLevelUs);
bool WaterHigh(unsigned long waterLevelUs);
void resetMeasurementState();
bool validateConfig(const Config &candidate);

void resetMeasurementState() {
  lastAcceptedUs = 0;
  pendingShortUs = 0;
  pendingShortCount = 0;
  invalidBurstCount = 0;
}

bool validateConfig(const Config &candidate) {
  if (candidate.waterMaxDuration == 0) return false;
  if (candidate.loopDelayMs == 0) return false;
  if (candidate.waterDelayMs == 0) return false;
  if (candidate.waterHighUs >= candidate.waterLowUs) return false;
  if (candidate.waterLowUs >= candidate.waterErrUs) return false;
  if (candidate.pulseTimeoutUs < candidate.waterErrUs) return false;
  if (candidate.nPings == 0 || candidate.nPings > MAX_CONFIGURABLE_PINGS) return false;
  if (candidate.minValidPings == 0 || candidate.minValidPings > candidate.nPings) return false;
  if (candidate.pingGapMs == 0) return false;
  if (candidate.minValidEchoUs == 0) return false;
  if (candidate.shortConfirmCount == 0) return false;
  if (candidate.maxHeldInvalidBursts == 0) return false;
  return true;
}

static inline unsigned int usToCm(unsigned long echoUs) {
  return (unsigned int)((echoUs + 29UL) / 58UL);
}

static inline unsigned long absDiffUs(unsigned long a, unsigned long b) {
  return (a >= b) ? (a - b) : (b - a);
}

static unsigned long readEchoUsOnce() {
  // Give a short LOW pulse beforehand to ensure a clean HIGH pulse.
  digitalWrite(TRIG, LOW);
  delayMicroseconds(5);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  return pulseIn(ECHO, HIGH, config.pulseTimeoutUs);
}

static void sortEchoSamples(unsigned long *samples, uint8_t countSamples) {
  for (uint8_t i = 1; i < countSamples; ++i) {
    unsigned long key = samples[i];
    int8_t j = (int8_t)i - 1;
    while ((j >= 0) && (samples[j] > key)) {
      samples[j + 1] = samples[j];
      --j;
    }
    samples[j + 1] = key;
  }
}

static unsigned long readMedianEchoUs() {
  unsigned long samples[MAX_CONFIGURABLE_PINGS];
  uint8_t validCount = 0;

  for (uint8_t i = 0; i < config.nPings; ++i) {
    unsigned long echoUs = readEchoUsOnce();

    if ((echoUs >= config.minValidEchoUs) && (echoUs <= config.waterErrUs)) {
      samples[validCount++] = echoUs;
    }

    if (i + 1 < config.nPings) {
      delay(config.pingGapMs);
    }
  }

  if (validCount < config.minValidPings) {
    return 0;
  }

  sortEchoSamples(samples, validCount);
  return samples[validCount / 2];
}

static unsigned long deglitchShortEchoUs(unsigned long candidateUs) {
  if (candidateUs == 0) {
    pendingShortCount = 0;
    ++invalidBurstCount;

    if ((lastAcceptedUs > 0) && (invalidBurstCount <= config.maxHeldInvalidBursts)) {
      return lastAcceptedUs;
    }

    return config.waterErrUs;
  }

  invalidBurstCount = 0;

  if (lastAcceptedUs == 0) {
    lastAcceptedUs = candidateUs;
    pendingShortCount = 0;
    return candidateUs;
  }

  if ((candidateUs + config.shortJumpUs) < lastAcceptedUs) {
    if ((pendingShortCount > 0) &&
        (absDiffUs(candidateUs, pendingShortUs) <= config.shortConfirmDeltaUs)) {
      ++pendingShortCount;
    } else {
      pendingShortUs = candidateUs;
      pendingShortCount = 1;
    }

    if (pendingShortCount < config.shortConfirmCount) {
      return lastAcceptedUs;
    }
  }

  pendingShortCount = 0;
  lastAcceptedUs = candidateUs;
  return candidateUs;
}

void setup() {
  if (!validateConfig(config)) {
    config = DEFAULT_CONFIG;
  }

  resetMeasurementState();

  pinMode(WATER, OUTPUT);
  pinMode(ERRLED, OUTPUT);
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  // Water OFF by default
  digitalWrite(WATER, HIGH);
  digitalWrite(ERRLED, HIGH);
  digitalWrite(TRIG, LOW);

  // Initialize serial communication
  Serial.begin(74800);
}

// The loop routine runs over and over again forever:
void loop() {
  delay(filling ? config.waterDelayMs : config.loopDelayMs);

  unsigned long waterLevelUs = WaterLevel();

  if (waterLevelUs >= config.waterErrUs) {
    if (DEBUG) Serial.println("water reading invalid, STOP water");
    digitalWrite(WATER, HIGH);
    digitalWrite(ERRLED, LOW);
    filling = false;
    count = 0;
    return;
  }

  digitalWrite(ERRLED, HIGH);

  if (WaterHigh(waterLevelUs)) {
    if (DEBUG) Serial.println("water level is HIGH, STOP water...");
    digitalWrite(WATER, HIGH);
    filling = false;
    count = 0;
    return;
  }

  if (WaterLow(waterLevelUs) || filling) {
    filling = true;
    if (DEBUG) Serial.println("water low, FILLING water");
    digitalWrite(WATER, LOW);

    // Stop watering if watering for too long.
    if (count++ > (int)config.waterMaxDuration) {
      digitalWrite(WATER, HIGH);
      digitalWrite(ERRLED, LOW);
      filling = false;
      count = 0;
      if (DEBUG) Serial.print("water has been ON for too long, ");
      if (DEBUG) Serial.println("emergency STOP, water off");
    }

    return;
  }

  if (DEBUG) Serial.println("water level is NORMAL, STOP water...");
  digitalWrite(WATER, HIGH);
  filling = false;
  count = 0;
}

unsigned long WaterLevel() {
  unsigned long measuredUs = readMedianEchoUs();
  unsigned long acceptedUs = deglitchShortEchoUs(measuredUs);
  unsigned long estimateUs = acceptedUs;

  if (acceptedUs < config.waterErrUs) {
    estimateUs = (unsigned long)(simpleKalmanFilter.updateEstimate((float)acceptedUs) + 0.5f);
  }

  if (DEBUG_M) {
    Serial.print("Measured: ");
    Serial.print(measuredUs);
    Serial.print(" us (");
    Serial.print(usToCm(measuredUs));
    Serial.print(" cm), Accepted: ");
    Serial.print(acceptedUs);
    Serial.print(" us (");
    Serial.print(usToCm(acceptedUs));
    Serial.print(" cm), Estimate: ");
    Serial.print(estimateUs);
    Serial.print(" us (");
    Serial.print(usToCm(estimateUs));
    Serial.println(" cm)");
  }

  return acceptedUs;
}

bool WaterLow(unsigned long waterLevelUs) {
  return (waterLevelUs >= config.waterLowUs) && (waterLevelUs < config.waterErrUs);
}

bool WaterHigh(unsigned long waterLevelUs) {
  return (waterLevelUs <= config.waterHighUs);
}
