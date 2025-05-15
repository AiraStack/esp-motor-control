# OpenBot ESP32 Motor Control

A simple ESP32-based motor control project for OpenBot. This project configures an ESP32 to control two DC motors through an H-bridge motor driver.

## Hardware Setup

- ESP32 development board
- H-Bridge motor driver
- Two DC motors

## Pin Configuration

| Function | ESP32 Pin |
|----------|-----------|
| PWMA     | 25        |
| AIN2     | 17        |
| AIN1     | 21        |
| BIN1     | 22        |
| BIN2     | 23        |
| PWMB     | 26        |

## Building and Uploading

This project uses PlatformIO for development. To build and upload:

1. Install PlatformIO Core or PlatformIO IDE extension for VSCode
2. Connect your ESP32 board via USB
3. Run `pio run -t upload` or use the PlatformIO upload button in VSCode

## Functionality

The code currently sets both motors to run forward at a fixed speed. 