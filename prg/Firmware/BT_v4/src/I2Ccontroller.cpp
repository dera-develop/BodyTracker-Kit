#include <Arduino.h>
#include <Wire.h>

void I2Ccon_init_I2C(uint32_t I2C_clock){
  delay(50);
  Wire.begin();
  Wire.setClock(I2C_clock);
  delay(50);
}

uint8_t I2Ccon_getData(uint8_t deviceAddress, uint8_t targetAddress){
  Wire.beginTransmission(deviceAddress);
  Wire.write(targetAddress);
  Wire.endTransmission(false);
  Wire.requestFrom(deviceAddress, 1);
  while (Wire.available()){
    return Wire.read();
  }
}

void I2Ccon_getDatas(uint8_t deviceAddress, uint8_t targetAddress, uint8_t* buffer, uint8_t dataLength){
  Wire.beginTransmission(deviceAddress);
  Wire.write(targetAddress);
  Wire.endTransmission(false);
  Wire.requestFrom(deviceAddress, dataLength);
  for(uint8_t i = 0;i < dataLength;i++){
    buffer[i] = Wire.read();
  }
}

void I2Ccon_setData (uint8_t deviceAddress, uint8_t targetAddress, uint8_t setValue){
  Wire.beginTransmission(deviceAddress);
  Wire.write(targetAddress);
  Wire.write(setValue);
  Wire.endTransmission();
}