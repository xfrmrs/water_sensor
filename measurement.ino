#include "src/common.h"

static inline unsigned int usToCm(unsigned long echoUs) {
  return (unsigned int)((echoUs + 29UL) / 58UL);
}

static inline unsigned long absDiffUs(unsigned long a, unsigned long b) {
  return (a >= b) ? (a - b) : (b - a);
}

static inline bool snapshotHasFlag(const MeasurementSnapshot &snapshot, uint8_t flag) {
  return (snapshot.flags & flag) != 0;
}

static inline void setSnapshotFlag(MeasurementSnapshot &snapshot, uint8_t flag, bool enabled) {
  if (enabled) {
    snapshot.flags |= flag;
  } else {
    snapshot.flags &= (uint8_t)(~flag);
  }
}

static inline bool snapshotValid(const MeasurementSnapshot &snapshot) {
  return snapshotHasFlag(snapshot, MEASUREMENT_FLAG_VALID);
}

static inline bool snapshotFilling(const MeasurementSnapshot &snapshot) {
  return snapshotHasFlag(snapshot, MEASUREMENT_FLAG_FILLING);
}

static inline bool snapshotWaterOutputOn(const MeasurementSnapshot &snapshot) {
  return snapshotHasFlag(snapshot, MEASUREMENT_FLAG_WATER_OUTPUT_ON);
}

static inline bool snapshotEmergencyStopActive(const MeasurementSnapshot &snapshot) {
  return snapshotHasFlag(snapshot, MEASUREMENT_FLAG_EMERGENCY_STOP);
}

static inline void setSnapshotValid(MeasurementSnapshot &snapshot, bool enabled) {
  setSnapshotFlag(snapshot, MEASUREMENT_FLAG_VALID, enabled);
}

static inline void setSnapshotFilling(MeasurementSnapshot &snapshot, bool enabled) {
  setSnapshotFlag(snapshot, MEASUREMENT_FLAG_FILLING, enabled);
}

static inline void setSnapshotWaterOutputOn(MeasurementSnapshot &snapshot, bool enabled) {
  setSnapshotFlag(snapshot, MEASUREMENT_FLAG_WATER_OUTPUT_ON, enabled);
}

static inline void setSnapshotEmergencyStop(MeasurementSnapshot &snapshot, bool enabled) {
  setSnapshotFlag(snapshot, MEASUREMENT_FLAG_EMERGENCY_STOP, enabled);
}

static inline HistorySample makeHistorySample(const MeasurementSnapshot &snapshot) {
  HistorySample sample = {
    snapshot.rawUs,
    snapshot.acceptedUs,
    snapshot.filteredUs,
    snapshot.sampleMs
  };
  return sample;
}

void resetMeasurementState() {
  lastAcceptedUs = 0;
  pendingShortUs = 0;
  pendingShortCount = 0;
  invalidBurstCount = 0;
  count = 0;
  filling = false;
}

void rebuildKalmanFilter() {
  if (kalmanFilter != nullptr) {
    delete kalmanFilter;
    kalmanFilter = nullptr;
  }

  kalmanFilter = new SimpleKalmanFilter(
    config.kalmanMeasurementError,
    config.kalmanEstimateError,
    config.kalmanProcessNoise
  );
}

void applyActiveRuntimeSettings(const Config &source) {
  activeTrigPin = source.trigPin;
  activeEchoPin = source.echoPin;
  activeWaterPin = source.waterPin;
  activeErrLedPin = source.errLedPin;
  activeSerialBaud = source.serialBaud;
}

void initializePins() {
  pinMode(activeWaterPin, OUTPUT);
  pinMode(activeErrLedPin, OUTPUT);
  pinMode(activeTrigPin, OUTPUT);
  pinMode(activeEchoPin, INPUT);

  setWaterOutput(false);
  setErrorIndicator(false);
  digitalWrite(activeTrigPin, LOW);
}

void setWaterOutput(bool enabled) {
  digitalWrite(activeWaterPin, enabled ? LOW : HIGH);
}

void setErrorIndicator(bool error) {
  digitalWrite(activeErrLedPin, error ? LOW : HIGH);
}

void activateEmergencyStop() {
  emergencyStopActive = true;
  filling = false;
  count = 0;
  setWaterOutput(false);
  setErrorIndicator(true);
  latestMeasurement.flags &= (uint8_t)(~MEASUREMENT_FLAG_FILLING);
  latestMeasurement.flags &= (uint8_t)(~MEASUREMENT_FLAG_WATER_OUTPUT_ON);
  latestMeasurement.flags |= MEASUREMENT_FLAG_EMERGENCY_STOP;
}

const char *measurementStateName(MeasurementState state) {
  switch (state) {
    case MEASUREMENT_STATE_HIGH:
      return "HIGH";
    case MEASUREMENT_STATE_NORMAL:
      return "NORMAL";
    case MEASUREMENT_STATE_LOW:
      return "LOW";
    case MEASUREMENT_STATE_ERROR:
    default:
      return "ERROR";
  }
}

static unsigned long readEchoUsOnce() {
  digitalWrite(activeTrigPin, LOW);
  delayMicroseconds(5);
  digitalWrite(activeTrigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(activeTrigPin, LOW);
  return pulseIn(activeEchoPin, HIGH, config.pulseTimeoutUs);
}

static unsigned long readMedianEchoUs() {
  unsigned long samples[MAX_PING_BUFFER_CAPACITY];
  uint8_t validCount = 0;

  for (uint8_t i = 0; i < config.nPings; ++i) {
    unsigned long echoUs = readEchoUsOnce();

    if ((echoUs >= config.minValidEchoUs) && (echoUs <= config.waterErrUs)) {
      int8_t j = (int8_t)validCount - 1;
      while ((j >= 0) && (samples[j] > echoUs)) {
        samples[j + 1] = samples[j];
        --j;
      }
      samples[j + 1] = echoUs;
      validCount++;
    }

    if (i + 1 < config.nPings) {
      delay(config.pingGapMs);
      yield();
    }
  }

  if (validCount < config.minValidPings) {
    return 0;
  }

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

MeasurementSnapshot measureWaterLevel() {
  MeasurementSnapshot snapshot = {0, 0, 0, millis(), 0, 0, 0, MEASUREMENT_STATE_ERROR, 0};
  unsigned long measuredUs = readMedianEchoUs();
  unsigned long acceptedUs = deglitchShortEchoUs(measuredUs);
  unsigned long filteredUs = acceptedUs;

  if (acceptedUs < config.waterErrUs && kalmanFilter != nullptr) {
    filteredUs = (unsigned long)(kalmanFilter->updateEstimate((float)acceptedUs) + 0.5f);
  }

  snapshot.rawUs = measuredUs;
  snapshot.acceptedUs = acceptedUs;
  snapshot.filteredUs = filteredUs;
  snapshot.rawCm = usToCm(measuredUs);
  snapshot.acceptedCm = usToCm(acceptedUs);
  snapshot.filteredCm = usToCm(filteredUs);
  setSnapshotValid(snapshot, acceptedUs < config.waterErrUs);
  setSnapshotFilling(snapshot, filling);
  setSnapshotWaterOutputOn(snapshot, false);
  setSnapshotEmergencyStop(snapshot, emergencyStopActive);

  if (!snapshotValid(snapshot)) {
    snapshot.state = MEASUREMENT_STATE_ERROR;
  } else if (WaterHigh(snapshot.filteredUs)) {
    snapshot.state = MEASUREMENT_STATE_HIGH;
  } else if (WaterLow(snapshot.filteredUs)) {
    snapshot.state = MEASUREMENT_STATE_LOW;
  } else {
    snapshot.state = MEASUREMENT_STATE_NORMAL;
  }

  if (config.debugMeasurementLogs) {
    Serial.print(F("Measured: "));
    Serial.print(snapshot.rawUs);
    Serial.print(F(" us ("));
    Serial.print(snapshot.rawCm);
    Serial.print(F(" cm), Accepted: "));
    Serial.print(snapshot.acceptedUs);
    Serial.print(F(" us ("));
    Serial.print(snapshot.acceptedCm);
    Serial.print(F(" cm), Filtered: "));
    Serial.print(snapshot.filteredUs);
    Serial.print(F(" us ("));
    Serial.print(snapshot.filteredCm);
    Serial.print(F(" cm), Valid: "));
    Serial.println(snapshotValid(snapshot) ? F("yes") : F("no"));
  }

  return snapshot;
}

static void handleMeasurementError(MeasurementSnapshot &snapshot) {
  setWaterOutput(false);
  setErrorIndicator(true);
  filling = false;
  count = 0;
  setSnapshotValid(snapshot, false);
  setSnapshotFilling(snapshot, false);
  setSnapshotWaterOutputOn(snapshot, false);
  snapshot.state = MEASUREMENT_STATE_ERROR;
}

static bool checkWaterTimeout(MeasurementSnapshot &snapshot) {
  if (filling) {
    ++count;
    if (count > (int)config.waterMaxDuration) {
      if (config.debugControlLogs) {
        Serial.println(F("water LOW too long, STOP water"));
      }
      handleMeasurementError(snapshot);
      return true;
    }
  } else {
    count = 0;
  }
  return false;
}

void applyMeasurementControl(MeasurementSnapshot &snapshot) {
  if (emergencyStopActive) {
    setWaterOutput(false);
    setErrorIndicator(true);
    filling = false;
    count = 0;
    setSnapshotFilling(snapshot, false);
    setSnapshotWaterOutputOn(snapshot, false);
    setSnapshotEmergencyStop(snapshot, true);
    return;
  }

  if (!snapshotValid(snapshot)) {
    if (config.debugControlLogs) {
      Serial.println(F("water reading invalid, STOP water"));
    }
    handleMeasurementError(snapshot);
    return;
  }

  setErrorIndicator(false);

  if (WaterHigh(snapshot.filteredUs)) {
    if (config.debugControlLogs) {
      Serial.println(F("water HIGH, STOP water"));
    }
    setWaterOutput(false);
    filling = false;
    count = 0;
    setSnapshotFilling(snapshot, filling);
    setSnapshotWaterOutputOn(snapshot, filling);
    return;
  }

  if (WaterLow(snapshot.filteredUs)) {
    if (checkWaterTimeout(snapshot)) {
      return;
    }

    if (config.debugControlLogs) {
      Serial.println(F("water LOW, START water"));
    }
    setWaterOutput(true);
    filling = true;
    setSnapshotFilling(snapshot, filling);
    setSnapshotWaterOutputOn(snapshot, filling);
    return;
  }

  if (config.debugControlLogs) {
    Serial.println(F("water NORMAL, keep current state"));
  }
  if (!filling) {
    setWaterOutput(false);
  }

  setSnapshotFilling(snapshot, filling);
  setSnapshotWaterOutputOn(snapshot, filling);
}

bool WaterLow(unsigned long waterLevelUs) {
  return waterLevelUs > config.waterLowUs;
}

bool WaterHigh(unsigned long waterLevelUs) {
  return waterLevelUs < config.waterHighUs;
}

void pushHistory(const MeasurementSnapshot &snapshot) {
  HistorySample sample = makeHistorySample(snapshot);
  unsigned long capacity = config.historyCapacity > 0 ? min(config.historyCapacity, (unsigned long)MAX_HISTORY_BUFFER_CAPACITY) : (unsigned long)MAX_HISTORY_BUFFER_CAPACITY;
  uint8_t bufferCapacity = (uint8_t)capacity;

  if (measurementHistoryCount < bufferCapacity) {
    measurementHistory[measurementHistoryCount++] = sample;
    return;
  }

  measurementHistory[measurementHistoryHead] = sample;
  measurementHistoryHead = (measurementHistoryHead + 1) % bufferCapacity;
}

void writeMeasurementJsonObject(JsonOutput &output, const MeasurementSnapshot &snapshot) {
  bool first = true;
  jsonWrite(output, "{");
  writeJsonULongField(output, first, "rawUs", snapshot.rawUs);
  writeJsonULongField(output, first, "acceptedUs", snapshot.acceptedUs);
  writeJsonULongField(output, first, "filteredUs", snapshot.filteredUs);
  writeJsonUIntField(output, first, "rawCm", snapshot.rawCm);
  writeJsonUIntField(output, first, "acceptedCm", snapshot.acceptedCm);
  writeJsonUIntField(output, first, "filteredCm", snapshot.filteredCm);
  writeJsonBoolField(output, first, "valid", snapshotValid(snapshot));
  writeJsonStringField(output, first, "state", measurementStateName(snapshot.state));
  writeJsonBoolField(output, first, "filling", snapshotFilling(snapshot));
  writeJsonBoolField(output, first, "waterOutputOn", snapshotWaterOutputOn(snapshot));
  writeJsonBoolField(output, first, "emergencyStopActive", snapshotEmergencyStopActive(snapshot));
  writeJsonULongField(output, first, "sampleMs", snapshot.sampleMs);
  jsonWrite(output, "}");
}

String buildTelemetryMessage(const MeasurementSnapshot &snapshot) {
  String json;
  json.reserve(320 + 48);
  json += F("{\"type\":\"telemetry\",\"data\":");
  JsonOutput output = makeStringJsonOutput(json);
  writeMeasurementJsonObject(output, snapshot);
  json += '}';
  return json;
}

void writeHistoryPointJsonObject(JsonOutput &output, const HistorySample &snapshot) {
  bool first = true;
  jsonWrite(output, "{");
  writeJsonULongField(output, first, "rawUs", snapshot.rawUs);
  writeJsonULongField(output, first, "acceptedUs", snapshot.acceptedUs);
  writeJsonULongField(output, first, "filteredUs", snapshot.filteredUs);
  writeJsonULongField(output, first, "sampleMs", snapshot.sampleMs);
  jsonWrite(output, "}");
}
