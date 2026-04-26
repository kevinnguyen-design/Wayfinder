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
const char* SERVICE_UUID = "19B10000-E8F2-537E-4F6C-D104768A1214";
const char* CHAR_UUID = "19B10001-E8F2-537E-4F6C-D104768A1214";

BLEService navService(SERVICE_UUID);
BLEByteCharacteristic navCharacteristic(CHAR_UUID, BLERead | BLEWrite);
Adafruit_DRV2605 drv;

void triggerHaptic(uint8_t effectID) {
  drv.setWaveform(0, effectID);
  drv.setWaveform(1, 0); // End of sequence
  drv.go();
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
  
  Serial.println(F("BLE Peripheral Ready"));

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
      Serial.println(F("Haptic: Left (Double Click)"));
      break;
    case CMD_RIGHT:
      triggerHaptic(HAPTIC_TRIPLE_CLICK);
      Serial.println(F("Haptic: Right (Triple Click)"));
      break;
    case CMD_ARRIVED:
      triggerHaptic(HAPTIC_BUZZ_100);
      Serial.println(F("Haptic: Arrived (Buzz)"));
      break;
    default:
      break;
  }
}

void loop() {
  static BLEDevice central;
  
  // Handle connection state transitions
  if (!central || !central.connected()) {
    central = BLE.central();
    if (central) {
      Serial.print(F("Connected to central: "));
      Serial.println(central.address());
    }
  }

  // Process data if connected
  if (central && central.connected()) {
    if (navCharacteristic.written()) {
      processCommand(navCharacteristic.value());
    }
  }
  
  // Add a small delay to avoid excessive CPU usage
  delay(1);
}
