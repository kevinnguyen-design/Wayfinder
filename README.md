# WAYFINDER: The Map You Can Feel

WAYFINDER is a mass-market, haptic wearable ecosystem designed for urban pedestrian navigation. It shifts the cognitive load of navigation from the eyes (screens) to the peripheral nervous system (tactile feedback).

## 🧭 Core Philosophy: "Split-Cognition"
Traditional navigation requires constant visual attention. WAYFINDER uses haptic pulses to guide users, allowing them to keep their eyes on their surroundings while maintaining perfect directional awareness.

## 🛠 Hardware Ecosystem

### The Universal Core
A tiny, waterproof, magnetic module containing the "Brain":
- **MCU:** Seeed Studio XIAO nRF52840 Sense
- **Haptic Driver:** TI DRV2605L (I2C)
- **Actuators:** ERM Coin Vibration Motors
- **Power:** 3.7V 100mAh LiPo (Native charge management)
- **Comms:** Bluetooth Low Energy (BLE)

### Physical Carriers
1. **Kinetic Sleeve (Armband):** For "grab-and-go" convenience. Handles **Alerts** (approaching turns, missed routes).
2. **3/4 Drop-In (Insole):** For heavy commuters. Channels haptics via bone conduction. Handles **Vectors** (continuous direction).

## 📡 BLE Interface

The device acts as a BLE Peripheral. Navigation commands are sent as byte values to the Navigation Characteristic.

| Value | Command | Haptic Response |
| :--- | :--- | :--- |
| `0x01` | Turn Left | Sharp Click (ID 11) |
| `0x02` | Turn Right | Double Sharp Click |
| `0x03` | Arrived | Long Buzz (ID 15) |

**Service UUID:** `19B10000-E8F2-537E-4F6C-D104768A1214`  
**Characteristic UUID:** `19B10001-E8F2-537E-4F6C-D104768A1214`

## 🚀 Getting Started

### Prerequisites
- [arduino-cli](https://arduino.github.io/arduino-cli/latest/)
- Seeeduino nRF52 mbedOS Core
- Libraries: `Adafruit DRV2605 Library`, `ArduinoBLE`

### Compilation
```bash
arduino-cli compile --fqbn Seeeduino:mbed:xiaonRF52840Sense Wayfinder.ino
```

## 📜 Project Constraints
- **Low Power:** No `delay()`. Uses non-blocking `millis()` and `delay(10)` yields for mbedOS.
- **Minimalist:** No screens, no buttons, no visible LEDs.
- **Priority Logic:** Alerts and Vectors are never triggered simultaneously to avoid cognitive clutter.
