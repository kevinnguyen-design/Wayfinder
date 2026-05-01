#include <ArduinoBLE.h>
#include <Wire.h>
#include <Adafruit_DRV2605.h>

BLEService navService("19B10010-E8F2-537E-4F6C-D104768A1214");
BLEByteCharacteristic navCharacteristic("19B10011-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite);

Adafruit_DRV2605 drv;

const uint8_t CMD_LEFT = 0x01;
const uint8_t CMD_RIGHT = 0x02;
const uint8_t CMD_ARRIVED = 0x03;

const uint8_t HAPTIC_LEFT = 10;
const uint8_t HAPTIC_RIGHT = 12;
const uint8_t HAPTIC_ARRIVED = 47;

void triggerHaptic(uint8_t effectId) {
  drv.setWaveform(0, effectId);
  drv.setWaveform(1, 0);
  drv.go();
}

void processCommand(uint8_t cmd) {
  switch (cmd) {
    case CMD_LEFT:
      triggerHaptic(HAPTIC_LEFT);
      break;
    case CMD_RIGHT:
      triggerHaptic(HAPTIC_RIGHT);
      break;
    case CMD_ARRIVED:
      triggerHaptic(HAPTIC_ARRIVED);
      break;
    default:
      break;
  }
}

void setup() {
  Serial.begin(9600);

  if (!drv.begin()) {
    while (1) {
    }
  }
  drv.selectLibrary(1);
  drv.setMode(DRV2605_MODE_INTTRIG);

  if (!BLE.begin()) {
    while (1) {
    }
  }

  BLE.setDeviceName("WAYFINDER-CORE-V2");
  BLE.setLocalName("WAYFINDER-CORE-V2");
  BLE.setAdvertisedService(navService);

  navService.addCharacteristic(navCharacteristic);
  BLE.addService(navService);

  navCharacteristic.writeValue((byte)0);
  BLE.advertise();
}

void loop() {
  BLEDevice central = BLE.central();

  if (central) {
    while (central.connected()) {
      if (navCharacteristic.written()) {
        processCommand(navCharacteristic.value());
      }
    }
  }
}
