#include <Arduino.h>
#include <SimpleKalmanFilter.h>

// ---------------- configuration ----------------
#define WATER_MAX_DURATION 90     // counts of control ticks (see CONTROL_PERIOD_MS)
#define DEBUG true

// Pins (WATER & ERRLED are active-LOW)
#define WATER   13
#define ERRLED  16
#define TRIG     4
#define ECHO     5

// Cadence (ms)
#define SAMPLE_PERIOD_MS   5000   // <-- poll sensor once every 5 s
#define CONTROL_PERIOD_MS  1500   // control tick (no sensor I/O here)

// Baud: keep as requested
#define SERIAL_BAUD 74880

// Thresholds in CENTIMETERS (distance ↑ as water level ↓)
#define WATER_LOW_CM    30
#define WATER_HIGH_CM   10
#define WATER_ERR_CM   400
#define WATER_MIN_CM     4

// Hysteresis bands (anti-chatter)
#define LOW_ON_CM       30
#define LOW_OFF_CM      26
#define HIGH_ON_CM      10
#define HIGH_OFF_CM     14
#define K_LOW_ON         2
#define K_LOW_OFF        2
#define K_HIGH_ON        2
#define K_HIGH_OFF       2

// Robust sampling
#define N_PINGS            5
#define MIN_VALID_PINGS    3
#define PULSE_TIMEOUT_US 30000UL
#define JUMP_CM            20

// ---------------- globals ----------------
int   count_ticks = 0;            // counts control periods (for WATER_MAX_DURATION)
bool  filling     = false;

int   d_est_cm    = 0;            // filtered estimate [cm] (telemetry)
int   last_valid_measured = 0;    // last valid measured [cm] (telemetry)
int   prev_raw_cm = 0;            // prior raw for glitch gate

// hysteresis state & debounce
static bool   low_state  = false;
static bool   high_state = false;
static uint8_t low_on_cnt = 0, low_off_cnt = 0;
static uint8_t high_on_cnt = 0, high_off_cnt = 0;

// schedulers
static unsigned long t_last_sample  = 0;
static unsigned long t_last_control = 0;

// Kalman
SimpleKalmanFilter simpleKalmanFilter(5, 2, 0.01);

// -------------- utilities --------------
static inline int usToCm(unsigned long t_us) {
  if (t_us == 0) return WATER_ERR_CM;
  int d = (int)lroundf((float)t_us / 58.0f);
  if (d <= 0) d = WATER_ERR_CM;
  return d;
}

static inline int readEchoCmOnce() {
  // trigger
  digitalWrite(TRIG, LOW);  delayMicroseconds(5);
  digitalWrite(TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  // echo
  unsigned long echo_us = pulseIn(ECHO, HIGH, PULSE_TIMEOUT_US);
  return usToCm(echo_us);
}

// ---- Serial Plotter + Monitor Telemetry ----
static inline void emitHeaderOnce() {
  static bool done = false;
  if (!done) {
    Serial.println(F("measured\tfiltered")); // two series: red & green
    done =
