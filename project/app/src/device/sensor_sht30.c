#include "device/sensor_sht30.h"

#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "config/app_config.h"
#include "device/sensor_common.h"

typedef struct sht30_sample {
    uint16_t temperature_raw;
    uint16_t humidity_raw;
} sht30_sample_t;

typedef struct sht30_sensor_ctx {
    int fd;
} sht30_sensor_ctx_t;

static int sht30_reopen(sht30_sensor_ctx_t *sensor_ctx)
{
    sensor_ctx->fd = open(SHT30_DEVICE_PATH, O_RDONLY);
    return sensor_ctx->fd < 0 ? -1 : 0;
}

int sht30_sensor_init(void **ctx)
{
    sht30_sensor_ctx_t *sensor_ctx;

    if (ctx == NULL) {
        return -1;
    }
    sensor_ctx = calloc(1, sizeof(*sensor_ctx));
    if (sensor_ctx == NULL) {
        return -1;
    }
    sensor_ctx->fd = -1;
    *ctx = sensor_ctx;
    return sht30_reopen(sensor_ctx);
}

int sht30_sensor_read(void *ctx, data_t *sample)
{
    sht30_sensor_ctx_t *sensor_ctx = ctx;
    sht30_sample_t raw;
    ssize_t bytes;

    if (sensor_ctx == NULL || sample == NULL) {
        return -1;
    }
    if (sensor_ctx->fd < 0 && sht30_reopen(sensor_ctx) != 0) {
        return -1;
    }
    bytes = read(sensor_ctx->fd, &raw, sizeof(raw));
    if (bytes != (ssize_t)sizeof(raw)) {
        close(sensor_ctx->fd);
        sensor_ctx->fd = -1;
        return -1;
    }
    sample->temp = sht30_raw_to_celsius(raw.temperature_raw);
    sample->hum = sht30_raw_to_humidity(raw.humidity_raw);
    sample->temp_valid = 1;
    sample->hum_valid = 1;
    sample->temp_source = TEMP_SOURCE_SHT30;
    return 0;
}

void sht30_sensor_deinit(void *ctx)
{
    sht30_sensor_ctx_t *sensor_ctx = ctx;

    if (sensor_ctx == NULL) {
        return;
    }
    if (sensor_ctx->fd >= 0) {
        close(sensor_ctx->fd);
    }
    free(sensor_ctx);
}
