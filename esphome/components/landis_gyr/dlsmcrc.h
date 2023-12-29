#ifndef DLMSCRC_H
#define DLMSCRC_H

#include <Arduino.h>

namespace esphome
{
  namespace landis_gyr
  {
    uint16_t reverseByte(uint16_t b, uint8_t nbits);

    class DlmsCRC
    {
    public:
      DlmsCRC();

      void update(const char *data, size_t length);
      uint16_t getResult() const;
      

    private:
      uint16_t crc;
      uint16_t polynomial;
      void calcbyte(uint8_t byte);
    };

  };
};

#endif // DLMSCRC_Hc