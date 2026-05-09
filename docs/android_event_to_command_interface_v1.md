# Android Event-to-Command Interface (v1)

## Turn Event Schema
```kotlin
data class TurnEvent(
  val type: TurnEventType, // LEFT, RIGHT, STOP
  val eventId: String,     // dedupe key from navigation source
  val eventTimestampMs: Long
)
```

## Translation Rules
- `LEFT` -> write `0x01` to left peripheral only.
- `RIGHT` -> write `0x02` to right peripheral only.
- `STOP` -> write `0x03` to both peripherals in same dispatch cycle.

## Deduping
- Ignore repeated events with same `(eventId, type)` for 30 seconds.
- Record an event as dispatched only after at least one target device accepts it for BLE dispatch.
- Keep one cue per event window to avoid repeated vibration spam.

## BLE Write Queue
- Serialize writes per device and wait for `onCharacteristicWrite` before sending the next queued command.
- Drop stale turn writes after 5 seconds.
- Drop stale stop writes after 15 seconds.
- When `STOP` is queued, discard older queued turn commands for that device.

## Required Logging Stages
- `EVENT`: turn event received from navigation stream.
- `WRITE`: BLE write attempted.
- `ACK`: BLE stack write callback received.
- `ERROR`: write failure or missing characteristic.
- `DISCONNECT`: peripheral disconnected.
- `DROP`: stale queued command expired.
- `DEDUP`: repeated turn event ignored.
- `SYNC`: stop dispatch timing delta (left+right dispatch window).

## Reference Implementation
- Kotlin skeleton: `android/WayfinderControllerV1.kt`
