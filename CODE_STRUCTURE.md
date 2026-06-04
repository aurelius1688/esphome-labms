# 代码结构说明

本文档详细说明 ESP32 锂安 BMS BLE 组件的代码结构和各模块功能。

## 目录结构

```
components/
└── lian_bms_ble/
    ├── __init__.py          # 组件配置和初始化
    ├── lian_bms_ble.h       # 头文件（类定义、寄存器映射）
    ├── lian_bms_ble.cpp     # 核心实现（BLE通信、数据解析）
    ├── sensor.py            # 传感器实体定义
    ├── switch.py            # 开关实体定义
    ├── text_sensor.py       # 文本传感器定义
    ├── binary_sensor.py     # 二进制传感器定义
    └── number.py            # 数字输入实体定义
```

## 核心文件详解

### 1. `lian_bms_ble.h` - 头文件

#### 寄存器映射常量

```cpp
static const uint16_t REG_MOS_TEMP = 0;           // MOS温度寄存器
static const uint16_t REG_BATTERY_TEMP_0 = 1;      // 电池温度1寄存器
static const uint16_t REG_BATTERY_TEMP_1 = 2;      // 电池温度2寄存器
static const uint16_t REG_TOTAL_VOLTAGE = 4;       // 总电压寄存器
static const uint16_t REG_CURRENT = 6;             // 电流寄存器
static const uint16_t REG_REMAINING_CAPACITY = 8;  // 剩余容量寄存器
static const uint16_t REG_PERC_AND_HEALTH = 20;    // SOC寄存器
static const uint16_t REG_BATTERY_COUNT_TYPE = 24; // 电池配置寄存器
static const uint16_t REG_FOR_COUNT = 26;          // 循环次数寄存器
static const uint16_t REG_FULL_CAPACITY = 27;      // 满充容量寄存器
static const uint16_t REG_DESIGN_CAPACITY = 28;    // 设计容量寄存器
static const uint16_t REG_MIN_BATTERY_ADDRESS = 32; // 电芯电压起始寄存器
```

#### 主类 `LianBmsBle`

继承自 `PollingComponent` 和 `BLEClientNode`，实现 BMS 数据读取和控制功能。

**公共方法：**
- `dump_config()` - 输出配置信息
- `update()` - 定时更新数据（轮询）
- `gattc_event_handler()` - 处理 GATT 客户端事件
- `gap_event_handler()` - 处理 GAP 事件
- `send_raw_command()` - 发送原始命令
- `write_register()` - 写入寄存器
- `apply_mac_address()` - 动态应用 MAC 地址

**传感器设置方法：**
- `set_total_voltage_sensor()` - 设置总电压传感器
- `set_current_sensor()` - 设置电流传感器
- `set_power_sensor()` - 设置功率传感器
- `set_capacity_remaining_sensor()` - 设置剩余容量传感器
- `set_state_of_charge_sensor()` - 设置SOC传感器
- `set_mos_temperature_sensor()` - 设置MOS温度传感器
- `set_temperature_sensor_1_sensor()` - 设置电池温度1传感器
- `set_temperature_sensor_2_sensor()` - 设置电池温度2传感器
- `set_min_cell_voltage_sensor()` - 设置最低电芯电压传感器
- `set_max_cell_voltage_sensor()` - 设置最高电芯电压传感器
- `set_delta_cell_voltage_sensor()` - 设置压差传感器
- `set_average_cell_voltage_sensor()` - 设置平均电芯电压传感器
- `set_cell_voltage_sensor()` - 设置单个电芯电压传感器
- `set_charging_cycles_sensor()` - 设置充电循环次数传感器
- `set_full_capacity_sensor()` - 设置满充容量传感器
- `set_design_capacity_sensor()` - 设置设计容量传感器
- `set_total_cycle_capacity_sensor()` - 设置总循环容量传感器

**开关设置方法：**
- `set_charge_switch()` - 设置充电开关
- `set_discharge_switch()` - 设置放电开关
- `set_heating_switch()` - 设置加热开关

**保护成员变量：**
- `char_handle_` - BLE 特征句柄
- `write_handle_` - 写入特征句柄
- `char_found_` - 特征是否找到
- `auth_done_` - 认证是否完成
- `cccd_written_` - CCCD 是否写入
- `ready_time_` - 就绪时间
- `read_cells_next_` - 下次是否读取电芯电压
- `frame_buffer_` - 帧缓冲区
- `no_response_count_` - 无响应计数
- `battery_capacity_` - 电池容量
- `cell_count_` - 电芯数量

#### 辅助类

**`LianBmsSwitch`** - 开关控制类
- 继承自 `Switch` 和 `Component`
- 支持自定义寄存器地址和值
- 实现开/关控制

**`LianBmsBatteryCapacityNumber`** - 电池容量数字输入类
- 继承自 `Number` 和 `Component`
- 用于设置电池容量

**`LianBmsCellCountNumber`** - 电芯数量数字输入类
- 继承自 `Number` 和 `Component`
- 用于设置电芯数量

### 2. `lian_bms_ble.cpp` - 核心实现

#### BLE 连接流程

```cpp
void LianBmsBle::gattc_event_handler(esp_gattc_cb_event_t event, ...) {
    switch (event) {
        case ESP_GATTC_CONNECT_EVT:
            // 连接成功，请求配对
            esp_ble_set_encryption(remote_bda, ESP_BLE_SEC_ENCRYPT);
            break;
        case ESP_GATTC_SEARCH_CMPL_EVT:
            // 服务发现完成，查找特征
            break;
        case ESP_GATTC_REG_FOR_NOTIFY_EVT:
            // 注册通知成功，写入CCCD
            break;
        case ESP_GATTC_WRITE_DESCR_EVT:
            // CCCD写入完成，标记就绪
            break;
        case ESP_GATTC_WRITE_CHAR_EVT:
            // 命令写入完成
            break;
        case ESP_GATTC_NOTIFY_EVT:
            // 收到通知数据
            break;
    }
}
```

#### 数据更新轮询

```cpp
void LianBmsBle::update() {
    if (!this->ready_) return;
    
    // 交替读取基本数据和电芯电压
    if (this->read_cells_next_) {
        this->send_read_command_(0x0020, 4);  // 电芯电压
    } else {
        this->send_read_command_(0x0000, 31); // 基本数据
    }
    this->read_cells_next_ = !this->read_cells_next_;
}
```

#### CRC16-XMODEM 实现

```cpp
uint16_t LianBmsBle::crc16_(const uint8_t *data, size_t len) {
    uint16_t r = 0;
    for (size_t i = 0; i < len; i++)
        r = (r << 8) ^ CRC16_TABLE[255 & ((r >> 8) ^ data[i])];
    return r;
}
```

#### 命令发送

```cpp
void LianBmsBle::send_read_command_(uint16_t reg, uint16_t count) {
    uint8_t cmd[8];
    cmd[0] = 0x16;  // 设备地址
    cmd[1] = 0x03;  // 功能码：读保持寄存器
    cmd[2] = (reg >> 8) & 0xFF;  // 寄存器地址高字节
    cmd[3] = reg & 0xFF;         // 寄存器地址低字节
    cmd[4] = (count >> 8) & 0xFF;  // 寄存器数量高字节
    cmd[5] = count & 0xFF;         // 寄存器数量低字节
    
    uint16_t crc = this->crc16_(cmd, 6);
    cmd[6] = crc & 0xFF;         // CRC低字节
    cmd[7] = (crc >> 8) & 0xFF;  // CRC高字节
    
    // 发送命令
    esp_ble_gattc_write_char(this->gattc_if_, this->conn_id_,
                            this->write_handle_, sizeof(cmd), cmd,
                            ESP_GATT_WRITE_TYPE_NO_RSP, ESP_GATT_AUTH_REQ_NONE);
}
```

#### 数据解析

```cpp
void LianBmsBle::process_response_() {
    // 检查帧头和功能码
    if (this->frame_buffer_[0] != 0x16 || this->frame_buffer_[1] != 0x03) {
        return;
    }
    
    // 解析数据
    uint8_t byte_count = this->frame_buffer_[2];
    const uint8_t *data = &this->frame_buffer_[3];
    
    // 解析各个寄存器
    this->parse_registers_(data, byte_count);
}
```

#### 温度转换

```cpp
float LianBmsBle::get_temperature_(uint16_t raw) {
    // 原始值单位为0.1K，转换为°C
    return (raw - 2731.0f) / 10.0f;
}
```

### 3. `sensor.py` - 传感器定义

定义所有传感器实体，包括：

```python
SENSOR_TYPES = {
    "total_voltage": {
        "name": "Total Voltage",
        "unit_of_measurement": "V",
        "device_class": "voltage",
        "state_class": "measurement",
    },
    "current": {
        "name": "Current",
        "unit_of_measurement": "A",
        "device_class": "current",
        "state_class": "measurement",
    },
    "power": {
        "name": "Power",
        "unit_of_measurement": "W",
        "device_class": "power",
        "state_class": "measurement",
    },
    # ... 其他传感器
}
```

### 4. `switch.py` - 开关定义

定义充放电和加热控制开关：

```python
SWITCH_TYPES = {
    "charge": {
        "name": "Charge MOS",
        "reg_on": 6,
        "reg_off": 7,
        "value_on": 0x0000,
        "value_off": 0x0000,
    },
    "discharge": {
        "name": "Discharge MOS",
        "reg_on": 3,
        "reg_off": 4,
        "value_on": 0x0000,
        "value_off": 0x0000,
    },
    "heating": {
        "name": "Heating",
        "reg_on": 32,
        "reg_off": 32,
        "value_on": 0x0001,
        "value_off": 0x0000,
    },
}
```

### 5. `text_sensor.py` - 文本传感器

定义文本传感器，包括：
- BLE 调试信息
- 连接时间文本
- MAC 地址显示
- MAC 地址输入

### 6. `binary_sensor.py` - 二进制传感器

定义在线状态传感器：
- BMS 是否在线
- BLE 连接状态

### 7. `number.py` - 数字输入

定义数字输入实体：
- 电池容量设置
- 电芯数量设置

## 数据流

```
1. ESP32 BLE 连接 BMS
2. 配对和认证
3. 发现服务和特征
4. 注册通知
5. 写入 CCCD 启用通知
6. 定时发送读取命令
7. 接收 BMS 响应
8. 解析数据并更新传感器
9. 通过 ESPHome API 同步到 Home Assistant
```

## 关键算法

### CRC16-XMODEM

```cpp
// 查找表（256项）
static const uint16_t CRC16_TABLE[256] = {
    0, 4129, 8258, 12387, 16516, 20645, 24774, 28903,
    33032, 37161, 41290, 45419, 49548, 53677, 57806, 61935,
    // ... 完整256项
};

uint16_t crc16_(const uint8_t *data, size_t len) {
    uint16_t r = 0;
    for (size_t i = 0; i < len; i++)
        r = (r << 8) ^ CRC16_TABLE[255 & ((r >> 8) ^ data[i])];
    return r;
}
```

### 交替读取策略

```cpp
// 奇数次：读取基本数据（寄存器0-30）
// 偶数次：读取电芯电压（寄存器32-35）
if (this->read_cells_next_) {
    this->send_read_command_(0x0020, 4);  // 电芯电压
} else {
    this->send_read_command_(0x0000, 31); // 基本数据
}
this->read_cells_next_ = !this->read_cells_next_;
```

## 扩展开发

### 添加新传感器

1. 在 `lian_bms_ble.h` 中添加传感器指针和设置方法
2. 在 `lian_bms_ble.cpp` 中实现数据解析
3. 在 `sensor.py` 中定义传感器类型
4. 在 `__init__.py` 中注册传感器

### 添加新控制功能

1. 在 `lian_bms_ble.h` 中添加开关指针和设置方法
2. 在 `lian_bms_ble.cpp` 中实现写入逻辑
3. 在 `switch.py` 中定义开关类型
4. 在 `__init__.py` 中注册开关

### 修改协议

1. 更新 `lian_bms_ble.h` 中的寄存器映射
2. 修改 `lian_bms_ble.cpp` 中的解析逻辑
3. 调整 `sensor.py` 中的传感器定义

## 调试技巧

### 启用详细日志

```yaml
logger:
  level: VERBOSE
  logs:
    esp32_ble_tracker: VERBOSE
    esp32_ble_client: VERBOSE
    lian_bms_ble: VERBOSE
```

### 使用 nRF Connect 验证

1. 使用手机 nRF Connect 应用连接 BMS
2. 手动发送命令验证协议
3. 对比 ESP32 日志和手机日志

### Python 验证脚本

```python
def crc16(data):
    # CRC16-XMODEM 实现
    table = [0, 4129, 8258, ...]  # 256项查找表
    r = 0
    for byte in data:
        r = (r << 8) ^ table[255 & ((r >> 8) ^ byte)]
    return r

# 验证命令
cmd = [0x16, 0x03, 0x00, 0x04, 0x00, 0x02]
crc = crc16(cmd)
print(f"CRC: {crc:04X}")
```
