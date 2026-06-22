#include "arduino_secrets.h" 
#include <math.h>
#include <WiFi.h>
#include <Wire.h>
#include <HTTPClient.h>
#include <LiquidCrystal_I2C.h>
#include "thingProperties.h"
#include "voices.h"

LiquidCrystal_I2C lcd(0x27, 16, 2);

#define sensor 35
double rL = 20.0;
double b = -0.5;
double yOne = -0.3980;
double xOne = 2.9030;
double rO = 0;
int calibrated = false;
unsigned long long connectionStartTime = 0;
unsigned long long lastNotificationSlight = 0;
unsigned long long lastNotificationDanger = 0;
unsigned long long lastAlertTime = 0;
int alertFiltering = 0;

void onCommentChange(){
  //
}
void onPpmChange(){
  //
}

double calibrateRO() {
  double gas = analogRead(sensor);
  if (gas < 248.18) {
    gas = 248.18;
  }
  double vO = gas * (3.3 / 4095);
  rO = (rL * (3.3 - vO)) / vO;
  return rO;
}

void sendNotification(String title, String message){
  if(WiFi.status() == WL_CONNECTED){
    HTTPClient http;
    http.begin("https://api.pushbullet.com/v2/pushes");
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Access-Token", PushBullet_Access_Token);
    String payload = "{\"type\":\"note\",\"title\":\"" + title + "\",\"body\":\"" + message + "\"}";
    http.POST(payload);
    http.end();
  }
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
    Comment = "Error: RO is 0!!";
    Serial.println("Error: RO is 0 !!");
    lcd.setCursor(0,0);
    lcd.print("Error: RO is 0!!     ");
    lcd.setCursor(0,1);
    lcd.print("Check the Sensor.     ");
  }

  else if (vS <= 0.1) {
    Comment = "Error: Sensor Disconnected?";
    Serial.println("Error: Sensor Disconected?");
    lcd.setCursor(0,0);
    lcd.print("Error: Sensor        ");
    lcd.setCursor(0,1);
    lcd.print("Disconnected?        ");
  }

  else if (rS / rO <= 0) {
    Comment = "Error: Invalid Ratio of RS/RO.";
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
    String msg = "";

    if (x < 200) {
      msg = "Air is normal";
      lcd.setCursor(0,0);
      lcd.print("Air is normal          ");
      lcd.setCursor(0,1);
      lcd.print("                       ");
    } else if (x >= 200 && x < 2000 && alertFiltering > 5) {
      playPCMSound(sp_CRITICAL, sp_CRITICAL_LEN);
      if((millis() - lastNotificationSlight) > 60000){
        lastNotificationSlight = millis();
        sendNotification("Gas Leakage Detected!", "Slight Gas leakage detected. Check area.");
      }
      msg = "Slight Gas leakage detected. Check area.";
      lcd.setCursor(0,0);
      lcd.print("Slight Gas leak         ");
      lcd.setCursor(0,1);
      lcd.print("Detected!Check area.    ");
    } else if (x > 2000 && alertFiltering > 5) {
      playPCMSound(sp_CRITICAL, sp_CRITICAL_LEN);
      if((millis() - lastNotificationDanger) > 20000){
        lastNotificationDanger = millis();
        sendNotification("Gas Leakage Detected!", "Alarming rise of Gas presence! Open Windows.");
      }
      msg = "Alarming rise of Gas presence! Open Windows.";
      lcd.setCursor(0,0);
      lcd.print("Gas leak Alarm!       ");
      lcd.setCursor(0,1);
      lcd.print("Vantile area.          ");
    } else if (x >= 200 && x < 2000 && alertFiltering <= 5) {
      alertFiltering++;
      msg = "Slight Gas leakage detected. Check area.";
      lcd.setCursor(0,0);
      lcd.print("Slight Gas leak         ");
      lcd.setCursor(0,1);
      lcd.print("Detected!Check area.    ");
    } else if (x > 2000 && alertFiltering <= 5) {
      alertFiltering++;
      msg = "Alarming rise of Gas presence! Open Windows.";
      lcd.setCursor(0,0);
      lcd.print("Gas leak Alarm!       ");
      lcd.setCursor(0,1);
      lcd.print("Vantile area.          ");
    } else {
      msg = "Something went wrong! Check the Sensor.";
      lcd.setCursor(0,0);
      lcd.print("Something wrong!       ");
      lcd.setCursor(0,1);
      lcd.print("Check the Sensor.      ");
    }
    PPM = x;
    Comment = msg;
    Serial.print("Reading Sensor: ");
    Serial.println(gas_value);
    Serial.print("VS: ");
    Serial.print(vS);
    Serial.print(" | RS: ");
    Serial.print(rS);
    Serial.print(" | log of X: ");
    Serial.println(log_x);
    Serial.print("Gas presence in Air (in PPM): ");
    Serial.println(x);
  }
}

void setup() {
  initProperties();
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
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  lcd.clear();
  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();
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
  ArduinoCloud.update();
  loopFunc();
  delay(250);
  if((millis() - connectionStartTime) > 86400000 && !calibrated){
    calibrated = true;
    rO = calibrateRO();
  }
  if((millis() - lastAlertTime) > 60000 && alertFiltering > 0){
    alertFiltering = 0;
    lastAlertTime = millis();
  }
}