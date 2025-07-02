# OpenBot ESP32 蓝牙集成指南

本文档详细说明如何将OpenBot协议库与蓝牙通信集成，实现通过蓝牙控制ESP32小车。

## 🔗 概述

OpenBot协议库完全支持蓝牙通信，可以：
- 接收来自Android应用的蓝牙命令
- 解析OpenBot标准协议格式
- 发送状态反馈和传感器数据
- 处理连接管理和错误恢复

## 📡 通信架构

```
┌─────────────────────┐    蓝牙BLE     ┌─────────────────────┐
│   OpenBot Android   │ ◄─────────────► │     ESP32小车        │
│        应用         │                │                     │
└─────────────────────┘                └─────────────────────┘
         │                                       │
         │                                       │
         ▼                                       ▼
    发送控制命令                              接收并执行
    c127,-127\n                             电机+LED+传感器
    l255,0\n                                控制动作
    i1,0\n
```

## 🛠️ 快速集成

### 方法1：桥接方式（推荐）

这是最简单的集成方式，无需修改现有库：

#### 1. 包含必要的头文件

```cpp
#include <OpenBotProtocol.h>
#include <BLEComm.h>
#include <MotorControl.h>
```

#### 2. 创建对象实例

```cpp
OpenBotProtocol protocol;
BLEComm bleComm("OpenBot-ESP32");
MotorControl motors(5, 18, 19, 16, 17, 4);
```

#### 3. 实现蓝牙桥接函数

```cpp
void onBleMessage(char header, const char* body) {
    Serial.printf("BLE: %c%s\n", header, body);
    
    // 将BLE消息转发给协议处理器
    String message = String(header) + String(body);
    protocol.processSerialMessage(message.c_str());
}
```

#### 4. 实现协议回调函数

```cpp
void onControlCommand(const ControlCommand& cmd) {
    // 控制电机
    if (cmd.left == 0 && cmd.right == 0) {
        motors.stopMotors();
    } else if (cmd.left == cmd.right) {
        if (cmd.left > 0) {
            motors.moveForward(abs(cmd.left));
        } else {
            motors.moveBackward(abs(cmd.left));
        }
    } else if (cmd.left > cmd.right) {
        motors.turnLeft(abs(cmd.left - cmd.right));
    } else {
        motors.turnRight(abs(cmd.right - cmd.left));
    }
    
    // 发送BLE响应
    String response = "c" + String(cmd.left) + "," + String(cmd.right);
    bleComm.sendData(response.c_str());
}
```

#### 5. 初始化和连接回调

```cpp
void setup() {
    Serial.begin(115200);
    
    // 初始化所有模块
    protocol.init();
    motors.init();
    bleComm.init();
    
    // 设置回调函数
    protocol.setControlCallback(onControlCommand);
    protocol.setLightCallback(onLightCommand);
    protocol.setIndicatorCallback(onIndicatorCommand);
    protocol.setHeartbeatCallback(onHeartbeatCommand);
    
    // 关键：设置BLE消息回调
    bleComm.setMessageCallback(onBleMessage);
    
    Serial.println("Waiting for BLE connection...");
}
```

#### 6. 主循环处理

```cpp
void loop() {
    bleComm.updateConnection();
    
    // 连接状态管理
    if (bleComm.isConnected()) {
        digitalWrite(LED_GREEN, HIGH);
    } else {
        digitalWrite(LED_GREEN, LOW);
        motors.stopMotors(); // 断连时停止
    }
    
    delay(50);
}
```

### 完整示例代码

```cpp
#include <OpenBotProtocol.h>
#include <BLEComm.h>
#include <MotorControl.h>

// 硬件配置
#define MOTOR_LEFT_PWM   5
#define MOTOR_LEFT_DIR1  18
#define MOTOR_LEFT_DIR2  19
#define MOTOR_RIGHT_PWM  16
#define MOTOR_RIGHT_DIR1 17
#define MOTOR_RIGHT_DIR2 4
#define FRONT_LED_PIN    21
#define BACK_LED_PIN     22
#define STATUS_LED_PIN   2

// 对象实例
OpenBotProtocol protocol;
BLEComm bleComm("OpenBot-ESP32");
MotorControl motors(MOTOR_LEFT_PWM, MOTOR_LEFT_DIR1, MOTOR_LEFT_DIR2,
                   MOTOR_RIGHT_PWM, MOTOR_RIGHT_DIR1, MOTOR_RIGHT_DIR2);

// 蓝牙消息桥接
void onBleMessage(char header, const char* body) {
    Serial.printf("BLE Received: %c%s\n", header, body);
    String message = String(header) + String(body);
    protocol.processSerialMessage(message.c_str());
}

// 电机控制回调
void onControlCommand(const ControlCommand& cmd) {
    Serial.printf("Motor: L=%d, R=%d\n", cmd.left, cmd.right);
    
    if (cmd.left == 0 && cmd.right == 0) {
        motors.stopMotors();
    } else if (cmd.left == cmd.right) {
        if (cmd.left > 0) {
            motors.moveForward(abs(cmd.left));
        } else {
            motors.moveBackward(abs(cmd.left));
        }
    } else if (cmd.left > cmd.right) {
        motors.turnLeft(abs(cmd.left - cmd.right));
    } else {
        motors.turnRight(abs(cmd.right - cmd.left));
    }
    
    // BLE反馈
    String response = "c" + String(cmd.left) + "," + String(cmd.right);
    bleComm.sendData(response.c_str());
}

// LED控制回调
void onLightCommand(const LightCommand& cmd) {
    Serial.printf("Light: Front=%d, Back=%d\n", cmd.front, cmd.back);
    analogWrite(FRONT_LED_PIN, cmd.front);
    analogWrite(BACK_LED_PIN, cmd.back);
    
    String response = "l" + String(cmd.front) + "," + String(cmd.back);
    bleComm.sendData(response.c_str());
}

// 指示灯控制回调
void onIndicatorCommand(const IndicatorCommand& cmd) {
    Serial.printf("Indicator: L=%d, R=%d\n", cmd.left, cmd.right);
    // 处理指示灯逻辑...
    
    String response = "i" + String(cmd.left) + "," + String(cmd.right);
    bleComm.sendData(response.c_str());
}

// 心跳回调
void onHeartbeatCommand(unsigned long interval) {
    Serial.printf("Heartbeat: %lu ms\n", interval);
    sendSystemStatus();
}

// 发送系统状态
void sendSystemStatus() {
    // 电池电压
    float voltage = 11.5; // 实际读取
    String voltageMsg = "v" + String(voltage, 2);
    bleComm.sendData(voltageMsg.c_str());
    
    // 功能支持
    bleComm.sendData("fESP32:v:i:s:wf:lf:lb:ls:");
}

void setup() {
    Serial.begin(115200);
    
    // 引脚初始化
    pinMode(FRONT_LED_PIN, OUTPUT);
    pinMode(BACK_LED_PIN, OUTPUT);
    pinMode(STATUS_LED_PIN, OUTPUT);
    
    // 模块初始化
    protocol.init();
    motors.init();
    bleComm.init();
    
    // 回调设置
    protocol.setControlCallback(onControlCommand);
    protocol.setLightCallback(onLightCommand);
    protocol.setIndicatorCallback(onIndicatorCommand);
    protocol.setHeartbeatCallback(onHeartbeatCommand);
    bleComm.setMessageCallback(onBleMessage);
    
    Serial.println("System ready. Waiting for BLE connection...");
    digitalWrite(STATUS_LED_PIN, HIGH);
}

void loop() {
    bleComm.updateConnection();
    
    // 连接状态指示
    static bool wasConnected = false;
    bool isConnected = bleComm.isConnected();
    
    if (isConnected && !wasConnected) {
        Serial.println("BLE connected!");
        sendSystemStatus();
    } else if (!isConnected && wasConnected) {
        Serial.println("BLE disconnected!");
        motors.stopMotors();
    }
    
    wasConnected = isConnected;
    digitalWrite(STATUS_LED_PIN, isConnected ? HIGH : LOW);
    
    delay(50);
}
```

## 📱 Android应用配置

### 1. 设备发现

确保您的OpenBot Android应用配置正确：

- **设备名称**: "OpenBot-ESP32"
- **服务UUID**: `61653dc3-4021-4d1e-ba83-8b4eec61d613`
- **写入特征值**: `06386c14-86ea-4d71-811c-48f97c58f8c9`
- **通知特征值**: `9bf1103b-834c-47cf-b149-c9e4bcf778a7`

### 2. 连接流程

```java
// Android端示例代码
private void connectToESP32() {
    BluetoothDevice device = bluetoothAdapter.getRemoteDevice(esp32Address);
    bluetoothGatt = device.connectGatt(this, false, gattCallback);
}

private void sendMotorCommand(int left, int right) {
    String command = "c" + left + "," + right + "\n";
    writeCharacteristic.setValue(command.getBytes());
    bluetoothGatt.writeCharacteristic(writeCharacteristic);
}
```

## 🔧 调试和测试

### 1. 使用BLE调试工具

推荐使用 **nRF Connect** 应用进行调试：

1. **扫描设备**: 找到 "OpenBot-ESP32"
2. **连接设备**: 点击 CONNECT
3. **找到服务**: 展开服务列表
4. **写入命令**: 在写入特征值中发送命令

#### 测试命令列表

| 命令 | 数据 | 功能 |
|------|------|------|
| 前进 | `c100,100` | 全速前进 |
| 后退 | `c-100,-100` | 全速后退 |
| 左转 | `c50,100` | 左转 |
| 右转 | `c100,50` | 右转 |
| 停止 | `c0,0` | 停止电机 |
| 前灯 | `l255,0` | 前灯全亮 |
| 后灯 | `l0,255` | 后灯全亮 |
| 左转向灯 | `i1,0` | 左转向灯开 |
| 右转向灯 | `i0,1` | 右转向灯开 |
| 心跳 | `h1000` | 1秒心跳 |

### 2. 串口监控

通过串口监视器观察：

```
BLE Received: c100,100
Motor: L=100, R=100
BLE Received: l255,0
Light: Front=255, Back=0
BLE connected!
Heartbeat: 1000 ms
```

### 3. 程序内测试

```cpp
void testBleProtocol() {
    Serial.println("=== BLE Protocol Test ===");
    
    // 模拟接收命令
    onBleMessage('c', "100,100");    // 前进
    delay(2000);
    onBleMessage('c', "100,-100");   // 右转
    delay(2000);
    onBleMessage('c', "0,0");        // 停止
    delay(1000);
    onBleMessage('l', "255,0");      // 前灯
    delay(1000);
    onBleMessage('i', "1,0");        // 左转向灯
    
    Serial.println("=== Test Complete ===");
}

// 在setup()中调用
void setup() {
    // ... 其他初始化代码
    
    // 延迟5秒后开始测试
    delay(5000);
    testBleProtocol();
}
```

## ⚠️ 常见问题

### Q1: 连接不上设备
**解决方案**:
- 检查设备名称是否正确
- 确认ESP32已启动并广播
- 清除手机蓝牙缓存
- 重启ESP32设备

### Q2: 命令无响应
**解决方案**:
- 检查串口输出确认消息接收
- 验证回调函数是否正确设置
- 确认命令格式正确（包含换行符）

### Q3: 连接频繁断开
**解决方案**:
- 检查电源供应是否稳定
- 增加心跳间隔
- 优化连接参数

### Q4: 电机动作异常
**解决方案**:
- 检查引脚配置
- 验证MotorControl库初始化
- 确认电机驱动器连接

## 🚀 高级功能

### 1. 自定义响应格式

```cpp
void sendCustomResponse(const char* type, float value) {
    String response = String(type) + String(value, 2);
    bleComm.sendData(response.c_str());
}

void onControlCommand(const ControlCommand& cmd) {
    // 执行控制...
    
    // 发送详细状态
    sendCustomResponse("battery:", readBatteryVoltage());
    sendCustomResponse("distance:", readSonarDistance());
}
```

### 2. 错误处理增强

```cpp
void onBleMessage(char header, const char* body) {
    // 输入验证
    if (header < 'a' || header > 'z') {
        bleComm.sendData("ERROR:Invalid header");
        return;
    }
    
    if (strlen(body) > 20) {
        bleComm.sendData("ERROR:Message too long");
        return;
    }
    
    // 转发处理
    String message = String(header) + String(body);
    if (!protocol.processSerialMessage(message.c_str())) {
        bleComm.sendData("ERROR:Parse failed");
    }
}
```

### 3. 性能监控

```cpp
unsigned long lastBleMessage = 0;
int messageCount = 0;

void onBleMessage(char header, const char* body) {
    messageCount++;
    lastBleMessage = millis();
    
    // 转发处理
    String message = String(header) + String(body);
    protocol.processSerialMessage(message.c_str());
    
    // 每100条消息报告一次
    if (messageCount % 100 == 0) {
        String stats = "stats:" + String(messageCount) + "," + String(millis() - lastBleMessage);
        bleComm.sendData(stats.c_str());
    }
}
```

---

**版本**: 1.0.0  
**作者**: AiraStack  
**更新日期**: 2024年12月  
**相关文档**: [协议设计文档](protocol_design.md) 