#include <ArduinoBLE.h>
#include <Wire.h>
#include <Adafruit_DRV2605.h>

const long BAUD_RATE = 115200;
const char* SERVICE_UUID = "19B10000-E8F2-537E-4F6C-D104768A1214";
const char* CHAR_UUID = "19B10001-E8F2-537E-4F6C-D104768A1214";

BLEService navService(SERVICE_UUID);
BLEByteCharacteristic navCharacteristic(CHAR_UUID, BLERead | BLEWrite);
Adafruit_DRV2605 drv;

void setup() {
  Serial.begin(BAUD_RATE);

  if (!BLE.begin()) {
    Serial.println("starting BLE failed!");
    while (1);
  }
  
  BLE.setLocalName("WAYFINDER-CORE");
  BLE.setAdvertisedService(navService);
  navService.addCharacteristic(navCharacteristic);
  BLE.addService(navService);
  navCharacteristic.writeValue(0);
  BLE.advertise();
  
  Serial.println("BLE Peripheral Ready");

  if (!drv.begin()) {
    Serial.println("Could not find DRV2605L");
    while (1);
  }
  drv.selectLibrary(1);
  drv.setMode(DRV2605_MODE_INTTRIG); 
}

void processCommand(uint8_t cmd) {
  switch (cmd) {
    case 0x01: // Left
      drv.setWaveform(0, 11); // Sharp Click - 100%
      drv.setWaveform(1, 0);  // end waveform
      drv.go();
      Serial.println("Haptic: Left");
      break;
    case 0x02: // Right
      drv.setWaveform(0, 11);
      drv.setWaveform(1, 11);
      drv.setWaveform(2, 0);
      drv.go();
      Serial.println("Haptic: Right");
      break;
    case 0x03: // Arrived
      drv.setWaveform(0, 15); // Long buzz
      drv.setWaveform(1, 0);
      drv.go();
      Serial.println("Haptic: Arrived");
      break;
    default:
      break;
  }
}

void loop() {
  BLEDevice central = BLE.central();
  if (central) {
    Serial.print("Connected to central: ");
    Serial.println(central.address());
    while (central.connected()) {
      if (navCharacteristic.written()) {
        processCommand(navCharacteristic.value());
      }
      delay(10); // Yield to background tasks
    }
    Serial.print("Disconnected from central: ");
    Serial.println(central.address());
  }
}
