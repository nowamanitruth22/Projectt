#include <ESP8266WiFi.h>
#include <ThingSpeak.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include "DHT.h"

#define DHTPIN D4         // DHT11 data pin connected to D4
#define DHTTYPE DHT11
#define MQ135_PIN A0      // Analog pin for MQ135
#define DUST_SENSOR_PIN D5 // Digital pin for Dust Sensor

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2); // I2C address 0x27, 16 columns and 2 rows

// WiFi credentials
const char* ssid = "Eddy";
const char* password = "@26537eddy";

// ThingSpeak settings
WiFiClient client;
unsigned long myChannelNumber = 2940462;
const char * myWriteAPIKey = "NYXOEYDR4W7LL6SW";

void setup() {
  Serial.begin(115200);
  dht.begin();
  lcd.init();
  lcd.backlight();

  pinMode(DUST_SENSOR_PIN, INPUT);

  WiFi.begin(ssid, password);
  lcd.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    lcd.print(".");
  }

  Serial.println("\nWiFi Connected");
  ThingSpeak.begin(client);
  lcd.clear();
}

void loop() {
  // Read DHT11
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  // Read MQ135
  int mq135Value = analogRead(MQ135_PIN);

  // Dust sensor: basic pulse timing method
  unsigned long duration = pulseIn(DUST_SENSOR_PIN, LOW);
float ratio = duration / 10000.0; // duration in microseconds over sampling time
float dustDensity = 0.1 * ratio - 0.035; // mg/m3, needs calibration
dustDensity = dustDensity * 1000.0; // Convert to µg/m3
if (dustDensity < 0) dustDensity = 0;
  // Display on LCD
  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(temperature);
  lcd.print(" H:");
  lcd.print(humidity);

  lcd.setCursor(0, 1);
  lcd.print("G:");
  lcd.print(mq135Value);
  lcd.print(" D:");
  lcd.print(dustDensity);

  // Print to Serial Monitor
  Serial.print("Temperature: "); Serial.print(temperature); Serial.println(" *C");
  Serial.print("Humidity: "); Serial.print(humidity); Serial.println(" %");
  Serial.print("Gas MQ135: "); Serial.println(mq135Value);
  Serial.print("Dust Density: "); Serial.print(dustDensity); Serial.println(" ug/m3");
  Serial.println("----------");

  // Send to ThingSpeak
  ThingSpeak.setField(1, temperature);
  ThingSpeak.setField(2, humidity);
  ThingSpeak.setField(3, mq135Value);
  ThingSpeak.setField(4, dustDensity);
  
  int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  if (x == 200) {
    Serial.println("Data pushed to ThingSpeak");
  } else {
    Serial.print("Error pushing to ThingSpeak: "); Serial.println(x);
  }

  delay(20000); // Wait 20 seconds to comply with ThingSpeak rate limits

  // Determine environment levels
String tempStatus = (temperature < 16) ? "Cold" : (temperature <= 37) ? "Comfort" : "Cool";
String humStatus = (humidity < 30) ? "Dry" : (humidity <= 60) ? "Comfort" : "Humid";
String gasStatus = (mq135Value < 100) ? "Excellent" : (mq135Value <= 300) ? "Good" : (mq135Value <= 500) ? "Moderate" : "Polluted";
String dustStatus = (dustDensity < 30) ? "Clean" : (dustDensity <= 100) ? "Moderate" : "Unhealthy";

// Display on serial
Serial.println("Status:");
Serial.println("Temp: " + tempStatus);
Serial.println("Humidity: " + humStatus);
Serial.println("Gas: " + gasStatus);
Serial.println("Dust: " + dustStatus);

// Optionally display status on LCD (alternate or scroll)
lcd.clear();
lcd.setCursor(0, 0);
lcd.print("Air: " + gasStatus);
lcd.setCursor(0, 1);
lcd.print("Dust: " + dustStatus);
delay(5000); // Wait to show status before updating again
}
