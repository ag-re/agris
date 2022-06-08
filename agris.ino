// Additional libraries
#include <DHT.h>
#include <MQ135.h>
#include <HCSR04.h>

// Set pins
#define DHTPIN 13
#define DHTTYPE DHT22 // Set DHT Type to DHT22 (AM2302)
#define MQANALOG 0
#define RAINSENSORPIN 1
#define MOTORPINA 2
#define MOTORPINB 4
#define PWM 3
#define PUMPPINA 9
#define PUMPPINB 10
#define ACTUATORPINA 11
#define ACTUATORPINB 12
#define TRIGPINA 8
#define ECHOPINA 7
#define TRIGPINB 6
#define ECHOPINB 5

// Initialize sensors
DHT dht(DHTPIN, DHTTYPE);
MQ135 gasSensor = MQ135(MQANALOG);
UltraSonicDistanceSensor distSensorA(TRIGPINA, ECHOPINA);
UltraSonicDistanceSensor distSensorB(TRIGPINB, ECHOPINB);

// Variables
String command;
boolean skylightStatus; // true for open, false for closed
int gasThreshold;
int rainThreshold;
byte distThreshold;
byte maxSpeed;

void setup()
{
  pinMode(MOTORPINA, OUTPUT);
  pinMode(MOTORPINB, OUTPUT);
  pinMode(PWM, OUTPUT);
  analogWrite(PWM, 0); // Safety reset of speed
  maxSpeed = 150;
  distThreshold = 12;

  pinMode(PUMPPINA, OUTPUT);
  pinMode(PUMPPINB, OUTPUT);
  digitalWrite(PUMPPINA, LOW);
  digitalWrite(PUMPPINB, LOW);
  pinMode(ACTUATORPINA, OUTPUT);
  pinMode(ACTUATORPINB, OUTPUT);
  digitalWrite(ACTUATORPINA, LOW);
  digitalWrite(ACTUATORPINB, LOW);
  skylightStatus = false;

  dht.begin();
  gasThreshold = 390;
  rainThreshold = 500;

  Serial.begin(9600);

  Serial.println("Type 'help' to see a list of commands.");
}

void loop()
{

  if (isRain() && skylightStatus)
  {
    Serial.println("It started to rain. Closing skylight...");
    closeSkylight();
  }

  else if (!isRain() && isGas() && !skylightStatus)
  {
    Serial.println("High CO2 detected. Opening skylight...");
    openSkylight();
  }


  if (Serial.available() > 0)
  {

    readCommand();

    if (command.equals("sensor"))
    {

      // Read and display from DHT22
      Serial.println("Temperature: " + String(getTemperature()) + "C");
      Serial.println("Humidity:" + String(getHumidity()) + "%");

      // Read and display from MQ135
      if (getGas() > gasThreshold)
      {
        Serial.println("CO2: High (" + String(getGas()) + " ppm)");
      }
      else
      {
        Serial.println("CO2: Low (" + String(getGas()) + " ppm)");
      }

      // Read and display from FC37
      if (getRain() < rainThreshold)
      {
        Serial.println("Weather: Rainy (" + String(getRain()) + ")");
      }
      else
      {
        Serial.println("Weather: Clear (" + String(getRain()) + ")");
      }
      Serial.println("");
    }

    else if (command.equals("spray"))
    {

      boolean runOnceFlag = true;
      while (getDistanceA() > distThreshold)
      {
        if (runOnceFlag)
        {
          Serial.println("Spraying in A direction...");
          goToA();
          runOnceFlag = false;
        }
        delay(25);
      }

      Serial.println("Stopping...");
      stopMoving();

      runOnceFlag = true;
      while (getDistanceB() > distThreshold)
      {
        if (runOnceFlag)
        {
          Serial.println("Spraying in B direction...");
          goToB();
          runOnceFlag = false;
        }
        delay(25);
      }

      Serial.println("Stopping...");
      stopMoving();
    }

    else if (command.equals("help"))
    {
      Serial.println(getAvailableCommands());
      Serial.println(getTroubleshootCommands());
    }

    else if (command.equals("vent_off"))
    {
      Serial.println("Closing skylight...");
      closeSkylight();
    }

    else if (command.equals("vent_on"))
    {
      Serial.println("Opening skylight...");
      openSkylight();
    }

    else if (command.equals("demo_vent"))
    {
      Serial.println("Ventilation Demo...");
      openSkylight();
      delay(1000);
      closeSkylight();
    }

    else if (command.equals("demo_pump"))
    {
      Serial.println("Pump Demo...");
      digitalWrite(PUMPPINA, HIGH);
      digitalWrite(PUMPPINB, LOW);
      delay(2000);
      digitalWrite(PUMPPINA, LOW);
      digitalWrite(PUMPPINB, LOW);
    }

    else
    {

      Serial.println("Unknown command!");
      Serial.println("Type 'help' to see a list of commands.");
    }
  }
}

String getAvailableCommands()
{
  return "Available commands:\nsensor\nspray\n";
}

String getTroubleshootCommands()
{
  return "Available troubleshooting commands:\ndemo_vent\ndemo_pump\nvent_on\nvent_off\n";
}

void readCommand()
{
  command = Serial.readStringUntil('\n');
  command.trim();
  command.toLowerCase();
}

float getTemperature()
{
  return dht.readTemperature();
}

float getHumidity()
{
  return dht.readHumidity();
}

float getGas()
{
  return gasSensor.getPPM();
}

boolean isGas()
{
  return getGas() >= gasThreshold;
}

float getRain()
{
  return analogRead(RAINSENSORPIN);
}

boolean isRain()
{
  return getRain() < rainThreshold;
}

float getDistanceA()
{
  return distSensorA.measureDistanceCm();
}

float getDistanceB()
{
  return distSensorB.measureDistanceCm();
}

void goToA()
{
  digitalWrite(MOTORPINA, HIGH);
  digitalWrite(MOTORPINB, LOW);
  digitalWrite(PUMPPINA, HIGH);
  digitalWrite(PUMPPINB, LOW);
  for (int i = 1; i < maxSpeed; i++)
  {
    analogWrite(PWM, i);
    delay(3);
  }
}

void goToB()
{
  digitalWrite(MOTORPINA, LOW);
  digitalWrite(MOTORPINB, HIGH);
  digitalWrite(PUMPPINA, HIGH);
  digitalWrite(PUMPPINB, LOW);
  for (int i = 1; i < maxSpeed; i++)
  {
    analogWrite(PWM, i);
    delay(3);
  }
}

void stopMoving()
{
  for (int i = maxSpeed - 1; i >= 0; i--)
  {
    analogWrite(PWM, i);
    delay(2);
  }
  digitalWrite(PUMPPINA, LOW);
  digitalWrite(PUMPPINB, LOW);
}

void openSkylight()
{
  if (!skylightStatus)
  {
    digitalWrite(ACTUATORPINA, HIGH);
    digitalWrite(ACTUATORPINB, LOW);
    delay(4500);
    digitalWrite(ACTUATORPINA, LOW);
    digitalWrite(ACTUATORPINB, LOW);
    skylightStatus = true;
  }
}

void closeSkylight()
{
  if (skylightStatus)
  {
    digitalWrite(ACTUATORPINA, LOW);
    digitalWrite(ACTUATORPINB, HIGH);
    delay(4500);
    digitalWrite(ACTUATORPINA, LOW);
    digitalWrite(ACTUATORPINB, LOW);
    skylightStatus = false;
  }
}
