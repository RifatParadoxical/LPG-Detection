#include "arduino_secrets.h" 
#include <math.h>
#include <WiFi.h>
#include <Wire.h>
#include <HTTPClient.h>
#include <LiquidCrystal_I2C.h>
#include "voices.h"

const char* ssid = SECRET_SSID;
const char* password = SECRET_PASS;

LiquidCrystal_I2C lcd(0x27, 16, 2);

#define sensor 35
double rL = 20.0;
double b = -0.5;
double yOne = -0.3980;
double xOne = 2.9030;
double rO = 0;
int status = 0;
int calibrated = false;

unsigned long long wifiStatus = 0;
unsigned long long connectionStartTime = 0;
unsigned long long lastAlertFiltered = 0;
int alertFiltering = 0;

double calibrateRO() {
  double gas = analogRead(sensor);
  if (gas < 248.18) {
    gas = 248.18;
  }
  double vO = gas * (3.3 / 4095);
  rO = (rL * (3.3 - vO)) / vO;
  return rO;
}

void sendData(double x, int status){
   if (WiFi.status() != WL_CONNECTED)
        return;

    HTTPClient http;

    if (!http.begin("https://lpg-detection.vercel.app/api/sensor")) {
        return;
    }

    http.addHeader("Content-Type", "application/json");
    String json =
        "{\"ppm\":"
        + String(x, 2)
        + ",\"status\":"
        + String(status)
        + "}";


    if (response > 0) {
        Serial.print("Server response: ");
        Serial.println(response);
    } else {
        Serial.print("HTTP Error: ");
        Serial.println(response);
    }
    http.end();

}

void playPCMSound(const unsigned char* sp_CRITICAL, int sp_CRITICAL_LEN){
    for(int i = 0; i < sp_CRITICAL_LEN; i++){
        dacWrite(25, pgm_read_byte(&(sp_CRITICAL[i])));
        delayMicroseconds(1000000 / 16000);
    }
}

void loopFunc() {
  double gas_value = analogRead(sensor);

  if (gas_value < 248.18) {
    gas_value = 248.18;
  }

  double vS = gas_value * (3.3 / 4095);
  double rS = (rL * (3.3 - vS)) / vS;

  if (rO == 0) {
    Serial.println("Error: RO is 0 !!");
    lcd.setCursor(0,0);
    lcd.print("Error: RO is 0!!     ");
    lcd.setCursor(0,1);
    lcd.print("Check the Sensor.     ");
  }

  else if (vS <= 0.1) {
    Serial.println("Error: Sensor Disconected?");
    lcd.setCursor(0,0);
    lcd.print("Error: Sensor        ");
    lcd.setCursor(0,1);
    lcd.print("Disconnected?        ");
  }

  else if (rS / rO <= 0) {
    Serial.println("Error: Invalid Ratio of RS/RO.");
    lcd.setCursor(0,0);
    lcd.print("Error: Invalid Ratio  ");
    lcd.setCursor(0,1);
    lcd.print("of RS/RO.            ");
  }

  else {
    double result = rS / rO;
    double y = log10(result);
    double log_x = ((y - yOne) / b) + xOne;
    double x = pow(10, log_x);

    if (x < 200) {
      lcd.setCursor(0,0);
      lcd.print("Air is normal          ");
      lcd.setCursor(0,1);
      lcd.print("                       ");
      status = 0;
    } else if (x >= 200 && x < 2000 && alertFiltering > 5) {
      status = 1;
      playPCMSound(sp_CRITICAL, sp_CRITICAL_LEN);
      lcd.setCursor(0,0);
      lcd.print("Slight Gas leak         ");
      lcd.setCursor(0,1);
      lcd.print("Detected!Check area.    ");
    } else if (x > 2000 && alertFiltering > 5) {
      status = 2;
      playPCMSound(sp_CRITICAL, sp_CRITICAL_LEN);
      lcd.setCursor(0,0);
      lcd.print("Gas leak Alarm!       ");
      lcd.setCursor(0,1);
      lcd.print("Vantilate area.          ");
    } else if (x >= 200 && x < 2000 && alertFiltering <= 5) {
      status = 1;
      alertFiltering++;
      lcd.setCursor(0,0);
      lcd.print("Slight Gas leak         ");
      lcd.setCursor(0,1);
      lcd.print("Detected!Check area.    ");
    } else if (x > 2000 && alertFiltering <= 5) {
      status = 2;
      alertFiltering++;
      lcd.setCursor(0,0);
      lcd.print("Gas leak Alarm!       ");
      lcd.setCursor(0,1);
      lcd.print("Vantilate area.          ");
    } else {
      lcd.setCursor(0,0);
      lcd.print("Something wrong!       ");
      lcd.setCursor(0,1);
      lcd.print("Check the Sensor.      ");
    }

    sendData(x, status);
    Serial.print("Reading Sensor: ");
    Serial.println(gas_value);
    Serial.print("VS: ");
    Serial.print(vS);
    Serial.print(" | RS: ");
    Serial.print(rS);
    Serial.print(" | log of x: ");
    Serial.println(log_x);
    Serial.print("Gas presence in Air (in PPM): ");
    Serial.println(x);
  }
}

void setup() {
  WiFi.begin(ssid, password);
  Wire.begin(13,14);
  lcd.init();
  lcd.backlight();
  Serial.begin(115200);
  Serial.println("MQ-5 Heating Up!!");
  lcd.setCursor(0, 0);
  lcd.print("MQ-5 Heating Up!    ");
  lcd.setCursor(0, 1);
  lcd.print("loading...         ");
  delay(10000);

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Getting Ready...");
  lcd.setCursor(0, 1);
  lcd.print("Ready to Go!");
  delay(5000);

  rO = calibrateRO();

  delay(1000);
  lcd.clear();
}

void loop() {
  if (millis() - wifiStatus >= 4000) {
    wifiStatus = millis();

    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi not connected. Reconnecting...");
      WiFi.begin(ssid, password); 
    }
  }

  loopFunc();
  delay(1000);
  if((millis() - connectionStartTime) > 86400000 && !calibrated){
    calibrated = true;
    rO = calibrateRO();
  }
  if((millis() - lastAlertFiltered) > 60000 && alertFiltering > 0){
    alertFiltering = 0;
    lastAlertFiltered = millis();
  }
}