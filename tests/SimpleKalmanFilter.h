#pragma once

class SimpleKalmanFilter {
public:
  SimpleKalmanFilter(float, float, float) {}

  float updateEstimate(float value) {
    return value;
  }
};
