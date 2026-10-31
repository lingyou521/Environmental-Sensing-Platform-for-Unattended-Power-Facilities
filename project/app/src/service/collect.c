#include "service/collect.h"

#include <string.h>
#include <time.h>
#include <unistd.h>

#include "config/app_config.h"
#include "core/log.h"
#include "core/mailbox.h"
#include "device/beep_control.h"
#include "device/sensor_gpio.h"
#include "device/sensor_mq2.h"
#include "device/sensor_sht30.h"

static void set_sensor_defaults(data_t *sample)
{
    sample->temp_valid = 0;
    sample->hum_valid = 0;
    sample->smoke_valid = 0;
    sample->door_valid = 0;
    sample->person_valid = 0;
    sample->smoke_alarm = 0;
    sample->facility_alarm = 0;
    sample->alarm_level = 0;
}

static void collect_sensors(void *sht30_ctx, void *mq2_ctx,
                            void *door_ctx, void *pir_ctx, data_t *sample)
{
    if (sht30_sensor_read(sht30_ctx, sample) != 0) {
        LOG_WARN("sht30 read failed");
    }
    if (mq2_sensor_read(mq2_ctx, sample) != 0) {
        LOG_WARN("mq2 read failed");
    }
    if (door_sensor_read(door_ctx, sample) != 0) {
        LOG_WARN("door sensor read failed");
    }
    if (pir_sensor_read(pir_ctx, sample) != 0) {
        LOG_WARN("pir sensor read failed");
    }
}

static void update_alarms(data_t *sample, int *facility_alarm,
                          int *assert_count, int *clear_count)
{
    int current_alarm = 0;

    if ((sample->smoke_valid && sample->smoke_raw >= SMOKE_ALARM_THRESHOLD) ||
        (sample->door_valid && sample->door_open) ||
        (sample->person_valid && sample->person_detected)) {
        current_alarm = 1;
    }

    if (current_alarm) {
        if (*assert_count < FACILITY_ALARM_ASSERT_COUNT) {
            ++(*assert_count);
        }
        *clear_count = 0;
    } else {
        if (*clear_count < FACILITY_ALARM_CLEAR_COUNT) {
            ++(*clear_count);
        }
        *assert_count = 0;
    }

    if (!*facility_alarm && *assert_count >= FACILITY_ALARM_ASSERT_COUNT) {
        *facility_alarm = 1;
        LOG_WARN("facility alarm asserted: smoke=%d door=%d person=%d",
                 sample->smoke_raw, sample->door_open, sample->person_detected);
    } else if (*facility_alarm && *clear_count >= FACILITY_ALARM_CLEAR_COUNT) {
        *facility_alarm = 0;
        LOG_INFO("facility alarm cleared");
    }

    sample->smoke_alarm = sample->smoke_valid &&
                          sample->smoke_raw >= SMOKE_ALARM_THRESHOLD;
    sample->facility_alarm = *facility_alarm;
    sample->alarm_level = sample->smoke_alarm ? 2 :
                          ((sample->door_open || sample->person_detected) ? 1 : 0);
}

static void update_legacy_aliases(data_t *sample)
{
    /* Keep old queue consumers source-compatible while exposing canonical fields. */
    sample->alcohol_raw = sample->smoke_raw;
    sample->alcohol_valid = sample->smoke_valid;
    sample->alcohol_alarm = sample->smoke_alarm;
    sample->motion_state = sample->door_open ? 1 :
                           (sample->person_detected ? 2 : 0);
    sample->motion_alarm = sample->facility_alarm;
}

void *collect_thread(void *arg)
{
    void *sht30_ctx = NULL;
    void *mq2_ctx = NULL;
    void *door_ctx = NULL;
    void *pir_ctx = NULL;
    void *beep_ctx = NULL;
    data_t sample;
    int facility_alarm = 0;
    int assert_count = 0;
    int clear_count = 0;

    (void)arg;
    (void)sht30_sensor_init(&sht30_ctx);
    (void)mq2_sensor_init(&mq2_ctx);
    (void)door_sensor_init(&door_ctx);
    (void)pir_sensor_init(&pir_ctx);
    (void)beep_control_init(&beep_ctx);

    LOG_INFO("power facility collector started: SHT30/MQ-2/door/PIR");
    while (1) {
        memset(&sample, 0, sizeof(sample));
        sample.sample_time = time(NULL);
        set_sensor_defaults(&sample);
        collect_sensors(sht30_ctx, mq2_ctx, door_ctx, pir_ctx, &sample);
        update_alarms(&sample, &facility_alarm, &assert_count, &clear_count);
        update_legacy_aliases(&sample);
        (void)beep_control_set(beep_ctx, facility_alarm ? BEEP_ON : BEEP_OFF);
        (void)mailbox_send_msg("lvgl_thread", sample);
        (void)mailbox_send_msg("mqtt_thread", sample);
        (void)mailbox_send_msg("sqlite_thread", sample);
        sleep(COLLECT_POLL_INTERVAL_SEC);
    }

    sht30_sensor_deinit(sht30_ctx);
    mq2_sensor_deinit(mq2_ctx);
    door_sensor_deinit(door_ctx);
    pir_sensor_deinit(pir_ctx);
    beep_control_deinit(beep_ctx);
    return NULL;
}
