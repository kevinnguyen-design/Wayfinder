# Wayfinder Hardware Bring-up Checklist

## Goal
Establish a stable DRV2605 I2C link on XIAO before any BLE testing.

## Pre-check (bench, power off)
1. DRV2605 headers fully soldered (no loose/friction-only contact).
2. Confirm harness labels: `3V3`, `GND`, `SDA`, `SCL`.
3. Continuity check each path end-to-end.
4. No shorts between adjacent DRV header pins.

## Wiring (XIAO nRF52840 Sense -> DRV2605)
1. `3V3 -> VIN`
2. `GND -> GND`
3. `D4 -> SDA`
4. `D5 -> SCL`

## Firmware for this stage
Use current `Wayfinder.ino` continuous probe.

## Validate I2C stability first (motor disconnected)
1. Upload probe sketch.
2. Open monitor at `115200`.
3. Observe at least 5 consecutive cycles.

Required pass criteria:
- `I2C devices total >= 1`
- `DRV 0x5A seen: YES`
- `drv.begin(): OK`

If fail:
1. Re-seat only 4-wire I2C/power harness.
2. Recheck continuity and pin orientation.
3. Re-test 5 cycles.

## Add motor only after I2C passes
1. Connect motor to `OUT+`/`OUT-`.
2. Confirm probe prints `Triggering effect 47` and motor response.

## BLE is last
Only after stable I2C + motor response, restore BLE firmware and re-test nRF Connect.

## Notes
- Ignore BLE errors until DRV I2C is stable.
- Loose DRV headers can produce intermittent `0x5A` disappearance.
