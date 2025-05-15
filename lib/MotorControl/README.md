# MotorControl

* Chinese version, please see: [中文](./README_CN.md)

A motor control library for ESP32-based robot chassis developed for the OpenBot project. This library abstracts the basic movements of a robot, making it easy for developers to control robot movement.

## Features

- Simple and easy-to-use motor control interface
- Support for basic movements: forward, backward, turns, and rotations
- Support for advanced movements: diagonal movements and predefined action sequences
- PWM control using ESP32's LEDC functionality

## Hardware Requirements

- ESP32 development board
- H-bridge motor driver (like L298N, TB6612, etc.)
- DC motors (2)

## Pin Configuration

Default pin configuration below, but can be customized when creating a MotorControl object:

| Function | ESP32 Pin |
|----------|-----------|
| PWMA     | 25        |
| AIN1     | 21        |
| AIN2     | 17        |
| BIN1     | 22        |
| BIN2     | 23        |
| PWMB     | 26        |

## Installation

1. Copy this library to the `lib` directory in your PlatformIO project
2. Include the header file in your code: `#include "MotorControl.h"`

## Usage

### Basic Usage

```cpp
#include <Arduino.h>
#include "MotorControl.h"

// Create a MotorControl object with default pin configuration
MotorControl motors(25, 21, 17, 26, 22, 23);

void setup() {
  // Initialize motors
  motors.init();
}

void loop() {
  // Move forward
  motors.moveForward(100);
  delay(2000);
  
  // Stop
  motors.stopMotors();
  delay(1000);
  
  // Move backward
  motors.moveBackward(100);
  delay(2000);
}
```

### Advanced Usage

See the example code in the `examples` directory for more usage details.

## API Reference

### Constructor

```cpp
MotorControl(uint16_t pwmA, uint16_t ain1, uint16_t ain2,
             uint16_t pwmB, uint16_t bin1, uint16_t bin2,
             int freq = 100000, uint16_t resolution = 8);
```

### Basic Methods

- `void init()` - Initialize motor control
- `void moveForward(uint16_t speed)` - Move forward
- `void moveBackward(uint16_t speed)` - Move backward
- `void turnLeft(uint16_t speed)` - Turn left
- `void turnRight(uint16_t speed)` - Turn right
- `void spinLeft(uint16_t speed)` - Rotate left in place
- `void spinRight(uint16_t speed)` - Rotate right in place
- `void stopMotors()` - Stop all motors

### Advanced Methods

- `void diagForwardLeft(uint16_t speed)` - Move diagonally forward left
- `void diagForwardRight(uint16_t speed)` - Move diagonally forward right
- `void performActionSequence(int actionNumber)` - Perform a predefined action sequence

## License

MIT
