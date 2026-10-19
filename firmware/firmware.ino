#include "arduino_secrets.h"
#include <WiFi.h>
#include <Wire.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <LiquidCrystal_I2C.h>
#include "voices.h"

const char* ssid = SECRET_SSID;
const char* password = SECRET_PASS;
const char* hostname = "ESP-GAS-DETECTION-DEVICE";
const char* display_device_key = "id: " SECRET_DEVICE_KEY;

WiFiClientSecure client;
LiquidCrystal_I2C lcd(0x27, 16, 2);

#define SENSOR 35
volatile bool status = false;
volatile bool statusChanged = false;
volatile bool need2Change = false;
unsigned long lastInterruptTime = 0;
unsigned long long wifiStatus = 0;

void sendData() {
  int response = 0;
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;

  if (!http.begin(client, "https://lpg-detection.vercel.app/devices/" + String(SECRET_DEVICE_KEY) + "/api/sensor")) {
    return;
  }

  http.addHeader("Content-Type", "application/json");
  String json =
    "{\"device_id\":\""
    + String(SECRET_DEVICE_KEY)
    + "\","
      "\"status\":"
    + String(status)
    + "}";


  response = http.POST(json);

  if (response == 200) {
    Serial.print("Successfully sent data.");
    lcd.setCursor(0, 1);
    lcd.print("Data sent successfully.");
  } else {
    Serial.print("Failed to send data.");
    lcd.setCursor(0, 1);
    lcd.print("Failed to send data.");
  }
  http.end();
}

void playPCMSound(const unsigned char* sp_CRITICAL, int sp_CRITICAL_LEN) {
  for (int i = 0; i < sp_CRITICAL_LEN; i++) {
    dacWrite(25, pgm_read_byte(&(sp_CRITICAL[i])));
    delayMicroseconds(1000000 / 16000);
  }
  delay(500);
}

void IRAM_ATTR onChangeFunction() {
  unsigned long now = millis();
  if (now - lastInterruptTime < 500) return;
  lastInterruptTime = now;
  status = (digitalRead(SENSOR) == HIGH);
  statusChanged = true;
}

void setup() {
  client.setInsecure();
  WiFi.hostname(hostname);
  WiFi.setAutoReconnect(true);
  WiFi.begin(ssid, password);
  Wire.begin(13, 14);  // SDA, SCL
  pinMode(SENSOR, INPUT);
  attachInterrupt(digitalPinToInterrupt(SENSOR), onChangeFunction, CHANGE);
  lcd.init();
  lcd.backlight();
  Serial.begin(115200);
  delay(2000);
  Serial.println("MQ-5 Heating Up!!");
  lcd.setCursor(0, 0);
  lcd.print("MQ-5 Heating Up!    ");
  lcd.setCursor(0, 1);
  lcd.print("loading...         ");
  // delay(10000);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Getting Ready...");
  lcd.setCursor(0, 1);
  lcd.print("Ready to Go!");
  delay(3000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Air is Normal.     ");
  lcd.setCursor(0, 1);
  lcd.print(display_device_key);
  sendData();
}

void loop() {
  if ((millis() - wifiStatus >= 15000) && (WiFi.status() != WL_CONNECTED)) {
    wifiStatus = millis();
    Serial.println("WiFi not connected. Reconnecting...");
    lcd.setCursor(0, 1);
    lcd.print("WiFi issue.          ");
    WiFi.disconnect();
    WiFi.reconnect();
  } else if (WiFi.status() == WL_CONNECTED) {
    lcd.setCursor(0, 1);
    lcd.print(display_device_key);
  }

  if (statusChanged) {
    statusChanged = false;
    need2Change = true;
    if (status) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Gas detected!!     ");
    } else {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Air is Normal.     ");
    }
  }
  if (status) {
    playPCMSound(sp_CRITICAL, sp_CRITICAL_LEN);
  }
  if (need2Change) {
    sendData();
    need2Change = false;
  }
}

// yet four improvements can be made:
// 1. Implement RTOS and add more sophisticated error handling and logging on LCD
// 2. More safety and security features including authentication, authorization and advanced security for server and database.
// 3. Audio filterring and noise cancellation for better audio quality and clarity.
// 4. Implement LAN connectivity.