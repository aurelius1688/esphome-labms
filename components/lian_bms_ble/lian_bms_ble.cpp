#include "lian_bms_ble.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#ifdef USE_ESP32
#include <esp_gattc_api.h>
#include <esp_gap_ble_api.h>
#include <cstring>

namespace esphome {
namespace lian_bms_ble {

static const char *const TAG = "lian_bms_ble";

// CRC16-XMODEM table (polynomial 0x1021, init 0x0000) - from LiAn mini-program
static const uint16_t CRC16_TABLE[256] = {
    0, 4129, 8258, 12387, 16516, 20645, 24774, 28903,
    33032, 37161, 41290, 45419, 49548, 53677, 57806, 61935,
    4657, 528, 12915, 8786, 21173, 17044, 29431, 25302,
    37689, 33560, 45947, 41818, 54205, 50076, 62463, 58334,
    9314, 13379, 1056, 5121, 25830, 29895, 17572, 21637,
    42346, 46411, 34088, 38153, 58862, 62927, 50604, 54669,
    13907, 9842, 5649, 1584, 30423, 26358, 22165, 18100,
    46939, 42874, 38681, 34616, 63455, 59390, 55197, 51132,
    18628, 22757, 26758, 30887, 2112, 6241, 10242, 14371,
    51660, 55789, 59790, 63919, 35144, 39273, 43274, 47403,
    23285, 19156, 31415, 27286, 6769, 2640, 14899, 10770,
    56317, 52188, 64447, 60318, 39801, 35672, 47931, 43802,
    27814, 31879, 19684, 23749, 11298, 15363, 3168, 7233,
    60846, 64911, 52716, 56781, 44330, 48395, 36200, 40265,
    32407, 28342, 24277, 20212, 15891, 11826, 7761, 3696,
    65439, 61374, 57309, 53244, 48923, 44858, 40793, 36728,
    37256, 33193, 45514, 41451, 53516, 49453, 61774, 57711,
    4224, 161, 12482, 8419, 20484, 16421, 28742, 24679,
    33721, 37784, 41979, 46042, 49981, 54044, 58239, 62302,
    689, 4752, 8947, 13010, 16949, 21012, 25207, 29270,
    46570, 42443, 38312, 34185, 62830, 58703, 54572, 50445,
    13538, 9411, 5280, 1153, 29798, 25671, 21540, 17413,
    42971, 47098, 34713, 38840, 59231, 63358, 50973, 55100,
    9939, 14066, 1681, 5808, 26199, 30326, 17941, 22068,
    55628, 51565, 63758, 59695, 39368, 35305, 47498, 43435,
    22596, 18533, 30726, 26663, 6336, 2273, 14466, 10403,
    52093, 56156, 60223, 64286, 35833, 39896, 43963, 48026,
    19061, 23124, 27191, 31254, 2801, 6864, 10931, 14994,
    64814, 60687, 56684, 52557, 48554, 44427, 40424, 36297,
    31782, 27655, 23652, 19525, 15522, 11395, 7392, 3265,
    61215, 65342, 53085, 57212, 44955, 49082, 36825, 40952,
    28183, 32310, 20053, 24180, 11923, 16050, 3793, 7920,
};

uint16_t LianBmsBle::crc16_(const uint8_t *d, size_t len) {
  uint16_t r = 0;
  for (size_t i = 0; i < len; i++)
    r = (r << 8) ^ CRC16_TABLE[255 & ((r >> 8) ^ d[i])];
  return r;
}

float LianBmsBle::get_temperature_(uint16_t raw) { return ((float) raw - 2731.0f) / 10.0f; }

void LianBmsBle::send_raw_command(const std::vector<uint8_t> &data) {
  if (!this->char_found_ || data.empty()) return;
  this->frame_buffer_.clear();
  ESP_LOGI(TAG, "RAW WRITE 0x%04X (%d bytes): %s", this->write_handle_, data.size(),
           format_hex_pretty(data.data(), data.size()).c_str());
  auto st = esp_ble_gattc_write_char(this->parent_->get_gattc_if(), this->parent_->get_conn_id(),
                                      this->write_handle_, data.size(), const_cast<uint8_t *>(data.data()),
                                      ESP_GATT_WRITE_TYPE_NO_RSP, ESP_GATT_AUTH_REQ_NONE);
  if (st) ESP_LOGW(TAG, "Write err: %d", st);
}

void LianBmsBle::write_register(uint16_t reg, uint16_t value) {
  if (!this->char_found_) return;
  // 功能码 0x10: 写多个寄存器
  // [0x16] [0x10] [regH] [regL] [countH] [countL] [byteCount] [dataH] [dataL] [crcLow] [crcHigh]
  uint8_t cmd[9] = {0x16, 0x10, 0, 0, 0, 0x01, 0x02, 0, 0};
  cmd[2] = (reg >> 8) & 0xFF;
  cmd[3] = reg & 0xFF;
  cmd[7] = (value >> 8) & 0xFF;
  cmd[8] = value & 0xFF;
  // CRC16-XMODEM 覆盖前9字节
  // 但实际需要计算整个帧的CRC，帧格式: addr+func+reg+count+bytecount+data = 9字节
  // 然后追加2字节CRC
  uint16_t crc = this->crc16_(cmd, 9);
  uint8_t frame[11];
  memcpy(frame, cmd, 9);
  frame[9] = crc & 0xFF;
  frame[10] = (crc >> 8) & 0xFF;
  ESP_LOGI(TAG, "WRITE_REG 0x%04X = 0x%04X: %s", reg, value,
           format_hex_pretty(frame, 11).c_str());
  auto st = esp_ble_gattc_write_char(this->parent_->get_gattc_if(), this->parent_->get_conn_id(),
                                      this->write_handle_, 11, frame, ESP_GATT_WRITE_TYPE_NO_RSP,
                                      ESP_GATT_AUTH_REQ_NONE);
  if (st) ESP_LOGW(TAG, "Write reg err: %d", st);
}

void LianBmsBle::gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
  switch (event) {
    case ESP_GAP_BLE_SEC_REQ_EVT:
      ESP_LOGI(TAG, "GAP SEC_REQ - rejecting (no bonding)");
      esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, false);
      break;
    case ESP_GAP_BLE_AUTH_CMPL_EVT:
      ESP_LOGI(TAG, "GAP AUTH_CMPL success=%d mode=%d", param->ble_security.auth_cmpl.success,
               param->ble_security.auth_cmpl.auth_mode);
      break;
    default:
      break;
  }
}

void LianBmsBle::gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
                                     esp_ble_gattc_cb_param_t *param) {
  switch (event) {
    case ESP_GATTC_CONNECT_EVT: {
      if (std::memcmp(param->connect.remote_bda, this->parent_->get_remote_bda(), 6) != 0)
        return;
      ESP_LOGI(TAG, "=== CONNECTED ===");
      break;
    }
    case ESP_GATTC_DISCONNECT_EVT:
      this->char_found_ = false;
      this->write_handle_ = 0;
      this->char_handle_ = 0;
      this->cccd_written_ = false;
      this->ready_time_ = 0;
      this->frame_buffer_.clear();
      this->node_state = espbt::ClientState::IDLE;
      this->publish_device_unavailable_();
      ESP_LOGW(TAG, "BLE disconnected reason=0x%02X", param->disconnect.reason);
      break;
    case ESP_GATTC_SEARCH_CMPL_EVT: {
      for (auto *svc : this->parent_->get_services()) {
        if (!svc) continue;
        svc->parse_characteristics();
        for (auto *ch : svc->characteristics) {
          if (!ch) continue;
          std::string uuid = ch->uuid.to_string();
          if (uuid.find("00002760") == std::string::npos) continue;
          ESP_LOGI(TAG, "BMS CHR: handle=0x%04X props=0x%02X uuid=%s", ch->handle, ch->properties, uuid.c_str());
          if (ch->properties & 0x04) {
            this->write_handle_ = ch->handle;
            ESP_LOGI(TAG, "BMS WRITE handle=0x%04X", ch->handle);
          }
          if ((ch->properties & 0x10) || (ch->properties & 0x20)) {
            this->char_handle_ = ch->handle;
            ESP_LOGI(TAG, "BMS NOTIFY handle=0x%04X", ch->handle);
          }
        }
      }
      if (!this->write_handle_ || !this->char_handle_) {
        ESP_LOGE(TAG, "BMS service not found!");
        break;
      }
      ESP_LOGI(TAG, "Registering for notify...");
      esp_ble_gattc_register_for_notify(this->parent_->get_gattc_if(),
                                         this->parent_->get_remote_bda(), this->char_handle_);
      break;
    }
    case ESP_GATTC_REG_FOR_NOTIFY_EVT:
      ESP_LOGI(TAG, "REG_NOTIFY handle=0x%04X status=%d", param->reg_for_notify.handle, param->reg_for_notify.status);
      if (param->reg_for_notify.status == 0 && param->reg_for_notify.handle == this->char_handle_) {
        uint16_t val = 0x0001;
        esp_ble_gattc_write_char_descr(this->parent_->get_gattc_if(),
                                        this->parent_->get_conn_id(),
                                        this->char_handle_ + 1, 2, (uint8_t *) &val,
                                        ESP_GATT_WRITE_TYPE_RSP, ESP_GATT_AUTH_REQ_NONE);
        ESP_LOGI(TAG, "Writing CCCD to 0x%04X...", this->char_handle_ + 1);
      }
      break;
    case ESP_GATTC_WRITE_DESCR_EVT:
      ESP_LOGI(TAG, "WRITE_DESCR handle=0x%04X status=%d", param->write.handle, param->write.status);
      if (param->write.status == 0 && param->write.handle == this->char_handle_ + 1) {
        this->cccd_written_ = true;
        this->ready_time_ = millis();
        this->char_found_ = true;
        this->node_state = espbt::ClientState::ESTABLISHED;
        this->no_response_count_ = 0;
        if (this->online_status_binary_sensor_) this->online_status_binary_sensor_->publish_state(true);
        ESP_LOGI(TAG, "=== BMS READY! write=0x%04X notify=0x%04X ===", this->write_handle_, this->char_handle_);
      }
      break;
    case ESP_GATTC_WRITE_CHAR_EVT:
      if (param->write.handle == this->write_handle_)
        ESP_LOGI(TAG, "WRITE_CHAR status=%d", param->write.status);
      break;
    case ESP_GATTC_READ_CHAR_EVT:
      ESP_LOGI(TAG, "READ_CHAR handle=0x%04X status=%d len=%d", param->read.handle, param->read.status, param->read.value_len);
      if (param->read.value_len > 0) {
        ESP_LOGI(TAG, "  DATA: %s", format_hex_pretty(param->read.value, param->read.value_len).c_str());
      }
      break;
    case ESP_GATTC_CFG_MTU_EVT:
      ESP_LOGI(TAG, "MTU=%d status=%d", param->cfg_mtu.mtu, param->cfg_mtu.status);
      break;
    case ESP_GATTC_NOTIFY_EVT: {
      ESP_LOGI(TAG, "!!! NOTIFY !!! handle=0x%04X len=%d is_notify=%d", param->notify.handle,
               param->notify.value_len, param->notify.is_notify);
      ESP_LOGI(TAG, "  DATA: %s", format_hex_pretty(param->notify.value, param->notify.value_len).c_str());
      if (this->response_text_sensor_) this->response_text_sensor_->publish_state(
          format_hex_pretty(param->notify.value, param->notify.value_len));
      this->frame_buffer_.insert(this->frame_buffer_.end(), param->notify.value,
                                   param->notify.value + param->notify.value_len);
      if (this->frame_buffer_.size() > 256) { this->frame_buffer_.clear(); break; }
      this->process_response_();
      break;
    }
    default:
      ESP_LOGD(TAG, "GATTC event=%d", (int) event);
      break;
  }
}

void LianBmsBle::update() {
  if (this->no_response_count_ < 5) this->no_response_count_++;
  if (this->no_response_count_ == 5) { this->publish_device_unavailable_(); this->no_response_count_++; }
  if (!this->char_found_) return;
  if (millis() - this->ready_time_ < 2000) return;
  ESP_LOGD(TAG, "Polling BMS...");
  if (this->read_cells_next_) {
    this->send_read_command_(0x0020, 4);  // 电芯电压 reg=32-35
  } else {
    this->send_read_command_(0x0000, 31); // 基本数据 reg=0-30
  }
  this->read_cells_next_ = !this->read_cells_next_;
}

void LianBmsBle::send_read_command_(uint16_t reg, uint16_t count) {
  uint8_t cmd[6] = {0x16, 0x03, 0, 0, 0, 0};
  cmd[2] = (reg >> 8) & 0xFF;
  cmd[3] = reg & 0xFF;
  cmd[4] = (count >> 8) & 0xFF;
  cmd[5] = count & 0xFF;
  uint16_t crc = this->crc16_(cmd, 6);
  uint8_t frame[8];
  memcpy(frame, cmd, 6);
  frame[6] = crc & 0xFF;
  frame[7] = (crc >> 8) & 0xFF;
  this->frame_buffer_.clear();
  ESP_LOGI(TAG, "WRITE 0x%04X: %s", this->write_handle_,
           format_hex_pretty(frame, 8).c_str());
  auto st = esp_ble_gattc_write_char(this->parent_->get_gattc_if(), this->parent_->get_conn_id(),
                                      this->write_handle_, 8, frame, ESP_GATT_WRITE_TYPE_NO_RSP,
                                      ESP_GATT_AUTH_REQ_NONE);
  if (st) ESP_LOGW(TAG, "Write err: %d", st);
}

void LianBmsBle::process_response_() {
  if (this->frame_buffer_.size() < 11) return;
  size_t fl = this->frame_buffer_.size();
  uint16_t rxc = (uint16_t)(this->frame_buffer_[fl - 1] << 8) | this->frame_buffer_[fl - 2];
  uint16_t calc = this->crc16_(this->frame_buffer_.data(), fl - 2);
  if (rxc != calc) { ESP_LOGW(TAG, "CRC fail 0x%04X!=0x%04X len=%d", rxc, calc, fl); this->frame_buffer_.clear(); return; }
  this->no_response_count_ = 0;
  if (this->frame_buffer_[0] != 0x16 || this->frame_buffer_[1] != 0x03) { this->frame_buffer_.clear(); return; }
  uint8_t dl = this->frame_buffer_[4];
  const uint8_t *d = this->frame_buffer_.data() + 5;
  ESP_LOGI(TAG, "DATA OK len=%d", dl);
  auto u16 = [&](size_t o) -> uint16_t { return (o + 1 < dl) ? ((uint16_t)(d[o] << 8) | d[o + 1]) : 0; };
  // 电芯电压响应 (reg=32, count=4, dl=8)
  if (dl == 8 && this->frame_buffer_[2] == 0x00 && this->frame_buffer_[3] == 0x20) {
    float mn = 100, mx = -100, sm = 0; uint8_t nf = 0;
    for (uint8_t i = 0; i < 4; i++) {
      float v = (float) u16(i * 2) / 1000.0f;
      if (this->cell_voltage_sensors_[i]) this->cell_voltage_sensors_[i]->publish_state(v);
      if (v > 0) { sm += v; nf++; if (v < mn) mn = v; if (v > mx) mx = v; }
    }
    if (nf) {
      if (this->min_cell_voltage_sensor_) this->min_cell_voltage_sensor_->publish_state(mn);
      if (this->max_cell_voltage_sensor_) this->max_cell_voltage_sensor_->publish_state(mx);
      if (this->delta_cell_voltage_sensor_) this->delta_cell_voltage_sensor_->publish_state(mx - mn);
      if (this->average_cell_voltage_sensor_) this->average_cell_voltage_sensor_->publish_state(sm / nf);
    }
    this->frame_buffer_.clear();
    return;
  }
  if (dl < 62) { this->frame_buffer_.clear(); return; }
  auto u32 = [&](size_t o) -> uint32_t {
    return (o + 3 < dl) ? (((uint32_t) d[o] << 24) | ((uint32_t) d[o + 1] << 16) | ((uint32_t) d[o + 2] << 8) | d[o + 3]) : 0;
  };
  float tv = (float) u32(REG_TOTAL_VOLTAGE * 2);
  float cr = (float)((int32_t) u32(REG_CURRENT * 2));
  float pw = tv * cr / 1000000.0f;
  uint8_t sc = u16(REG_PERC_AND_HEALTH * 2) & 0xFF;
  uint8_t cc = (dl > REG_BATTERY_COUNT_TYPE * 2 + 1) ? d[REG_BATTERY_COUNT_TYPE * 2] : 4;
  if (!cc || cc > 32) cc = 4;
  ESP_LOGI(TAG, "V=%.3f I=%.3f P=%.2f SOC=%d cells=%d", tv / 1000, cr / 1000, pw, sc, cc);
  if (this->total_voltage_sensor_) this->total_voltage_sensor_->publish_state(tv / 1000.0f);
  if (this->current_sensor_) this->current_sensor_->publish_state(cr / 1000.0f);
  if (this->power_sensor_) this->power_sensor_->publish_state(pw);
  if (this->state_of_charge_sensor_) this->state_of_charge_sensor_->publish_state((float) sc);
  if (this->capacity_remaining_sensor_) this->capacity_remaining_sensor_->publish_state((float) u32(REG_REMAINING_CAPACITY * 2) / 1000.0f);
  if (this->mos_temperature_sensor_) this->mos_temperature_sensor_->publish_state(get_temperature_(u16(REG_MOS_TEMP * 2)));
  if (this->temperature_sensor_1_) this->temperature_sensor_1_->publish_state(get_temperature_(u16(REG_BATTERY_TEMP_0 * 2)));
  if (this->temperature_sensor_2_) this->temperature_sensor_2_->publish_state(get_temperature_(u16(REG_BATTERY_TEMP_1 * 2)));
  if (this->full_capacity_sensor_) this->full_capacity_sensor_->publish_state((float) u16(REG_FULL_CAPACITY * 2) / 100.0f);
  if (this->design_capacity_sensor_) this->design_capacity_sensor_->publish_state((float) u16(REG_DESIGN_CAPACITY * 2) / 100.0f);
  if (this->charging_cycles_sensor_) this->charging_cycles_sensor_->publish_state((float)(u16(REG_FOR_COUNT * 2) & 0xFF));
  if (this->total_cycle_capacity_sensor_) this->total_cycle_capacity_sensor_->publish_state((float)(u16(REG_FOR_COUNT * 2) & 0xFF) * this->battery_capacity_);
  if (this->connected_time_sensor_) {
    uint32_t sec = this->get_connected_time();
    this->connected_time_sensor_->publish_state((float) sec);
  }
  if (this->connected_time_text_sensor_) {
    uint32_t sec = this->get_connected_time();
    char buf[32];
    if (sec < 60) {
      snprintf(buf, sizeof(buf), "%us", sec);
    } else if (sec < 3600) {
      snprintf(buf, sizeof(buf), "%um%us", sec / 60, sec % 60);
    } else if (sec < 86400) {
      snprintf(buf, sizeof(buf), "%uh%um%us", sec / 3600, (sec % 3600) / 60, sec % 60);
    } else {
      snprintf(buf, sizeof(buf), "%ud%uh%um%us", sec / 86400, (sec % 86400) / 3600, (sec % 3600) / 60, sec % 60);
    }
    this->connected_time_text_sensor_->publish_state(buf);
  }
  float mn = 100, mx = -100, sm = 0; uint8_t nf = 0;
  for (uint8_t i = 0; i < cc && i < 32; i++) {
    size_t o = (REG_MIN_BATTERY_ADDRESS + i) * 2;
    if (o + 1 >= dl) break;
    float v = (float) u16(o) / 1000.0f;
    if (this->cell_voltage_sensors_[i]) this->cell_voltage_sensors_[i]->publish_state(v);
    if (v > 0) { sm += v; nf++; if (v < mn) mn = v; if (v > mx) mx = v; }
  }
  if (nf) {
    if (this->min_cell_voltage_sensor_) this->min_cell_voltage_sensor_->publish_state(mn);
    if (this->max_cell_voltage_sensor_) this->max_cell_voltage_sensor_->publish_state(mx);
    if (this->delta_cell_voltage_sensor_) this->delta_cell_voltage_sensor_->publish_state(mx - mn);
    if (this->average_cell_voltage_sensor_) this->average_cell_voltage_sensor_->publish_state(sm / nf);
  }
  this->frame_buffer_.clear();
}

void LianBmsBle::publish_device_unavailable_() {
  if (this->online_status_binary_sensor_) this->online_status_binary_sensor_->publish_state(false);
  auto n = [](sensor::Sensor *s) { if (s) s->publish_state(NAN); };
  n(total_voltage_sensor_); n(current_sensor_); n(power_sensor_); n(capacity_remaining_sensor_);
  n(state_of_charge_sensor_); n(mos_temperature_sensor_); n(temperature_sensor_1_); n(temperature_sensor_2_);
  n(min_cell_voltage_sensor_); n(max_cell_voltage_sensor_); n(delta_cell_voltage_sensor_);
  n(average_cell_voltage_sensor_); for (int i = 0; i < 32; i++) n(cell_voltage_sensors_[i]);
}

void LianBmsBle::dump_config() {
  ESP_LOGCONFIG(TAG, "LiAn BMS BLE v1.0.32:");
  LOG_SENSOR("", "Total Voltage", this->total_voltage_sensor_);
  LOG_SENSOR("", "Current", this->current_sensor_);
  LOG_SENSOR("", "Power", this->power_sensor_);
  LOG_SENSOR("", "SOC", this->state_of_charge_sensor_);
  LOG_BINARY_SENSOR("", "Online Status", this->online_status_binary_sensor_);
  LOG_TEXT_SENSOR("", "Response", this->response_text_sensor_);
  LOG_TEXT_SENSOR("", "Debug", this->debug_text_sensor_);
  LOG_SWITCH("", "Charge", this->charge_switch_);
  LOG_SWITCH("", "Discharge", this->discharge_switch_);
  LOG_SWITCH("", "Heating", this->heating_switch_);
}

uint32_t LianBmsBle::get_connected_time() const {
  if (!this->char_found_ || this->ready_time_ == 0) return 0;
  return (millis() - this->ready_time_) / 1000;
}

void LianBmsSwitch::write_state(bool state) {
  if (state) {
    this->parent_->write_register(this->reg_on_, this->value_on_);
  } else {
    this->parent_->write_register(this->reg_off_, this->value_off_);
  }
  this->publish_state(state);
}

void LianBmsBle::apply_mac_address(const std::string &mac) {
  ESP_LOGI(TAG, "Applying new MAC address: %s", mac.c_str());
  // 解析 MAC 字符串 "58:C8:01:19:31:30" 为 uint64_t
  uint64_t addr = 0;
  int v[6];
  if (sscanf(mac.c_str(), "%x:%x:%x:%x:%x:%x", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5]) == 6) {
    for (int i = 0; i < 6; i++) addr = (addr << 8) | (v[i] & 0xFF);
    this->set_address(addr);  // 更新 BLE 客户端 MAC
    if (this->mac_address_text_sensor_) this->mac_address_text_sensor_->publish_state(mac);
    ESP_LOGI(TAG, "MAC address updated to: %s", mac.c_str());
  } else {
    ESP_LOGW(TAG, "Invalid MAC format: %s (expected XX:XX:XX:XX:XX:XX)", mac.c_str());
  }
}

void LianBmsBatteryCapacityNumber::control(float value) {
  this->publish_state(value);
  if (this->parent_) {
    this->parent_->set_battery_capacity(value);
    ESP_LOGI(TAG, "Battery capacity set to: %.1f Ah", value);
  }
}

void LianBmsCellCountNumber::control(float value) {
  this->publish_state(value);
  if (this->parent_) {
    this->parent_->set_cell_count(value);
    ESP_LOGI(TAG, "Cell count set to: %.0f", value);
  }
}

}  // namespace lian_bms_ble
}  // namespace esphome
#endif
