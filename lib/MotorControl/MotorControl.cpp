#include "MotorControl.h"

// 构造函数
MotorControl::MotorControl(
    uint16_t pwmA, uint16_t ain1, uint16_t ain2,
    uint16_t pwmB, uint16_t bin1, uint16_t bin2, 
    int freq, uint16_t resolution
) : 
    _pwmA(pwmA), _ain1(ain1), _ain2(ain2),
    _pwmB(pwmB), _bin1(bin1), _bin2(bin2),
    _freq(freq), _resolution(resolution),
    _channelA(0), _channelB(1)
{
}

// 初始化函数
void MotorControl::init() {
    // 设置引脚模式
    pinMode(_ain1, OUTPUT);
    pinMode(_ain2, OUTPUT);
    pinMode(_pwmA, OUTPUT);
    pinMode(_bin1, OUTPUT);
    pinMode(_bin2, OUTPUT);
    pinMode(_pwmB, OUTPUT);

    // 设置PWM通道
    ledcSetup(_channelA, _freq, _resolution);
    ledcAttachPin(_pwmA, _channelA);

    ledcSetup(_channelB, _freq, _resolution);
    ledcAttachPin(_pwmB, _channelB);
    
    // 初始停止电机
    stopMotors();
}

// 单个电机控制函数
void MotorControl::forwardA(uint16_t pwm) {
    digitalWrite(_ain1, LOW);
    digitalWrite(_ain2, HIGH);
    ledcWrite(_channelA, pwm);
}

void MotorControl::forwardB(uint16_t pwm) {
    digitalWrite(_bin1, LOW);
    digitalWrite(_bin2, HIGH);
    ledcWrite(_channelB, pwm);
}

void MotorControl::backwardA(uint16_t pwm) {
    digitalWrite(_ain1, HIGH);
    digitalWrite(_ain2, LOW);
    ledcWrite(_channelA, pwm);
}

void MotorControl::backwardB(uint16_t pwm) {
    digitalWrite(_bin1, HIGH);
    digitalWrite(_bin2, LOW);
    ledcWrite(_channelB, pwm);
}

void MotorControl::stopA() {
    digitalWrite(_ain1, LOW);
    digitalWrite(_ain2, LOW);
    ledcWrite(_channelA, 0);
}

void MotorControl::stopB() {
    digitalWrite(_bin1, LOW);
    digitalWrite(_bin2, LOW);
    ledcWrite(_channelB, 0);
}

// 基本动作接口实现
void MotorControl::moveForward(uint16_t speed) {
    forwardA(speed);
    forwardB(speed);
}

void MotorControl::moveBackward(uint16_t speed) {
    backwardA(speed);
    backwardB(speed);
}

void MotorControl::turnLeft(uint16_t speed) {
    stopA();
    forwardB(speed);
}

void MotorControl::turnRight(uint16_t speed) {
    forwardA(speed);
    stopB();
}

void MotorControl::spinLeft(uint16_t speed) {
    backwardA(speed);
    forwardB(speed);
}

void MotorControl::spinRight(uint16_t speed) {
    forwardA(speed);
    backwardB(speed);
}

void MotorControl::stopMotors() {
    stopA();
    stopB();
}

// 高级动作接口实现
void MotorControl::diagForwardLeft(uint16_t speed) {
    forwardA(speed / 2);
    forwardB(speed);
}

void MotorControl::diagForwardRight(uint16_t speed) {
    forwardA(speed);
    forwardB(speed / 2);
}

// 简单动作序列实现
void MotorControl::performActionSequence(int actionNumber) {
    switch (actionNumber) {
        case 0:
            moveForward(100);
            break;
        case 1:
            moveBackward(100);
            break;
        case 2:
            turnLeft(100);
            break;
        case 3:
            turnRight(100);
            break;
        case 4:
            spinLeft(100);
            break;
        case 5:
            spinRight(100);
            break;
        case 6:
            diagForwardLeft(100);
            break;
        case 7:
            diagForwardRight(100);
            break;
        case 8:
            stopMotors();
            delay(500);
            turnLeft(100);
            break;
        case 9:
            for (int i = 20; i <= 100; i += 20) {
                moveForward(i);
                delay(400);
            }
            break;
        default:
            stopMotors();
            break;
    }
} 