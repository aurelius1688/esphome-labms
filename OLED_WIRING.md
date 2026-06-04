# OLED 显示屏接线说明

## 硬件要求

- ESP32-C3 开发板
- SH1106 128x64 OLED 显示屏（I2C接口）
- 4根杜邦线

## 接线图

```
ESP32-C3          SH1106 OLED
┌─────────┐      ┌─────────────┐
│         │      │             │
│  GPIO4  ├──────┤ SDA         │
│         │      │             │
│  GPIO5  ├──────┤ SCL         │
│         │      │             │
│  3.3V   ├──────┤ VCC         │
│         │      │             │
│  GND    ├──────┤ GND         │
│         │      │             │
└─────────┘      └─────────────┘
```

## 引脚对应表

| ESP32-C3 引脚 | OLED 引脚 | 功能说明 |
|--------------|-----------|----------|
| GPIO4 | SDA | I2C 数据线 |
| GPIO5 | SCL | I2C 时钟线 |
| 3.3V | VCC | 电源正极 |
| GND | GND | 电源负极 |

## 接线步骤

### 1. 确认引脚位置

ESP32-C3 开发板引脚布局：
```
        ┌───────────────┐
        │   ESP32-C3    │
        │               │
   GPIO4├─●           ●─┤3V3
   GPIO5├─●           ●─┤GND
        │               │
        └───────────────┘
```

### 2. 连接电源

- OLED VCC → ESP32-C3 3.3V
- OLED GND → ESP32-C3 GND

### 3. 连接I2C信号

- OLED SDA → ESP32-C3 GPIO4
- OLED SCL → ESP32-C3 GPIO5

## 注意事项

### 电压要求
- ✅ 使用 3.3V 供电（推荐）
- ⚠️ 部分OLED模块支持5V，请查看模块说明
- ❌ 不要超过3.3V（可能损坏OLED）

### I2C地址
- 默认地址：0x3C
- 部分模块地址：0x3D
- 如地址不匹配，需修改yaml配置

### 接线检查
1. 确认所有连接牢固
2. 检查是否有短路
3. 上电前再次核对引脚

## 常见问题

### OLED不显示
1. 检查接线是否正确
2. 确认I2C地址是否正确
3. 尝试扫描I2C设备：
   ```yaml
   i2c:
     sda: GPIO4
     scl: GPIO5
     scan: true  # 启用扫描
   ```

### 显示乱码
1. 检查OLED型号是否为SH1106
2. 确认分辨率设置为128x64
3. 尝试调整对比度

### 显示方向错误
修改yaml配置中的rotation参数：
```yaml
display:
  - platform: ssd1306_i2c
    model: SH1106_128X64
    address: 0x3C
    rotation: 0°    # 可选: 0°, 90°, 180°, 270°
```

## 扩展接线（可选）

### 添加按钮控制
如需添加按钮切换显示页面：
```
ESP32-C3          按钮
┌─────────┐      ┌─────┐
│  GPIO6  ├──────┤ BTN ├──────┤GND
└─────────┘      └─────┘
```

### 多个I2C设备
如需连接多个I2C设备，所有设备共享SDA和SCL线：
```
ESP32-C3     OLED    传感器
GPIO4  ──────┤SDA├────┤SDA├──
GPIO5  ──────┤SCL├────┤SCL├──
3.3V   ──────┤VCC├────┤VCC├──
GND    ──────┤GND├────┤GND├──
```

## 参考资料

- [ESP32-C3 引脚图](https://docs.espressif.com/projects/esp-idf/en/latest/esp32c3/hw-reference/esp32c3/user-guide-devkitc-02.html)
- [SH1106 数据手册](https://www.solomon-systech.com/product/sh1106/)
- [ESPHome SSD1306 组件](https://esphome.io/components/display/ssd1306.html)
