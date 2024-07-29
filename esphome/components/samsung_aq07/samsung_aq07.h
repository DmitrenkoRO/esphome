#pragma once

#include "esphome/components/climate_ir/climate_ir.h"

namespace esphome {
namespace samsung_aq07 {

// Values for Samsung AQ07A1AE IR Controller
// Temperature
const uint8_t SAMSUNG_AQ07_TEMP_MIN = 10;  // Celsius
const uint8_t SAMSUNG_AQ07_TEMP_MAX = 30;  // Celsius

// Modes
const uint8_t SAMSUNG_AQ07_MODE_AUTO = 0x00;
const uint8_t SAMSUNG_AQ07_MODE_COOL = 0x30;
const uint8_t SAMSUNG_AQ07_MODE_HEAT = 0x40;
const uint8_t SAMSUNG_AQ07_MODE_DRY = 0x20;
const uint8_t SAMSUNG_AQ07_MODE_FAN = 0x60;
const uint8_t SAMSUNG_AQ07_ZB_OFF = 0xС0;
const uint8_t SAMSUNG_AQ07_ZB_ON = 0xF0;

// Fan Speed
const uint8_t SAMSUNG_AQ07_FAN_AUTO = 0xA0;
const uint8_t SAMSUNG_AQ07_FAN_SILENT = 0xB0;
const uint8_t SAMSUNG_AQ07_FAN_1 = 0x30;
const uint8_t SAMSUNG_AQ07_FAN_2 = 0x40;
const uint8_t SAMSUNG_AQ07_FAN_3 = 0x50;
const uint8_t SAMSUNG_AQ07_FAN_4 = 0x60;
const uint8_t SAMSUNG_AQ07_FAN_5 = 0x70;

// IR Transmission
const uint32_t SAMSUNG_AQ07_IR_FREQUENCY = 38000;
const uint32_t SAMSUNG_AQ07_HEADER_MARK = 3000;
const uint32_t SAMSUNG_AQ07_HEADER_SPACE = 9000;
const uint32_t SAMSUNG_AQ07_BIT_MARK = 450;
const uint32_t SAMSUNG_AQ07_ONE_SPACE = 1500;
const uint32_t SAMSUNG_AQ07_ZERO_SPACE = 500;
const uint32_t SAMSUNG_AQ07_MESSAGE_SPACE = 32300;

// State Frame size
const uint8_t SAMSUNG_AQ07_STATE_FRAME_SIZE = 19;

class SamsungAq07Climate : public climate_ir::ClimateIR {
 public:
  SamsungAq07Climate()
      : climate_ir::ClimateIR(SAMSUNG_AQ07_TEMP_MIN, SAMSUNG_AQ07_TEMP_MAX, 1.0f, true, true,
                              {climate::CLIMATE_FAN_AUTO, climate::CLIMATE_FAN_LOW, climate::CLIMATE_FAN_MEDIUM,
                               climate::CLIMATE_FAN_HIGH},
                              {climate::CLIMATE_SWING_OFF, climate::CLIMATE_SWING_VERTICAL,
                               climate::CLIMATE_SWING_HORIZONTAL, climate::CLIMATE_SWING_BOTH}) {}

 protected:
  // Transmit via IR the state of this climate controller.
  void transmit_state() override;
  uint8_t zero_byte_();  
  uint8_t operation_mode_();
  uint16_t fan_speed_();
  uint8_t temperature_();
  // Handle received IR Buffer
  bool on_receive(remote_base::RemoteReceiveData data) override;
  bool parse_state_frame_(const uint8_t frame[]);
};

}  // namespace samsung_aq07
}  // namespace esphome
