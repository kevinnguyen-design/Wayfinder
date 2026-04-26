# WAYFINDER Phase 2 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [x]`) syntax for tracking.

**Goal:** Refine the BLE integration and haptic mapping according to WAYFINDER_STATE_V2 specifications.

**Architecture:** BLE Peripheral with refined command-to-waveform mappings and strict non-blocking logic.

**Tech Stack:** C++ (Arduino), ArduinoBLE, Adafruit DRV2605.

---

### Task 1: Refine Haptic Mappings & Cleanup

**Files:**
- Modify: `Wayfinder.ino`

- [x] **Step 1: Update processCommand with Phase 2 mappings**

```cpp
void processCommand(uint8_t cmd) {
  switch (cmd) {
    case 0x01: // Left -> Double Click (ID 10)
      drv.setWaveform(0, 10);
      drv.setWaveform(1, 0);
      drv.go();
      Serial.println("Haptic: Left (Double Click)");
      break;
    case 0x02: // Right -> Triple Click (ID 12)
      drv.setWaveform(0, 12);
      drv.setWaveform(1, 0);
      drv.go();
      Serial.println("Haptic: Right (Triple Click)");
      break;
    case 0x03: // Arrived -> Buzz 100% (ID 47)
      drv.setWaveform(0, 47);
      drv.setWaveform(1, 0);
      drv.go();
      Serial.println("Haptic: Arrived (Buzz)");
      break;
    default:
      break;
  }
}
```

- [x] **Step 2: Remove redundant serial prints and ensure non-blocking**

Check that timing uses `millis()` scheduling, no blocking `delay()`, and `Serial` usage is minimal.

- [x] **Step 3: Verify Compilation**

Run: `arduino-cli compile --fqbn Seeeduino:mbed:xiaonRF52840Sense Wayfinder.ino`
Expected: Success

- [x] **Step 4: Commit**

```bash
git add Wayfinder.ino
git commit -m "feat: update haptic mappings to Phase 2 specs"
```

---

### Task 2: Final Verification & Readme Update

**Files:**
- Modify: `README.md`
- Modify: `TODO.md`

- [x] **Step 1: Update README with Phase 2 Mappings**

Ensure the table in README matches:
- 0x01 -> Double Click (ID 10)
- 0x02 -> Triple Click (ID 12)
- 0x03 -> Buzz 100% (ID 47)

- [x] **Step 2: Update TODO.md**

Add Phase 2 status.

- [x] **Step 3: Commit**

```bash
git add README.md TODO.md
git commit -m "docs: sync readme with phase 2 mappings"
```
