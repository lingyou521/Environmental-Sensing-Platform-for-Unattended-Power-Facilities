# 电力设施 Web 端设计

## 边界

板端负责 SHT30、MQ-2、门磁和 PIR 的采集、告警判断、LVGL 展示、SQLite 本地记录和 MQTT 上报。Ubuntu Web 端负责接收转发后的 JSON、缓存最近样本、落库和浏览器展示，不读取板端设备节点或板端数据库。

## 数据流

```text
i.MX6ULL MQTT
      |
      | mosquitto_sub + mqtt_forward.sh
      v
POST /ingest/telemetry
      |
      +--> realtime_cache --> GET /api/realtime
      +--> SQLite device_samples / sensor_samples_flat
      +--> GET /api/history /api/alarms /api/export
```

## 设施快照

实时接口返回：

```json
{
  "device": "power_facility_01",
  "online": true,
  "facility": {
    "temperature": 24.5,
    "humidity": 48.0,
    "smoke_raw": 420,
    "smoke_alarm": false,
    "door_open": false,
    "person_detected": false,
    "facility_alarm": false,
    "alarm_level": 0
  }
}
```

历史查询使用 `sensor_samples_flat`，支持 `temperature`、`humidity`、`smoke_raw`、`smoke_alarm`、`door_open`、`person_detected`、`facility_alarm` 和 `alarm_level`。

## 配置

`web_config.json` 保存采集周期、烟雾阈值 `smoke_high` 和入侵确认次数 `intrusion_count`。配置接口只更新 Ubuntu Web 本地行为，不向设备下发参数。

## 运行约束

- 服务默认绑定 `127.0.0.1`。
- 设置 `SENSOR_WEB_BIND=0.0.0.0` 时必须同时设置长随机 `SENSOR_WEB_TOKEN`。
- MQTT 由外部转发脚本接入，C 后端不内置 MQTT 客户端。
- 生产环境需要 HTTPS、设备身份校验、请求体上限、限流和防重放。
