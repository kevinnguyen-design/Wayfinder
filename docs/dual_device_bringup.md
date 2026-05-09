# Dual Device Bring-Up (v1)

## 1. Flash left firmware build
Compile and upload with left role flag in the same command:

```bash
rtk arduino-cli compile --upload -p <LEFT_PORT> --fqbn Seeeduino:mbed:xiaonRF52840Sense --build-property compiler.cpp.extra_flags="-DWAYFINDER_ROLE_LEFT=1" Wayfinder.ino
```

## 2. Flash right firmware build
Compile and upload with right role flag in the same command:

```bash
rtk arduino-cli compile --upload -p <RIGHT_PORT> --fqbn Seeeduino:mbed:xiaonRF52840Sense --build-property compiler.cpp.extra_flags="-DWAYFINDER_ROLE_RIGHT=1" Wayfinder.ino
```

The firmware will not compile unless exactly one role macro is set to `1`.

## 3. Verify advertisement identity
- Confirm one device advertises as `WAYFINDER-LEFT`.
- Confirm the other advertises as `WAYFINDER-RIGHT`.
- Confirm both expose:
  - Service UUID `19B10010-E8F2-537E-4F6C-D104768A1214`
  - Characteristic UUID `19B10011-E8F2-537E-4F6C-D104768A1214`

## 4. Verify role behavior (bench)
- Send `0x01` (`LEFT`) to both devices:
  - Left should vibrate.
  - Right should ignore.
- Send `0x02` (`RIGHT`) to both devices:
  - Right should vibrate.
  - Left should ignore.
- Send `0x03` (`STOP`) to both devices:
  - Both should vibrate.

## 5. Verify reconnect behavior (controller)
- Start controller and connect to both devices.
- Power-cycle one device.
- Confirm controller reconnects and resumes writes to recovered side.
- Confirm controller continues issuing cues to still-connected side while reconnecting.
