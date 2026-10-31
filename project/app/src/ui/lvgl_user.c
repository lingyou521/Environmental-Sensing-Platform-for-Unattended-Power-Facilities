#include "lvgl/lvgl.h"
#include "lv_drivers/display/fbdev.h"
#include "lv_drivers/indev/evdev.h"
#include "ui.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>

#include "core/mailbox.h"

#define DISP_BUF_SIZE (128 * 1024)

static void set_status_color(lv_obj_t *object, int alarm)
{
    lv_obj_set_style_text_color(object,
                                alarm ? lv_palette_main(LV_PALETTE_RED) :
                                        lv_palette_main(LV_PALETTE_GREEN),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
}

void my_timer(lv_timer_t *timer)
{
    data_t sample;
    char text[96];

    (void)timer;
    if (mailbox_recv_msg(&sample) <= 0) {
        return;
    }

    snprintf(text, sizeof(text), sample.temp_valid ? "%.2f C" : "N/A", sample.temp);
    lv_label_set_text(ui_TempLabel, text);
    snprintf(text, sizeof(text), sample.hum_valid ? "%.2f %%" : "N/A", sample.hum);
    lv_label_set_text(ui_HumLabel, text);

    snprintf(text, sizeof(text), sample.smoke_valid ? "%d" : "N/A", sample.smoke_raw);
    lv_label_set_text(ui_alcLabel, text);
    snprintf(text, sizeof(text), sample.door_valid ? (sample.door_open ? "OPEN" : "CLOSED") : "N/A");
    lv_label_set_text(ui_acceLabelx, text);
    snprintf(text, sizeof(text), sample.person_valid ? (sample.person_detected ? "DETECTED" : "CLEAR") : "N/A");
    lv_label_set_text(ui_acceLabely, text);
    snprintf(text, sizeof(text), sample.facility_alarm ? "ALARM" : "NORMAL");
    lv_label_set_text(ui_acceLabelz, text);
    lv_label_set_text(ui_stateLabel, sample.alarm_level == 2 ? "SMOKE" :
                                      (sample.alarm_level == 1 ? "INTRUSION" : "NORMAL"));
    lv_label_set_text(ui_alarmLabel, sample.facility_alarm ? "ON" : "OFF");
    set_status_color(ui_stateLabel, sample.facility_alarm);
    set_status_color(ui_alarmLabel, sample.facility_alarm);
}

void *lvgl_thread(void *arg)
{
    (void)arg;
    lv_init();
    fbdev_init();

    static lv_color_t buffer[DISP_BUF_SIZE];
    static lv_disp_draw_buf_t draw_buf;
    lv_disp_draw_buf_init(&draw_buf, buffer, NULL, DISP_BUF_SIZE);

    static lv_disp_drv_t display_driver;
    lv_disp_drv_init(&display_driver);
    display_driver.draw_buf = &draw_buf;
    display_driver.flush_cb = fbdev_flush;
    display_driver.hor_res = 800;
    display_driver.ver_res = 480;
    lv_disp_drv_register(&display_driver);

    evdev_init();
    static lv_indev_drv_t input_driver;
    lv_indev_drv_init(&input_driver);
    input_driver.type = LV_INDEV_TYPE_POINTER;
    input_driver.read_cb = evdev_read;
    lv_indev_drv_register(&input_driver);

    ui_init();
    lv_timer_create(my_timer, 1000, NULL);
    while (1) {
        lv_timer_handler();
        usleep(5000);
    }
    return NULL;
}

uint32_t custom_tick_get(void)
{
    static uint64_t start_ms;
    struct timeval now;
    uint64_t now_ms;

    gettimeofday(&now, NULL);
    now_ms = (uint64_t)now.tv_sec * 1000ULL + (uint64_t)now.tv_usec / 1000ULL;
    if (start_ms == 0) {
        start_ms = now_ms;
    }
    return (uint32_t)(now_ms - start_ms);
}
