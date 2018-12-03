#include "base.h"
#include "blink.h"

#include <Arduino.h>

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#include <WiFi.h>

#define uS_TO_S_FACTOR 1000000

int scanTime = 10;
BLEScan* pBLEScan;

volatile int numOfDevices = 0;

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice device) {
      numOfDevices++;

      DL("-------------------");
      DF("  Address: %s\n", device.getAddress().toString().c_str());
      DF("  RSSI: %d\n", device.getRSSI());

      if (device.haveName()) {
        DF("  Name: %s\n", device.getName().c_str());
      }

      if (device.haveAppearance()) {
        DF("  Appearance: %d\n", device.getAppearance());
      }

      if (device.haveManufacturerData()) {
        std::string md = device.getManufacturerData();
        uint8_t* mdp = (uint8_t*)device.getManufacturerData().data();
        char *pHex = BLEUtils::buildHexData(nullptr, mdp, md.length());
        DF("  ManufacturerData: %s\n", pHex);
        free(pHex);
      }

      if (device.haveServiceUUID()) {
        DF("  ServiceUUID: %s\n", device.getServiceUUID().toString().c_str());
      }

      if (device.haveTXPower()) {
        DF("  TxPower: %d\n", (int)device.getTXPower());
      }


      DF("Advertised Device: %s\n", device.toString().c_str());
      DL("-------------------");
    }
};

void setup() {
  blink();
  WiFi.mode(WIFI_OFF);
  Serial.begin(115200);
  DL("\n\nScanning...");

  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  // less or equal setInterval value
}

void loop() {
  // put your main code here, to run repeatedly:
  // https://github.com/moononournation/Arduino_BLE_Scanner/blob/master/Arduino_BLE_Scanner.ino
  numOfDevices = 0;
  BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
  DF("numOfDevices: %d\n", numOfDevices);
  /*
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
  */

  pBLEScan->clearResults();   // delete results fromBLEScan buffer to release memory

  // https://github.com/espressif/arduino-esp32/blob/master/libraries/ESP32/examples/DeepSleep/TimerWakeUp/TimerWakeUp.ino
  esp_sleep_enable_timer_wakeup(5*uS_TO_S_FACTOR);
  DL("going to sleep");
  Serial.flush();
  esp_deep_sleep_start();
}
