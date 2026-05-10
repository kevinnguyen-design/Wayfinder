#include <ArduinoBLE.h>
#include <Wire.h>
#include <Adafruit_DRV2605.h>

// Role config:
// Compile with -DWAYFINDER_ROLE_LEFT=1 or -DWAYFINDER_ROLE_RIGHT=1.
#ifndef WAYFINDER_ROLE_LEFT
#define WAYFINDER_ROLE_LEFT 0
#endif

#ifndef WAYFINDER_ROLE_RIGHT
#define WAYFINDER_ROLE_RIGHT 0
#endif

#if (WAYFINDER_ROLE_LEFT + WAYFINDER_ROLE_RIGHT) != 1
#error "Exactly one role must be enabled with -DWAYFINDER_ROLE_LEFT=1 or -DWAYFINDER_ROLE_RIGHT=1"
#endif

#if WAYFINDER_ROLE_LEFT
const char *kDeviceName = "WAYFINDER-LEFT";
#else
const char *kDeviceName = "WAYFINDER-RIGHT";
#endif

// BLE contract v1
const char *kNavServiceUuid = "19B10010-E8F2-537E-4F6C-D104768A1214";
const char *kNavCharacteristicUuid = "19B10011-E8F2-537E-4F6C-D104768A1214";
const char *kMotionTelemetryCharacteristicUuid = "19B10012-E8F2-537E-4F6C-D104768A1214";

BLEService navService(kNavServiceUuid);
BLEByteCharacteristic navCharacteristic(kNavCharacteristicUuid, BLERead | BLEWrite);
BLECharacteristic motionTelemetryCharacteristic(kMotionTelemetryCharacteristicUuid, BLENotify, 8);

Adafruit_DRV2605 drv;

const uint8_t CMD_LEFT = 0x01;
const uint8_t CMD_RIGHT = 0x02;
const uint8_t CMD_STOP = 0x03;

const uint8_t HAPTIC_LEFT = 10;
const uint8_t HAPTIC_RIGHT = 12;
const uint8_t HAPTIC_STOP = 47;
const uint8_t IMU_ADDR_PRIMARY = 0x6A;
const uint8_t IMU_ADDR_FALLBACK = 0x6B;
const uint8_t IMU_REG_WHO_AM_I = 0x0F;
const uint8_t IMU_REG_CTRL1_XL = 0x10;
const uint8_t IMU_REG_CTRL2_G = 0x11;
const uint8_t IMU_REG_OUTX_L_G = 0x22;
const uint8_t IMU_REG_OUTX_L_XL = 0x28;
const uint8_t IMU_WHO_AM_I_EXPECTED = 0x69;
const uint8_t MOTION_STATE_UNKNOWN = 0;
const uint8_t MOTION_STATE_IDLE = 1;
const uint8_t MOTION_STATE_MOVING = 2;
const unsigned long kTelemetryIntervalMs = 250;
const int32_t kMotionDeltaThreshold = 5500;
const int16_t kYawDeadband = 80;

#ifndef WAYFINDER_DEBUG_SERIAL
#define WAYFINDER_DEBUG_SERIAL 0
#endif

struct CommandCounters {
  uint32_t left;
  uint32_t right;
  uint32_t stop;
  uint32_t ignored;
};

CommandCounters counters = {0, 0, 0, 0};
bool imuAvailable = false;
uint8_t imuAddress = IMU_ADDR_PRIMARY;
uint8_t currentMotionState = MOTION_STATE_UNKNOWN;
int32_t lastAccelMagnitude = 0;
unsigned long lastTelemetryMs = 0;
bool hasAccelBaseline = false;

void debugPrintCounters() {
#if WAYFINDER_DEBUG_SERIAL
  static unsigned long lastPrintMs = 0;
  const unsigned long now = millis();
  if (now - lastPrintMs < 1000) {
    return;
  }

  lastPrintMs = now;
  Serial.print("counters left=");
  Serial.print(counters.left);
  Serial.print(" right=");
  Serial.print(counters.right);
  Serial.print(" stop=");
  Serial.print(counters.stop);
  Serial.print(" ignored=");
  Serial.println(counters.ignored);
#endif
}

void triggerHaptic(uint8_t effectId) {
  drv.setWaveform(0, effectId);
  drv.setWaveform(1, 0);
  drv.go();
}

bool imuWriteRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(imuAddress);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

bool imuReadRegisters(uint8_t reg, uint8_t *buffer, uint8_t length) {
  Wire.beginTransmission(imuAddress);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }

  const uint8_t bytesRead = Wire.requestFrom(imuAddress, length);
  if (bytesRead != length) {
    return false;
  }

  for (uint8_t i = 0; i < length; i++) {
    buffer[i] = Wire.read();
  }
  return true;
}

bool imuProbeAddress(uint8_t address) {
  imuAddress = address;
  uint8_t whoAmI = 0;
  return imuReadRegisters(IMU_REG_WHO_AM_I, &whoAmI, 1) && whoAmI == IMU_WHO_AM_I_EXPECTED;
}

bool initImu() {
  if (!imuProbeAddress(IMU_ADDR_PRIMARY) && !imuProbeAddress(IMU_ADDR_FALLBACK)) {
    return false;
  }

  // LSM6DS3 family: 104 Hz accelerometer and gyro, enough for motion confidence telemetry.
  return imuWriteRegister(IMU_REG_CTRL1_XL, 0x40) && imuWriteRegister(IMU_REG_CTRL2_G, 0x40);
}

int16_t readInt16LE(const uint8_t *buffer, uint8_t offset) {
  return (int16_t)(buffer[offset] | (buffer[offset + 1] << 8));
}

void updateMotionTelemetry() {
  const unsigned long now = millis();
  if (now - lastTelemetryMs < kTelemetryIntervalMs) {
    return;
  }
  lastTelemetryMs = now;

  uint8_t gyroRaw[6] = {0};
  uint8_t accelRaw[6] = {0};
  uint8_t flags = 0;
  int16_t yawDelta = 0;
  uint8_t motionState = MOTION_STATE_UNKNOWN;

  if (imuAvailable && imuReadRegisters(IMU_REG_OUTX_L_G, gyroRaw, 6) &&
      imuReadRegisters(IMU_REG_OUTX_L_XL, accelRaw, 6)) {
    const int16_t gyroZ = readInt16LE(gyroRaw, 4);
    const int16_t accelX = readInt16LE(accelRaw, 0);
    const int16_t accelY = readInt16LE(accelRaw, 2);
    const int16_t accelZ = readInt16LE(accelRaw, 4);
    const int32_t accelMagnitude =
        abs((int32_t)accelX) + abs((int32_t)accelY) + abs((int32_t)accelZ);
    const int32_t accelDelta = hasAccelBaseline ? abs(accelMagnitude - lastAccelMagnitude) : 0;

    motionState =
        hasAccelBaseline && accelDelta > kMotionDeltaThreshold ? MOTION_STATE_MOVING : MOTION_STATE_IDLE;
    yawDelta = abs(gyroZ) > kYawDeadband ? gyroZ : 0;
    lastAccelMagnitude = accelMagnitude;
    hasAccelBaseline = true;
  } else if (imuAvailable) {
    flags |= 0x01;
  } else {
    flags |= 0x02;
  }

  currentMotionState = motionState;
  uint8_t payload[8] = {
      1,
#if WAYFINDER_ROLE_LEFT
      1,
#else
      2,
#endif
      motionState,
      0,
      (uint8_t)(yawDelta & 0xFF),
      (uint8_t)((yawDelta >> 8) & 0xFF),
      flags,
      0};
  motionTelemetryCharacteristic.writeValue(payload, sizeof(payload));
}

bool shouldExecuteCommand(uint8_t cmd) {
#if WAYFINDER_ROLE_LEFT
  return cmd == CMD_LEFT || cmd == CMD_STOP;
#else
  return cmd == CMD_RIGHT || cmd == CMD_STOP;
#endif
}

void processCommand(uint8_t cmd) {
  if (!shouldExecuteCommand(cmd)) {
    counters.ignored++;
    return;
  }

  switch (cmd) {
    case CMD_LEFT:
      counters.left++;
      triggerHaptic(HAPTIC_LEFT);
      break;
    case CMD_RIGHT:
      counters.right++;
      triggerHaptic(HAPTIC_RIGHT);
      break;
    case CMD_STOP:
      counters.stop++;
      triggerHaptic(HAPTIC_STOP);
      break;
    default:
      counters.ignored++;
      break;
  }
}

void handleCommandWritten(BLEDevice central, BLECharacteristic characteristic) {
  (void)central;
  if (characteristic.valueLength() < 1) {
    counters.ignored++;
    return;
  }
  processCommand(characteristic.value()[0]);
}

void haltWithDebugMessage(const char *message) {
#if WAYFINDER_DEBUG_SERIAL
  Serial.println(message);
#else
  (void)message;
#endif
  while (1) {
  }
}

void setup() {
#if WAYFINDER_DEBUG_SERIAL
  Serial.begin(115200);
#endif

  Wire.begin();

  if (!drv.begin()) {
    haltWithDebugMessage("DRV2605 init failed");
  }
  drv.selectLibrary(1);
  drv.setMode(DRV2605_MODE_INTTRIG);

  if (!BLE.begin()) {
    haltWithDebugMessage("BLE init failed");
  }

  BLE.setDeviceName(kDeviceName);
  BLE.setLocalName(kDeviceName);
  BLE.setAdvertisedService(navService);

  navService.addCharacteristic(navCharacteristic);
  navService.addCharacteristic(motionTelemetryCharacteristic);
  BLE.addService(navService);

  navCharacteristic.setEventHandler(BLEWritten, handleCommandWritten);
  navCharacteristic.writeValue((byte)0);
  uint8_t initialTelemetry[8] = {
      1,
#if WAYFINDER_ROLE_LEFT
      1,
#else
      2,
#endif
      MOTION_STATE_UNKNOWN,
      0,
      0,
      0,
      0x02,
      0};
  motionTelemetryCharacteristic.writeValue(initialTelemetry, sizeof(initialTelemetry));
  imuAvailable = initImu();
  BLE.advertise();
}

void loop() {
  BLE.poll();
  updateMotionTelemetry();
  debugPrintCounters();
}
