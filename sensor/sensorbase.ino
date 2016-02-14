// This code created and copyright Zack Sheppard - 2016

// This #include statement was automatically added by the Spark IDE.
#include "Adafruit_DHT.h"

#define DEVICETYPE "sensorbase-1"
const String deviceString = DEVICETYPE;

// -----------------
// Read temperature
// -----------------

#define DHTTYPE DHT11
#define DHTPIN 0
#define LIGHTPIN D7

DHT dht(DHTPIN, DHTTYPE);

int temperature;
int humidity;

void setup() {
    Particle.variable("device", deviceString);

    // Declare variables
    Particle.variable("temp", temperature);
    Particle.variable("humidity", humidity);

    pinMode(LIGHTPIN, OUTPUT);

    // Start DHT sensor
    dht.begin();

    Serial.begin(9600);
}

void loop() {
    if (Particle.connected() == false) {
        Particle.connect();
    }

    // Humidity measurement
    temperature = dht.getTempCelcius();

    // Humidity measurement
    humidity = dht.getHumidity();

    Serial.print("Temperature reading is " + String(temperature) + "\n");
    Serial.print("Humidity reading is    " + String(humidity) + "\n");

    // Check that the temperature reading is not 0C - usually indicates failure.
    if (temperature != 0) {
      Particle.publish("zs-read-temp", String(temperature), 60, PRIVATE);
      digitalWrite(LIGHTPIN, HIGH);
    } else {
      digitalWrite(LIGHTPIN, LOW);
    }

    // Resolution is every 30 seconds
    delay(30000);
}
