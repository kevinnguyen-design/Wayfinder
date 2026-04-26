#include <ArduinoBLE.h>
#include <Wire.h>
#include <Adafruit_DRV2605.h>

// Command Constants
const uint8_t CMD_LEFT    = 0x01;
const uint8_t CMD_RIGHT   = 0x02;
const uint8_t CMD_ARRIVED = 0x03;

// Haptic Effect Constants (Library 1)
const uint8_t HAPTIC_DOUBLE_CLICK = 10;
const uint8_t HAPTIC_TRIPLE_CLICK = 12;
const uint8_t HAPTIC_BUZZ_100     = 47;

const long BAUD_RATE = 115200;
const uint32_t CONNECTED_POLL_INTERVAL_MS = 5;
const uint32_t IDLE_POLL_INTERVAL_MS = 50;
const char* SERVICE_UUID = "19B10000-E8F2-537E-4F6C-D104768A1214";
const char* CHAR_UUID = "19B10001-E8F2-537E-4F6C-D104768A1214";

#ifndef WAYFINDER_DEBUG_SERIAL
#define WAYFINDER_DEBUG_SERIAL 1
#endif

BLEService navService(SERVICE_UUID);
BLEByteCharacteristic navCharacteristic(CHAR_UUID, BLERead | BLEWrite);
Adafruit_DRV2605 drv;

#if WAYFINDER_DEBUG_SERIAL
uint32_t unknownCommandCount = 0;
#endif

void triggerHaptic(uint8_t effectID) {
  drv.setWaveform(0, effectID);
  drv.setWaveform(1, 0); // End of sequence
  drv.go();
}

void idleSleep() {
#if defined(ARDUINO_ARCH_MBED)
  __WFI();
#else
  yield();
#endif
}

void setup() {
  Serial.begin(BAUD_RATE);

  if (!BLE.begin()) {
    Serial.println(F("starting BLE failed!"));
    while (1);
  }
  
  BLE.setLocalName("WAYFINDER-CORE");
  BLE.setAdvertisedService(navService);
  navService.addCharacteristic(navCharacteristic);
  BLE.addService(navService);
  navCharacteristic.writeValue(0);
  BLE.advertise();

#if WAYFINDER_DEBUG_SERIAL
  Serial.println(F("BLE Peripheral Ready"));
#endif

  if (!drv.begin()) {
    Serial.println(F("Could not find DRV2605L"));
    while (1);
  }
  drv.selectLibrary(1);
  drv.setMode(DRV2605_MODE_INTTRIG); 
}

void processCommand(uint8_t cmd) {
  switch (cmd) {
    case CMD_LEFT:
      triggerHaptic(HAPTIC_DOUBLE_CLICK);
#if WAYFINDER_DEBUG_SERIAL
      Serial.println(F("Haptic: Left (Double Click)"));
#endif
      break;
    case CMD_RIGHT:
      triggerHaptic(HAPTIC_TRIPLE_CLICK);
#if WAYFINDER_DEBUG_SERIAL
      Serial.println(F("Haptic: Right (Triple Click)"));
#endif
      break;
    case CMD_ARRIVED:
      triggerHaptic(HAPTIC_BUZZ_100);
#if WAYFINDER_DEBUG_SERIAL
      Serial.println(F("Haptic: Arrived (Buzz)"));
#endif
      break;
    default:
#if WAYFINDER_DEBUG_SERIAL
      unknownCommandCount++;
      Serial.print(F("Unknown command: 0x"));
      Serial.println(cmd, HEX);
      Serial.print(F("Unknown command count: "));
      Serial.println(unknownCommandCount);
#endif
      break;
  }
}

void loop() {
  static BLEDevice central;
  static bool wasConnected = false;
  static uint32_t lastPollAtMs = 0;
  const bool connected = central && central.connected();
  const uint32_t pollIntervalMs = connected ? CONNECTED_POLL_INTERVAL_MS : IDLE_POLL_INTERVAL_MS;
  const uint32_t nowMs = millis();

  if ((uint32_t)(nowMs - lastPollAtMs) >= pollIntervalMs) {
    lastPollAtMs = nowMs;
    BLE.poll();

    if (!connected) {
      BLEDevice candidate = BLE.central();
      if (candidate) {
        central = candidate;
        wasConnected = true;
#if WAYFINDER_DEBUG_SERIAL
        Serial.print(F("Connected to central: "));
        Serial.println(central.address());
#endif
      } else if (wasConnected) {
#if WAYFINDER_DEBUG_SERIAL
        Serial.println(F("Disconnected from central"));
#endif
        central = BLEDevice();
        wasConnected = false;
      }
    }
  }

  // Process data if connected
  if (central && central.connected() && navCharacteristic.written()) {
    processCommand(navCharacteristic.value());
  }

  if (!central || !central.connected()) {
    idleSleep();
  }
}
