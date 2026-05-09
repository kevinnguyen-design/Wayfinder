#include <ArduinoBLE.h>
#include <Wire.h>
#include <Adafruit_DRV2605.h>

// Role config:
// Compile with -DWAYFINDER_ROLE_LEFT=1 or -DWAYFINDER_ROLE_RIGHT=1.
#ifndef WAYFINDER_ROLE_LEFT
#define WAYFINDER_ROLE_LEFT 0
#endif

#ifndef WAYFINDER_ROLE_RIGHT
#define WAYFINDER_ROLE_RIGHT 0
#endif

#if (WAYFINDER_ROLE_LEFT + WAYFINDER_ROLE_RIGHT) != 1
#error "Exactly one role must be enabled with -DWAYFINDER_ROLE_LEFT=1 or -DWAYFINDER_ROLE_RIGHT=1"
#endif

#if WAYFINDER_ROLE_LEFT
const char *kDeviceName = "WAYFINDER-LEFT";
#else
const char *kDeviceName = "WAYFINDER-RIGHT";
#endif

// BLE contract v1
const char *kNavServiceUuid = "19B10010-E8F2-537E-4F6C-D104768A1214";
const char *kNavCharacteristicUuid = "19B10011-E8F2-537E-4F6C-D104768A1214";

BLEService navService(kNavServiceUuid);
BLEByteCharacteristic navCharacteristic(kNavCharacteristicUuid, BLERead | BLEWrite);

Adafruit_DRV2605 drv;

const uint8_t CMD_LEFT = 0x01;
const uint8_t CMD_RIGHT = 0x02;
const uint8_t CMD_STOP = 0x03;

const uint8_t HAPTIC_LEFT = 10;
const uint8_t HAPTIC_RIGHT = 12;
const uint8_t HAPTIC_STOP = 47;

#ifndef WAYFINDER_DEBUG_SERIAL
#define WAYFINDER_DEBUG_SERIAL 0
#endif

struct CommandCounters {
  uint32_t left;
  uint32_t right;
  uint32_t stop;
  uint32_t ignored;
};

CommandCounters counters = {0, 0, 0, 0};

void debugPrintCounters() {
#if WAYFINDER_DEBUG_SERIAL
  static unsigned long lastPrintMs = 0;
  const unsigned long now = millis();
  if (now - lastPrintMs < 1000) {
    return;
  }

  lastPrintMs = now;
  Serial.print("counters left=");
  Serial.print(counters.left);
  Serial.print(" right=");
  Serial.print(counters.right);
  Serial.print(" stop=");
  Serial.print(counters.stop);
  Serial.print(" ignored=");
  Serial.println(counters.ignored);
#endif
}

void triggerHaptic(uint8_t effectId) {
  drv.setWaveform(0, effectId);
  drv.setWaveform(1, 0);
  drv.go();
}

bool shouldExecuteCommand(uint8_t cmd) {
#if WAYFINDER_ROLE_LEFT
  return cmd == CMD_LEFT || cmd == CMD_STOP;
#else
  return cmd == CMD_RIGHT || cmd == CMD_STOP;
#endif
}

void processCommand(uint8_t cmd) {
  if (!shouldExecuteCommand(cmd)) {
    counters.ignored++;
    return;
  }

  switch (cmd) {
    case CMD_LEFT:
      counters.left++;
      triggerHaptic(HAPTIC_LEFT);
      break;
    case CMD_RIGHT:
      counters.right++;
      triggerHaptic(HAPTIC_RIGHT);
      break;
    case CMD_STOP:
      counters.stop++;
      triggerHaptic(HAPTIC_STOP);
      break;
    default:
      counters.ignored++;
      break;
  }
}

void handleCommandWritten(BLEDevice central, BLECharacteristic characteristic) {
  (void)central;
  if (characteristic.valueLength() < 1) {
    counters.ignored++;
    return;
  }
  processCommand(characteristic.value()[0]);
}

void haltWithDebugMessage(const char *message) {
#if WAYFINDER_DEBUG_SERIAL
  Serial.println(message);
#else
  (void)message;
#endif
  while (1) {
  }
}

void setup() {
#if WAYFINDER_DEBUG_SERIAL
  Serial.begin(115200);
#endif

  if (!drv.begin()) {
    haltWithDebugMessage("DRV2605 init failed");
  }
  drv.selectLibrary(1);
  drv.setMode(DRV2605_MODE_INTTRIG);

  if (!BLE.begin()) {
    haltWithDebugMessage("BLE init failed");
  }

  BLE.setDeviceName(kDeviceName);
  BLE.setLocalName(kDeviceName);
  BLE.setAdvertisedService(navService);

  navService.addCharacteristic(navCharacteristic);
  BLE.addService(navService);

  navCharacteristic.setEventHandler(BLEWritten, handleCommandWritten);
  navCharacteristic.writeValue((byte)0);
  BLE.advertise();
}

void loop() {
  BLE.poll();
  debugPrintCounters();
}
