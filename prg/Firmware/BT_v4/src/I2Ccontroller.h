#pragma once

#include <Arduino.h>

void I2Ccon_init_I2C(uint32_t I2C_clock=100000);
uint8_t I2Ccon_getData   (uint8_t deviceAddress, uint8_t targetAddress);
void    I2Ccon_getDatas  (uint8_t deviceAddress, uint8_t targetAddress, uint8_t* buffer, uint8_t dataLength);
void    I2Ccon_setData   (uint8_t deviceAddress, uint8_t targetAddress, uint8_t setValue);