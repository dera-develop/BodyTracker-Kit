#pragma once
#include <Arduino.h>

class UDPpackageCreator{
  private:
    uint8_t buf[128];
    uint8_t pos;

  public:
    UDPpackageCreator();
    void reset();
    void addUint32(uint32_t data);
    void addUint64(uint64_t data);
    void addFloat(float data);
    void addString(const char* data);
    void addUint8(uint8_t data);
    void addBytes(uint8_t* datas, uint8_t len);
    uint8_t* getUDPdatas();
    uint8_t getSize();
};