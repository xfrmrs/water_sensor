//pin assignments 
#define WATER 12
#define D0 14
#define SENSOR_VCC 15
#define ERRLED 16

// delay constants
#define LOOP_DELAY 10000
#define WATER_DELAY 500
#define EMER_DELAY 10


//error condition constants
#define MAX_WATERING_DURATION 90
#define DEBUG 1

int sensorValueAnalog = 0, count = 0;
bool sensorValueDigital = 0, waterOn = 0;
bool waterLow();

void setup() {
  // initialize serial communication
  Serial.begin(74800);

  pinMode(D0, INPUT);
  pinMode(A0, INPUT);

  pinMode(WATER, OUTPUT);
  pinMode(SENSOR_VCC, OUTPUT);
  pinMode(ERRLED, OUTPUT);

  //water OFF by default
  digitalWrite(WATER,HIGH);
  digitalWrite(ERRLED,HIGH);
}

// the loop routine runs over and over again forever:
void loop() {

  while (waterLow())
  {
    delay(WATER_DELAY); 

    if (DEBUG) Serial.println("water low, START water");
    digitalWrite(WATER,LOW);

    // stop watering if watering for too long    
    if (count++>MAX_WATERING_DURATION) 
    {
      if (DEBUG) Serial.print("water has been ON for too long, ");
      digitalWrite(ERRLED,LOW);

      while(waterOn) 
      {
        if (DEBUG) Serial.println("emergency STOP, water off");
        digitalWrite(WATER,HIGH);
                           
        if (!waterLow()) 
        {
          if (DEBUG) Serial.println("error cleared");
          digitalWrite(ERRLED,HIGH);
          break;
        }
        delay(EMER_DELAY); 
      }
    }
  }
  
  // reset count
  count = 0;

  if (DEBUG) Serial.println("water level is NORMAL, STOP water...");
  digitalWrite(WATER,HIGH);
  
  delay(LOOP_DELAY);        // delay in between reads for stability                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 
}

bool waterLow()
{
  //power up the sensor and wait to stabilize
  digitalWrite(SENSOR_VCC, HIGH); delay(50); 
 
  // read the analog input A0:
  sensorValueAnalog = analogRead(A0);
  // read discrete input DO:
  sensorValueDigital = digitalRead(D0);

  //power down the sensor
  digitalWrite(SENSOR_VCC, LOW); //power down the sensor
  
  // print out the value read:
  if (DEBUG) 
  {
    Serial.print(sensorValueAnalog); 
    Serial.print(":"); 
    Serial.println(sensorValueDigital);
  }

  // analog reading is not reliable, combine with discrete
  waterOn = (sensorValueDigital && (sensorValueAnalog > 900));

  return waterOn;
}
