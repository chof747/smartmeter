#include "landis_gyr.h"
#include "esphome/core/log.h"

using namespace esphome::landis_gyr;

static const char *const TAG = "landys_gyr";

#define DISABLE_ENCRYPTION()             \
  ESP_LOGI(TAG, "Disabling decryption"); \
  return;

void LandysGyrReader::setup()
//***************************************************************************************
{
  this->message_ = new char[this->max_message_len_];
}

void LandysGyrReader::loop()
//***************************************************************************************
{
  static bool reading_frame = false;

  int block = 0, beg = 0;
  pos_ = 0;

  while ((block = available()))
  {
    for (int i = 0; i < block; ++i)
    {
      uint8_t b = read();
      yield();

      switch (state_)
      {
      case ParseState::NONE:
        if (0xdb == b)
        {
          ctx_ = EMPTY_CONTEXT;
          state_ = ParseState::SYSNAME_SIZE;
          ESP_LOGV(TAG, "STEP00: Started Reading a telegram!");
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
          ctx_.iv[pos_ - beg] = b; //we are at the position of the last byte so add it extra
          state_ = ParseState::FRAMETYPE;
        }
        ESP_LOGV(TAG, "STEP02: System Name Read!");
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
        ESP_LOGV(TAG, "STEP03: Secflag = %d", ctx_.secFlg);
        break;

      case ParseState::FRAMEID:
        if (pos_ == next_parse_pos_)
        {
          memcpy(&ctx_.frameid, message_ + beg, FRAMEID_LENGTH);
          char* ls = reinterpret_cast<char*>(&ctx_.frameid) + 3;
          memcpy(ls, &b, 1);

          memcpy(ctx_.iv + 8, &ctx_.frameid, FRAMEID_LENGTH);

          next_parse_pos_ = pos_ + CIPHER_LENGTH;
          ctx_.cipherStart = pos_ + 1;
          state_ = ParseState::CIPHER;
          ESP_LOGV(TAG, "STEP03: FrameID = %d", ctx_.frameid);
        }
        break;

      case ParseState::CIPHER:
        if (pos_ == next_parse_pos_)
        {
          state_ = ParseState::TAIL;
          next_parse_pos_ = pos_ + TAIL_LENGTH;
          beg = pos_ + 1;
        }
        break;

      case ParseState::TAIL:
        if (pos_ == next_parse_pos_)
        {
          memcpy(ctx_.tail, message_ + beg, pos_ - beg);
          ctx_.tail[pos_ - beg] = b; //we are at the position of the last byte so add it extra
          state_ = ParseState::COMPLETE;
        }
        break;

      case ParseState::COMPLETE:
        if (0x00 != b)
        {
          ESP_LOGE(TAG, "Invalid end of message!");
          state_ = ParseState::ERROR;
          break;
        }
        logContext();
        state_ = ParseState::NONE;
        break;

      default:
        return;
      }

      ESP_LOGV(TAG, "State: %d, Byte: %02x", state_, b);


      if (ParseState::ERROR == state_)
      {
        state_ = ParseState::NONE;
        pos_ = 0;
      }
      else if (ParseState::NONE != state_)
      {
        message_[pos_++] = b;
      }
    }

    ESP_LOGD(TAG, "Block size = %d", block);
  }
  if (pos_ > 0)
  {
    logMessage(pos_);
  }
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

bool LandysGyrReader::parseMessage(size_t len)
//***************************************************************************************
{
  logMessage(len);

  return false;
}

void LandysGyrReader::logMessage(size_t len, size_t begin)
//***************************************************************************************
{
  char row[50];
  int f = 1;

  for (size_t i = begin; i < (len + begin); ++i)
  {
    sprintf(row + ((i % 16) * 3), "%02x ", message_[i]);

    if (0 == (i + 1) % 16)
    {
      ESP_LOGD(TAG, "%04d : %s", f++, row);
      yield();
    }
  }

  ESP_LOGD(TAG, "%04d : %s", f++, row);
}

void LandysGyrReader::logContext()
//***************************************************************************************
{
  char hexstr[50];

  for (int i = 0; i < IV_LENGTH; ++i)
  {
    sprintf(hexstr + ((i % 16) * 3), "%02x ", ctx_.iv[i]);
  }
  ESP_LOGD(TAG, "IV   = %s", hexstr);

  for (int i = 0; i < TAIL_LENGTH; ++i)
  {
    sprintf(hexstr + ((i % 16) * 3), "%02x ", ctx_.tail[i]);
  }
  ESP_LOGD(TAG, "TAIL = %s", hexstr);

  logMessage(pos_ - ctx_.cipherStart - TAIL_LENGTH, ctx_.cipherStart);
}