#include <Arduino.h>
#include "UDPpackage.h"

UDPpackageCreator::UDPpackageCreator(){
  pos = 0;
}

void UDPpackageCreator::reset(){
  pos = 0;
}

void UDPpackageCreator::addUint32(uint32_t data){
  for(int8_t i = 24;i >= 0;i-=8){
    buf[pos++] = (uint8_t)((data >> i) & 0xff);
  }
}

void UDPpackageCreator::addUint64(uint64_t data){
  for(int8_t i = 56;i >= 0;i-=8){
    buf[pos++] = (uint8_t)((data >> i) & 0xff);
  }
}

void UDPpackageCreator::addFloat(float data){
  union { float fv; uint32_t iv;}  uData;
  uData.fv = data;
  addUint32(uData.iv);
}

void UDPpackageCreator::addString(const char* data){
  uint8_t len = strlen(data);
  buf[pos++] = len;
  for(uint8_t i = 0;i < len;i++){
    buf[pos++] = (uint8_t)data[i];
  }
}

void UDPpackageCreator::addUint8(uint8_t data){
  buf[pos++] = data;
}

void UDPpackageCreator::addBytes(uint8_t* datas, uint8_t len){
  for(uint8_t i = 0;i < len;i++){
    buf[pos++] = datas[i];
  }
}

uint8_t* UDPpackageCreator::getUDPdatas(){
  return buf;
}

uint8_t UDPpackageCreator::getSize(){
  return pos;
}