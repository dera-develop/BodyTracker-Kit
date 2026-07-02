#include <Arduino.h>
#include "IMUcontroller_I2C.h"
#include "I2Ccontroller.h"

#define   ADDR_FUNC_CFG_ACCESS  0x01

#define   ADDR_EMB_FUNC_EN_A      0x04
#define   ADDR_EMB_FUNC_FIFO_EN_A 0x44
#define   ADDR_SFLP_ODR           0x3e

#define   ADDR_FIFO_CTRL2 0x08
#define   ADDR_FIFO_CTRL3 0x09
#define   ADDR_FIFO_CTRL4 0x0a

#define   ADDR_WHOAMI     0x0f

#define   ADDR_FIFO_STATUS1       0x1b
#define   ADDR_FIFO_STATUS2       0x1c
#define   ADDR_FIFO_DATA_OUT_TAG  0x78

#define   TAG_QUATERNION  0x13

#define   ADDR_CTRL1      0x10
#define   ADDR_CTRL2      0x11
#define   ADDR_CTRL3      0x12
#define   ADDR_STATUS     0x1e

#define   MASK_BOOTSTATUS 0x08


float halfToFloat(uint16_t halfFloat) {
  uint32_t sign = (halfFloat >> 15) & 1;
  uint32_t exp = (halfFloat >> 10) & 0x1F;
  uint32_t mantissa = halfFloat & 0x3FF;

  if (exp == 0) {
    if (mantissa == 0) return sign ? -0.0f : 0.0f;
    while (!(mantissa & 0x400)) {
      mantissa <<= 1;
      exp--;
    }
    exp++;
    mantissa &= ~0x400;
  } else if (exp == 31) {
    return 0.0f;
  }
  exp = exp + (127 - 15);
  uint32_t floatBits = (sign << 31) | (exp << 23) | (mantissa << 13);
  float result;
  memcpy(&result, &floatBits, 4);
  return result;
}

IMUcontroller_I2C::IMUcontroller_I2C(){}

bool IMUcontroller_I2C::init(uint8_t setSensorID, uint8_t setDeviceAddress, bool checkWAI){
  deviceAddress = setDeviceAddress;
  sensorID = setSensorID;
  if(I2Ccon_getData(deviceAddress, ADDR_WHOAMI) != 0x70 && checkWAI) return false;
  while(true){
    if((I2Ccon_getData(deviceAddress, ADDR_STATUS) & MASK_BOOTSTATUS) == 0) break;
    delay(100);
  }
  I2Ccon_setData(deviceAddress, ADDR_CTRL3, 0x01);
  delay(50);
  I2Ccon_setData(deviceAddress, ADDR_CTRL3, 0x44);
  I2Ccon_setData(deviceAddress, ADDR_FIFO_CTRL2, 0x00);
  I2Ccon_setData(deviceAddress, ADDR_FIFO_CTRL3, 0x00);
  I2Ccon_setData(deviceAddress, ADDR_FIFO_CTRL4, 0x06);
  I2Ccon_setData(deviceAddress, ADDR_FUNC_CFG_ACCESS, 0x80);
  delay(10);
  I2Ccon_setData(deviceAddress, ADDR_EMB_FUNC_EN_A, 0x02);
  I2Ccon_setData(deviceAddress, ADDR_EMB_FUNC_FIFO_EN_A, 0x22);
  I2Ccon_setData(deviceAddress, ADDR_SFLP_ODR, 0x03);
  I2Ccon_setData(deviceAddress, ADDR_FUNC_CFG_ACCESS, 0x00);
  delay(10);
  I2Ccon_setData(deviceAddress, ADDR_CTRL1, 0x64);
  I2Ccon_setData(deviceAddress, ADDR_CTRL2, 0x6c);
  delay(100);
  return true;
}

bool IMUcontroller_I2C::getQuaternions(){
  uint8_t fifoStatus1 = I2Ccon_getData(deviceAddress, ADDR_FIFO_STATUS1);
  uint8_t fifoStatus2 = I2Ccon_getData(deviceAddress, ADDR_FIFO_STATUS2);
  uint16_t fifo_nowBuf = fifoStatus1 | ((fifoStatus2 & 0x01) << 8);

  if(fifo_nowBuf > 0){
    uint8_t fifo_buf[7];
    I2Ccon_getDatas(deviceAddress, ADDR_FIFO_DATA_OUT_TAG, fifo_buf, 7);
    
    uint8_t tag = (fifo_buf[0] >> 3) & 0x1f;

    if(tag == TAG_QUATERNION){
      uint16_t qx = fifo_buf[1] | (fifo_buf[2] << 8);
      uint16_t qy = fifo_buf[3] | (fifo_buf[4] << 8);
      uint16_t qz = fifo_buf[5] | (fifo_buf[6] << 8);

      quaternions[1] = halfToFloat(qx);
      quaternions[2] = halfToFloat(qy);
      quaternions[3] = halfToFloat(qz);
      float w_sum = quaternions[1]*quaternions[1] + quaternions[2]*quaternions[2] + quaternions[3]*quaternions[3];
      if(w_sum <= 1.0f) quaternions[0] = sqrt(1.0f - w_sum);
      else              quaternions[0] = 0.0f;
      return true;
    }
  }
  return false;
}
