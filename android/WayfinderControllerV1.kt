package com.wayfinder.controller

import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothGatt
import android.bluetooth.BluetoothGattCallback
import android.bluetooth.BluetoothGattCharacteristic
import android.bluetooth.BluetoothGattDescriptor
import android.bluetooth.BluetoothGattService
import android.bluetooth.BluetoothProfile
import android.content.Context
import android.os.Build
import android.os.Handler
import android.os.Looper
import android.os.SystemClock
import java.util.ArrayDeque
import java.util.UUID
import java.util.concurrent.ConcurrentLinkedQueue

enum class TurnEventType { LEFT, RIGHT, STOP }

data class TurnEvent(
  val type: TurnEventType,
  val eventId: String,
  val eventTimestampMs: Long = SystemClock.elapsedRealtime()
)

data class WayfinderLogEvent(
  val timestampMs: Long = SystemClock.elapsedRealtime(),
  val stage: String,
  val side: String,
  val detail: String
)

private const val CMD_LEFT: Byte = 0x01
private const val CMD_RIGHT: Byte = 0x02
private const val CMD_STOP: Byte = 0x03

private const val DEVICE_NAME_LEFT = "WAYFINDER-LEFT"
private const val DEVICE_NAME_RIGHT = "WAYFINDER-RIGHT"
private const val TURN_EVENT_TTL_MS = 30_000L
private const val TURN_WRITE_TTL_MS = 5_000L
private const val STOP_WRITE_TTL_MS = 15_000L
private const val GATT_WRITE_SUCCESS = 0

private val NAV_SERVICE_UUID = UUID.fromString("19b10010-e8f2-537e-4f6c-d104768a1214")
private val NAV_CHARACTERISTIC_UUID = UUID.fromString("19b10011-e8f2-537e-4f6c-d104768a1214")

class WayfinderControllerV1(private val context: Context) {
  private val logger = ConcurrentLinkedQueue<WayfinderLogEvent>()
  private val mainHandler = Handler(Looper.getMainLooper())
  private val recentEvents = ArrayDeque<RecentEvent>()
  private var closed = false

  private var leftLink: DeviceLink? = null
  private var rightLink: DeviceLink? = null

  fun attachDevices(leftDevice: BluetoothDevice, rightDevice: BluetoothDevice) {
    if (closed) return
    leftLink?.close()
    rightLink?.close()
    leftLink = DeviceLink("LEFT", leftDevice)
    rightLink = DeviceLink("RIGHT", rightDevice)
    leftLink?.connect()
    rightLink?.connect()
  }

  fun onTurnEvent(event: TurnEvent) {
    if (closed) return
    log("EVENT", "BOTH", "turn=${event.type} id=${event.eventId}")
    pruneRecentEvents()
    if (hasRecentEvent(event)) {
      log("DEDUP", "BOTH", "ignored id=${event.eventId} type=${event.type}")
      return
    }

    val accepted = when (event.type) {
      TurnEventType.LEFT -> leftLink?.send(CMD_LEFT, event) == true
      TurnEventType.RIGHT -> rightLink?.send(CMD_RIGHT, event) == true
      TurnEventType.STOP -> {
        val dispatchStartedAt = SystemClock.elapsedRealtime()
        val leftAccepted = leftLink?.send(CMD_STOP, event) == true
        val rightAccepted = rightLink?.send(CMD_STOP, event) == true
        val dispatchDeltaMs = SystemClock.elapsedRealtime() - dispatchStartedAt
        log("SYNC", "BOTH", "stop_dispatch_delta_ms=$dispatchDeltaMs")
        leftAccepted || rightAccepted
      }
    }

    if (accepted) {
      recentEvents.addLast(RecentEvent(event.eventId, event.type, SystemClock.elapsedRealtime()))
    }
  }

  fun close() {
    closed = true
    mainHandler.removeCallbacksAndMessages(null)
    leftLink?.close()
    rightLink?.close()
    leftLink = null
    rightLink = null
    recentEvents.clear()
  }

  fun connectionSnapshot(): Map<String, Boolean> = mapOf(
    "LEFT" to (leftLink?.isConnected() ?: false),
    "RIGHT" to (rightLink?.isConnected() ?: false)
  )

  fun drainLogs(): List<WayfinderLogEvent> {
    val items = mutableListOf<WayfinderLogEvent>()
    while (true) {
      val item = logger.poll() ?: break
      items.add(item)
    }
    return items
  }

  private fun log(stage: String, side: String, detail: String) {
    logger.add(WayfinderLogEvent(stage = stage, side = side, detail = detail))
  }

  private fun pruneRecentEvents() {
    val now = SystemClock.elapsedRealtime()
    while (recentEvents.isNotEmpty() && now - recentEvents.first().timestampMs > TURN_EVENT_TTL_MS) {
      recentEvents.removeFirst()
    }
  }

  private fun hasRecentEvent(event: TurnEvent): Boolean =
    recentEvents.any { it.eventId == event.eventId && it.type == event.type }

  private data class RecentEvent(
    val eventId: String,
    val type: TurnEventType,
    val timestampMs: Long
  )

  private data class PendingWrite(
    val command: Byte,
    val event: TurnEvent,
    val enqueuedAtMs: Long
  )

  private inner class DeviceLink(
    private val side: String,
    private val device: BluetoothDevice
  ) {
    private var gatt: BluetoothGatt? = null
    private var navCharacteristic: BluetoothGattCharacteristic? = null
    private var connected = false
    private var reconnectDelayMs = 1000L
    private var reconnectScheduled = false
    private var writeInFlight = false
    private var closed = false
    private val pendingWrites = ArrayDeque<PendingWrite>()

    private val callback = object : BluetoothGattCallback() {
      override fun onConnectionStateChange(gatt: BluetoothGatt, status: Int, newState: Int) {
        mainHandler.post {
          if (closed) return@post
          if (newState == BluetoothProfile.STATE_CONNECTED) {
            connected = true
            reconnectScheduled = false
            reconnectDelayMs = 1000L
            log("CONNECT", side, "connected status=$status")
            gatt.discoverServices()
          } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
            connected = false
            navCharacteristic = null
            writeInFlight = false
            closeGatt()
            log("DISCONNECT", side, "disconnected status=$status; scheduling_retry")
            scheduleReconnect()
          }
        }
      }

      override fun onServicesDiscovered(gatt: BluetoothGatt, status: Int) {
        mainHandler.post {
          if (closed) return@post
          if (status != BluetoothGatt.GATT_SUCCESS) {
            log("ERROR", side, "service_discovery_status=$status")
            return@post
          }

          val service: BluetoothGattService? = gatt.getService(NAV_SERVICE_UUID)
          val characteristic = service?.getCharacteristic(NAV_CHARACTERISTIC_UUID)
          navCharacteristic = characteristic
          if (characteristic != null && characteristic.isWritable()) {
            log("READY", side, "nav characteristic ready")
            dispatchNextWrite()
          } else {
            log("ERROR", side, "nav characteristic missing_or_not_writable")
          }
        }
      }

      override fun onCharacteristicWrite(
        gatt: BluetoothGatt,
        characteristic: BluetoothGattCharacteristic,
        status: Int
      ) {
        mainHandler.post {
          if (closed) return@post
          writeInFlight = false
          log(if (status == BluetoothGatt.GATT_SUCCESS) "ACK" else "ERROR", side, "write_status=$status")
          dispatchNextWrite()
        }
      }

      override fun onDescriptorWrite(
        gatt: BluetoothGatt,
        descriptor: BluetoothGattDescriptor,
        status: Int
      ) {
        log("DESC", side, "descriptor_write_status=$status")
      }
    }

    fun connect() {
      if (closed || gatt != null) {
        return
      }
      if (!nameLooksValid()) {
        log("WARN", side, "unexpected_name=${device.name ?: "null"}")
      }
      gatt = device.connectGatt(context, false, callback)
      log("CONNECT", side, "connect_requested")
    }

    fun isConnected(): Boolean = connected

    fun send(command: Byte, event: TurnEvent): Boolean {
      if (closed) return false
      if (!connected || navCharacteristic == null) {
        log("WARN", side, "write_skipped_not_ready event=${event.eventId}")
        return false
      }

      if (command == CMD_STOP) {
        pendingWrites.removeAll { it.command != CMD_STOP }
      }

      pendingWrites.addLast(PendingWrite(command, event, SystemClock.elapsedRealtime()))
      dispatchNextWrite()
      return true
    }

    fun close() {
      closed = true
      connected = false
      reconnectScheduled = false
      writeInFlight = false
      pendingWrites.clear()
      closeGatt()
    }

    private fun dispatchNextWrite() {
      if (closed || writeInFlight || !connected) return
      val characteristic = navCharacteristic ?: return
      val gatt = gatt ?: return

      val pending = nextFreshWrite() ?: return
      val command = pending.command
      log("WRITE", side, "attempt cmd=${command.toInt() and 0xFF} event=${pending.event.eventId}")
      writeInFlight = true

      val accepted = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
        gatt.writeCharacteristic(
          characteristic,
          byteArrayOf(command),
          BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT
        ) == GATT_WRITE_SUCCESS
      } else {
        characteristic.writeType = BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT
        @Suppress("DEPRECATION")
        characteristic.value = byteArrayOf(command)
        @Suppress("DEPRECATION")
        gatt.writeCharacteristic(characteristic)
      }

      if (!accepted) {
        writeInFlight = false
        log("ERROR", side, "write_failed event=${pending.event.eventId}")
        dispatchNextWrite()
      }
    }

    private fun nextFreshWrite(): PendingWrite? {
      val now = SystemClock.elapsedRealtime()
      while (pendingWrites.isNotEmpty()) {
        val pending = pendingWrites.removeFirst()
        val ttl = if (pending.command == CMD_STOP) STOP_WRITE_TTL_MS else TURN_WRITE_TTL_MS
        if (now - pending.enqueuedAtMs <= ttl) {
          return pending
        }
        log("DROP", side, "expired event=${pending.event.eventId}")
      }
      return null
    }

    private fun nameLooksValid(): Boolean {
      val expected = if (side == "LEFT") DEVICE_NAME_LEFT else DEVICE_NAME_RIGHT
      return device.name == expected
    }

    private fun BluetoothGattCharacteristic.isWritable(): Boolean =
      properties and (BluetoothGattCharacteristic.PROPERTY_WRITE or BluetoothGattCharacteristic.PROPERTY_WRITE_NO_RESPONSE) != 0

    private fun scheduleReconnect() {
      if (closed || reconnectScheduled) {
        return
      }
      reconnectScheduled = true
      val delay = reconnectDelayMs
      reconnectDelayMs = (reconnectDelayMs * 2).coerceAtMost(30000L)
      log("RETRY", side, "reconnect_in_ms=$delay")
      mainHandler.postDelayed({
        if (closed) return@postDelayed
        reconnectScheduled = false
        reconnect()
      }, delay)
    }

    private fun reconnect() {
      gatt?.close()
      gatt = null
      connect()
    }

    private fun closeGatt() {
      gatt?.close()
      gatt = null
    }
  }
}
