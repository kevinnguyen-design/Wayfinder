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

## 📡 BLE Interface (V1 Dual-Device)

Wayfinder v1 uses a phone-hub architecture:
- Each wearable is a BLE Peripheral (`WAYFINDER-LEFT` and `WAYFINDER-RIGHT`).
- The phone controller app is a BLE Central connected to both.
- The phone translates map turn events into BLE command writes.

Navigation commands are sent as byte values to the Navigation Characteristic.

| Value | Command | Haptic Response |
| :--- | :--- | :--- |
| `0x01` | Turn Left | Double Click (ID 10) |
| `0x02` | Turn Right | Triple Click (ID 12) |
| `0x03` | Stop / Arrived | Buzz 100% (ID 47) |

**Service UUID:** `19B10010-E8F2-537E-4F6C-D104768A1214`
**Characteristic UUID:** `19B10011-E8F2-537E-4F6C-D104768A1214`

Role behavior:
- Left device executes `LEFT` and `STOP`; ignores `RIGHT`.
- Right device executes `RIGHT` and `STOP`; ignores `LEFT`.

Shared contract doc: `docs/ble_command_contract_v1.md`

## 🚀 Getting Started

### Prerequisites
- [arduino-cli](https://arduino.github.io/arduino-cli/latest/)
- Seeeduino nRF52 mbedOS Core
- `adafruit-nrfutil` available in `PATH` (required by Seeeduino mbed toolchain)
- Libraries: `Adafruit DRV2605 Library`, `ArduinoBLE`
- `rtk` command wrapper (or run commands without the `rtk` prefix if not using this Codex setup)

### Compilation
```bash
rtk arduino-cli compile --fqbn Seeeduino:mbed:xiaonRF52840Sense --build-property compiler.cpp.extra_flags="-DWAYFINDER_ROLE_LEFT=1" Wayfinder.ino
rtk arduino-cli compile --fqbn Seeeduino:mbed:xiaonRF52840Sense --build-property compiler.cpp.extra_flags="-DWAYFINDER_ROLE_RIGHT=1" Wayfinder.ino
```

The firmware intentionally fails compilation unless exactly one role macro is set to `1`.

Bring-up workflow: `docs/dual_device_bringup.md`

## 📜 Project Constraints
- **Low Power:** No blocking `delay()`. Use `millis()` scheduling for BLE polling and enter idle sleep while disconnected.
- **Minimalist:** No screens, no buttons, no visible LEDs.
- **Priority Logic:** Alerts and Vectors are never triggered simultaneously to avoid cognitive clutter.

## 🔍 Runtime Notes
- Firmware uses ArduinoBLE event handlers for command writes and calls `BLE.poll()` continuously in `loop()`.
- Unknown BLE commands are counted and logged only when `WAYFINDER_DEBUG_SERIAL=1`.
