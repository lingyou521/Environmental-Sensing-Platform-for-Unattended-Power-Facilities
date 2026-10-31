#ifndef __APP_CONFIG_H__
#define __APP_CONFIG_H__

#define SHT30_DEVICE_PATH "/dev/sht30_misc"
#define MQ2_DEVICE_PATH "/dev/mq2_misc"
#define DOOR_DEVICE_PATH "/dev/door_misc"
#define PIR_DEVICE_PATH "/dev/pir_misc"
#define BEEP_DEVICE_PATH "/dev/beep_misc"
#define APP_LOG_PATH "./app.log"
#define SQLITE_DB_PATH "./sensor_history.db"
#define MQTT_BROKER "tcp://127.0.0.1:1883"
#define MQTT_DEVICE_NAME "power_facility_01"
#define MQTT_CLIENT_ID MQTT_DEVICE_NAME
#define MQTT_PRODUCT_ID "power-facility-demo"
/* Credentials are supplied at runtime through POWER_MQTT_PASSWORD. */
#define MQTT_PASSWORD_ENV "POWER_MQTT_PASSWORD"
#define MQTT_TEMP_KEY "temperature"
#define MQTT_HUMIDITY_KEY "humidity"
#define MQTT_SMOKE_KEY "smoke_raw"
#define MQTT_SMOKE_ALARM_KEY "smoke_alarm"
#define MQTT_DOOR_KEY "door_open"
#define MQTT_PERSON_KEY "person_detected"
#define MQTT_FACILITY_ALARM_KEY "facility_alarm"
#define MQTT_ENABLE_REPLY_SUBSCRIBE 0
#define MQTT_QOS 0
#define MQTT_TIMEOUT_MS 10000L

#define COLLECT_POLL_INTERVAL_SEC 1
#define MQTT_RETRY_INTERVAL_SEC 5
#define SQLITE_RETRY_INTERVAL_SEC 5

#define SMOKE_ALARM_THRESHOLD 1800
#define SMOKE_ALARM_ASSERT_COUNT 3
#define SMOKE_ALARM_CLEAR_COUNT 3
#define FACILITY_ALARM_ASSERT_COUNT 2
#define FACILITY_ALARM_CLEAR_COUNT 3

/* Compatibility names kept for standalone legacy examples. */
#define LM75_DEVICE_PATH SHT30_DEVICE_PATH
#define MQ3_DEVICE_PATH MQ2_DEVICE_PATH
#define ADXL345_DEVICE_PATH DOOR_DEVICE_PATH
#define DHT11_DEVICE_PATH PIR_DEVICE_PATH

#endif
