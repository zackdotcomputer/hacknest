// This code created and copyright Zack Sheppard - 2015
#include "Adafruit_DHT.h"

#define DEVICETYPE "thermostat-1"

// Allow the temperature to underrun our target
// (i.e. if we're a heater with target 10, don't turn on until 10 - n)
#define UNDERRUN 3
// When we run the device, overshoot our target
// (i.e. if we're a heater with target 10, run until we hit 10 + n)
#define OVERRUN 0

// If true, the program assumes closing the relay increases the room's temperature
#define ISHEATER true
#define RELAYPIN D7

#define DHTTYPE DHT11
#define DHTPIN 2

// Routine for syncing time to the cloud
#define ONE_DAY_MILLIS (24 * 60 * 60 * 1000)
#define FIVE_MIN_MILLIS (5 * 60 * 1000)
unsigned long lastSync = millis();

// Variable for the last reported temperature and the time it was received
int temperature = 0;
unsigned long lastReadTempAt = 0;

int localTemperature = 0;

// The target temperature we're trying to maintain. Default on cold start is 59F
double target = 15;

// Whether on the last loop the relay was closed
bool relayRunning;

const String deviceString = DEVICETYPE;

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Particle.variable("device", deviceString);

  Particle.variable("target", target);
  Particle.variable("temp", localTemperature);
  Particle.variable("lasttemp", temperature);

  Particle.function("settarget", setTarget);
  Particle.function("setftarget", convertThenSetTarget);
  Particle.function("setallft", convertThenSetTargetThenBroadcast);

  Particle.subscribe("zs-set-target", handleSetTemperature, MY_DEVICES);
  Particle.subscribe("zs-read-temp", handleTemperatureReading, MY_DEVICES);

  target = 15; // Default on cold start is 59F
  relayRunning = false;

  pinMode(RELAYPIN, OUTPUT);

  // Start DHT sensor
  dht.begin();

  Serial.begin(9600);
}

void loop() {
  unsigned long now = millis();

  // Make sure we're connected and time-synced to the Particle cloud
  if (Particle.connected() == false) {
    Serial.print("Reconnecting to cloud\n");
    Particle.connect();
  } else if (now - lastSync > ONE_DAY_MILLIS) {
    // Request time synchronization from the Particle Cloud
    Serial.print("Resyncing time\n");
    Particle.syncTime();
    lastSync = millis();
    now = lastSync;
  }

  localTemperature = dht.getTempCelcius();

  bool hasRemoteTemperature = !(now - lastReadTempAt > FIVE_MIN_MILLIS || temperature == 0);
  bool hasLocalTemperature = (localTemperature != 0);

  bool desiredState = relayRunning;

  // If the last temperature reading we got was over 5 minutes ago (or 0C), just
  // turn everything off and wait for the next reading to be reported.
  if (!hasRemoteTemperature && !hasLocalTemperature) {
    Serial.print("Temperature reading is out of date\n");
    desiredState = false;
  } else {
    int calcTemp = temperature;
    if (hasLocalTemperature) { // Always prefer local if available
      calcTemp = localTemperature;
    }

    // If it's too hot
    if (calcTemp > target) {
      // If it's too hot and we're a heater
      if (ISHEATER) {
        // We want to be on iff we are already on and haven't yet hit our overrun
        desiredState = relayRunning && (calcTemp < (target + OVERRUN));
      } else { // If it's too hot and we're a cooler
        // Leave it running if it's already on, turn it on if it's hotter than our underrun
        desiredState = relayRunning || (calcTemp >= (target + UNDERRUN));
      }
    } else {
      // If it's too cold and we're a heater
      if (ISHEATER) {
        // Leave it running if we're already on, turn it on of it's colder than our underrun
        desiredState = relayRunning || (calcTemp <= (target - UNDERRUN));
      } else { // If it's too cold and we're a cooler
        // We want to be on iff we are already on and haven't yet hit our overrun
        desiredState = relayRunning && (calcTemp > (target - OVERRUN));
      }
    }

    Serial.print("Calculations complete, desired state is " + String(desiredState) + "\n");
  }

  digitalWrite(RELAYPIN, desiredState);

  if (relayRunning != desiredState) {
    if (desiredState) {
      if (ISHEATER) {
        Particle.publish("zs-heater-state", "on", 60, PRIVATE);
      } else {
        Particle.publish("zs-cooler-state", "on", 60, PRIVATE);
      }
    } else {
      if (ISHEATER) {
        Particle.publish("zs-heater-state", "off", 60, PRIVATE);
      } else {
        Particle.publish("zs-cooler-state", "off", 60, PRIVATE);
      }
    }
  }

  relayRunning = desiredState;

  delay(5000); // 5 second resolution
}

void handleTemperatureReading(const char *event, const char *data) {
  if (strcmp(event, "zs-read-temp") == 0) {
    recordNewReading(String(data));
  }
}

int recordNewReading(String newReadingC) {
  int reading = newReadingC.toInt();
  if (reading != 0) { // Ignore 0's, usually bugs and/or why is your place so cold!?
    temperature = reading;
    lastReadTempAt = millis();
    return 1;
  } else {
    return -1;
  }
}

void handleSetTemperature(const char *event, const char *data) {
  if (strcmp(event, "zs-set-target") == 0) {
    setTarget(String(data));
  }
}

int convertThenSetTargetThenBroadcast(String newTargetF) {
  double cTarget = convertThenSetTarget(newTargetF);
  if (cTarget != -1) {
    Particle.publish("zs-set-target", String(cTarget), 60, PRIVATE);
    return 1;
  } else {
    return -1;
  }
}

double convertFToC(String fValue) {
  int targetFInt = fValue.toInt();
  if (targetFInt == 0 && fValue != "0") {
    return -1; // If toInt returns 0 for an input other than "0" it means it wasn't an int
  }
  return (targetFInt - 32.0) / 1.8;
}

int convertThenSetTarget(String newTargetF) {
  double targetC = convertFToC(newTargetF);
  if (targetC != -1) {
    target = targetC;
    return target;
  }
  return -1;
}

int setTarget(String newTargetC) {
  int targetInt = newTargetC.toInt();
  if (targetInt == 0 && newTargetC != "0") {
    return -1; // If toInt returns 0 for an input other than "0" it means it wasn't an int
  }

  target = targetInt;
  return 1;
}
