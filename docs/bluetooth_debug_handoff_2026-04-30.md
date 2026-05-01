# Wayfinder BLE Debug Handoff (2026-04-30)

## Current status
BLE connectivity is unstable/inconsistent on XIAO nRF52840 Sense with `Seeeduino:mbed:xiaonRF52840Sense` + `ArduinoBLE`.

## Observed failure patterns (nRF Connect)
- Case A: Connect succeeds (`status 0`), service discovery takes ~30s, then `No services found`, then disconnect `0x16` (local host terminate).
- Case B: Subsequent reconnect often fails with `0x93` (GATT connection timeout).
- Case C: In later attempts with minimal BLE sketch, immediate `0x85` (GATT error 133) before discovery.

## What was tested
1. Original BLE + DRV2605 sketch.
2. Loop/polling refactors:
- timed polling + sleep variants
- continuous `BLE.poll()` variants
- blocking and non-blocking connection handling variants
3. Identity/advertising tweaks:
- added `BLE.setDeviceName("WAYFINDER-CORE")`
- kept `BLE.setLocalName("WAYFINDER-CORE")`
4. Minimal BLE-only sketch (single custom service + byte characteristic, no DRV code).

## Key inference
The failures are not limited to app logic in Wayfinder command handling. Symptoms persisted even with minimal BLE-only firmware, pointing to stack/core/phone-session instability (or an mbed+ArduinoBLE compatibility issue on this board setup).

## Recommended BLE next steps (deferred)
1. Start from clean phone stack each run (forget device, BT off/on, single attempt).
2. Test minimal sketch with a standard SIG service (e.g., Battery Service) to compare against custom UUID service behavior.
3. If still unstable, migrate BLE implementation path:
- either `Seeeduino:nrf52` core + Bluefruit BLE stack,
- or another known-good BLE library/core combo for XIAO nRF52840 Sense.

## Hardware sanity checks already proven
- Board can run independent smoke-test firmware (LED + serial heartbeat).
- Coin motor can be driven in direct transistor-based tests.

