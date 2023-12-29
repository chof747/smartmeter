#include "dlsmcrc.h"
#include "esphome/core/log.h"

using namespace esphome::landis_gyr;

uint16_t esphome::landis_gyr::reverseByte(uint16_t b, uint8_t nbits = 8)
{
  uint16_t reflection = 0;
    for (uint8_t bit = 0; bit < nbits; ++bit) {
        if (b & (1 << bit)) {
            reflection |= (1 << ((nbits - 1) - bit));
        }
    }
    return reflection;
}

DlmsCRC::DlmsCRC() : crc(0xFFFF), polynomial(0x1021) {}

void DlmsCRC::calcbyte(uint8_t byte)
{

  uint8_t reversedByte = reverseByte(byte);
  crc ^= (uint16_t)reversedByte << 8;
  
  for (uint8_t j = 0; j < 8; j++)
  {
    if (crc & 0x8000)
      crc = (crc << 1) ^ polynomial;
    else
      crc <<= 1;
  }
}

void DlmsCRC::update(const char *data, size_t length)
{
  for (size_t i = 0; i < length; i++)
  {
    calcbyte((uint8_t)data[i]);
  }
}

uint16_t DlmsCRC::getResult() const
{
  return reverseByte(crc,16) ^ 0xFFFF;
}