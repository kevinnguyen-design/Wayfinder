#include <ArduinoBLE.h>

const long BAUD_RATE = 115200;
BLEService navService("19B10000-E8F2-537E-4F6C-D104768A1214");
BLEByteCharacteristic navCharacteristic("19B10001-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite);

void setup() {
  Serial.begin(BAUD_RATE);
  while (!Serial); // Optional: wait for serial port to connect

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
}

void loop() {
  BLEDevice central = BLE.central();
  if (central) {
    Serial.print("Connected to central: ");
    Serial.println(central.address());
    while (central.connected()) {
      // Loop here
    }
    Serial.print("Disconnected from central: ");
    Serial.println(central.address());
  }
}
