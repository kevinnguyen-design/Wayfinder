# Review Notes — Phase 2 Handoff

Date: 2026-05-01
Reviewer: codex
Phase: Dual-device bring-up complete, production-ready testing pending

Status: Superseded by the dual-device hardening implementation.

Implemented follow-up:
- Firmware now requires exactly one role macro and uses ArduinoBLE `BLEWritten` callbacks.
- Android controller now serializes per-device GATT writes, uses the API 33+ write path, supports lifecycle shutdown, closes stale GATTs before reconnect, and uses TTL-based `(eventId, type)` dedupe.
- Bring-up docs now use compile-and-upload commands that keep role flags attached to the uploaded build.

Remaining validation:
- Run bench tests on both physical devices.
- Verify Android BLE permissions and write behavior inside a real Android project.
- Measure STOP dispatch skew across repeated field trials.

---

## Commendations

- Clean dual-device architecture with compile-time role separation.
- Consistent BLE UUIDs across firmware and Android (`docs/ble_command_contract_v1.centralized.md` and `Wayfinder.ino`).
- Proper non-blocking loop in `Wayfinder.ino` (no `delay()`, uses `millis()`).
- Deduplication guard in controller prevents double-dispatch (`WayfinderControllerV1.kt:56`).
- Good auto-reconnect pattern in Android.

---

## Action Items

### Priority 1 — Stability

| # | File | Issue |
| :--- | :--- | :--- |
| 1 | `WayfinderControllerV1.kt:108` | **Reconnect storm.** `onConnectionStateChange` triggers `reconnect()` immediately on every disconnect. Rapid connect/disconnect cycles drain battery and may flood the BLE stack. Add exponential backoff (e.g., initial delay 1s, doubling, max 30s). |
| 2 | `Wayfinder.ino:138-148` | **Reconnection polling gap.** The outer `if (central)` guard prevents BLE polling when disconnected. Add explicit `BLE.central()` check or `BLE.poll()` outside the connected block so the device can accept a new central without a full power cycle. |
| 3 | `WayfinderControllerV1.kt:169-172` | **Immediate retry loop.** `reconnect()` calls `connect()` synchronously. Without backoff this is equivalent to issue #1. Implement a delay loop before reconnect. |

### Priority 2 — Correctness

| # | File | Issue |
| :--- | :--- | :--- |
| 4 | `WayfinderControllerV1.kt:63-67` | **Sequential STOP dispatch.** `leftLink?.send()` and `rightLink?.send()` execute sequentially. If timing alignment matters (e.g., both devices should vibrate at the same instant), dispatch both concurrently using separate coroutines or a dedicated dispatch thread. |
| 5 | `Wayfinder.ino:85-108` | **No command deduplication.** Controller has dedup at line 56, but firmware has no guard. If a stale `TurnEvent` replays on reconnect (e.g., controller crash mid-route), the same command may execute twice. Add a simple debounce or sequence number to the protocol. |
| 6 | `Wayfinder.ino:139-148` | **Inner loop polling.** While connected, `central.connected()` is checked every iteration with no yield. On very short loops this may cause excessive CPU. Consider adding a small `delay(5)` or using `BLE.poll()` with a fixed interval. |

### Priority 3 — Observability / Docs

| # | File | Issue |
| :--- | :--- | :--- |
| 7 | `README.centralized.md:69` | **Stale polling spec.** Documents "polls BLE on `millis()` intervals (`5ms` connected, `50ms` disconnected)" but these values are not present in the current code. Either update the spec or add the intervals to `Wayfinder.ino`. |
| 8 | `README.centralized.md:56` | **Assumes `rtk` wrapper.** The build commands use `rtk arduino-cli`, but `rtk` is not defined in the docs. Add a note, alias definition, or `.codex/` hint. |
| 9 | General | **No heartbeat/keepalive.** If neither device receives a write for ~10s while connected, some BLE stacks drop the link silently. Add a periodic no-op write from the controller (e.g., every 5s if idle) to maintain the connection. |

---

## Protocol Extension (future)

If command debounce (#5) is implemented, consider a lightweight sequence number:

```
Byte 0: Command (0x01 LEFT, 0x02 RIGHT, 0x03 STOP)
Byte 1: Sequence number (0x00–0xFF, controller increments per turn event)
```

Firmware ignores a command if its sequence matches the last seen. Controller persists last sequence across reconnects.

---

## Open Questions

1. Is the STOP command timing (#4) actually perceptible by the user? If not, sequential dispatch is acceptable.
2. What is the expected reconnect behavior after a full power cycle—auto-reconnect to the last paired central, or manual pairing each time?
3. Is battery life a shipped constraint or a prototype concern? Determines urgency of #1 and #3.
