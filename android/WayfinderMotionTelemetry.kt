package com.wayfinder.controller

enum class WayfinderDeviceRole {
  UNKNOWN,
  LEFT,
  RIGHT
}

data class WayfinderMotionTelemetry(
  val protocolVersion: Int,
  val role: WayfinderDeviceRole,
  val motionState: WearableMotionState,
  val stepDelta: Int,
  val yawDeltaRaw: Int,
  val flags: Int
) {
  val imuReadError: Boolean
    get() = flags and 0x01 != 0

  val imuUnavailable: Boolean
    get() = flags and 0x02 != 0
}

object WayfinderMotionTelemetryDecoder {
  fun decode(payload: ByteArray): WayfinderMotionTelemetry? {
    if (payload.size < 8) {
      return null
    }

    val yawDeltaRaw = (payload[4].toInt() and 0xFF) or ((payload[5].toInt() and 0xFF) shl 8)
    return WayfinderMotionTelemetry(
      protocolVersion = payload[0].toInt() and 0xFF,
      role = payload[1].toRole(),
      motionState = payload[2].toMotionState(),
      stepDelta = payload[3].toInt() and 0xFF,
      yawDeltaRaw = yawDeltaRaw.toShort().toInt(),
      flags = payload[6].toInt() and 0xFF
    )
  }

  private fun Byte.toRole(): WayfinderDeviceRole =
    when (toInt() and 0xFF) {
      1 -> WayfinderDeviceRole.LEFT
      2 -> WayfinderDeviceRole.RIGHT
      else -> WayfinderDeviceRole.UNKNOWN
    }

  private fun Byte.toMotionState(): WearableMotionState =
    when (toInt() and 0xFF) {
      1 -> WearableMotionState.IDLE
      2 -> WearableMotionState.MOVING
      else -> WearableMotionState.UNKNOWN
    }
}
