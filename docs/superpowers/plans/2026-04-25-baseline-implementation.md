# WAYFINDER Baseline Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [x]`) syntax for tracking.

**Goal:** Initialize the Wayfinder Arduino project, establish BLE connectivity, and implement the haptic driver logic.

**Architecture:** BLE Peripheral with a single characteristic for commands. Non-blocking I2C communication with DRV2605L.

**Tech Stack:** C++ (Arduino), arduino-cli, nRF52 mbedOS, Adafruit DRV2605, ArduinoBLE.

---

### Task 1: Project Initialization

**Files:**
- Create: `Wayfinder.ino`

- [x] **Step 1: Create the base .ino file with minimal setup/loop**

```cpp
void setup() {
  Serial.begin(115200);
}

void loop() {
}
```

- [x] **Step 2: Verify project structure**

Run: `ls -R`
Expected: `Wayfinder.ino` exists.

- [x] **Step 3: Commit**

```bash
git add Wayfinder.ino
git commit -m "chore: init wayfinder project"
```

---

### Task 2: BLE Service Setup

**Files:**
- Modify: `Wayfinder.ino`

- [x] **Step 1: Define BLE Service and Characteristic**

```cpp
#include <ArduinoBLE.h>

BLEService navService("19B10000-E8F2-537E-4F6C-D104768A1214"); // Custom UUID
BLEByteCharacteristic navCharacteristic("19B10001-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite);

void setup() {
  Serial.begin(115200);
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
```

- [x] **Step 2: Compile to verify syntax**

Run: `arduino-cli compile --fqbn Seeeduino:mbed:xiaonRF52840Sense Wayfinder.ino`
Expected: Success

- [x] **Step 3: Commit**

```bash
git add Wayfinder.ino
git commit -m "feat: add BLE service and characteristic"
```

---

### Task 3: Haptic Driver Initialization

**Files:**
- Modify: `Wayfinder.ino`

- [x] **Step 1: Include Adafruit_DRV2605 and initialize in setup**

```cpp
#include <Wire.h>
#include <Adafruit_DRV2605.h>

Adafruit_DRV2605 drv;

void setup() {
  // ... previous BLE setup ...
  if (!drv.begin()) {
    Serial.println("Could not find DRV2605L");
    while (1);
  }
  drv.selectLibrary(1);
  drv.setMode(DRV2605_MODE_INTTRIG); 
}
```

- [x] **Step 2: Compile to verify library inclusion**

Run: `arduino-cli compile --fqbn Seeeduino:mbed:xiaonRF52840Sense Wayfinder.ino`
Expected: Success

- [x] **Step 3: Commit**

```bash
git add Wayfinder.ino
git commit -m "feat: init DRV2605L haptic driver"
```

---

### Task 4: Haptic Command Logic

**Files:**
- Modify: `Wayfinder.ino`

- [x] **Step 1: Implement non-blocking command processing**

```cpp
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
    while (central.connected()) {
      if (navCharacteristic.written()) {
        processCommand(navCharacteristic.value());
      }
    }
  }
}
```

- [x] **Step 2: Final Compilation**

Run: `arduino-cli compile --fqbn Seeeduino:mbed:xiaonRF52840Sense Wayfinder.ino`
Expected: Success

- [x] **Step 3: Commit**

```bash
git add Wayfinder.ino
git commit -m "feat: implement haptic command logic"
```
