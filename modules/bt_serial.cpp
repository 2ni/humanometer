
#include <Arduino.h>
#include "BluetoothSerial.h"

#include "base.h"
#include "blink.h"

#include <WiFi.h>

BluetoothSerial SerialBT;

void setup() {
  blink();
  WiFi.mode(WIFI_OFF);
  Serial.begin(115200);
  DL("\n");

  if (!SerialBT.begin("ESP chalet")) {
    DL("***ERROR*** Could not start bluetooth");
  }
  DL("ready for pairing. Pong service.");
  DL("use app 'Serial Bluetooth Terminal' to send a message");
}

void loop() {
  while (SerialBT.available()) {
    char in = (char)SerialBT.read();
    DF("%c", in);
  }

  delay(50);
}
