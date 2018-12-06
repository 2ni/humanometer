#include "base.h"
#include "blink.h"

#include <Arduino.h>

#include <SPIFFS.h>

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#include <WiFi.h>

#define SDA 21
#define SCL 22
#include <Wire.h>
#include "SSD1306Wire.h"

SSD1306Wire display(0x3c, SDA, SCL);

#define uS_TO_S_FACTOR 1000000
#define INTERVALL 30 // in sec

int scanTime = 5;
BLEScan* pBLEScan;

static bool keepalive = false;
uint64_t nextScan = 0;

enum states {
  SCANNING,  // scan in progress
  COMPLETED, // scan completed
  IDLING // waiting
};

enum states state;

const char* LOG = "/bluetooth.txt";

void show_help() {
  DL("*************************************");
  DL("h - help");
  DF("k - toggle keep alive (current: %d)\n", keepalive);
  DL("s - start scan");
  DL("l - show log");
  DL("d - delete log");
  DL("r - reset");
  DL("*************************************");
  DL();
}

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice device) {
      DF("%s | ", device.getAddress().toString().c_str());

      DF("RSSI: %d | ", device.getRSSI());

      if (device.haveName()) {
        DF("Name: %s | ", device.getName().c_str());
      }

      if (device.haveAppearance()) {
        DF("Appearance: %d\n", device.getAppearance());
      }

      if (device.haveManufacturerData()) {
        std::string md = device.getManufacturerData();
        uint8_t* mdp = (uint8_t*)device.getManufacturerData().data();
        char *pHex = BLEUtils::buildHexData(nullptr, mdp, md.length());
        DF("ManufacturerData: %s | ", pHex);
        free(pHex);
      }

      if (device.haveServiceUUID()) {
        DF("ServiceUUID: %s | ", device.getServiceUUID().toString().c_str());
      }

      if (device.haveTXPower()) {
        DF("TxPower: %d | ", (int)device.getTXPower());
      }
      DL();

      // DF("Advertised Device: %s\n", device.toString().c_str());
    }
};

void updateScreen(uint16_t value) {
  display.clear();
  display.setFont(ArialMT_Plain_24);
  display.setTextAlignment(TEXT_ALIGN_CENTER); // coords define center of text
  display.drawString(64, 20, String(value));
  display.display();
}

uint16_t numDevices = 0;
void scanCompleted(BLEScanResults foundDevices) {
  numDevices = foundDevices.getCount();
  state = COMPLETED;
}

void setup() {
  blink();
  WiFi.mode(WIFI_OFF);
  Serial.begin(115200);
  DL("\n");

  state = IDLING;

  show_help();

  if (!SPIFFS.begin(true)) {
    DL("***ERROR*** could not mount SPIFFS");
  }

  // 128x64 (x y)
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_CENTER); // coords define center of text
  display.drawString(64, 24, "Humanometer");
  display.drawHorizontalLine(10, 42, 108);
  display.display();

  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  // less or equal setInterval value
}

void loop() {
  switch (state) {
    case IDLING:
      if (esp_timer_get_time() > nextScan) {
        state = SCANNING;
        DL("Scanning...");
        pBLEScan->start(scanTime, &scanCompleted);

        display.clear();
        display.setFont(ArialMT_Plain_16);
        display.setTextAlignment(TEXT_ALIGN_CENTER); // coords define center of text
        display.drawString(64, 24, "Scanning...");
        display.display();
      }
      break;
    case COMPLETED:
      DF("Scan completed. %d devices found\n\n", numDevices);
      updateScreen(numDevices);

      // write to file
      File log = SPIFFS.open(LOG, FILE_APPEND);
      if (!log) {
        DL("***ERROR*** Could not open SPIFFS file to append");
      }

      char l[5] = "";
      sprintf(l, "%u;", numDevices);
      if (!log.print(l)) {
        DL("***ERROR*** Could not append line to SPIFFS file");
      }

      log.close();

      // deep sleep
      if (!keepalive) {
        // https://github.com/espressif/arduino-esp32/blob/master/libraries/ESP32/examples/DeepSleep/TimerWakeUp/TimerWakeUp.ino
        esp_sleep_enable_timer_wakeup(INTERVALL*uS_TO_S_FACTOR);
        DF("sleep %u sec\n", INTERVALL);
        Serial.flush();
        esp_deep_sleep_start();
      }

      uint64_t now = esp_timer_get_time();
      nextScan = now+INTERVALL*uS_TO_S_FACTOR;
      // DF("current: %llu, next: %llu\n", now, nextScan);

      state = IDLING;
      break;
  }

  // https://github.com/moononournation/Arduino_BLE_Scanner/blob/master/Arduino_BLE_Scanner.ino
  if (Serial.available()) {
    char in = (char)Serial.read();
    if (in == 'k') {
      keepalive = !keepalive;
      DF("keepalive toggled (%d)\n", keepalive);
    } else if (in == 'h') {
      show_help();
    } else if (in == 's') {
      state = IDLING;
      nextScan = 0;
    } else if (in == 'l') {
      File log = SPIFFS.open(LOG);
      if (!log) {
        DL("***ERROR*** Unable to open SPIFFS file");
      } else {
        while (log.available()) {
          Serial.write(log.read());
        }
        DL();
      }
      log.close();
    } else if (in == 'd') {
      if ( !SPIFFS.remove(LOG)) {
        DL("***ERROR*** Unable to remove SPIFFS file");
      } else {
        DL("SPIFFS file deleted");
      }
    } else if (in == 'r') {
      ESP.restart();
    }
  }
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

  /*
  pBLEScan->clearResults();   // delete results fromBLEScan buffer to release memory

  */
}
