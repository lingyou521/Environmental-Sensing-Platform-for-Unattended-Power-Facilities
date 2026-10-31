#include "device/sensor_gpio.h"

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include "config/app_config.h"

typedef struct gpio_sensor_ctx {
    int fd;
    const char *path;
} gpio_sensor_ctx_t;

static int gpio_sensor_init(void **ctx, const char *path)
{
    gpio_sensor_ctx_t *sensor_ctx;

    if (ctx == NULL || path == NULL) {
        return -1;
    }
    sensor_ctx = calloc(1, sizeof(*sensor_ctx));
    if (sensor_ctx == NULL) {
        return -1;
    }
    sensor_ctx->fd = open(path, O_RDONLY);
    sensor_ctx->path = path;
    *ctx = sensor_ctx;
    return sensor_ctx->fd < 0 ? -1 : 0;
}

static int gpio_sensor_read(gpio_sensor_ctx_t *sensor_ctx, int *value)
{
    ssize_t bytes;

    if (sensor_ctx == NULL || value == NULL) {
        return -1;
    }
    if (sensor_ctx->fd < 0) {
        sensor_ctx->fd = open(sensor_ctx->path, O_RDONLY);
        if (sensor_ctx->fd < 0) {
            return -1;
        }
    }
    bytes = read(sensor_ctx->fd, value, sizeof(*value));
    if (bytes != (ssize_t)sizeof(*value)) {
        close(sensor_ctx->fd);
        sensor_ctx->fd = -1;
        return -1;
    }
    *value = *value ? 1 : 0;
    return 0;
}

static void gpio_sensor_deinit(void *ctx)
{
    gpio_sensor_ctx_t *sensor_ctx = ctx;

    if (sensor_ctx == NULL) {
        return;
    }
    if (sensor_ctx->fd >= 0) {
        close(sensor_ctx->fd);
    }
    free(sensor_ctx);
}

int door_sensor_init(void **ctx) { return gpio_sensor_init(ctx, DOOR_DEVICE_PATH); }
int door_sensor_read(void *ctx, data_t *sample)
{
    int value;
    if (sample == NULL || gpio_sensor_read(ctx, &value) != 0) return -1;
    sample->door_open = value;
    sample->door_valid = 1;
    return 0;
}
void door_sensor_deinit(void *ctx) { gpio_sensor_deinit(ctx); }

int pir_sensor_init(void **ctx) { return gpio_sensor_init(ctx, PIR_DEVICE_PATH); }
int pir_sensor_read(void *ctx, data_t *sample)
{
    int value;
    if (sample == NULL || gpio_sensor_read(ctx, &value) != 0) return -1;
    sample->person_detected = value;
    sample->person_valid = 1;
    return 0;
}
void pir_sensor_deinit(void *ctx) { gpio_sensor_deinit(ctx); }
