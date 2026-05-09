# Wayfinder BLE Command Contract v1

This document is the single source of truth for command values and BLE UUIDs shared by firmware and controller apps.

## Transport
- Peripheral role: each Wayfinder device is a BLE peripheral.
- Central role: phone app is the BLE central and connects to both devices.

## Device Roles
- Left peripheral advertises as `WAYFINDER-LEFT`.
- Right peripheral advertises as `WAYFINDER-RIGHT`.

## UUIDs
- Service UUID: `19B10010-E8F2-537E-4F6C-D104768A1214`
- Characteristic UUID: `19B10011-E8F2-537E-4F6C-D104768A1214`
- Characteristic properties: `READ | WRITE`

## Command Table
| Byte | Command | Left Device | Right Device | DRV Effect |
| --- | --- | --- | --- | --- |
| `0x01` | `LEFT` | Execute | Ignore | `10` |
| `0x02` | `RIGHT` | Ignore | Execute | `12` |
| `0x03` | `STOP` (arrived/stop cue) | Execute | Execute | `47` |

## Controller Dispatch Rules
- Upcoming left turn: send `0x01` to left device only.
- Upcoming right turn: send `0x02` to right device only.
- Arrived/stop: send `0x03` to both devices in the same dispatch cycle.

## Reliability Expectations
- Controller logs: event received, BLE write attempt, BLE write ack/error, and send timestamp.
- If one side disconnects, continue navigation with connected side and raise operator warning.
- v1 commands are one byte and do not include a sequence number. Protocol-level dedupe requires a future v1.1 contract update.
