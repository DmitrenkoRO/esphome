#pragma once

#include "esphome/components/climate_ir/climate_ir.h"

namespace esphome {
namespace samsung_aq07 {

// Values for Samsung AQ07A1AE IR Controller
// Temperature
const uint8_t SAMSUNG_AQ07_TEMP_MIN = 18;  // Celsius
const uint8_t SAMSUNG_AQ07_TEMP_MAX = 30;  // Celsius

typedef struct {
    uint8_t zero : 4;    // Первые 4 бита
    uint8_t mode0;      // Полный байт
    uint8_t data0;      // Полный байт
    uint8_t data1;      // Полный байт
    uint8_t data2;      // Полный байт
    uint8_t csum;      // Полный байт
    uint8_t data3;      // Полный байт
    uint8_t last : 4;     // Последние 4 бита
} remote_state_t;

// Modes
const uint8_t SAMSUNG_AQ07_MODE_AUTO = 0x00;
const uint8_t SAMSUNG_AQ07_MODE_COOL = 0x01;
const uint8_t SAMSUNG_AQ07_MODE_DRY = 0x02;
const uint8_t SAMSUNG_AQ07_MODE_FAN = 0x03;
const uint8_t SAMSUNG_AQ07_MODE_HEAT = 0x04;
const uint8_t SAMSUNG_AQ07_MODE_TREE = 0x40; //Hidden button with tree icon on the lcd, idk what it is.
const uint8_t SAMSUNG_AQ07_MODE_ON = 0x01;
const uint8_t SAMSUNG_AQ07_MODE_OFF = 0x00;
const uint8_t SAMSUNG_AQ07_ZB_OFF = 0xC;
const uint8_t SAMSUNG_AQ07_ZB_ON = 0xF;

// Fan Speed
const uint8_t SAMSUNG_AQ07_FAN_AUTO = 0x00;
const uint8_t SAMSUNG_AQ07_FAN_SILENT = 0xB0;
const uint8_t SAMSUNG_AQ07_FAN_1 = 0x40;
const uint8_t SAMSUNG_AQ07_FAN_2 = 0x80;
const uint8_t SAMSUNG_AQ07_FAN_3 = 0xA0;
const uint8_t SAMSUNG_AQ07_FAN_4 = 0x60;
const uint8_t SAMSUNG_AQ07_FAN_5 = 0x70;

const uint8_t SAMSUNG_AQ07_DATA3_DEF = 0x1F;
const uint8_t SAMSUNG_AQ07_V_SWING = 0x05;
const uint8_t SAMSUNG_AQ07_H_SWING = 0x02;
const uint8_t SAMSUNG_AQ07_VH_SWING = 0x03;

const uint8_t SAMSUNG_AQ07_TURBO   = 0x60;

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
                              {climate::CLIMATE_FAN_AUTO, climate::CLIMATE_FAN_QUIET ,climate::CLIMATE_FAN_LOW, climate::CLIMATE_FAN_MEDIUM,
                               climate::CLIMATE_FAN_HIGH},
                              {climate::CLIMATE_SWING_OFF, climate::CLIMATE_SWING_VERTICAL,
                               climate::CLIMATE_SWING_HORIZONTAL, climate::CLIMATE_SWING_BOTH}) {}

 protected:
  // Transmit via IR the state of this climate controller.
  void transmit_state() override;
  uint8_t zero_byte_();  
  uint8_t operation_mode_();
  uint8_t fan_speed_();
  uint8_t temperature_();
  uint8_t swing_turbo_mode_();
  // Handle received IR Buffer
  bool on_receive(remote_base::RemoteReceiveData data) override;
  bool parse_state_frame_(const uint8_t frame[]);
};

}  // namespace samsung_aq07
}  // namespace esphome
