static inline unsigned int usToCm(unsigned long echoUs) {
  return (unsigned int)((echoUs + 29UL) / 58UL);
}

static inline bool snapshotValid(const MeasurementSnapshot &snapshot) {
  return (snapshot.flags & MEASUREMENT_FLAG_VALID) != 0;
}

static inline bool snapshotFilling(const MeasurementSnapshot &snapshot) {
  return (snapshot.flags & MEASUREMENT_FLAG_FILLING) != 0;
}

static inline bool snapshotWaterOutputOn(const MeasurementSnapshot &snapshot) {
  return (snapshot.flags & MEASUREMENT_FLAG_WATER_OUTPUT_ON) != 0;
}

static inline void setSnapshotValid(MeasurementSnapshot &snapshot, bool enabled) {
  if (enabled) {
    snapshot.flags |= MEASUREMENT_FLAG_VALID;
  } else {
    snapshot.flags &= (uint8_t)(~MEASUREMENT_FLAG_VALID);
  }
}

static inline void setSnapshotFilling(MeasurementSnapshot &snapshot, bool enabled) {
  if (enabled) {
    snapshot.flags |= MEASUREMENT_FLAG_FILLING;
  } else {
    snapshot.flags &= (uint8_t)(~MEASUREMENT_FLAG_FILLING);
  }
}

static inline void setSnapshotWaterOutputOn(MeasurementSnapshot &snapshot, bool enabled) {
  if (enabled) {
    snapshot.flags |= MEASUREMENT_FLAG_WATER_OUTPUT_ON;
  } else {
    snapshot.flags &= (uint8_t)(~MEASUREMENT_FLAG_WATER_OUTPUT_ON);
  }
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

static unsigned long readEchoUsOnce() {
  digitalWrite(activeTrigPin, LOW);
  delayMicroseconds(5);
  digitalWrite(activeTrigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(activeTrigPin, LOW);
  return pulseIn(activeEchoPin, HIGH, config.pulseTimeoutUs);
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
      yield();
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
        (((candidateUs >= pendingShortUs) ? (candidateUs - pendingShortUs) : (pendingShortUs - candidateUs)) <= config.shortConfirmDeltaUs)) {
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

  if (!snapshotValid(snapshot)) {
    snapshot.state = MEASUREMENT_STATE_ERROR;
  } else if (snapshot.filteredUs < config.waterHighUs) {
    snapshot.state = MEASUREMENT_STATE_HIGH;
  } else if (snapshot.filteredUs > config.waterLowUs) {
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

void applyMeasurementControl(MeasurementSnapshot &snapshot) {
  if (!snapshotValid(snapshot)) {
    if (config.debugControlLogs) {
      Serial.println(F("water reading invalid, STOP water"));
    }

    setWaterOutput(false);
    setErrorIndicator(true);
    filling = false;
    count = 0;
    setSnapshotFilling(snapshot, false);
    setSnapshotWaterOutputOn(snapshot, false);
    snapshot.state = MEASUREMENT_STATE_ERROR;
    return;
  }

  setErrorIndicator(false);

  if (snapshot.filteredUs < config.waterHighUs) {
    if (config.debugControlLogs) {
      Serial.println(F("water HIGH, STOP water"));
    }

    setWaterOutput(false);
    filling = false;
    count = 0;
  } else if (snapshot.filteredUs > config.waterLowUs) {
    if (filling) {
      ++count;
      if (count > (int)config.waterMaxDuration) {
        if (config.debugControlLogs) {
          Serial.println(F("water LOW too long, STOP water"));
        }

        setWaterOutput(false);
        filling = false;
        count = 0;
        setSnapshotValid(snapshot, false);
        snapshot.state = MEASUREMENT_STATE_ERROR;
        setErrorIndicator(true);
        setSnapshotFilling(snapshot, false);
        setSnapshotWaterOutputOn(snapshot, false);
        return;
      }
    } else {
      count = 0;
    }

    if (config.debugControlLogs) {
      Serial.println(F("water LOW, START water"));
    }

    setWaterOutput(true);
    filling = true;
  } else {
    if (config.debugControlLogs) {
      Serial.println(F("water NORMAL, keep current state"));
    }

    if (!filling) {
      setWaterOutput(false);
    }
  }

  setSnapshotFilling(snapshot, filling);
  setSnapshotWaterOutputOn(snapshot, filling);
}

void pushHistory(const MeasurementSnapshot &snapshot) {
  HistorySample sample = {
    snapshot.rawUs,
    snapshot.acceptedUs,
    snapshot.filteredUs,
    snapshot.sampleMs
  };

  if (measurementHistoryCount < HISTORY_CAPACITY) {
    measurementHistory[measurementHistoryCount++] = sample;
    return;
  }

  measurementHistory[measurementHistoryHead] = sample;
  measurementHistoryHead = (measurementHistoryHead + 1) % HISTORY_CAPACITY;
}

void writeMeasurementJsonObject(JsonOutput &output, const MeasurementSnapshot &snapshot) {
  bool first = true;
  const char *stateName = "ERROR";

  switch (snapshot.state) {
    case MEASUREMENT_STATE_HIGH:
      stateName = "HIGH";
      break;
    case MEASUREMENT_STATE_NORMAL:
      stateName = "NORMAL";
      break;
    case MEASUREMENT_STATE_LOW:
      stateName = "LOW";
      break;
    case MEASUREMENT_STATE_ERROR:
    default:
      break;
  }

  jsonWrite(output, "{");
  writeJsonULongField(output, first, "rawUs", snapshot.rawUs);
  writeJsonULongField(output, first, "acceptedUs", snapshot.acceptedUs);
  writeJsonULongField(output, first, "filteredUs", snapshot.filteredUs);
  writeJsonULongField(output, first, "rawCm", snapshot.rawCm);
  writeJsonULongField(output, first, "acceptedCm", snapshot.acceptedCm);
  writeJsonULongField(output, first, "filteredCm", snapshot.filteredCm);
  writeJsonBoolField(output, first, "valid", snapshotValid(snapshot));
  writeJsonStringField(output, first, "state", stateName);
  writeJsonBoolField(output, first, "filling", snapshotFilling(snapshot));
  writeJsonBoolField(output, first, "waterOutputOn", snapshotWaterOutputOn(snapshot));
  writeJsonULongField(output, first, "sampleMs", snapshot.sampleMs);
  jsonWrite(output, "}");
}
