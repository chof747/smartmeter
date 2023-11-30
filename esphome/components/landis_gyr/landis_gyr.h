#ifndef LANDIS_GYR_H
#define LANDIS_GYR_H

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/uart/uart.h"

#include <vector>


namespace esphome
{
  namespace landis_gyr
  {

    using namespace uart;


    const uint8_t SYSTEM_NAME_MAX_CNT = 8;
    const uint8_t FRAMEID_LENGTH = 4;
    const uint8_t CIPHER_LENGTH = 76;
    const uint8_t IV_LENGTH = 12;
    const uint8_t TAIL_LENGTH = 12;
    const uint8_t ENERGY_CONSUMPTION_TOTAL_START = 35;
    const uint8_t CURRENT_POWER_USAGE_START = ENERGY_CONSUMPTION_TOTAL_START + 20;
    
    struct ParseContext
    {
      char iv[IV_LENGTH];
      bool secFlg;
      uint32_t frameid;
      char tail[TAIL_LENGTH];
      size_t cipherStart;
      size_t cipherEnd;
    };
    
    const ParseContext EMPTY_CONTEXT = (struct ParseContext) {
      "", false, 0, "", 0, 0
    };

    class LandysGyrReader : public Component, public UARTDevice
    {
    public:
      void setup() override;
      void loop() override;
      void dump_config() override;

      void set_smartmeter_decryption_key(const std::string decryption_key);
      void set_max_message_length(size_t length) { this->max_message_len_ = length; }
      void set_energy_sensor(sensor::Sensor* energy) { this->energy_sensor_ = energy; }
      void set_power_sensor(sensor::Sensor* power) { this->power_sensor_ = power; }

    protected:
      std::vector<uint8_t> decryption_key_{};
      size_t max_message_len_;
      char *message_{nullptr};

      sensor::Sensor *energy_sensor_{nullptr};
      sensor::Sensor *power_sensor_{nullptr};

    private:
      enum ParseState { NONE, SYSNAME_SIZE, SYSNAME, FRAMETYPE, SECFLAG, FRAMEID, CIPHER, TAIL, COMPLETE, ERROR};

      bool parseDecryptionKey(const std::string key);
      void deleteMessage();

      float readValue(esphome::sensor::Sensor *sensor, uint8_t pos, float factor, const char* sensor_name = "");
      float readValue(esphome::sensor::Sensor *sensor, uint8_t pos, float factor, float offset, const char* sensor_name = "");
      float parseMessage();
      bool decryptCypher();
      bool validateMessage();
      void logMessage(size_t len, size_t begin = 0, bool debug=false);
      void logContext();

      size_t pos_=0;
      size_t next_parse_pos_=0;
      ParseState state_=ParseState::NONE;
      ParseContext ctx_;

      
    };


  } // namespace: landis_gyr
} // namespce esphome

#endif // LANDIS_GYR_H