#include "samsung_aq07.h"
#include "esphome/components/remote_base/remote_base.h"

namespace esphome {
namespace samsung_aq07 {

static const char *const TAG = "samsung_aq07.climate";

void SamsungAq07Climate::transmit_state() {
  uint8_t remote_state[35] = {0x11, 0xDA, 0x27, 0x00, 0xC5, 0x00, 0x00, 0xD7, 0x11, 0xDA, 0x27, 0x00,
                              0x42, 0x49, 0x05, 0xA2, 0x11, 0xDA, 0x27, 0x00, 0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x06, 0x60, 0x00, 0x00, 0xC0, 0x00, 0x00, 0x00};

  //remote_state[0]  = this->zero_byte_();
  //remote_state[21] = this->operation_mode_();
  //remote_state[22] = this->temperature_();
  uint8_t fan_speed_and_temp = (fan_speed_()&0xE0)| (this->temperature_()&0x1F);

  remote_state_t state;

  state.zero = this->zero_byte_();          // 1st 4 bits
  state.mode0 = this->operation_mode_();    // 1st full byte
  state.data0 = fan_speed_and_temp;         // 2nd full byte
  state.data1 = 0x32;                         // 3rd full byte  //Always 0x32 if RC is not sending SET TIME/TIMER CMDS
  state.data2 = this->swing_turbo_mode_();  // 4th full byte
  state.csum = 0xEE;                        // 5th full byte
  state.data3 = 0x20;                       // Last full byte //Seems whose last bytes always static
  state.last = 0x1;                         // Last 4 bits
  ESP_LOGD("samsung_aq07", "state.mode0():%02X",state.mode0);  
  remote_state[0] = (state.zero<<4 )  | ((state.mode0>>4)&0x0F);
  remote_state[1] = (state.mode0<<4 ) | ((state.data0>>4)&0x0F);
  remote_state[2] = (state.data0<<4 ) | ((state.data1>>4)&0x0F);
  remote_state[3] = (state.data1<<4 ) | ((state.data2>>4)&0x0F);
  remote_state[4] = (state.data2<<4 ) | ((state.csum>>4)&0x0F);
  remote_state[5] = (state.csum<<4  ) | ((state.data3>>4)&0x0F);
  remote_state[6] = (state.data3<<4 ) | ((state.last)&0x0F);
  
  //0x80477F84C4880F TEST on cool 18c
  //0xF0112321FEE201 UNREVERSED BITS

  //remote_state[0] = 0xF0;
  // remote_state[1] = 0x11;
  // remote_state[2] = 0x23;
  // remote_state[3] = 0x21;
  // remote_state[4] = 0xFE;
  // remote_state[5] = 0xE2;
  // remote_state[6] = 0x01;
  //TEST

  // Calculate checksum
  int sum=0x0;
    for (int i = 6; i > -1; i--) {
        for (int j=0;j<8;j++){
            if((i==4)&(j<4)||(i==5)&(j>4)){
                sum += 0;
            }
            else
            {
                sum += (*(remote_state+i) & (1 << j)) ? 1 : 0;
            }
        }
            //printf("%01X",*(remote_state+i));
            //remote_bit[i][j] = bit & 0x01;          
    }
    sum= ~sum;
    //printf("\nSum:%d\n",sum);
    //printf("SumX:%08b\n",sum&0xFF);
    remote_state[4] &= 0xF0;    remote_state[5] &= 0x0F;
    remote_state[4] |= sum>>4&0x0F;//remote_state[4]<<4&0xF0 | sum>>4&0x0F;
    remote_state[5] |= ((sum<<4)&0xf0);//remote_state[5]&0x0F | sum<<4&0xF0;

  auto transmit = this->transmitter_->transmit();
  auto *data = transmit.get_data();
  data->set_carrier_frequency(SAMSUNG_AQ07_IR_FREQUENCY);

  data->mark(SAMSUNG_AQ07_HEADER_MARK);
  data->space(SAMSUNG_AQ07_HEADER_SPACE);

  ESP_LOGD("samsung_aq07", "remote_state=%02X%02X%02X%02X%02X%02X%02X",remote_state[0],remote_state[1],remote_state[2],remote_state[3],remote_state[4],remote_state[5],remote_state[6]);

  for (int i = 6; i > -1; i--) {
    for (int j=0;j<8;j++){
      data->mark(SAMSUNG_AQ07_BIT_MARK);      
      bool bit = (remote_state[i] & (1 << (j))) ? 1 : 0;
      data->space(bit ? SAMSUNG_AQ07_ONE_SPACE : SAMSUNG_AQ07_ZERO_SPACE); 
    }
  }
  data->mark(SAMSUNG_AQ07_BIT_MARK);
  data->space(0);

  transmit.perform();
}

uint8_t SamsungAq07Climate::zero_byte_() {
  uint8_t zero_byte = SAMSUNG_AQ07_ZB_ON; //0b1111 ON 0b1100 OFF 0b0000 TIME/TIMERS SET
  switch (this->mode) {
    case climate::CLIMATE_MODE_COOL:
      break;
    case climate::CLIMATE_MODE_DRY:
      break;
    case climate::CLIMATE_MODE_FAN_ONLY:
      break;    
    case climate::CLIMATE_MODE_HEAT:
      break;  
    case climate::CLIMATE_MODE_OFF:
      zero_byte = SAMSUNG_AQ07_ZB_OFF;
      break;  
    default:
      zero_byte = SAMSUNG_AQ07_ZB_OFF;
      break;
  }
  return zero_byte;
}

uint8_t SamsungAq07Climate::operation_mode_() {
  uint8_t operating_mode = SAMSUNG_AQ07_MODE_AUTO;
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
      operating_mode |= SAMSUNG_AQ07_MODE_COOL;
      break;
    case climate::CLIMATE_MODE_FAN_ONLY:
      operating_mode |= SAMSUNG_AQ07_MODE_FAN;
      break;
    default:
      operating_mode = SAMSUNG_AQ07_MODE_COOL;
      break;
  }
  ESP_LOGD("samsung_aq07", "operation_mode_():%02X",operating_mode);

  return operating_mode;
}

uint8_t SamsungAq07Climate::fan_speed_() {
  uint8_t fan_speed;
  switch (this->fan_mode.value()) {
    case climate::CLIMATE_FAN_QUIET:
      fan_speed = SAMSUNG_AQ07_FAN_1;
      break;
    case climate::CLIMATE_FAN_LOW:
      fan_speed = SAMSUNG_AQ07_FAN_2;
      break;
    case climate::CLIMATE_FAN_MEDIUM:
      fan_speed = SAMSUNG_AQ07_FAN_3;
      break;
    case climate::CLIMATE_FAN_HIGH:
      fan_speed = SAMSUNG_AQ07_FAN_AUTO;
      break;
    case climate::CLIMATE_FAN_AUTO:
    default:
      fan_speed = SAMSUNG_AQ07_FAN_AUTO;
  }

  // If swing is enabled switch first 4 bits to 1111
  // switch (this->swing_mode) {
  //   case climate::CLIMATE_SWING_VERTICAL:
  //     fan_speed |= 0x0F00;
  //     break;
  //   case climate::CLIMATE_SWING_HORIZONTAL:
  //     fan_speed |= 0x000F;
  //     break;
  //   case climate::CLIMATE_SWING_BOTH:
  //     fan_speed |= 0x0F0F;
  //     break;
  //   default:
  //     break;
  // }
  ESP_LOGD("samsung_aq07", "fan_speed_():%02X",fan_speed);
  return fan_speed;
}

uint8_t SamsungAq07Climate::swing_turbo_mode_() {
  uint8_t swing_turbo_byte;
  switch (this->fan_mode.value()) {
    case climate::CLIMATE_FAN_HIGH:
      swing_turbo_byte = SAMSUNG_AQ07_DATA3_DEF + SAMSUNG_AQ07_TURBO;
      break;
    default:
      swing_turbo_byte = SAMSUNG_AQ07_DATA3_DEF;
  }

  //If swing is enabled switch first 4 bits to 1111
  switch (this->swing_mode) {
    case climate::CLIMATE_SWING_VERTICAL:
      swing_turbo_byte -= SAMSUNG_AQ07_V_SWING;
      break;
    case climate::CLIMATE_SWING_HORIZONTAL:
      swing_turbo_byte -= SAMSUNG_AQ07_H_SWING;
      break;
    case climate::CLIMATE_SWING_BOTH:
      swing_turbo_byte -= SAMSUNG_AQ07_VH_SWING;
      break;
    default:
      break;
  }
  ESP_LOGD("samsung_aq07", "swing_turbo_mode_():%02X",swing_turbo_byte);
  return swing_turbo_byte;
}

uint8_t SamsungAq07Climate::temperature_() {
  // Force special temperatures depending on the mode
  switch (this->mode) {
    case climate::CLIMATE_MODE_FAN_ONLY:
      return 0x5E; //small fan speed
    case climate::CLIMATE_MODE_HEAT_COOL:
    case climate::CLIMATE_MODE_DRY:
      //return 0xc0;
    default:
      uint8_t temperature = (uint8_t) roundf(clamp<float>(this->target_temperature, SAMSUNG_AQ07_TEMP_MIN, SAMSUNG_AQ07_TEMP_MAX));
      ESP_LOGD("samsung_aq07", "temperature_():%u",temperature);
      ESP_LOGD("samsung_aq07", "temperature_() << 1:%u",temperature << 1); 
      return temperature;
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
