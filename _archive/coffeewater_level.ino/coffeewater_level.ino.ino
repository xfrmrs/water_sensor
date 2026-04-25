#define WATER 12
#define D0 14
#define VCC 15
#define ERRLED 16

//#define MIN15 900000
#define MIN15 10000

#define DEBUG 1

int sensorValueAnalog = 0, count = 0, readDelay = MIN15;
bool sensorValueDigital = 0, waterOn = 0;
bool waterLow();

void setup() {
  // initialize serial communication
  Serial.begin(74800);

  pinMode(D0, INPUT);

  pinMode(WATER, OUTPUT);
  pinMode(VCC, OUTPUT);
  pinMode(ERRLED, OUTPUT);

  digitalWrite(WATER,HIGH);
  digitalWrite(ERRLED,HIGH);
}

// the loop routine runs over and over again forever:
void loop() {
  while (waterLow())
  {
    if (DEBUG) Serial.println("water low, START water");
    digitalWrite(WATER,LOW);
    
    // monitor for water on for too long    
    if (count++>180) 
    {
      if (DEBUG) Serial.print("watering too long, ");
      digitalWrite(ERRLED,LOW);

      while(1) 
      {
        if (DEBUG) Serial.println("STOP water");
        digitalWrite(WATER,HIGH);

        if (!waterLow()) 
        {
          if (DEBUG) Serial.println("error cleared");
          digitalWrite(ERRLED,HIGH);
          break;
        }
        delay(readDelay/90); //about 1 second
      }
      // reset count
      count = 0;
    }
    delay(50);        // delay the loop
  }
  
  //water OFF by default
  digitalWrite(WATER,HIGH);
  if (DEBUG) Serial.println("water NORMAL OFF...");

  delay(readDelay);        // delay in between reads for stability
}

bool waterLow()
{
  digitalWrite(VCC, HIGH); 
  delay(5); //power up the sensor and wait to stabilize
  // read the input on analog pin 0:
  sensorValueAnalog = analogRead(A0);
  sensorValueDigital = digitalRead(D0);
  digitalWrite(VCC, LOW); //power down the sensor
  // print out the value read:
  if (DEBUG) 
  {
    Serial.print(sensorValueAnalog); 
    Serial.print("\t"); 
    Serial.println(sensorValueDigital);
  }

  waterOn = (sensorValueDigital && (sensorValueAnalog > 900));

  return waterOn;
}
