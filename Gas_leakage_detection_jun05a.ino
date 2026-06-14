#include "arduino_secrets.h"  // secret header file for WiFi credentials and other sensitive information
#include <math.h>
#include <WiFi.h>
#include <Wire.h>
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

void onCommentChange(){
  //
}
void onPpmChange(){
  //
}

void playPCMSound(const unsigned char* sp_CRITICAL, int sp_CRITICAL_LEN){
    for(int i = 0; i < sp_CRITICAL_LEN; i++){
        dacWrite(25, pgm_read_byte(&(sp_CRITICAL[i])));
        delayMicroseconds(1000000 / 16000);
    }
}

void loopFunc() {
  double gas_value = analogRead(sensor);
  double vS = gas_value * (3.3 / 4095);
  double rS = (rL * (3.3 - vS)) / vS;

  if (rO == 0) {
    Comment = "Error: RO is 0!!";
    Serial.println("Error: RO is 0 !!");
  }

  else if (vS <= 0.1) {
    Comment = "Error: Sensor Disconnected?";
    Serial.println("Error: Sensor Disconected?");
  }

  else if (rS / rO <= 0) {
    Comment = "Error: Invalid Ratio of RS/RO.";
    Serial.println("Error: Invalid Ratio of RS/RO.");
  }

  else {
    double result = rS / rO;
    double y = log10(result);
    double log_x = ((y - yOne) / b) + xOne;
    double x = pow(10, log_x);
    String msg = "";

    if (x < 500) {
      msg = "Air is normal";
    } else if (x >= 500 && x < 5000) {
      playPCMSound(sp_CRITICAL, sp_CRITICAL_LEN);
      msg = "Slight Gas leakage detected. Check area.";
    } else if (x > 5000) {
      playPCMSound(sp_CRITICAL, sp_CRITICAL_LEN);
      msg = "Alarming rise of Gas presence! Open Windows.";
    } else {
      msg = "Something went wrong! Check the Sensor.";
    }
    PPM = x;
    Comment = msg;
    lcd.setCursor(0, 0);
    lcd.print("Gas in Air(PPM):      ");
    lcd.setCursor(0, 1);
    lcd.print(x);
    lcd.print("           ");
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
  lcd.print("Wait a minute...      ");
  delay(60000);
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  lcd.clear();
  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();
  lcd.setCursor(0, 0);
  lcd.print("Getting Ready...");
  lcd.setCursor(0, 1);
  lcd.print("Ready to Go!");

  double gas = analogRead(sensor);
  double vO = gas * (3.3 / 4095);
  rO = (rL * (3.3 - vO)) / vO;
  delay(1000);
  lcd.clear();
}

void loop() {
  ArduinoCloud.update();
  loopFunc();
  delay(500);
  if((connectionStartTime - millis()) > 86400000 && !calibrated){
    calibrated = true;
    double gas = analogRead(sensor);
    double vO = gas * (3.3 / 4095);
    rO = (rL * (3.3 - vO)) / vO;
  }
}