#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>

const char DEVICE_LOGIN_NAME[]  = "7a57b671-934b-4129-8162-97e43f53bb34";

const char SSID[]               = SECRET_SSID;
const char PASS[]               = SECRET_OPTIONAL_PASS;
const char DEVICE_KEY[]  = SECRET_DEVICE_KEY;

void onCommentChange();
void onPpmChange();

String Comment;
float PPM;

void initProperties(){

  ArduinoCloud.setBoardId(DEVICE_LOGIN_NAME);
  ArduinoCloud.setSecretDeviceKey(DEVICE_KEY);
  ArduinoCloud.addProperty(Comment, READWRITE, ON_CHANGE, onCommentChange);
  ArduinoCloud.addProperty(PPM, READWRITE, ON_CHANGE, onPpmChange);

}

WiFiConnectionHandler ArduinoIoTPreferredConnection(SSID, PASS);
