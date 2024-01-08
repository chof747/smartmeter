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
    const uint8_t CIPHER_LENGTH = 75;
    const uint8_t IV_LENGTH = 12;
    const uint8_t HEAD_LENGTH = 10;
    const uint8_t TAIL_LENGTH = 2;
    const uint8_t ENERGY_CONSUMPTION_TOTAL_START = 35;
    const uint8_t ENERGY_PRODUCTION_TOTAL_START = ENERGY_CONSUMPTION_TOTAL_START + 5;
    const uint8_t BLIND_ENERGY_CONSUMPTION_TOTAL_START = ENERGY_CONSUMPTION_TOTAL_START + 10;
    const uint8_t BLIND_ENERGY_GENERATED_TOTAL_START = ENERGY_CONSUMPTION_TOTAL_START + 15;
    const uint8_t CURRENT_POWER_USAGE_START = ENERGY_CONSUMPTION_TOTAL_START + 20;
    const uint8_t CURRENT_POWER_PRODUCED_START = ENERGY_CONSUMPTION_TOTAL_START + 25;
    const uint8_t CURRENT_BLINDPOWERIN_START = ENERGY_CONSUMPTION_TOTAL_START + 30;
    const uint8_t CURRENT_BLINDPOWEROUT_START = ENERGY_CONSUMPTION_TOTAL_START + 35;
    const uint8_t EXPECTED_TOTAL_MESSAGE_LENGTH = 105;

    struct ParseContext
    {
      char iv[IV_LENGTH];
      bool secFlg;
      uint32_t frameid;
      uint16_t crc;
      size_t cipherStart;
      size_t cipherEnd;
      size_t messageLength;
      size_t telegramStart;
    };

    const ParseContext EMPTY_CONTEXT = (struct ParseContext){
        "", false, 0, 0, 0, 0, 0, 0};

    class LandysGyrReader : public Component, public UARTDevice
    {
    public:
      void setup() override;
      void loop() override;
      void dump_config() override;

      void set_smartmeter_decryption_key(const std::string decryption_key);
      void set_max_message_length(size_t length) { this->max_message_len_ = length; }
      void set_energy_sensor(sensor::Sensor *energy_sensor) { this->energy_sensor_ = energy_sensor; }
      void set_power_sensor(sensor::Sensor *power_sensor) { this->power_sensor_ = power_sensor; }
      void set_powerout_sensor(sensor::Sensor *powerout_sensor) { this->powerout_sensor_ = powerout_sensor; }
      void set_energyout_sensor(sensor::Sensor *energyout_sensor) { this->energyout_sensor_ = energyout_sensor; }
      void set_blindenergyin_sensor(sensor::Sensor *blindenergyin_sensor) { this->blindenergyin_sensor_ = blindenergyin_sensor; }
      void set_blindenergyout_sensor(sensor::Sensor *blindenergyout_sensor) { this->blindenergyout_sensor_ = blindenergyout_sensor; }
      void set_blindpowerin_sensor(sensor::Sensor *blindpowerin_sensor) { this->blindpowerin_sensor_ = blindpowerin_sensor; }
      void set_blindpowerout_sensor(sensor::Sensor *blindpowerout_sensor) { this->blindpowerout_sensor_ = blindpowerout_sensor; }
       void set_telegram_count_sensor(sensor::Sensor *sensor) { telegram_count_sensor_ = sensor; }

      uint16_t getTelegramCountOverLastHour();

    protected:
      std::vector<uint8_t> decryption_key_{};
      size_t max_message_len_;
      char *message_{nullptr};

      sensor::Sensor *energy_sensor_{nullptr};
      sensor::Sensor *power_sensor_{nullptr};
      sensor::Sensor *powerout_sensor_{nullptr};
      sensor::Sensor *energyout_sensor_{nullptr};
      sensor::Sensor *blindenergyin_sensor_{nullptr};
      sensor::Sensor *blindenergyout_sensor_{nullptr};
      sensor::Sensor *blindpowerin_sensor_{nullptr};
      sensor::Sensor *blindpowerout_sensor_{nullptr};
      sensor::Sensor *telegram_count_sensor_{nullptr};

    private:
      enum ParseState
      {
        NONE,
        SIZE,
        TELEGRAM,
        SYSNAME_SIZE,
        SYSNAME,
        FRAMETYPE,
        SECFLAG,
        FRAMEID,
        CIPHER,
        CRC,
        COMPLETE,
        ERROR
      };

      bool parseDecryptionKey(const std::string key);
      void deleteMessage();

      void readSerial();
      uint16_t checkSecurityFlag(uint8_t b);
      void checkFrameType(uint8_t b);
      void readMessageSize(uint8_t b);
      void startReadingFrame(uint8_t b);
      void checkCRC();
      void startReadingTelegram();
      uint16_t readSystemNameSize(uint8_t b);
      void readSystemName(u_int16_t beg, uint8_t b);

      void processTelegram();
      float parseMessage();
      bool decryptCypher();
      bool validateMessage();

      float readValue(esphome::sensor::Sensor *sensor, uint8_t pos, float factor, const char *sensor_name = "");
      float readValue(esphome::sensor::Sensor *sensor, uint8_t pos, float factor, float offset, const char *sensor_name = "");

      void updateTelegramCounter();

      void logMessage(size_t len, size_t begin = 0, bool debug = false);
      void logContext();

      size_t pos_ = 0;
      size_t next_parse_pos_ = 0;
      ParseState state_ = ParseState::NONE;
      ParseContext ctx_;
      std::array<uint16_t, 60> telegrams_received_{};
      size_t rix_ = 0;
      uint32_t lastminute_update_ = 0;
    };

  } // namespace: landis_gyr
} // namespce esphome

#endif // LANDIS_GYR_H