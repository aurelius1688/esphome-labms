#pragma once

#include "esphome/core/component.h"
#include "esphome/components/ble_client/ble_client.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/number/number.h"
#ifdef USE_ESP32
#include <esp_gattc_api.h>
#include <vector>

namespace esphome {
namespace lian_bms_ble {

namespace espbt = esphome::esp32_ble_tracker;

static const uint16_t REG_MOS_TEMP = 0;
static const uint16_t REG_BATTERY_TEMP_0 = 1;
static const uint16_t REG_BATTERY_TEMP_1 = 2;
static const uint16_t REG_TOTAL_VOLTAGE = 4;
static const uint16_t REG_CURRENT = 6;
static const uint16_t REG_REMAINING_CAPACITY = 8;
static const uint16_t REG_PERC_AND_HEALTH = 20;
static const uint16_t REG_BATTERY_COUNT_TYPE = 24;
static const uint16_t REG_FOR_COUNT = 26;
static const uint16_t REG_FULL_CAPACITY = 27;
static const uint16_t REG_DESIGN_CAPACITY = 28;
static const uint16_t REG_MIN_BATTERY_ADDRESS = 32;

class LianBmsBle : public PollingComponent, public ble_client::BLEClientNode {
 public:
  void dump_config() override;
  void update() override;
  void gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
                           esp_ble_gattc_cb_param_t *param) override;
  void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) override;

  void set_total_voltage_sensor(sensor::Sensor *s) { this->total_voltage_sensor_ = s; }
  void set_current_sensor(sensor::Sensor *s) { this->current_sensor_ = s; }
  void set_power_sensor(sensor::Sensor *s) { this->power_sensor_ = s; }
  void set_capacity_remaining_sensor(sensor::Sensor *s) { this->capacity_remaining_sensor_ = s; }
  void set_state_of_charge_sensor(sensor::Sensor *s) { this->state_of_charge_sensor_ = s; }
  void set_mos_temperature_sensor(sensor::Sensor *s) { this->mos_temperature_sensor_ = s; }
  void set_temperature_sensor_1_sensor(sensor::Sensor *s) { this->temperature_sensor_1_ = s; }
  void set_temperature_sensor_2_sensor(sensor::Sensor *s) { this->temperature_sensor_2_ = s; }
  void set_min_cell_voltage_sensor(sensor::Sensor *s) { this->min_cell_voltage_sensor_ = s; }
  void set_max_cell_voltage_sensor(sensor::Sensor *s) { this->max_cell_voltage_sensor_ = s; }
  void set_delta_cell_voltage_sensor(sensor::Sensor *s) { this->delta_cell_voltage_sensor_ = s; }
  void set_average_cell_voltage_sensor(sensor::Sensor *s) { this->average_cell_voltage_sensor_ = s; }
  void set_cell_voltage_sensor(uint8_t cell, sensor::Sensor *s) { this->cell_voltage_sensors_[cell] = s; }
  void set_charging_cycles_sensor(sensor::Sensor *s) { this->charging_cycles_sensor_ = s; }
  void set_full_capacity_sensor(sensor::Sensor *s) { this->full_capacity_sensor_ = s; }
  void set_design_capacity_sensor(sensor::Sensor *s) { this->design_capacity_sensor_ = s; }
  void set_total_cycle_capacity_sensor(sensor::Sensor *s) { this->total_cycle_capacity_sensor_ = s; }
  void set_online_status_binary_sensor(binary_sensor::BinarySensor *s) { this->online_status_binary_sensor_ = s; }
  void set_response_text_sensor(text_sensor::TextSensor *s) { this->response_text_sensor_ = s; }
  void set_debug_text_sensor(text_sensor::TextSensor *s) { this->debug_text_sensor_ = s; }

  void set_charge_switch(switch_::Switch *s) { this->charge_switch_ = s; }
  void set_discharge_switch(switch_::Switch *s) { this->discharge_switch_ = s; }
  void set_heating_switch(switch_::Switch *s) { this->heating_switch_ = s; }
  void set_connected_time_sensor(sensor::Sensor *s) { this->connected_time_sensor_ = s; }
  void set_connected_time_text_sensor(text_sensor::TextSensor *s) { this->connected_time_text_sensor_ = s; }
  void set_battery_capacity_number(number::Number *n) { this->battery_capacity_number_ = n; }
  void set_cell_count_number(number::Number *n) { this->cell_count_number_ = n; }
  void set_mac_address_text_sensor(text_sensor::TextSensor *s) { this->mac_address_text_sensor_ = s; }
  void set_mac_input_text_sensor(text_sensor::TextSensor *s) { this->mac_input_text_sensor_ = s; }

  void send_raw_command(const std::vector<uint8_t> &data);
  void write_register(uint16_t reg, uint16_t value);
  uint32_t get_connected_time() const;
  void apply_mac_address(const std::string &mac);
  float get_battery_capacity() const { return this->battery_capacity_; }
  float get_cell_count() const { return this->cell_count_; }
  void set_battery_capacity(float val) { this->battery_capacity_ = val; }
  void set_cell_count(float val) { this->cell_count_ = val; }

 protected:
  void send_read_command_(uint16_t reg, uint16_t count);
  void process_response_();
  void publish_device_unavailable_();
  float get_temperature_(uint16_t raw);
  uint16_t crc16_(const uint8_t *data, size_t len);

  uint16_t char_handle_{0};
  uint16_t write_handle_{0};
  bool char_found_{false};
  bool auth_done_{false};
  bool cccd_written_{false};
  uint32_t ready_time_{0};
  bool read_cells_next_{false};

  std::vector<uint8_t> frame_buffer_;
  uint8_t no_response_count_{0};

  float battery_capacity_{40.0f};
  float cell_count_{4.0f};

  sensor::Sensor *total_voltage_sensor_{nullptr};
  sensor::Sensor *current_sensor_{nullptr};
  sensor::Sensor *power_sensor_{nullptr};
  sensor::Sensor *capacity_remaining_sensor_{nullptr};
  sensor::Sensor *state_of_charge_sensor_{nullptr};
  sensor::Sensor *mos_temperature_sensor_{nullptr};
  sensor::Sensor *temperature_sensor_1_{nullptr};
  sensor::Sensor *temperature_sensor_2_{nullptr};
  sensor::Sensor *min_cell_voltage_sensor_{nullptr};
  sensor::Sensor *max_cell_voltage_sensor_{nullptr};
  sensor::Sensor *delta_cell_voltage_sensor_{nullptr};
  sensor::Sensor *average_cell_voltage_sensor_{nullptr};
  sensor::Sensor *charging_cycles_sensor_{nullptr};
  sensor::Sensor *full_capacity_sensor_{nullptr};
  sensor::Sensor *design_capacity_sensor_{nullptr};
  sensor::Sensor *total_cycle_capacity_sensor_{nullptr};
  sensor::Sensor *cell_voltage_sensors_[32]{};

  binary_sensor::BinarySensor *online_status_binary_sensor_{nullptr};
  text_sensor::TextSensor *response_text_sensor_{nullptr};
  text_sensor::TextSensor *debug_text_sensor_{nullptr};
  text_sensor::TextSensor *connected_time_text_sensor_{nullptr};
  text_sensor::TextSensor *mac_address_text_sensor_{nullptr};
  text_sensor::TextSensor *mac_input_text_sensor_{nullptr};

  switch_::Switch *charge_switch_{nullptr};
  switch_::Switch *discharge_switch_{nullptr};
  switch_::Switch *heating_switch_{nullptr};
  sensor::Sensor *connected_time_sensor_{nullptr};
  number::Number *battery_capacity_number_{nullptr};
  number::Number *cell_count_number_{nullptr};
};

class LianBmsSwitch : public switch_::Switch, public Component {
 public:
  void set_parent(LianBmsBle *parent) { this->parent_ = parent; }
  void set_reg_on(uint16_t reg) { this->reg_on_ = reg; }
  void set_reg_off(uint16_t reg) { this->reg_off_ = reg; }
  void set_value_on(uint16_t val) { this->value_on_ = val; }
  void set_value_off(uint16_t val) { this->value_off_ = val; }

 protected:
  void write_state(bool state) override;

  LianBmsBle *parent_{nullptr};
  uint16_t reg_on_{0};
  uint16_t reg_off_{0};
  uint16_t value_on_{0};
  uint16_t value_off_{0};
};

class LianBmsBatteryCapacityNumber : public number::Number, public Component {
 public:
  void set_parent(LianBmsBle *parent) { this->parent_ = parent; }
 protected:
  void control(float value) override;
  LianBmsBle *parent_{nullptr};
};

class LianBmsCellCountNumber : public number::Number, public Component {
 public:
  void set_parent(LianBmsBle *parent) { this->parent_ = parent; }
 protected:
  void control(float value) override;
  LianBmsBle *parent_{nullptr};
};

}  // namespace lian_bms_ble
}  // namespace esphome
#endif
