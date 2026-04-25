#include <ArduinoBLE.h>
#include <Wire.h>
#include <Adafruit_DRV2605.h>

const long BAUD_RATE = 115200;
BLEService navService("19B10000-E8F2-537E-4F6C-D104768A1214");
BLEByteCharacteristic navCharacteristic("19B10001-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite);
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

void loop() {
  BLEDevice central = BLE.central();
  if (central) {
    Serial.print("Connected to central: ");
    Serial.println(central.address());
    while (central.connected()) {
      delay(10); // Yield to background tasks
    }
    Serial.print("Disconnected from central: ");
    Serial.println(central.address());
  }
}
