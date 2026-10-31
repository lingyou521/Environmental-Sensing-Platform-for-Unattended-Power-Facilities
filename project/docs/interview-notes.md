# 无人值守电力设施环境感知平台：面试要点

## 一句话介绍

我在 i.MX6ULL 嵌入式 Linux 上实现了一套无人值守电力设施环境感知平台，自己完成 SHT30、MQ-2、门磁、PIR 和蜂鸣器的 Linux 驱动/用户态接入，并用多线程 mailbox 组织采集、告警、LVGL、SQLite 和 MQTT 上报。

## 技术栈

- i.MX6ULL、Embedded Linux、U-Boot、设备树、交叉编译
- C、pthread、mailbox、链表队列、日志和 SQLite
- I2C、IIO ADC、GPIO、Platform、misc device
- LVGL、framebuffer、触摸屏、MQTT、Ubuntu C Web 服务

## 重点问题

### 为什么用户态还需要 mailbox？

采集线程产生的是一次完整的采样快照。通过 mailbox 将同一份快照发送给显示、存储和 MQTT 线程，避免多个线程分别读取设备节点导致时间不一致，也避免用全局变量传递半更新数据。

### MQ-2 为什么只在驱动返回原始值？

驱动只负责 IIO 通道读取和错误返回，滑动均值、阈值和连续确认属于业务策略，放在用户态后可以现场调整阈值而不重新编译内核模块。

### 如何减少误报？

烟雾、门磁和 PIR 都进入统一状态机。异常需要连续确认，恢复也需要连续清除；声光输出只跟随综合告警状态变化，避免单次 ADC 抖动频繁鸣叫。

### MQTT 断线怎么处理？

MQTT 线程独立维护连接状态，连接失败按固定间隔重试。凭据通过 `POWER_MQTT_PASSWORD` 注入，源码只保留 broker、设备名和属性名等非敏感配置。

### 如何定位设备问题？

先看 `dmesg` 确认驱动 probe，再用 `/dev/sht30_misc`、`/dev/mq2_misc`、`/dev/door_misc`、`/dev/pir_misc` 单独验证数据，最后检查主应用日志、SQLite 新增记录和 Web `/api/realtime`。
