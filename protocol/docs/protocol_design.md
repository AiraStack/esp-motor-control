---
**English Bluetooth guide**: see `bluetooth_integration_en.md` in the same folder.
---

# OpenBot ESP小车控制协议设计文档

## 概述

本文档描述了基于OpenBot项目整理的ESP小车控制协议，该协议支持两种通信方式：
1. **串口协议** - 用于与Android应用程序通信
2. **网络协议** - 用于与远程控制器（如Python控制器）通信

## 1. 协议架构

```
┌─────────────────┐    JSON协议     ┌─────────────────┐    串口协议     ┌─────────────────┐
│   Python控制器   │ ──────────────► │   Android应用    │ ──────────────► │    ESP32小车     │
│                 │    TCP Socket   │                 │   USB/蓝牙       │                 │
└─────────────────┘                 └─────────────────┘                 └─────────────────┘
```

## 2. 串口协议（ESP端实现）

### 2.1 消息格式

所有串口消息采用以下格式：
```
<header><body><end_char>
```

- **header**: 单个字符，表示命令类型
- **body**: 命令参数，使用逗号分隔
- **end_char**: 结束符，固定为换行符 `\n`

### 2.2 命令类型

| Header | 命令类型 | 格式 | 说明 |
|--------|----------|------|------|
| `c` | 电机控制 | `c<left>,<right>\n` | left/right: -255到255 |
| `l` | LED控制 | `l<front>,<back>\n` | front/back: 0到255 |
| `i` | 指示灯控制 | `i<left>,<right>\n` | left/right: 0或1 |
| `h` | 心跳 | `h<interval>\n` | interval: 毫秒 |
| `f` | 功能查询 | `f\n` | 查询支持的功能 |
| `v` | 电压查询 | `v<interval>\n` | 查询电压信息 |
| `s` | 声纳传感器 | `s<interval>\n` | 设置读取间隔 |
| `b` | 碰撞传感器 | `b<interval>\n` | 设置读取间隔 |
| `w` | 轮速传感器 | `w<interval>\n` | 设置读取间隔 |
| `n` | 状态LED | `n<led>,<state>\n` | led: y/g/b, state: 0/1 |

### 2.3 响应格式

ESP32向控制端发送的响应格式：

| 类型 | 格式 | 说明 |
|------|------|------|
| 功能信息 | `f<robot_type>:<features>:\n` | 如：`fESP32:v:i:s:wf:lf:lb:ls:\n` |
| 电压信息 | `vmin:<min>\nvlow:<low>\nvmax:<max>\n` | 电压范围信息 |
| 传感器数据 | `<type><value>\n` | 如：`s15.6\n`（声纳距离） |

### 2.4 示例

```
// 控制命令
c127,-127\n          // 左轮前进，右轮后退
l255,0\n             // 前灯全亮，后灯关闭
i1,0\n               // 左转向灯开，右转向灯关
h1000\n              // 心跳间隔1秒

// 响应
fESP32:v:i:s:wf:lf:lb:ls:\n    // 功能列表
vmin:2.50\nvlow:9.00\nvmax:12.60\n  // 电压信息
s15.6\n                             // 声纳距离15.6cm
```

## 3. 网络协议（JSON格式）

### 3.1 驱动控制命令

用于控制小车运动的JSON格式：

```json
{
  "driveCmd": {
    "l": <left_speed>,
    "r": <right_speed>
  }
}
```

- `left_speed`, `right_speed`: 浮点数，范围 -1.0 到 1.0
- 正值表示前进，负值表示后退

### 3.2 普通命令

用于发送功能性命令的JSON格式：

```json
{
  "command": "<command_name>"
}
```

支持的命令包括：
- `NOISE` - 切换噪音模式
- `LOGS` - 切换日志记录
- `INDICATOR_LEFT` - 左转向灯
- `INDICATOR_RIGHT` - 右转向灯
- `INDICATOR_STOP` - 停止转向灯
- `NETWORK` - 网络模式切换
- `DRIVE_MODE` - 驱动模式切换

### 3.3 示例

```json
// 驱动命令
{"driveCmd": {"l": 1.0, "r": 1.0}}     // 全速前进
{"driveCmd": {"l": -1.0, "r": -1.0}}   // 全速后退
{"driveCmd": {"l": 0.5, "r": -0.5}}    // 右转

// 功能命令
{"command": "NOISE"}                    // 切换噪音
{"command": "INDICATOR_LEFT"}           // 左转向灯
```

## 4. 协议转换

### 4.1 速度值转换

网络协议和串口协议使用不同的速度表示：

```cpp
// JSON (-1.0 到 1.0) 转换为 串口 (-255 到 255)
int serialValue = (int)(jsonValue * 255.0f);

// 串口 (-255 到 255) 转换为 JSON (-1.0 到 1.0)
float jsonValue = serialValue / 255.0f;
```

### 4.2 命令映射

部分JSON命令需要转换为串口命令：

| JSON命令 | 串口命令 | 说明 |
|----------|----------|------|
| `INDICATOR_LEFT` | `i1,0\n` | 左转向灯开 |
| `INDICATOR_RIGHT` | `i0,1\n` | 右转向灯开 |
| `INDICATOR_STOP` | `i0,0\n` | 转向灯关闭 |

## 5. 库使用说明

### 5.1 基本初始化

```cpp
#include <OpenBotProtocol.h>

OpenBotProtocol protocol;

void setup() {
    protocol.init();
    
    // 设置回调函数
    protocol.setControlCallback(onControlCommand);
    protocol.setLightCallback(onLightCommand);
    // ... 其他回调
}
```

### 5.2 串口数据处理

```cpp
void loop() {
    while (Serial.available()) {
        char data = Serial.read();
        protocol.processSerialData(data);
    }
}
```

### 5.3 网络数据处理

```cpp
// 处理接收到的JSON字符串
if (client.available()) {
    String jsonMessage = client.readStringUntil('\n');
    protocol.parseJsonMessage(jsonMessage.c_str());
}
```

### 5.4 回调函数实现

```cpp
void onControlCommand(const ControlCommand& cmd) {
    // 控制电机
    leftMotor.setSpeed(cmd.left);
    rightMotor.setSpeed(cmd.right);
}

void onLightCommand(const LightCommand& cmd) {
    // 控制LED
    analogWrite(FRONT_LED_PIN, cmd.front);
    analogWrite(BACK_LED_PIN, cmd.back);
}
```

## 6. 蓝牙集成

OpenBot协议库完全支持蓝牙通信，可以与现有的BLEComm库无缝集成，实现通过蓝牙接收和解析OpenBot命令。

### 6.1 蓝牙数据流程

```
Android/控制器应用
        ↓ (蓝牙发送：c127,-127\n)
   BLE特征值写入
        ↓
  BLEComm::MyCallbacks::onWrite()
        ↓ (逐字符解析)
   BLEComm内部解析器
        ↓ (header='c', body="127,-127")
   onBleMessage(char header, const char* body)
        ↓ (转发给协议处理器)
   OpenBotProtocol::processSerialMessage()
        ↓ (解析并触发回调)
   onControlCommand(ControlCommand cmd)
        ↓ (执行具体动作)
     电机控制 + BLE响应
```

### 6.2 蓝牙集成方案

#### 方案1：桥接方式（推荐）

这是最简单且不需要修改现有库的方式：

```cpp
#include <OpenBotProtocol.h>
#include <BLEComm.h>
#include <MotorControl.h>

OpenBotProtocol protocol;
BLEComm bleComm("OpenBot-ESP32");
MotorControl motors(5, 18, 19, 16, 17, 4);

// 蓝牙消息桥接函数
void onBleMessage(char header, const char* body) {
    Serial.printf("BLE Received: %c%s\n", header, body);
    
    // 将BLE消息转发给协议处理器
    String message = String(header) + String(body);
    protocol.processSerialMessage(message.c_str());
}

// 协议回调函数
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
    } else {
        // 转向逻辑
        motors.turnLeft(abs(cmd.left - cmd.right));
    }
    
    // 通过BLE发送状态反馈
    String response = "c" + String(cmd.left) + "," + String(cmd.right);
    bleComm.sendData(response.c_str());
}

void setup() {
    // 初始化
    protocol.init();
    motors.init();
    bleComm.init();
    
    // 设置回调
    protocol.setControlCallback(onControlCommand);
    bleComm.setMessageCallback(onBleMessage); // 关键：桥接BLE和协议
}

void loop() {
    bleComm.updateConnection();
    delay(50);
}
```

#### 方案2：直接集成方式

如果需要更深度的集成，可以修改BLEComm库：

```cpp
// 在BLEComm.h中添加
class OpenBotProtocol; // 前向声明

class BLEComm {
private:
    OpenBotProtocol* _protocolProcessor;
public:
    void setProtocolProcessor(OpenBotProtocol* processor);
};

// 在BLEComm.cpp的parseMessage()中添加
void BLEComm::parseMessage() {
    if (_messageCallback) {
        _msgBuf[_msgIdx] = '\0';
        _messageCallback(_header, _msgBuf);
    }
    
    // 直接调用协议处理器
    if (_protocolProcessor) {
        String message = String(_header) + String(_msgBuf);
        _protocolProcessor->processSerialMessage(message.c_str());
    }
    
    _msgPart = HEADER;
}
```

### 6.3 支持的蓝牙命令格式

蓝牙命令格式与串口协议完全一致：

| 命令类型 | 蓝牙数据 | 解析结果 | 功能说明 |
|----------|----------|----------|----------|
| 电机控制 | `c127,-127\n` | header='c', body="127,-127" | 左轮前进，右轮后退 |
| 灯光控制 | `l255,128\n` | header='l', body="255,128" | 前灯全亮，后灯半亮 |
| 指示灯 | `i1,0\n` | header='i', body="1,0" | 左转向灯开，右转向灯关 |
| 心跳检测 | `h1000\n` | header='h', body="1000" | 心跳间隔1秒 |
| 功能查询 | `f\n` | header='f', body="" | 查询支持的功能 |
| 电压查询 | `v100\n` | header='v', body="100" | 设置电压读取间隔 |
| 状态通知 | `ny,1\n` | header='n', body="y,1" | 黄色状态LED开启 |

### 6.4 蓝牙连接管理

```cpp
void loop() {
    // 更新BLE连接状态
    bleComm.updateConnection();
    
    // 检查连接状态
    if (bleComm.isConnected()) {
        if (!digitalRead(STATUS_LED_G)) {
            digitalWrite(STATUS_LED_G, HIGH);
            Serial.println("BLE reconnected");
            
            // 重新连接时发送系统状态
            protocol.sendFeatureResponse("ESP32:v:i:s:wf:lf:lb:ls:");
            protocol.sendVoltageInfo(2.5, 9.0, 12.6);
        }
    } else {
        digitalWrite(STATUS_LED_G, LOW);
        digitalWrite(STATUS_LED_Y, HIGH);
        
        // 连接断开时停止所有运动
        motors.stopMotors();
        Serial.println("BLE disconnected - motors stopped");
    }
    
    delay(50);
}
```

### 6.5 蓝牙状态反馈

协议库支持通过蓝牙发送状态反馈：

```cpp
void onControlCommand(const ControlCommand& cmd) {
    // 执行电机控制...
    
    // 发送执行确认
    String response = "c" + String(cmd.left) + "," + String(cmd.right);
    bleComm.sendData(response.c_str());
}

void sendSystemStatus() {
    // 发送电池电压
    float voltage = readBatteryVoltage();
    String voltageMsg = "v" + String(voltage, 2);
    bleComm.sendData(voltageMsg.c_str());
    
    // 发送传感器数据
    float distance = readSonarDistance();
    if (distance > 0) {
        String distanceMsg = "s" + String(distance, 1);
        bleComm.sendData(distanceMsg.c_str());
    }
    
    // 发送功能支持信息
    bleComm.sendData("fESP32:v:i:s:wf:lf:lb:ls:");
}
```

### 6.6 蓝牙协议测试

可以使用以下方法测试蓝牙协议：

#### 使用OpenBot Android应用
1. 连接到ESP32设备（设备名："OpenBot-ESP32"）
2. 使用控制界面发送命令
3. 观察ESP32的响应和动作

#### 使用BLE调试工具
可以使用nRF Connect等BLE调试应用：

1. **连接设备**：搜索并连接到"OpenBot-ESP32"
2. **找到服务**：UUID `61653dc3-4021-4d1e-ba83-8b4eec61d613`
3. **写入特征值**：UUID `06386c14-86ea-4d71-811c-48f97c58f8c9`
4. **发送命令**：
   ```
   c100,100    # 前进
   c-100,-100  # 后退
   c100,-100   # 右转
   l255,0      # 前灯开
   i1,0        # 左转向灯
   c0,0        # 停止
   ```

#### 程序内测试
```cpp
void testBleProtocol() {
    Serial.println("=== BLE Protocol Test ===");
    
    // 模拟BLE接收到的命令
    onBleMessage('c', "127,-127");  // 电机测试
    delay(1000);
    onBleMessage('l', "255,128");   // 灯光测试
    delay(1000);
    onBleMessage('i', "1,0");       // 指示灯测试
    delay(1000);
    onBleMessage('c', "0,0");       // 停止
    
    Serial.println("=== Test Complete ===");
}
```

### 6.7 错误处理

蓝牙通信的错误处理：

```cpp
void onBleMessage(char header, const char* body) {
    // 验证消息格式
    if (header < 'a' || header > 'z') {
        Serial.printf("Invalid BLE header: %c\n", header);
        return;
    }
    
    // 转发给协议处理器
    String message = String(header) + String(body);
    if (!protocol.processSerialMessage(message.c_str())) {
        Serial.printf("Protocol parsing failed: %s\n", message.c_str());
        
        // 发送错误响应
        bleComm.sendData("ERROR:Invalid command");
    }
}
```

### 6.8 性能优化

- **消息缓冲**：BLEComm库已经实现了消息缓冲，避免数据丢失
- **连接检测**：定期检查连接状态，及时处理断连
- **响应控制**：避免频繁发送响应消息，防止蓝牙拥塞

## 7. 硬件接口

### 7.1 推荐引脚定义

```cpp
// 电机控制引脚
#define MOTOR_LEFT_PWM   5
#define MOTOR_LEFT_DIR1  18
#define MOTOR_LEFT_DIR2  19
#define MOTOR_RIGHT_PWM  16
#define MOTOR_RIGHT_DIR1 17
#define MOTOR_RIGHT_DIR2 4

// LED控制引脚
#define FRONT_LED_PIN    21
#define BACK_LED_PIN     22
#define STATUS_LED_Y     23
#define STATUS_LED_G     25
#define STATUS_LED_B     26

// 传感器引脚
#define SONAR_TRIG_PIN   12
#define SONAR_ECHO_PIN   14
#define BUMPER_PIN       27
#define VOLTAGE_PIN      A0
```

### 7.2 集成现有库

协议库可以与现有的MotorControl和BLEComm库集成：

```cpp
#include <OpenBotProtocol.h>
#include <MotorControl.h>
#include <BLEComm.h>

OpenBotProtocol protocol;
MotorControl motors(/* 引脚配置 */);
BLEComm bleComm("OpenBot-ESP32");

void onControlCommand(const ControlCommand& cmd) {
    // 使用MotorControl库
    if (cmd.left > 0 && cmd.right > 0) {
        motors.moveForward(max(cmd.left, cmd.right));
    } else if (cmd.left < 0 && cmd.right < 0) {
        motors.moveBackward(max(-cmd.left, -cmd.right));
    } else if (cmd.left > cmd.right) {
        motors.turnLeft(abs(cmd.left - cmd.right));
    } else if (cmd.right > cmd.left) {
        motors.turnRight(abs(cmd.right - cmd.left));
    } else {
        motors.stopMotors();
    }
}
```

## 8. 错误处理

### 8.1 协议错误

- **消息格式错误**: 忽略无效消息，继续处理
- **参数范围错误**: 使用constrain函数限制到有效范围
- **JSON解析错误**: 返回false，记录错误日志

### 8.2 连接管理

- **心跳检测**: 5秒内无心跳认为连接断开
- **自动重连**: 连接断开时自动尝试重新连接
- **超时处理**: 连接超时时停止所有运动

## 9. 性能优化

### 9.1 内存管理

- 使用固定大小的缓冲区避免动态内存分配
- JSON文档大小限制为1024字节
- 消息缓冲区大小为64字节

### 9.2 处理效率

- 使用状态机进行消息解析
- 避免在中断中进行复杂计算
- 批量处理多个串口字符

## 10. 安全考虑

### 10.1 输入验证

- 所有输入参数都进行范围检查
- 防止缓冲区溢出攻击
- 验证JSON格式有效性

### 10.2 网络安全

- 限制连接数量防止DoS攻击
- 实现简单的认证机制
- 记录异常访问日志

## 11. 扩展性

### 11.1 新命令添加

要添加新命令类型：

1. 在`CommandType`枚举中添加新类型
2. 定义对应的命令结构体
3. 添加回调函数类型
4. 实现对应的处理函数
5. 在`parseSerialMessage()`中添加case

### 11.2 传感器集成

协议支持多种传感器类型，可以轻松添加新的传感器支持：

```cpp
// 添加新传感器数据发送
protocol.sendSensorData("temp", temperature);
protocol.sendSensorData("humid", humidity);
```

---

**版本**: 1.0.0  
**作者**: AiraStack  
**更新日期**: 2024年12月 