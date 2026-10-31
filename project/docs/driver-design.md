# 电力设施传感器驱动设计

## 统一接口

每个驱动注册一个 misc 设备，用户态只依赖 `/dev/*` 节点和固定的 `read()` 数据格式。驱动内部负责总线访问、GPIO 读取、互斥保护和 `copy_to_user()` 错误处理。

| 节点 | 内核框架 | 读取格式 |
| --- | --- | --- |
| `/dev/sht30_misc` | I2C | `uint16_t temperature_raw + uint16_t humidity_raw` |
| `/dev/mq2_misc` | IIO consumer | `int smoke_raw` |
| `/dev/door_misc` | GPIO descriptor + Platform | `int door_open` |
| `/dev/pir_misc` | GPIO descriptor + Platform | `int person_detected` |
| `/dev/beep_misc` | GPIO + misc | `write(int)` 设置开关 |

## SHT30

用户态每次读取前发送单次测量命令 `0x24 0x00`，读取两个 16 位原始值并校验两段 CRC-8。温度和湿度按 SHT30 数据手册公式转换：

```text
T = -45 + 175 * raw_temperature / 65535
RH = 100 * raw_humidity / 65535
```

## MQ-2

MQ-2 由设备树提供 IIO ADC 通道 `mq2`。驱动在读取时调用 `iio_read_channel_raw()`，只返回原始采样值；滑动均值和阈值确认放在用户态采集线程，便于调整策略而不重新编译内核。

## GPIO 传感器

门磁和 PIR 均使用 `gpiod_get(..., GPIOD_IN)` 获得输入 GPIO。驱动统一将电平归一化为 0/1，极性差异通过设备树 GPIO flags 配置。

## 验证

目标板验证顺序：

1. 检查 `dmesg` 中各驱动 probe 是否成功。
2. 用 `dd if=/dev/sht30_misc bs=1 count=4` 检查 SHT30 读取。
3. 用小程序连续读取 MQ-2、门磁和 PIR，确认节点断开后返回错误。
4. 启动主应用，确认 MQTT、SQLite、LVGL 三个消费者都收到同一采样快照。
