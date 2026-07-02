#pragma once

#include <Arduino.h>

class IMUcontroller_I2C{
public:
  uint8_t deviceAddress;    // I2C address
  int16_t datA_rawQuat[3];
  uint8_t sensorID;
  float quaternions[4] = {1.0f, 0.0f, 0.0f, 0.0f}; // w, x, y, z

  IMUcontroller_I2C();
  bool init(uint8_t setSensorID, uint8_t setDeviceAddress, bool checkWAI=true);
  bool getQuaternions();
};