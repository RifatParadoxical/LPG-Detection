#include <math.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "voices.h"
#include "BluetoothA2DPSource.h"

LiquidCrystal_I2C lcd(0x27, 16, 2);
BluetoothA2DPSource a2dp;
unsigned int dataPlayTrack = 0;
volatile bool toggleplay = false;

#define sensor 35
double rL = 20.0;
double b = -0.5;
double yOne = -0.3980;
double xOne = 2.9030;
double rO = 0;
bool calibrated = false;
unsigned long long connectionStartTime = 0;
unsigned long lastLoopTime = 0;

// void onCommentChange(){}
// void onPpmChange(){}

double calibrateRO() {
  double gas = analogRead(sensor);
  if (gas < 248.18) {
    gas = 248.18;
  }
  double vO = gas * (3.3 / 4095);
  rO = (rL * (3.3 - vO)) / vO;
  return rO;
}

long int playVoice(Frame* data, long int len){
  for(long int i = 0; i < len; i++){
  if(toggleplay){
    if((dataPlayTrack/3) >= sp_CRITICAL_LEN){
      dataPlayTrack = 0;
      toggleplay = false;
    } else {
      int16_t value = pgm_read_byte(&(sp_CRITICAL[(dataPlayTrack/3)]));
      data[i].channel1 = value;
      data[i].channel2 = value;
      dataPlayTrack++;
    }
  } else {
    data[i].channel1 = 125;
    data[i].channel2 = 125;
  }

  }
  return len;
}

void loopFunc() {
  double gas_value = analogRead(sensor);

  if (gas_value < 248.18) {
    gas_value = 248.18;
  }

  double vS = gas_value * (3.3 / 4095);
  double rS = (rL * (3.3 - vS)) / vS;

  if (rO == 0) {
    //Comment = "Error: RO is 0!!";
    Serial.println(F("Error: RO is 0 !!"));
  }

  else if (vS <= 0.1) {
    //Comment = "Error: Sensor Disconnected?";
    Serial.println(F("Error: Sensor Disconected?"));
  }

  else if (rS / rO <= 0) {
    //Comment = "Error: Invalid Ratio of RS/RO.";
    Serial.println(F("Error: Invalid Ratio of RS/RO."));
  }

  else {
    double result = rS / rO;
    double y = log10(result);
    double log_x = ((y - yOne) / b) + xOne;
    double x = pow(10, log_x);
    // String msg = "";

    if (x < 500) {
      // msg = "Air is normal";
    } else if (x >= 500 && x < 5000) {
      toggleplay = true;
      // msg = "Slight Gas leakage detected. Check area.";
    } else if (x > 5000) {
      toggleplay = true;
      // msg = "Alarming rise of Gas presence! Open Windows.";
    } else {
      // msg = "Something went wrong! Check the Sensor.";
    }
    // PPM = x;
    //Comment = msg;
    lcd.setCursor(0, 0);
    lcd.print("Gas in Air(PPM):      ");
    lcd.setCursor(0, 1);
    lcd.print(x);
    lcd.print("           ");
    Serial.print(F("Reading Sensor: "));
    Serial.println(gas_value);
  }
}

void setup() {
  // initProperties();
  Wire.begin(13,14);
  lcd.init();
  lcd.backlight();
  Serial.begin(115200);
  Serial.println(F("MQ-5 Heating Up!!"));
  lcd.setCursor(0, 0);
  lcd.print("MQ-5 Heating Up!    ");
  lcd.setCursor(0, 1);
  lcd.print("loading...         ");
  // ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  lcd.clear();
  // setDebugMessageLevel(2);
  // ArduinoCloud.printDebugInfo();
  lcd.setCursor(0, 0);
  lcd.print("Ready to Go!      ");
  lcd.setCursor(0, 1);
  lcd.print("Trying to connect BT...   ");
  vTaskDelay(12000);
  a2dp.set_auto_reconnect(true);
  a2dp.start("T20", playVoice);
  a2dp.set_volume(100);

  vTaskDelay(3000);
  rO = calibrateRO();

  while(!a2dp.is_connected()){
    vTaskDelay(200);
    Serial.println(F("Waiting for Bluetooth Connection..."));
  }
  Serial.println(F("Bluetooth Connected!"));
  lcd.clear();
}

void loop() {
  // ArduinoCloud.update();
  if (millis() - lastLoopTime >= 500) {
    lastLoopTime = millis();
    loopFunc();
    if (!a2dp.is_connected()) {
      a2dp.reconnect();
      Serial.println(F("Bluetooth Disconnected! Attempting to Reconnect..."));
    }
  }
  if((millis() - connectionStartTime) > 86400000 && !calibrated){
    calibrated = true;
    rO = calibrateRO();
  }
}