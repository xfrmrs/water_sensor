#include <Arduino.h>
#include <SimpleKalmanFilter.h>

//error condition constants
#define WATER_MAX_DURATION 90
#define DEBUG false
#define DEBUG_M true

// delay constants
#define LOOP_DELAY 1000
#define WATER_DELAY 1500

// water level constants (in cm/100 )
#define WATER_LOW 1200   
#define WATER_HIGH 500
#define WATER_ERR 2500
#define WATER_MIN 4

//pin assignments 
#define TRIG 4    // yellow
#define ECHO 5    // green
#define WATER 13  // white w/black stripe
#define ERRLED 16 // N/A

// global variables
int count = 0;
bool filling = false;
float duration = 0;

/* 
 SimpleKalmanFilter(e_mea, e_est, q);
 e_mea: Measurement Uncertainty 
 e_est: Estimation Uncertainty 
 q: Process Noise
*/
SimpleKalmanFilter simpleKalmanFilter(5, 2, 0.01);

// function declarations
int WaterLevel();
bool WaterLow();
bool WaterHigh();

void setup() {

  pinMode(WATER, OUTPUT);
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  //water OFF by default
  digitalWrite(WATER,HIGH);
  digitalWrite(ERRLED,HIGH);

  // initialize serial communication
  Serial.begin(74800);

  int count = 0;
  bool filling = false;
}

// the loop routine runs over and over again forever:
void loop() {
  delay(LOOP_DELAY);        // delay in between reads for stability                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 
  while(!WaterHigh()){
    delay(WATER_DELAY); 
    if (WaterLow() || filling)
    {
      filling = true;
      if (DEBUG) Serial.println("water low, FILLING water");
      digitalWrite(WATER,LOW);

      // stop watering if watering for too long    
      if (count++>WATER_MAX_DURATION) 
      {
        digitalWrite(WATER,HIGH);
        digitalWrite(ERRLED,LOW);
        count = 0;
        if (DEBUG) Serial.print("water has been ON for too long, ");
        if (DEBUG) Serial.println("emergency STOP, water off");
      }
    }
  }

  if (DEBUG) Serial.println("water level is NORMAL, STOP water...");
  digitalWrite(WATER,HIGH);
  filling = false;
  // reset count
  count = 0;
}


int WaterLevel()
{
  int waterLevel = WATER_MIN;
  // The sensor is triggered by a HIGH pulse of 10 or more microseconds.
  // Give a short LOW pulse beforehand to ensure a clean HIGH pulse:
  digitalWrite(TRIG, LOW);
  delayMicroseconds(5);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  waterLevel = pulseIn(ECHO, HIGH);

  if (DEBUG_M) Serial.print("Measured: ");
  if (DEBUG_M) Serial.print(waterLevel);

  waterLevel = simpleKalmanFilter.updateEstimate(waterLevel);

  if (DEBUG_M) Serial.print(", Estimate: ");
  if (DEBUG_M) Serial.println(waterLevel);

  return waterLevel;
}

bool WaterLow()
{
  int waterLevel = WaterLevel();
  bool waterLow = (waterLevel >= WATER_LOW) && (waterLevel < WATER_ERR);
  return waterLow;
}

bool WaterHigh()
{
  int waterLevel = WaterLevel();
  bool waterLow = (waterLevel <= WATER_HIGH) || (waterLevel >= WATER_ERR);
  return waterLow;
}
