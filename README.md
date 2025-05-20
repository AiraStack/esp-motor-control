# OpenBot ESP32 Motor Control

A simple ESP32-based motor control project for OpenBot. This project configures an ESP32 to control two DC motors through an H-bridge motor driver.

## Hardware Setup

- ESP32 development board
- H-Bridge motor driver
- Two DC motors
- Optional: Ultrasonic sensor (HC-SR04)
- Optional: Voltage divider for battery monitoring
- Optional: LED indicators

## Pin Configuration

### Motor Control
| Function | ESP32 Pin |
|----------|-----------|
| PWM_L1   | 13        |
| PWM_L2   | 12        |
| PWM_R1   | 27        |
| PWM_R2   | 33        |

### Sensors and Indicators
| Function | ESP32 Pin |
|----------|-----------|
| Speed LF | 5         |
| Speed RF | 18        |
| VIN      | 39        |
| Trigger  | 25        |
| Echo     | 26        |
| LED_LI   | 22        |
| LED_RI   | 16        |

## Building and Uploading

This project uses PlatformIO for development. To build and upload:

1. Install PlatformIO Core or PlatformIO IDE extension for VSCode
2. Connect your ESP32 board via USB
3. Run `pio run -t upload` or use the PlatformIO upload button in VSCode

## Functionality

The firmware provides the following features:

1. Motor Control
   - PWM-based motor control for both motors
   - Independent control of left and right motors
   - Forward and reverse operation

2. Communication
   - Serial communication (115200 baud)
   - Bluetooth Low Energy (BLE) support
   - Command protocol for motor control and sensor configuration

3. Sensors
   - Ultrasonic distance measurement (optional)
   - Battery voltage monitoring (optional)
   - Speed sensor support (optional)

4. Safety Features
   - Automatic stop when obstacles are detected
   - Heartbeat monitoring
   - Low battery protection

## Configuration

The firmware can be configured through the following defines:

```cpp
#define HAS_BLUETOOTH 1              // Enable/disable Bluetooth
#define HAS_VOLTAGE_DIVIDER 1        // Enable/disable voltage monitoring
#define HAS_INDICATORS 1             // Enable/disable LED indicators
#define HAS_SONAR 1                  // Enable/disable ultrasonic sensor
#define HAS_SPEED_SENSORS_FRONT 1    // Enable/disable speed sensors
```

## Communication Protocol

The firmware supports both serial and Bluetooth communication with the following commands:

- Control command: `c<left>,<right>\n`
  - Example: `c100,100\n` sets both motors to full forward speed
- Sensor configuration: `s<config>\n`
  - Example: `s1\n` enables sensor 1

## License

This project is licensed under the MIT License - see the LICENSE file for details. 