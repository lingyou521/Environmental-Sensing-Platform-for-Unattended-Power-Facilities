# 无人值守电力设施环境感知平台

> 基于 NXP i.MX6ULL 与嵌入式 Linux 的电力设施环境监测和分级告警终端

## 项目定位

平台面向变电箱、配电房和通信机房等无人值守场所，持续采集温湿度、烟雾、门禁和人员活动状态，在本地完成异常判断、声光告警、LCD 展示和 SQLite 记录，并通过 MQTT 将遥测上传到 Ubuntu Web 管理端。

板端使用 i.MX6ULL ARM Cortex-A7 运行 Linux。SHT30 通过 I2C 采集温湿度，MQ-2 通过 IIO ADC 采集烟雾模拟量，门磁和 PIR 通过 Platform GPIO 驱动接入。采集、告警、显示、存储和网络上传使用 pthread + mailbox 异步解耦。

## 功能链路

```text
SHT30(I2C)       MQ-2(IIO ADC)       门磁/PIR(GPIO)
       \             |                  /
        \            |                 /
         i.MX6ULL Linux 主控采集线程
              |             |
       分级告警判断       SQLite 本地记录
              |             |
       LVGL LCD + 蜂鸣器/LED
              |
        MQTT 遥测上报
              |
     Ubuntu Web 实时监控/历史/告警
```

### 采集与告警规则

| 模块 | 设备节点 | 采集内容 | 告警条件 |
| --- | --- | --- | --- |
| SHT30 | `/dev/sht30_misc` | 温度、相对湿度 | 数据失联记录故障 |
| MQ-2 | `/dev/mq2_misc` | ADC 原始值 | 连续 3 次超过烟雾阈值 |
| 门磁 | `/dev/door_misc` | 门开/关 | 无人值守时检测到开门 |
| PIR | `/dev/pir_misc` | 人体活动 | 检测到人员活动 |
| 声光输出 | `/dev/beep_misc` | 蜂鸣器控制 | 综合告警时联动 |

烟雾告警使用 `SMOKE_ALARM_THRESHOLD`，门磁和 PIR 事件进入统一的设施告警状态机。告警需要连续确认，恢复需要连续清除，避免单次抖动造成误报。

## 目录

```text
project/
├── app/
│   ├── src/core/       # pthread mailbox、链表队列、日志
│   ├── src/device/     # SHT30、MQ-2、门磁、PIR、蜂鸣器用户态封装
│   ├── src/service/    # 采集调度与告警状态机
│   ├── src/comm/       # MQTT 上报
│   ├── src/storage/    # SQLite 历史记录
│   ├── src/ui/         # LVGL 本地界面
│   ├── include/        # 数据模型、配置和第三方头文件
│   ├── drivers/        # Linux misc/platform/I2C/IIO 驱动
│   └── Makefile
├── web_project/        # Ubuntu C Web 服务和静态监控页面
├── 传感器驱动/          # 独立驱动练习和设备树资料
├── stm32-firmware/     # 保留的 STM32 外设实验代码，不参与本项目主流程
├── 日志/                # 开发记录与面试说明
└── skills/              # 驱动生成、源码契约检查工具
```

## 主控线程模型

```text
main
 ├── collect_thread  -> 读取四类传感器，执行告警状态机
 ├── lvgl_thread     -> framebuffer/触摸屏刷新本地页面
 ├── sqlite_thread   -> 写入 sensor_history.db
 └── mqtt_thread     -> MQTT 连接、重连、遥测发布
```

线程之间通过 `mailbox + linkqueue` 传递不可变采样快照，避免使用全局变量传递半更新状态。采样快照同时发给显示、存储和上传线程，任一线程短暂阻塞都不会改变其他线程的采集节奏。

## Linux 驱动接口

所有板端驱动使用 misc 设备提供简单的字符设备接口：

| 驱动 | 总线/框架 | 用户态 `read()` |
| --- | --- | --- |
| `sht30_drv` | I2C | 两个 `uint16_t` 原始值：温度、湿度 |
| `mq2_drv` | IIO ADC + Platform | 一个 `int` ADC 原始值 |
| `door_drv` | GPIO descriptor + Platform | 一个 `int`：0 关闭，1 打开 |
| `pir_drv` | GPIO descriptor + Platform | 一个 `int`：0 无人，1 检测到 |
| `beep_drv` | GPIO | `write(int)` 控制开关 |

设备树节点使用以下兼容字符串：

```dts
compatible = "power,sht30";
compatible = "power,mq2";
compatible = "power,door-sensor";
compatible = "power,pir-sensor";
```

## 构建

### i.MX6ULL 主应用和驱动

需要准备 `arm-linux-gnueabihf-gcc`、目标 Linux 内核源码、LVGL、SQLite3、Paho MQTT C 和 OpenSSL：

```bash
cd project
make all
make test
make clean
```

驱动的内核源码路径和 NFS 根文件系统路径可通过 `KERNEL_DIR`、`ROOTFS_DIR` 覆盖。主应用默认安装到 `~/nfs/rootfs`，设备端运行前需要创建 `/dev/sht30_misc`、`/dev/mq2_misc`、`/dev/door_misc`、`/dev/pir_misc` 和 `/dev/beep_misc`。

MQTT 密码不再写入源码，运行时设置：

```bash
export POWER_MQTT_PASSWORD='设备或平台提供的密码'
```

### Ubuntu Web 管理端

```bash
sudo apt install -y build-essential make libsqlite3-dev sqlite3 curl mosquitto-clients
cd web_project
make clean
make
make test
./bin/webserver -p 8080 -r ./www --db ./sensor_history.db --config ./web_config.json
```

Web 端默认监听 `127.0.0.1`。局域网联调时显式设置绑定地址和后端 Token：

```bash
export SENSOR_WEB_BIND=0.0.0.0
export SENSOR_WEB_TOKEN='换成一段长随机字符串'
```

访问 `http://服务器IP:8080/login.html`。登录页是演示门禁，真正的 API 和数据写入保护依赖后端 Token。

## Web 接口

```text
GET  /api/realtime
     返回最近一次设施快照：温湿度、烟雾、门磁、人员和综合告警。

GET  /api/history?sensor=temperature&limit=100
     查询 temperature、humidity、smoke_raw、door_open、person_detected、facility_alarm 等趋势。

GET  /api/alarms?limit=50
     查询烟雾、门磁和人员入侵事件。

GET  /api/export?sensor=smoke_raw&limit=1000
     导出历史 CSV。

GET/PUT /api/config
     读取或保存采集周期、烟雾阈值和入侵确认次数。

POST /ingest/telemetry
     接收扁平 JSON 遥测。
```

示例遥测：

```json
{
  "device": "power_facility_01",
  "timestamp": 1719849600,
  "temperature": 24.5,
  "humidity": 48.0,
  "smoke_raw": 420,
  "smoke_alarm": false,
  "door_open": false,
  "person_detected": false,
  "facility_alarm": false,
  "alarm_level": 0
}
```

MQTT 转发仍由 `scripts/mqtt_forward.sh` 配合 `mosquitto_sub + curl` 完成。Web 后端不直接读取板端设备节点或板端 SQLite。

## 验证范围

- `project/app`：传感器换算、日志和基础队列单元测试。
- `web_project`：JSON 解析、设施告警字段、实时缓存、SQLite payload/cache 和配置保存测试。
- `stm32-firmware`：保留 CAN 源码契约测试，但不属于电力设施平台主链路。

当前没有真实 i.MX6ULL、GPIO、I2C、ADC 到 MQTT 再到浏览器的硬件端到端测试；部署到目标板后应补充设备树、驱动 probe、断线重连和告警恢复验证。

## 安全边界

- MQTT 凭据必须通过环境变量或设备侧密钥注入，禁止提交到 Git。
- Web 公网部署前必须增加 HTTPS、可靠登录、CSRF 防护、限流和请求签名。
- `/ingest/telemetry` 只接受受信任转发源；生产环境应增加设备身份和防重放校验。
- 轻量 JSON 解析器适合固定字段联调，不应直接作为公网不可信 JSON 边界。

## 许可证

MIT License
