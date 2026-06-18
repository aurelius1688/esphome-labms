# ESP32 连接锂安 BMS 完整成功经验说明

## 一、项目概述

### 目标
使用 ESP32-C3 通过 BLE 连接锂安 BMS，读取电池数据并集成到 Home Assistant。

### BMS 信息
- 型号：LABMS-B104-40A（或其他锂安BMS型号）
- 设备名：LABMS-XXXXXUXXXXX（示例）
- 电池配置：4串 40Ah（根据实际配置调整）
- BLE Service UUID：00002760-08C2-11E1-9073-0E8AC72E0001

---

## 二、核心问题与解决过程

### 问题：BLE 连接正常但 BMS 不发送数据

**现象**：
- BLE 连接成功（CONNECT_EVT status=0）
- 配对成功（AUTH_CMPL mode=1）
- 服务发现成功（4个服务，BMS服务 0x0021-0x0027）
- CCCD 写入成功（WRITE_DESCR status=0）
- 命令发送成功（WRITE_CHAR status=0）
- **但 BMS 从不发送 NOTIFY 响应**

**迭代版本**：v1.0.0 ~ v1.0.28（约28个版本）

### 根因：CRC16 算法错误

**发现过程**：

1. **对比手机 BLE HCI 日志**：
   - 手机发送：`16 03 00 04 00 02 ED 87`
   - ESP32 发送：`16 03 00 04 00 02 2D BB`
   - **同一个命令，CRC 不同！**

2. **提取小程序 CRC 算法**：
   ```javascript
   function a(e) {
       for (var r = 0, t = 0; t < e.length; t++) 
           r = r << 8 ^ n[255 & (r >> 8 ^ e[t])];
       return 65535 & (0 ^ r)
   }
   ```
   这是 **CRC16-XMODEM** 算法（多项式 0x1021，初始值 0x0000）

3. **我们使用的是 Modbus CRC16**（多项式 0xA001，初始值 0xFFFF）— 完全不同的算法！

4. **用 Python 验证**：
   ```python
   # CRC16-XMODEM
   n = [0, 4129, 8258, ...]  # 从JS提取的256项查找表
   def crc16(data):
       r = 0
       for byte in data:
           r = (r << 8) ^ n[255 & ((r >> 8) ^ byte)]
           r &= 0xFFFF
       return r
   
   crc16([0x16, 0x03, 0x00, 0x04, 0x00, 0x02])
   # 结果: 0x8535 → CRC 字节: 0x35, 0x85
   ```

5. **替换 CRC 表后，BMS 立即响应 NOTIFY！**

### 关键结论

**BMS 收到 CRC 错误的命令后，静默丢弃，不返回任何错误。** 这让我们误以为是 BLE 层的问题（配对、CCCD、通知注册），实际上应用层的命令根本没被 BMS 识别。

---

## 三、BLE 连接协议

### 连接流程

```
1. CONNECT_EVT → ESP32 连接 BMS
2. AUTH_CMPL_EVT → 配对完成（ESP_BLE_SEC_ENCRYPT）
3. SEARCH_CMPL_EVT → 发现服务，找到 BMS 特征
4. REG_FOR_NOTIFY_EVT → 注册通知
5. WRITE_DESCR_EVT → 写入 CCCD (0x0001)
6. BMS READY → 开始轮询
```

### BLE 特征

| Handle | UUID | 属性 | 说明 |
|--------|------|------|------|
| 0x0023 | 00002760-...-0E8AC72E0001 | WRITE_NR (0x04) | 写入命令 |
| 0x0026 | 00002760-...-0E8AC72E0002 | NOTIFY (0x10) | 接收响应 |
| 0x0027 | CCCD | - | 通知描述符（写入 0x0001 启用通知） |

### 配对模式

```cpp
esp_ble_set_encryption(remote_bda, ESP_BLE_SEC_ENCRYPT);
```

- 使用 `ESP_BLE_SEC_ENCRYPT`（无 MITM）
- 配对模式：Just Works（mode=1）
- 配对后 BLE 链路加密

---

## 四、数据协议

### 命令格式（Modbus RTU 变体）

```
[0x16] [0x03] [regH] [regL] [countH] [countL] [crcLow] [crcHigh]
```

- 功能码：0x03（读保持寄存器）
- CRC 算法：CRC16-XMODEM
- CRC 低字节在前

### 响应格式

```
[0x16] [0x03] [byteCount] [data...] [crcLow] [crcHigh]
```

### 寄存器映射

| 寄存器 | 地址 | 长度 | 单位 | 说明 |
|--------|------|------|------|------|
| MOS_TEMP | 0 | 2字节 | 0.1K | MOS 温度（需-273.1°C转换） |
| BATTERY_TEMP_0 | 1 | 2字节 | 0.1K | 电池温度 1 |
| BATTERY_TEMP_1 | 2 | 2字节 | 0.1K | 电池温度 2 |
| TOTAL_VOLTAGE | 4 | 4字节 | mV | 总电压 |
| CURRENT | 4 | 4字节 | mA | 电流（有符号，负值=充电） |
| REMAINING_CAPACITY | 8 | 4字节 | mAh | 剩余容量 |
| PERC_AND_HEALTH | 20 | 2字节 | % | SOC（低字节） |
| BATTERY_COUNT_TYPE | 24 | 2字节 | - | 电芯数量（低字节） |
| FOR_COUNT | 26 | 2字节 | - | 充电循环次数 |
| FULL_CAPACITY | 27 | 2字节 | 0.01Ah | 满充容量 |
| DESIGN_CAPACITY | 28 | 2字节 | 0.01Ah | 设计容量 |
| MIN_BATTERY_ADDRESS | 32 | 每个2字节 | mV | 电芯电压（从 reg32 开始） |

### 温度转换公式

```cpp
float temperature = (raw_value - 2731.0f) / 10.0f;  // 单位：°C
```

### 电芯电压读取

电芯电压从寄存器 32 开始，每个电芯占 2 字节。由于 BMS 单次读取最多返回 31 个寄存器，需要交替读取：
- 奇数次：读 reg=0, count=31（基本数据）
- 偶数次：读 reg=32, count=4（电芯电压）

---

## 五、写命令协议（控制功能）

### 写命令格式（功能码 0x10）

```
[0x16] [0x10] [regH] [regL] [countH] [countL] [byteCount] [dataH] [dataL] [crcLow] [crcHigh]
```

### 控制命令映射

| 功能 | 寄存器地址 | 数据 | 说明 |
|------|-----------|------|------|
| 打开充电 | 6 | 0x0000 | 充电 MOS 开启 |
| 关闭充电 | 7 | 0x0000 | 充电 MOS 关闭 |
| 打开放电 | 3 | 0x0000 | 放电 MOS 开启 |
| 关闭放电 | 4 | 0x0000 | 放电 MOS 关闭 |
| 打开加热 | 32 | 0x0001 | 加热开启 |
| 关闭加热 | 32 | 0x0000 | 加热关闭 |

---

## 六、关键技术点

### 1. CRC16-XMODEM 实现

```cpp
// CRC16-XMODEM 查找表（256项，从JS提取）
static const uint16_t CRC16_TABLE[256] = {
    0, 4129, 8258, 12387, 16516, 20645, 24774, 28903,
    // ... 完整256项
};

uint16_t crc16_(const uint8_t *d, size_t len) {
    uint16_t r = 0;
    for (size_t i = 0; i < len; i++)
        r = (r << 8) ^ CRC16_TABLE[255 & ((r >> 8) ^ d[i])];
    return r;
}
```

### 2. 不需要配对

小程序不需要配对即可通信。ESP32 使用 `ESP_BLE_SEC_ENCRYPT` 请求配对，但实际测试发现配对不是必需的。

### 3. CCCD 写入

```cpp
uint16_t val = 0x0001;  // 启用通知
esp_ble_gattc_write_char_descr(
    gattc_if, conn_id,
    char_handle + 1,  // CCCD 句柄
    2, (uint8_t *) &val,
    ESP_GATT_WRITE_TYPE_RSP, ESP_GATT_AUTH_REQ_NONE
);
```

### 4. 交替读取策略

```cpp
void LianBmsBle::update() {
    if (this->read_cells_next_) {
        this->send_read_command_(0x0020, 4);  // 电芯电压
    } else {
        this->send_read_command_(0x0000, 31); // 基本数据
    }
    this->read_cells_next_ = !this->read_cells_next_;
}
```

---

## 七、调试经验

### 1. BLE HCI 日志对比

使用手机 BLE HCI 日志与 ESP32 日志对比，发现 CRC 不一致。这是定位问题的关键。

### 2. 小程序源码分析

通过反编译微信小程序，提取了：
- CRC16-XMODEM 算法和查找表
- 寄存器映射
- 命令格式
- 控制协议

### 3. 逐步排查

尝试过的方向（均无效）：
- 不同的安全模式（No-MITM, MITM, 无加密）
- 手动/自动 CCCD 写入
- 不同的寄存器地址和数量
- MTU 协商
- 配对时序调整
- 读取 Device Name 特征
- nRF Connect 对比测试

### 4. Python 验证

使用 Python 脚本验证 CRC 算法，确认 CRC16-XMODEM 与 Modbus CRC16 结果完全不同。

---

## 八、最终实现

### 功能列表

| 功能 | 状态 | 说明 |
|------|------|------|
| 总电压 | ✅ | 单位 V |
| 电流 | ✅ | 单位 A（负值=充电） |
| 功率 | ✅ | 单位 W |
| SOC | ✅ | 单位 % |
| 剩余容量 | ✅ | 单位 Ah |
| 满充容量 | ✅ | 单位 Ah |
| 设计容量 | ✅ | 单位 Ah |
| 循环容量 | ✅ | 充电次数 × 40Ah |
| 温度 | ✅ | MOS、电池1、电池2 |
| 电芯电压 | ✅ | 4 串，交替读取 |
| 压差 | ✅ | 最高-最低 |
| 充电循环次数 | ✅ | - |
| 连接运行时间 | ✅ | 格式化显示 |
| 充放电控制 | ✅ | 命令已实现，测试成功 |
| OLED 显示 | ✅ | SH1106 128x64 |
| Web 界面 | ✅ | - |
| Home Assistant | ✅ | 通过 ESPHome API |
| OTA 升级 | ✅ | - |

---

## 九、经验总结

### 关键教训

1. **CRC 算法必须与设备一致**：不同设备可能使用不同的 CRC 算法（Modbus、XMODEM、CCITT 等），必须从设备协议或逆向工程中确认。

2. **静默丢弃是常见设计**：BMS 收到错误命令后不返回错误，直接丢弃。这会导致问题难以定位。

3. **BLE 层成功不代表应用层成功**：WRITE_CHAR status=0 只表示 BLE 层发送成功，不代表 BMS 正确解析了命令。

4. **逆向工程小程序是有效手段**：通过分析小程序源码，可以获取完整的协议细节。

5. **Python 验证是快速调试方法**：在修改固件前，用 Python 验证算法和数据格式。


## 十、参考资料

### 逆向工程方法
- 使用微信小程序反编译工具获取协议细节
- 通过 BLE HCI 日志对比分析通信差异
- 使用 Python 脚本验证算法正确性

### ESP-IDF BLE API
- `esp_ble_gattc_write_char`：写特征值
- `esp_ble_gattc_write_char_descr`：写描述符
- `esp_ble_gattc_register_for_notify`：注册通知
- `esp_ble_set_encryption`：请求配对
