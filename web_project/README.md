# 电力设施环境感知 Ubuntu Web 端

Web 端独立运行在 Ubuntu，接收 i.MX6ULL 通过 MQTT 或转发脚本送来的设施遥测，更新内存实时缓存、写入 SQLite，并提供浏览器监控页面。

## 页面

- `/login.html`：演示登录入口。
- `/index.html`：温湿度、烟雾、门磁、人员和综合告警实时状态。
- `/history.html`：按传感器查询趋势并导出 CSV。
- `/alarms.html`：烟雾、门磁、人员入侵告警记录。
- `/config.html`：采集周期、烟雾阈值、入侵确认次数和服务状态。

## 编译运行

```bash
sudo apt install -y build-essential make libsqlite3-dev sqlite3 curl mosquitto-clients
make clean
make
make test
./bin/webserver -p 8080 -r ./www --db ./sensor_history.db --config ./web_config.json
```

默认只监听 `127.0.0.1`。需要局域网访问时：

```bash
export SENSOR_WEB_BIND=0.0.0.0
export SENSOR_WEB_TOKEN='换成一段长随机字符串'
```

## 数据接口

```text
GET  /api/realtime
GET  /api/status
GET  /api/history?sensor=smoke_raw&limit=100
GET  /api/alarms?limit=50
GET  /api/export?sensor=temperature&limit=1000
GET  /api/config
PUT  /api/config
POST /ingest/telemetry
```

扁平遥测字段：

```text
device, timestamp, temperature, humidity,
smoke_raw, smoke_alarm, door_open, person_detected,
facility_alarm, alarm_level
```

示例：

```bash
curl -X POST http://127.0.0.1:8080/ingest/telemetry \
  -H 'Content-Type: application/json' \
  -d '{"device":"power_facility_01","timestamp":1719849600,"temperature":24.5,"humidity":48,"smoke_raw":420,"smoke_alarm":false,"door_open":false,"person_detected":false,"facility_alarm":false,"alarm_level":0}'
```

MQTT 接入由 `scripts/mqtt_forward.sh` 完成：脚本使用 `mosquitto_sub` 读取 JSON，再 POST 到 `/ingest/telemetry`。Web 后端不直接访问板端设备节点或板端数据库。

## 认证与部署

前端登录仅用于演示页面入口。设置 `SENSOR_WEB_TOKEN` 后，后端会校验 `/api/*` 和 `/ingest/*` 的 Bearer token 或 `X-Device-Token`。公网部署还需要 HTTPS、可靠用户认证、CSRF 防护、限流、设备签名和防重放机制。

JSON 解析器针对固定遥测字段实现，适合内网联调。生产边界应替换为完整 JSON 库并限制请求体大小。
