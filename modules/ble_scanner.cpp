#include "base.h"

#include <Arduino.h>
#include <sstream>

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#include <WiFi.h>

int scanTime = 10;
BLEScan* pBLEScan;
std::stringstream ss;

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      DF("Advertised Device: %s\n", advertisedDevice.toString().c_str());
    }
};

void setup() {
  WiFi.mode(WIFI_OFF);
  Serial.begin(115200);
  DL("\n\nScanning...");

  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan(); //create new scan
  // pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  // less or equal setInterval value
}

void loop() {
  // put your main code here, to run repeatedly:
  // https://github.com/moononournation/Arduino_BLE_Scanner/blob/master/Arduino_BLE_Scanner.ino
  BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
  int count = foundDevices.getCount();
  DF("\n***** Devices found: %i\n", count);

  for (int i = 0; i < count; i++) {
    DF("%d)\n", i);
    BLEAdvertisedDevice d = foundDevices.getDevice(i);
    DF("  Address: %s\n", d.getAddress().toString().c_str());
    DF("  RSSI: %d\n", d.getRSSI());

    if (d.haveName()) {
      DF("  Name: %s\n", d.getName().c_str());
    }

    if (d.haveAppearance()) {
      DF("  Appearance: %d\n", d.getAppearance());
    }

    if (d.haveManufacturerData()) {
      std::string md = d.getManufacturerData();
      uint8_t* mdp = (uint8_t*)d.getManufacturerData().data();
      char *pHex = BLEUtils::buildHexData(nullptr, mdp, md.length());
      DF("  ManufacturerData: %s\n", pHex);
      free(pHex);
    }

    if (d.haveServiceUUID()) {
      DF("  ServiceUUID: %s\n", d.getServiceUUID().toString().c_str());
    }

    if (d.haveTXPower()) {
      DF("  TxPower: %d\n", (int)d.getTXPower());
    }
  }

  pBLEScan->clearResults();   // delete results fromBLEScan buffer to release memory
  delay(2000);
}
