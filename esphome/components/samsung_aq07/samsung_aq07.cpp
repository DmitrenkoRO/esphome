#include "samsung_aq07.h"
#include "esphome/components/remote_base/remote_base.h"

namespace esphome {
namespace samsung_aq07 {

static const char *const TAG = "samsung_aq07.climate";

void SamsungAq07Climate::transmit_state() {
  uint8_t remote_state[35] = {0x11, 0xDA, 0x27, 0x00, 0xC5, 0x00, 0x00, 0xD7, 0x11, 0xDA, 0x27, 0x00,
                              0x42, 0x49, 0x05, 0xA2, 0x11, 0xDA, 0x27, 0x00, 0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x06, 0x60, 0x00, 0x00, 0xC0, 0x00, 0x00, 0x00};

  remote_state[21] = this->operation_mode_();
  remote_state[22] = this->temperature_();
  uint16_t fan_speed = this->fan_speed_();
  remote_state[24] = fan_speed >> 8;
  remote_state[25] = fan_speed & 0xff;

  // Calculate checksum
  for (int i = 16; i < 34; i++) {
    remote_state[34] += remote_state[i];
  }

  auto transmit = this->transmitter_->transmit();
  auto *data = transmit.get_data();
  data->set_carrier_frequency(SAMSUNG_AQ07_IR_FREQUENCY);

  data->mark(SAMSUNG_AQ07_HEADER_MARK);
  data->space(SAMSUNG_AQ07_HEADER_SPACE);
  for (int i = 0; i < 8; i++) {
    for (uint8_t mask = 1; mask > 0; mask <<= 1) {  // iterate through bit mask
      data->mark(SAMSUNG_AQ07_BIT_MARK);
      bool bit = remote_state[i] & mask;
      data->space(bit ? SAMSUNG_AQ07_ONE_SPACE : SAMSUNG_AQ07_ZERO_SPACE);
    }
  }
  data->mark(SAMSUNG_AQ07_BIT_MARK);
  data->space(SAMSUNG_AQ07_MESSAGE_SPACE);
  data->mark(SAMSUNG_AQ07_HEADER_MARK);
  data->space(SAMSUNG_AQ07_HEADER_SPACE);

  for (int i = 8; i < 16; i++) {
    for (uint8_t mask = 1; mask > 0; mask <<= 1) {  // iterate through bit mask
      data->mark(SAMSUNG_AQ07_BIT_MARK);
      bool bit = remote_state[i] & mask;
      data->space(bit ? SAMSUNG_AQ07_ONE_SPACE : SAMSUNG_AQ07_ZERO_SPACE);
    }
  }
  data->mark(SAMSUNG_AQ07_BIT_MARK);
  data->space(SAMSUNG_AQ07_MESSAGE_SPACE);
  data->mark(SAMSUNG_AQ07_HEADER_MARK);
  data->space(SAMSUNG_AQ07_HEADER_SPACE);

  for (int i = 16; i < 35; i++) {
    for (uint8_t mask = 1; mask > 0; mask <<= 1) {  // iterate through bit mask
      data->mark(SAMSUNG_AQ07_BIT_MARK);
      bool bit = remote_state[i] & mask;
      data->space(bit ? SAMSUNG_AQ07_ONE_SPACE : SAMSUNG_AQ07_ZERO_SPACE);
    }
  }
  data->mark(SAMSUNG_AQ07_BIT_MARK);
  data->space(0);

  transmit.perform();
}

uint8_t SamsungAq07Climate::operation_mode_() {
  uint8_t operating_mode = SAMSUNG_AQ07_MODE_ON;
  switch (this->mode) {
    case climate::CLIMATE_MODE_COOL:
      operating_mode |= SAMSUNG_AQ07_MODE_COOL;
      break;
    case climate::CLIMATE_MODE_DRY:
      operating_mode |= SAMSUNG_AQ07_MODE_DRY;
      break;
    case climate::CLIMATE_MODE_HEAT:
      operating_mode |= SAMSUNG_AQ07_MODE_HEAT;
      break;
    case climate::CLIMATE_MODE_HEAT_COOL:
      operating_mode |= SAMSUNG_AQ07_MODE_AUTO;
      break;
    case climate::CLIMATE_MODE_FAN_ONLY:
      operating_mode |= SAMSUNG_AQ07_MODE_FAN;
      break;
    case climate::CLIMATE_MODE_OFF:
    default:
      operating_mode = SAMSUNG_AQ07_MODE_OFF;
      break;
  }

  return operating_mode;
}

uint16_t SamsungAq07Climate::fan_speed_() {
  uint16_t fan_speed;
  switch (this->fan_mode.value()) {
    case climate::CLIMATE_FAN_LOW:
      fan_speed = SAMSUNG_AQ07_FAN_1 << 8;
      break;
    case climate::CLIMATE_FAN_MEDIUM:
      fan_speed = SAMSUNG_AQ07_FAN_3 << 8;
      break;
    case climate::CLIMATE_FAN_HIGH:
      fan_speed = SAMSUNG_AQ07_FAN_5 << 8;
      break;
    case climate::CLIMATE_FAN_AUTO:
    default:
      fan_speed = SAMSUNG_AQ07_FAN_AUTO << 8;
  }

  // If swing is enabled switch first 4 bits to 1111
  switch (this->swing_mode) {
    case climate::CLIMATE_SWING_VERTICAL:
      fan_speed |= 0x0F00;
      break;
    case climate::CLIMATE_SWING_HORIZONTAL:
      fan_speed |= 0x000F;
      break;
    case climate::CLIMATE_SWING_BOTH:
      fan_speed |= 0x0F0F;
      break;
    default:
      break;
  }
  return fan_speed;
}

uint8_t SamsungAq07Climate::temperature_() {
  // Force special temperatures depending on the mode
  switch (this->mode) {
    case climate::CLIMATE_MODE_FAN_ONLY:
      return 0x32;
    case climate::CLIMATE_MODE_HEAT_COOL:
    case climate::CLIMATE_MODE_DRY:
      return 0xc0;
    default:
      uint8_t temperature = (uint8_t) roundf(clamp<float>(this->target_temperature, SAMSUNG_AQ07_TEMP_MIN, SAMSUNG_AQ07_TEMP_MAX));
      return temperature << 1;
  }
}

bool SamsungAq07Climate::parse_state_frame_(const uint8_t frame[]) {
  uint8_t checksum = 0;
  for (int i = 0; i < (SAMSUNG_AQ07_STATE_FRAME_SIZE - 1); i++) {
    checksum += frame[i];
  }
  if (frame[SAMSUNG_AQ07_STATE_FRAME_SIZE - 1] != checksum)
    return false;
  uint8_t mode = frame[5];
  if (mode & SAMSUNG_AQ07_MODE_ON) {
    switch (mode & 0xF0) {
      case SAMSUNG_AQ07_MODE_COOL:
        this->mode = climate::CLIMATE_MODE_COOL;
        break;
      case SAMSUNG_AQ07_MODE_DRY:
        this->mode = climate::CLIMATE_MODE_DRY;
        break;
      case SAMSUNG_AQ07_MODE_HEAT:
        this->mode = climate::CLIMATE_MODE_HEAT;
        break;
      case SAMSUNG_AQ07_MODE_AUTO:
        this->mode = climate::CLIMATE_MODE_HEAT_COOL;
        break;
      case SAMSUNG_AQ07_MODE_FAN:
        this->mode = climate::CLIMATE_MODE_FAN_ONLY;
        break;
    }
  } else {
    this->mode = climate::CLIMATE_MODE_OFF;
  }
  uint8_t temperature = frame[6];
  if (!(temperature & 0xC0)) {
    this->target_temperature = temperature >> 1;
  }
  uint8_t fan_mode = frame[8];
  uint8_t swing_mode = frame[9];
  if (fan_mode & 0xF && swing_mode & 0xF) {
    this->swing_mode = climate::CLIMATE_SWING_BOTH;
  } else if (fan_mode & 0xF) {
    this->swing_mode = climate::CLIMATE_SWING_VERTICAL;
  } else if (swing_mode & 0xF) {
    this->swing_mode = climate::CLIMATE_SWING_HORIZONTAL;
  } else {
    this->swing_mode = climate::CLIMATE_SWING_OFF;
  }
  switch (fan_mode & 0xF0) {
    case SAMSUNG_AQ07_FAN_1:
    case SAMSUNG_AQ07_FAN_2:
    case SAMSUNG_AQ07_FAN_SILENT:
      this->fan_mode = climate::CLIMATE_FAN_LOW;
      break;
    case SAMSUNG_AQ07_FAN_3:
      this->fan_mode = climate::CLIMATE_FAN_MEDIUM;
      break;
    case SAMSUNG_AQ07_FAN_4:
    case SAMSUNG_AQ07_FAN_5:
      this->fan_mode = climate::CLIMATE_FAN_HIGH;
      break;
    case SAMSUNG_AQ07_FAN_AUTO:
      this->fan_mode = climate::CLIMATE_FAN_AUTO;
      break;
  }
  this->publish_state();
  return true;
}

bool SamsungAq07Climate::on_receive(remote_base::RemoteReceiveData data) {
  uint8_t state_frame[SAMSUNG_AQ07_STATE_FRAME_SIZE] = {};
  if (!data.expect_item(SAMSUNG_AQ07_HEADER_MARK, SAMSUNG_AQ07_HEADER_SPACE)) {
    return false;
  }
  for (uint8_t pos = 0; pos < SAMSUNG_AQ07_STATE_FRAME_SIZE; pos++) {
    uint8_t byte = 0;
    for (int8_t bit = 0; bit < 8; bit++) {
      if (data.expect_item(SAMSUNG_AQ07_BIT_MARK, SAMSUNG_AQ07_ONE_SPACE)) {
        byte |= 1 << bit;
      } else if (!data.expect_item(SAMSUNG_AQ07_BIT_MARK, SAMSUNG_AQ07_ZERO_SPACE)) {
        return false;
      }
    }
    state_frame[pos] = byte;
    if (pos == 0) {
      // frame header
      if (byte != 0x11)
        return false;
    } else if (pos == 1) {
      // frame header
      if (byte != 0xDA)
        return false;
    } else if (pos == 2) {
      // frame header
      if (byte != 0x27)
        return false;
    } else if (pos == 3) {  // NOLINT(bugprone-branch-clone)
      // frame header
      if (byte != 0x00)
        return false;
    } else if (pos == 4) {
      // frame type
      if (byte != 0x00)
        return false;
    }
  }
  return this->parse_state_frame_(state_frame);
}

}  // namespace samsung_aq07
}  // namespace esphome
