#include "arduino_secrets.h"
#include <math.h>
#include <WiFi.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "thingProperties.h"

LiquidCrystal_I2C lcd(0x27, 16, 2);

#define sensor 35
double rL = 20.0;
double b = -0.5;
double yOne = -0.3980;
double xOne = 2.9030;
double rO = 0;

void onCommentChange(){
  //
}
void onPpmChange(){
  //
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
    } else if (x >= 500 && x < 1000) {
      msg = "Slight LPG detected. Check area.";
    } else if (x >= 1000 && x < 5000) {
      msg = "Alarming rise of LPG! Open Windows.";
    } else if (x >= 5000 && x < 15000) {
      msg = "DANGER: High LPG! Evacuate!";
    } else {
      msg = "CRITICAL: Explosive level! Leave NOW!";
    }
    PPM = x;
    Comment = msg;
    lcd.setCursor(0, 0);
    lcd.print("LPG PPM in Air:      ");
    lcd.setCursor(0, 1);
    lcd.print(x);
    Serial.print("Reading Sensor: ");
    Serial.println(gas_value);
    Serial.print("VS: ");
    Serial.print(vS);
    Serial.print(" | RS: ");
    Serial.print(rS);
    Serial.print(" | log of X: ");
    Serial.println(log_x);
    Serial.print("LPG presence in Air (in PPM): ");
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
  lcd.print("Wait 5 minutes      ");
  delay(300000);
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
}
