# OpenBot ESP32 – Bluetooth Integration Guide

This guide explains how to integrate the OpenBot protocol library with Bluetooth communication on an ESP32-based car so that you can control the vehicle directly from the OpenBot Android application or any BLE client.

---

## 1 Overview

The OpenBot protocol library fully supports Bluetooth communication and enables you to:

* Receive motor and light commands from an Android or desktop client via BLE.
* Parse and execute the **serial protocol** commands defined by OpenBot (e.g. `c127,-127\n`).
* Send status and sensor data back over BLE.
* Monitor the connection and stop the vehicle automatically if the link is lost.

---

## 2 Data Flow

```
┌─────────────────────┐   BLE packets   ┌─────────────────────┐
│   OpenBot Android   │ ───────────────►│       ESP32         │
│       (Phone)       │◄─────────────── │     (Robot Car)     │
└─────────────────────┘                 └─────────────────────┘
        │                                        │
        │ 1. writes c127,-127\n                  │
        ▼                                        ▼
 Characteristic Write                    BLEComm::onWrite()
        │                                        │
        ▼                                        ▼
 OpenBotProtocol::processSerialMessage() →  MotorControl callback
```

*Every byte eventually ends up in `OpenBotProtocol`, so you only need to forward the header (`c`, `l`, `i`, …) and the payload (`127,-127`).*

---

## 3 Quick Start (Bridge Method – Recommended)

### 3.1 Include the libraries

```cpp
#include <OpenBotProtocol.h>
#include <BLEComm.h>
#include <MotorControl.h>
```

### 3.2 Create global objects

```cpp
OpenBotProtocol protocol;          // protocol parser & helpers
BLEComm         ble("OpenBot-ESP32"); // BLE wrapper (name can be changed)
MotorControl    motors(5, 18, 19, 16, 17, 4); // adjust pins to your board
```

### 3.3 Bridge BLE data to the protocol parser

```cpp
void onBleMessage(char header, const char* body) {
  // Forward the full message to the protocol instance
  String msg = String(header) + String(body);
  protocol.processSerialMessage(msg.c_str());
}
```

### 3.4 Handle control callbacks

```cpp
void onControl(const ControlCommand& cmd) {
  // Basic differential-drive mapping
  motors.setMotorSpeeds(cmd.left, cmd.right);
  
  // Optional feedback to the phone
  String ack = "c" + String(cmd.left) + "," + String(cmd.right);
  ble.sendData(ack.c_str());
}
```

### 3.5 Setup function

```cpp
void setup() {
  Serial.begin(115200);

  protocol.init();
  motors.init();
  ble.init();

  protocol.setControlCallback(onControl);
  ble.setMessageCallback(onBleMessage);
}
```

### 3.6 Loop function

```cpp
void loop() {
  ble.updateConnection();          // keep BLE alive

  if (!protocol.isConnected()) {   // failsafe: lost heartbeat >5 s
    motors.stopMotors();
  }
  delay(20);
}
```

---

## 4 Command Reference (BLE = Serial Protocol)

| Type  | Example           | Meaning                                  |
|-------|-------------------|------------------------------------------|
| Motor | `c127,-127\n`     | Left wheel forward, right wheel reverse |
| Light | `l255,128\n`      | Front LED 100 %, rear LED 50 %           |
| Ind.  | `i1,0\n`          | Left indicator ON, right OFF             |
| HB    | `h1000\n`         | Heartbeat every 1000 ms                  |
| Query | `f\n`             | Request feature list                     |
| Volt  | `v100\n`          | Set voltage report interval (ms)         |
| Stat  | `ny,1\n`          | Turn yellow status LED ON                |

All commands **must end with a newline (\n)**.

---

## 5 Connection Handling & Safety

* `OpenBotProtocol::isConnected()` returns `true` if a heartbeat (`h…`) has been received within the last **5 s**.  
  You can adjust or remove this check if you implement your own safety logic.
* When the BLE link drops, call `motors.stopMotors()` immediately.

---

## 6 Testing Tips

### 6.1 nRF Connect (Android / iOS)

1. Scan for **"OpenBot-ESP32"** and connect.  
2. Locate the service `61653dc3-4021-4d1e-ba83-8b4eec61d613`.  
3. Write to the TX characteristic `06386c14-86ea-4d71-811c-48f97c58f8c9`.  
4. Send e.g. `c100,100` followed by **LF** or **CRLF**.

### 6.2 Serial Monitor

The library prints useful debug information, for example:
```
BLE Received: c100,100
Motor: L=100 R=100
Heartbeat: 1000 ms
```

---

## 7 FAQ

**Q: How long will the car drive after a `c...` command?**  
A: Indefinitely – until a new `c` command arrives or the heartbeat times out (default 5 s). Send `c0,0` to stop.

**Q: Why does my phone disconnect after a minute?**  
A: Make sure you keep sending heartbeat frames (`h1000`) every second.

---

## 8 Revision History

| Version | Date        | Notes            |
|---------|------------|------------------|
| 1.0.0   | 2024-12-01 | Initial English version |

</rewritten_file> 