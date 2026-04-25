#define VCC 4
#define D0 14
#define DEBUG 1
//#define MIN15 900000
#define MIN15 1000

int sensorValueAnalog = 0, readDelay = MIN15;
bool sensorValueDigital = 0, waterOn = 0;

void setup() {
  // initialize serial communication
  Serial.begin(74800);
  pinMode(VCC, OUTPUT);

}

// the loop routine runs over and over again forever:
void loop() {

  while (readSensor())
  {
    if (!waterOn)
    {
      waterOn = true;
      // activate water 
    }
    if (DEBUG) Serial.println("watering...");
    delay(5);        // delay in between reads for stability
  }
  
  //deactivate water
  waterOn = false;
  if (DEBUG) Serial.println("not watering...");
  delay(readDelay);        // delay in between reads for stability
}

bool readSensor()
{
  digitalWrite(VCC, HIGH); //power up the sensor
  delay(5);  // wait for sensor to stabilize
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
  return (sensorValueDigital && (sensorValueAnalog > 900));
}
