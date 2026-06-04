# ESP32 锂安 BMS BLE 组件 - GitHub 开源项目说明

## 项目概述

这是一个用于通过 BLE（蓝牙低功耗）连接锂安 BMS 的 ESPHome 自定义组件。该组件允许 ESP32 设备读取锂安 BMS 的电池数据，并通过 Home Assistant 进行监控和控制。

## 功能特性

### 数据读取功能
- ✅ 总电压、电流、功率监测
- ✅ SOC（电量百分比）显示
- ✅ 剩余容量、满充容量、设计容量
- ✅ 温度监测（MOS温度、电池温度1、电池温度2）
- ✅ 电芯电压（支持多串电池，交替读取）
- ✅ 压差（最高-最低电压差）
- ✅ 充电循环次数和总循环容量
- ✅ 连接运行时间

### 控制功能
- ✅ 充电 MOS 开关控制
- ✅ 放电 MOS 开关控制
- ✅ 加热开关控制

### 显示支持
- ✅ OLED 显示屏（SH1106 128x64）
- ✅ Web 界面
- ✅ Home Assistant 集成

## 硬件要求

### 必需硬件
- ESP32-C3 开发板（或其他 ESP32 系列）
- 锂安 BMS（支持 BLE 的型号）

### 可选硬件
- SH1106 OLED 显示屏（I2C接口）
- 电源模块

## 快速开始

### 1. 克隆项目

```bash
git clone https://github.com/your-username/esp32-lian-bms.git
cd esp32-lian-bms
```

### 2. 配置 WiFi 和 BMS 信息

复制示例配置文件并修改：

```bash
cp example.yaml my-bms.yaml
```

编辑 `my-bms.yaml`，修改以下配置：

```yaml
wifi:
  ssid: "YOUR_WIFI_SSID"          # 替换为你的WiFi名称
  password: "YOUR_WIFI_PASSWORD"   # 替换为你的WiFi密码

ble_client:
  - mac_address: "YOUR_BMS_MAC_ADDRESS"  # 替换为你的BMS MAC地址
    id: client0
```

### 3. 编译和烧录

使用 ESPHome 编译和烧录固件：

```bash
# 安装 ESPHome
pip install esphome

# 编译固件
esphome compile my-bms.yaml

# 烧录固件（通过USB）
esphome run my-bms.yaml
```

### 4. 获取 BMS MAC 地址

如果不知道 BMS 的 MAC 地址，可以：

1. 使用手机 BLE 扫描应用（如 nRF Connect）
2. 搜索设备名包含 "LABMS" 的设备
3. 记录 MAC 地址

## 项目结构

```
esp32-lian-bms/
├── components/
│   └── lian_bms_ble/
│       ├── __init__.py          # 组件配置
│       ├── lian_bms_ble.h       # 头文件（寄存器映射、类定义）
│       ├── lian_bms_ble.cpp     # 实现（CRC、BLE通信、数据解析）
│       ├── sensor.py            # 传感器定义
│       ├── switch.py            # 开关定义（充放电控制）
│       ├── text_sensor.py       # 文本传感器
│       ├── binary_sensor.py     # 二进制传感器
│       └── number.py            # 数字输入
├── example.yaml                 # 示例配置文件
├── ESP32连接锂安BMS完整经验说明_开源版.md  # 详细技术文档
├── CODE_STRUCTURE.md            # 代码结构说明
├── LICENSE                      # 开源许可证
└── README.md                    # 本文件
```

## 技术细节

### BLE 协议

- 使用 BLE 连接锂安 BMS
- 服务 UUID：`00002760-08C2-11E1-9073-0E8AC72E0001`
- 写入特征：`00002760-08C2-11E1-9073-0E8AC72E0001`（WRITE_NR）
- 通知特征：`00002760-08C2-11E1-9073-0E8AC72E0002`（NOTIFY）

### 数据协议

- 命令格式：Modbus RTU 变体
- CRC 算法：CRC16-XMODEM（多项式 0x1021，初始值 0x0000）
- 功能码：0x03（读保持寄存器）、0x10（写多个寄存器）

### 关键发现

经过约28个版本的调试，发现 BMS 使用 CRC16-XMODEM 而非标准的 Modbus CRC16。BMS 收到 CRC 错误的命令后会静默丢弃，不返回任何错误。

详细技术文档请参考：[ESP32连接锂安BMS完整经验说明_开源版.md](ESP32连接锂安BMS完整经验说明_开源版.md)

## 故障排除

### BMS 无响应

1. 检查 MAC 地址是否正确
2. 确认 BMS 已开机且在范围内
3. 检查 BLE 连接状态（查看日志）
4. 验证 CRC 算法是否正确

### 数据异常

1. 检查寄存器映射是否正确
2. 验证数据单位转换
3. 确认电池配置（串数、容量）

### 连接不稳定

1. 调整 BLE 扫描参数
2. 检查 WiFi 信号强度
3. 增加重试机制

## 贡献指南

欢迎提交 Issue 和 Pull Request！

1. Fork 项目
2. 创建功能分支
3. 提交更改
4. 推送到分支
5. 创建 Pull Request

## 许可证

本项目采用 MIT 许可证，详见 [LICENSE](LICENSE) 文件。

## 致谢

- 感谢 ESPHome 项目提供的框架
- 感谢锂安 BMS 提供的硬件支持
- 感谢社区成员的测试和反馈

## 联系方式

如有问题或建议，请通过以下方式联系：

- 提交 GitHub Issue
- 发送邮件至：your-email@example.com

## 更新日志

### v1.0.0 (2026-05-22)
- 初始版本发布
- 实现 BLE 连接和数据读取
- 实现充放电控制
- 支持 OLED 显示
- 支持 Home Assistant 集成

## 隐私说明

本项目已进行隐私化处理，移除了以下敏感信息：
- WiFi 密码和 SSID
- API 加密密钥
- BMS MAC 地址
- OTA 密码
- 个人文件路径

请在使用前替换为自己的配置信息。
