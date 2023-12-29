#include "landis_gyr.h"
#include "esphome/core/log.h"
#include <Crypto.h>
#include <AES.h>
#include <GCM.h>

#include "dlsmcrc.h"

using namespace esphome::landis_gyr;

static const char *const TAG = "landys_gyr";

#define DISABLE_ENCRYPTION()             \
  ESP_LOGI(TAG, "Disabling decryption"); \
  return;

uint32_t bigToLittleEndian(uint32_t value)
{
  return ((value & 0xFF) << 24) |
         (((value >> 8) & 0xFF) << 16) |
         (((value >> 16) & 0xFF) << 8) |
         ((value >> 24) & 0xFF);
}

uint16_t bigToLittleEndian(uint16_t value)
//***************************************************************************************
{
  return ((value & 0xFF) << 8) | ((value >> 8) & 0xFF);
}

void LandysGyrReader::setup()
//***************************************************************************************
{
  this->message_ = new char[this->max_message_len_];
}

void LandysGyrReader::loop()
//***************************************************************************************
{
  if (EXPECTED_TOTAL_MESSAGE_LENGTH >= Serial.available())
  {
    readSerial();
  }
}

void LandysGyrReader::readSerial()
//***************************************************************************************
{
  int block = 0;
  static int beg;
  static uint8_t lastByte = 0x00;

  while ((block = available()))
  {
    ESP_LOGV(TAG, "New block of data of %u bytes", block);
    for (int i = 0; i < block; ++i)
    {
      uint8_t b = read();
      // yield();

      switch (state_)
      {

      case ParseState::NONE:
        if ((0xa0 == b) && (0x7e == lastByte))
        {
          ESP_LOGV(TAG, "Starting new block pos was %u", pos_);
          state_ = ParseState::SIZE;
          pos_ = 0;

          for (i = 0; i < max_message_len_; ++i)
            message_[i] = 0;

          ctx_ = EMPTY_CONTEXT;
          message_[pos_++] = 0x7e;
          beg = 1;
        }
        break;

      case ParseState::SIZE:
      {
        ctx_.messageLength = b;
        ESP_LOGV(TAG, "Size of message %u", ctx_.messageLength);
        next_parse_pos_ = pos_ + HEAD_LENGTH;
        state_ = ParseState::TELEGRAM;
      }
      break;

      case ParseState::TELEGRAM:
        if (pos_ == next_parse_pos_)
        {
          if (0xdb == b)
          {
            state_ = ParseState::SYSNAME_SIZE;
            ctx_.telegramStart = pos_;
            ESP_LOGV(TAG, "STEP00: Started Reading a telegram!");
          }
          else
          {
            ESP_LOGE(TAG, "Wrong start for Telegram expected 0xDB got %02x", b);
            state_ = ParseState::ERROR;
          }
        }
        break;

      case ParseState::SYSNAME_SIZE:
        if (SYSTEM_NAME_MAX_CNT < b)
        {
          ESP_LOGE(TAG, "Wrong size for system name received %d instead of max %d", b, SYSTEM_NAME_MAX_CNT);
          state_ = ParseState::ERROR;
          break;
        }
        next_parse_pos_ = pos_ + b;
        beg = pos_ + 1;
        state_ = ParseState::SYSNAME;
        ESP_LOGV(TAG, "STEP01: System Name Size = %d", b);
        break;

      case ParseState::SYSNAME:
        if (pos_ == next_parse_pos_)
        {
          memcpy(ctx_.iv, message_ + beg, pos_ - beg);
          ctx_.iv[pos_ - beg] = b; // we are at the position of the last byte so add it extra
          state_ = ParseState::FRAMETYPE;
          ESP_LOGV(TAG, "STEP02: System Name Read!");
        }
        break;

      case ParseState::FRAMETYPE:
        if (0x4f != b)
        {
          ESP_LOGE(TAG, "Invalid message block received, frametype not 4F but %02x!", b);
          state_ = ParseState::ERROR;
          break;
        }

        ESP_LOGV(TAG, "STEP03: Frame type confirmed!");
        state_ = ParseState::SECFLAG;
        break;

      case ParseState::SECFLAG:
        ctx_.secFlg = (0x10 != b);

        state_ = ParseState::FRAMEID;
        next_parse_pos_ = pos_ + FRAMEID_LENGTH;
        beg = pos_ + 1;
        ESP_LOGV(TAG, "STEP04: Secflag = %d", ctx_.secFlg);
        break;

      case ParseState::FRAMEID:
        if (pos_ == next_parse_pos_)
        {
          memcpy(&ctx_.frameid, message_ + beg, FRAMEID_LENGTH);
          char *ls = reinterpret_cast<char *>(&ctx_.frameid) + 3;
          memcpy(ls, &b, 1);

          memcpy(ctx_.iv + 8, &ctx_.frameid, FRAMEID_LENGTH);

          next_parse_pos_ = pos_ + CIPHER_LENGTH;
          ctx_.cipherStart = pos_ + 1;
          state_ = ParseState::CIPHER;
          ESP_LOGV(TAG, "STEP05: FrameID = %u", ctx_.frameid);
          logMessage(pos_, 0, false);
        }
        break;

      case ParseState::CIPHER:
        if (pos_ == next_parse_pos_)
        {
          state_ = ParseState::CRC;
          next_parse_pos_ = pos_ + TAIL_LENGTH;
          beg = pos_;
          ctx_.cipherEnd = beg;
          ESP_LOGV(TAG, "STEP06: Cypher from %u to %u - last: %02x ", ctx_.cipherStart, ctx_.cipherEnd - 1, message_[pos_ - 1]);

          logMessage(pos_, 0);
        }
        break;

      case ParseState::CRC:
        if (pos_ == next_parse_pos_)
        {
          memcpy(&ctx_.crc, message_ + beg, 2);
          //ctx_.crc = bigToLittleEndian(ctx_.crc);
          if (0x7e == b)
          {
            DlmsCRC crc;
            crc.update(message_ + 1, pos_ - 3);
            uint16_t hcrc = crc.getResult();
            ESP_LOGV(TAG, "CRC msg = %02x %02x", hcrc >> 8, hcrc & 0xFF);

            if (ctx_.crc != hcrc)
            {
              state_ = ParseState::ERROR;
              ESP_LOGW(TAG, "Skipping message: CRC received = 0x%02x%02x CRC calculated from message = 0x%02x%02x", ctx_.crc >> 8, ctx_.crc & 0xFF, hcrc >> 8, hcrc & 0xFF);
            }
            else
            {
              processTelegram();
              state_ = ParseState::NONE;
            }
          }

          else
          {
            ESP_LOGE(TAG, "Invalid end of message = %02x!", b);
            state_ = ParseState::ERROR;
          }
        }
        break;

      case ParseState::ERROR:
        // should net be reached
        continue;

      default:
        return;
      }

      ESP_LOGV(TAG, "State: %d, Byte: %02x", state_, b);

      if (ParseState::ERROR == state_)
      {
        state_ = ParseState::NONE;
        ESP_LOGD(TAG, "Wrong message: ============");
        logMessage(pos_, 0, true);
        pos_ = 0;
      }
      else if (ParseState::NONE != state_)
      {
        message_[pos_++] = b;
      }

      lastByte = b;
    }
  }
}

void LandysGyrReader::processTelegram()
//***************************************************************************************
{
  logContext();
  state_ = ParseState::NONE;

  decryptCypher();

  if (!validateMessage())
  {
    state_ = ParseState::ERROR;
  }
  
  float econsumed = parseMessage();
  ESP_LOGD(TAG, "Read frameID: %u energy = %.2f", bigToLittleEndian(ctx_.frameid), econsumed);
  ESP_LOGV(TAG, "Correct message: ============");
  logMessage(pos_, 0, false);
  // delay(100);
}

void LandysGyrReader::dump_config()
//***************************************************************************************
{
  ESP_LOGCONFIG(TAG, "Landis-Gyr Smart Meter:");
  ESP_LOGCONFIG(TAG, "  Maximum message length: %d", this->max_message_len_);
  ESP_LOGCONFIG(TAG, "  Decryption is %sactivated", (0 == this->decryption_key_.size()) ? "de" : "");
}

void LandysGyrReader::set_smartmeter_decryption_key(const std::string decryption_key)
//***************************************************************************************
{
  if (decryption_key.length() == 0)
  {
    DISABLE_ENCRYPTION();
  }

  if (decryption_key.length() != 32)
  {
    ESP_LOGE(TAG, "Error, decryption key must be 32 character long");
    return;
  }

  this->decryption_key_.clear();

  ESP_LOGI(TAG, "Decryption is activated");
  // Verbose level prints decryption key
  ESP_LOGV(TAG, "Using key for message decryption: %s", decryption_key.c_str());

  if (!parseDecryptionKey(decryption_key))
  {
    DISABLE_ENCRYPTION();
    return;
  }
}

bool LandysGyrReader::parseDecryptionKey(const std::string key)
//***************************************************************************************
{
  char token[3] = {0};
  for (int i = 0; i < 16; i++)
  {
    strncpy(token, &(key.c_str()[i * 2]), 2);
    uint8_t chiffre = std::strtoul(token, nullptr, 16);

    if ((0 == chiffre) && (0 == strcmp("00", token)))
    {
      ESP_LOGW(TAG, "%s at position %d of the encryption key is not a valid hex value!", token, i);
      return false;
    }

    this->decryption_key_.push_back(chiffre);
  }

  return true;
}

inline void LandysGyrReader::deleteMessage()
//***************************************************************************************
{
  delete[] this->message_;
  this->message_ = nullptr;
}

bool LandysGyrReader::decryptCypher()
//***************************************************************************************
{
  GCM<AES128> *gcmaes128 = 0;
  gcmaes128 = new GCM<AES128>();
  gcmaes128->setKey(decryption_key_.data(), decryption_key_.size());
  gcmaes128->setIV((const uint8_t *)ctx_.iv, IV_LENGTH);
  gcmaes128->decrypt((uint8_t *)(message_ + ctx_.cipherStart), (uint8_t *)(message_ + ctx_.cipherStart), ctx_.cipherEnd - ctx_.cipherStart + 2);
  delete gcmaes128;
  return true;
}

bool LandysGyrReader::validateMessage()
//***************************************************************************************
{
  char *p = message_ + ctx_.cipherStart;
  char *nextp = p;

  // check if byte 2 - 5 of the cipher are equal to the frame id
  p++;
  uint32_t decrypted_frame_id;
  std::memcpy(&decrypted_frame_id, p, sizeof(uint32_t));
  // yield();

  if (ctx_.frameid != decrypted_frame_id)
  {
    ESP_LOGE(TAG, "Decrypted message has the wrong frame id %04d instead of %04d", decrypted_frame_id, ctx_.frameid);
    return false;
  }

  // check if the datetime is present both from position 7 - 18 and from 23 - 34
  p = p + 5;
  nextp = p + 12;
  while (p < nextp)
  {
    // ESP_LOGD(TAG, "%02x, %02x", *p, *(p+16));
    if (std::memcmp(p, p + 16, sizeof(char)) != 0)
    {
      ESP_LOGE(TAG, "Date time is not equal at the beginning of the telegram!");
      return false;
    }
    p++;
    // yield();
  }

  // check if the separators 0x06 are where they should be
  p += 16;

  for (int i = 0; i < 8; ++i)
  {
    if (6 != (*p))
    {
      ESP_LOGE(TAG, "Separator before value blog %d is not 0x06!", i);
      return false;
    }
    p += 5;
    // yield();
  }

  return true;
}

float LandysGyrReader::readValue(esphome::sensor::Sensor *sensor, uint8_t start, float factor, const char *sensor_name)
//***************************************************************************************
{
  return readValue(sensor, start, factor, 0, sensor_name);
}

float LandysGyrReader::readValue(esphome::sensor::Sensor *sensor, uint8_t start, float factor, float offset, const char *sensor_name)
//***************************************************************************************
{
  if (nullptr != sensor)
  {
    uint32_t sensor_raw;
    float sensor_value;

    char *p = message_ + ctx_.cipherStart + start;
    memcpy(&sensor_raw, p, sizeof(uint32_t));
    sensor_raw = bigToLittleEndian(sensor_raw);

    ESP_LOGV(TAG, "%s raw value: %u", sensor_name, sensor_raw);

    sensor_value = sensor_raw / factor + offset;
    sensor->publish_state(sensor_value);

    return sensor_value;
  }
  else
  {
    return 0;
  }
}

float LandysGyrReader::parseMessage()
//***************************************************************************************
{
  float result = 0;
  result = readValue(energy_sensor_, ENERGY_CONSUMPTION_TOTAL_START, 1000.0, "Energy consumption total");
  readValue(power_sensor_, CURRENT_POWER_USAGE_START, 1.0, "Current Power Usage");

  return result;
}

void LandysGyrReader::logMessage(size_t len, size_t begin, bool debug)
//***************************************************************************************
{
  char row[50];
  int f = 1;

  for (size_t i = begin; i < (len + begin); ++i)
  {
    char c = message_[i];
    sprintf(row + ((i % 16) * 3), "%02x ", c);

    if (0 == (i + 1) % 16)
    {
      if (debug)
      {
        ESP_LOGD(TAG, "%04d : %s", f++, row);
      }
      else
      {
        ESP_LOGV(TAG, "%04d : %s", f++, row);
      }

      // yield();
    }
  }

  if (debug)
  {
    ESP_LOGD(TAG, "%04d : %s", f++, row);
  }
  else
  {
    ESP_LOGV(TAG, "%04d : %s", f++, row);
  }
}

void LandysGyrReader::logContext()
//***************************************************************************************
{
  char hexstr[50];

  for (int i = 0; i < IV_LENGTH; ++i)
  {
    sprintf(hexstr + ((i % 16) * 3), "%02x ", ctx_.iv[i]);
  }
  ESP_LOGV(TAG, "IV   = %s", hexstr);
  // yield();

  ESP_LOGV(TAG, "CRC = %04d", ctx_.crc);

  logMessage(pos_ - ctx_.cipherStart - TAIL_LENGTH, ctx_.cipherStart, false);
}