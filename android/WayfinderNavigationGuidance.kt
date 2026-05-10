package com.wayfinder.controller

import android.os.SystemClock
import kotlin.math.abs
import kotlin.math.max

enum class NavigationState {
  EN_ROUTE,
  ARRIVED,
  OFF_ROUTE,
  REROUTING,
  PAUSED,
  UNKNOWN
}

enum class GuidanceManeuver {
  LEFT,
  RIGHT,
  STRAIGHT,
  SLIGHT_LEFT,
  SLIGHT_RIGHT,
  SHARP_LEFT,
  SHARP_RIGHT,
  U_TURN_LEFT,
  U_TURN_RIGHT,
  FORK_LEFT,
  FORK_RIGHT,
  ROUNDABOUT_LEFT,
  ROUNDABOUT_RIGHT,
  ARRIVE,
  UNKNOWN
}

enum class WearableMotionState {
  UNKNOWN,
  IDLE,
  MOVING
}

data class RouteRequest(
  val originLatitude: Double? = null,
  val originLongitude: Double? = null,
  val destinationLatitude: Double,
  val destinationLongitude: Double,
  val routeId: String? = null
)

data class GuidanceUpdate(
  val routeVersion: String,
  val maneuverId: String,
  val maneuver: GuidanceManeuver,
  val distanceToManeuverMeters: Float,
  val locationAccuracyMeters: Float?,
  val speedMetersPerSecond: Float?,
  val navigationState: NavigationState,
  val wearableMotionState: WearableMotionState = WearableMotionState.UNKNOWN,
  val timestampMs: Long = SystemClock.elapsedRealtime()
)

data class CueEngineConfig(
  val baseCueDistanceMeters: Float = 10f,
  val secondsAhead: Float = 4f,
  val minimumCueDistanceMeters: Float = 8f,
  val maxLocationAccuracyMeters: Float = 20f,
  val stationaryHoldMs: Long = 3_000L
)

interface NavigationGuidanceSource {
  fun start(routeRequest: RouteRequest)
  fun stop()
  fun observeGuidance(callback: (GuidanceUpdate) -> Unit)
}

class WayfinderCueEngine(
  private val config: CueEngineConfig = CueEngineConfig()
) {
  private var activeRouteVersion: String? = null
  private val cuedManeuvers = mutableSetOf<String>()
  private var stationarySinceMs: Long? = null

  fun evaluate(update: GuidanceUpdate): TurnEvent? {
    if (activeRouteVersion != update.routeVersion) {
      activeRouteVersion = update.routeVersion
      cuedManeuvers.clear()
      stationarySinceMs = null
    }

    if (update.navigationState == NavigationState.ARRIVED || update.maneuver == GuidanceManeuver.ARRIVE) {
      return cueOnce(update, TurnEventType.STOP)
    }

    if (update.navigationState != NavigationState.EN_ROUTE) {
      return null
    }

    val accuracy = update.locationAccuracyMeters
    if (accuracy != null && accuracy > config.maxLocationAccuracyMeters) {
      return null
    }

    if (isStationaryTooLong(update)) {
      return null
    }

    val cueType = update.maneuver.toTurnEventType() ?: return null
    if (update.distanceToManeuverMeters > cueThresholdMeters(update)) {
      return null
    }

    return cueOnce(update, cueType)
  }

  private fun cueOnce(update: GuidanceUpdate, type: TurnEventType): TurnEvent? {
    val cueKey = "${update.routeVersion}:${update.maneuverId}:$type"
    if (cueKey in cuedManeuvers) {
      return null
    }

    cuedManeuvers.add(cueKey)
    return TurnEvent(type = type, eventId = cueKey, eventTimestampMs = update.timestampMs)
  }

  private fun isStationaryTooLong(update: GuidanceUpdate): Boolean {
    if (update.wearableMotionState != WearableMotionState.IDLE) {
      stationarySinceMs = null
      return false
    }

    val since = stationarySinceMs ?: update.timestampMs.also { stationarySinceMs = it }
    return update.timestampMs - since >= config.stationaryHoldMs
  }

  private fun cueThresholdMeters(update: GuidanceUpdate): Float {
    val speed = update.speedMetersPerSecond ?: 0f
    return max(config.minimumCueDistanceMeters, max(config.baseCueDistanceMeters, speed * config.secondsAhead))
  }

  private fun GuidanceManeuver.toTurnEventType(): TurnEventType? =
    when (this) {
      GuidanceManeuver.LEFT,
      GuidanceManeuver.SLIGHT_LEFT,
      GuidanceManeuver.SHARP_LEFT,
      GuidanceManeuver.U_TURN_LEFT,
      GuidanceManeuver.FORK_LEFT,
      GuidanceManeuver.ROUNDABOUT_LEFT -> TurnEventType.LEFT
      GuidanceManeuver.RIGHT,
      GuidanceManeuver.SLIGHT_RIGHT,
      GuidanceManeuver.SHARP_RIGHT,
      GuidanceManeuver.U_TURN_RIGHT,
      GuidanceManeuver.FORK_RIGHT,
      GuidanceManeuver.ROUNDABOUT_RIGHT -> TurnEventType.RIGHT
      GuidanceManeuver.STRAIGHT,
      GuidanceManeuver.UNKNOWN,
      GuidanceManeuver.ARRIVE -> null
    }
}

class WayfinderNavigationSession(
  private val guidanceSource: NavigationGuidanceSource,
  private val cueEngine: WayfinderCueEngine,
  private val controller: WayfinderControllerV1,
  private val log: (WayfinderLogEvent) -> Unit = {}
) {
  fun start(routeRequest: RouteRequest) {
    guidanceSource.observeGuidance { update ->
      val cue = cueEngine.evaluate(update)
      log(
        WayfinderLogEvent(
          stage = "GUIDANCE",
          side = "BOTH",
          detail = "maneuver=${update.maneuver} distance=${formatMeters(update.distanceToManeuverMeters)} cue=${cue?.type}"
        )
      )
      if (cue != null) {
        controller.onTurnEvent(cue)
      }
    }
    guidanceSource.start(routeRequest)
  }

  fun stop() {
    guidanceSource.stop()
  }

  private fun formatMeters(value: Float): String =
    if (abs(value) >= 100f) value.toInt().toString() else "%.1f".format(value)
}
