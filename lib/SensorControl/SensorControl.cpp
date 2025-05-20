#include "SensorControl.h"

SensorControl::SensorControl(
    uint16_t triggerPin,
    uint16_t echoPin,
    uint16_t vinPin,
    uint16_t ledLeftPin,
    uint16_t ledRightPin
) : _triggerPin(triggerPin),
    _echoPin(echoPin),
    _vinPin(vinPin),
    _ledLeftPin(ledLeftPin),
    _ledRightPin(ledRightPin)
{
}

void SensorControl::init() {
    pinMode(_triggerPin, OUTPUT);
    pinMode(_echoPin, INPUT);
    pinMode(_vinPin, INPUT);
    pinMode(_ledLeftPin, OUTPUT);
    pinMode(_ledRightPin, OUTPUT);
}

void SensorControl::startPing() {
    digitalWrite(_triggerPin, LOW);
    delayMicroseconds(2);
    digitalWrite(_triggerPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(_triggerPin, LOW);
    _startTime = micros();
    _pingSuccess = false;
}

bool SensorControl::checkEcho() {
    if (digitalRead(_echoPin) == HIGH) {
        _echoTime = micros() - _startTime;
        _distance = _echoTime * US_TO_CM;
        _pingSuccess = true;
        return true;
    }
    return false;
}

unsigned int SensorControl::getDistance() {
    return _distance;
}

void SensorControl::updateDistanceEstimate() {
    if (_distance < MAX_SONAR_DISTANCE) {
        _distanceEstimate = _distance;
    }
}

void SensorControl::updateVoltage() {
    _vinArray[_vinCounter % _vinArraySz] = analogRead(_vinPin);
    _vinCounter++;
}

float SensorControl::getVoltage() {
    int sum = 0;
    for (unsigned int i = 0; i < _vinArraySz; i++) {
        sum += _vinArray[i];
    }
    return (sum / _vinArraySz) * ADC_FACTOR * VOLTAGE_DIVIDER_FACTOR;
}

bool SensorControl::isLowBattery() {
    return getVoltage() < VOLTAGE_LOW;
}

void SensorControl::setLeftLED(bool state) {
    digitalWrite(_ledLeftPin, state);
}

void SensorControl::setRightLED(bool state) {
    digitalWrite(_ledRightPin, state);
}

void SensorControl::setSonarInterval(unsigned long interval) {
    _sonarInterval = interval;
}

void SensorControl::setVoltageInterval(unsigned long interval) {
    _voltageInterval = interval;
} 