# 电力设施驱动验证

## 加载模块

```bash
insmod sht30_drv.ko
insmod mq2_drv.ko
insmod door_drv.ko
insmod pir_drv.ko
insmod beep_drv.ko
```

检查日志和节点：

```bash
dmesg | tail -n 50
ls -l /dev/sht30_misc /dev/mq2_misc /dev/door_misc /dev/pir_misc /dev/beep_misc
```

## 用户态快速检查

SHT30 返回 4 字节，前两个字节是温度原始值，后两个字节是湿度原始值：

```bash
dd if=/dev/sht30_misc of=/tmp/sht30.raw bs=4 count=1
od -An -tu2 /tmp/sht30.raw
```

MQ-2、门磁和 PIR 返回一个 `int`：

```bash
dd if=/dev/mq2_misc of=/tmp/mq2.raw bs=4 count=1
dd if=/dev/door_misc of=/tmp/door.raw bs=4 count=1
dd if=/dev/pir_misc of=/tmp/pir.raw bs=4 count=1
od -An -td4 /tmp/mq2.raw /tmp/door.raw /tmp/pir.raw
```

蜂鸣器验证：

```bash
printf '\001\000\000\000' > /dev/beep_misc
printf '\000\000\000\000' > /dev/beep_misc
```

## 应用链路检查

1. 启动主应用并确认日志出现 `SHT30/MQ-2/door/PIR`。
2. 改变门磁或 PIR 输入，确认 LCD、蜂鸣器和 SQLite 同时变化。
3. 设置 `POWER_MQTT_PASSWORD` 后确认 MQTT payload 包含 `smoke_raw`、`door_open`、`person_detected` 和 `facility_alarm`。
4. 启动 Ubuntu Web，向 `/ingest/telemetry` 写入示例 JSON，检查实时、历史和告警页面。
