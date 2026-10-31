#include "device/sensor_mq2.h"

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include "config/app_config.h"

typedef struct mq2_sensor_ctx {
    int fd;
} mq2_sensor_ctx_t;

static int mq2_reopen(mq2_sensor_ctx_t *sensor_ctx)
{
    sensor_ctx->fd = open(MQ2_DEVICE_PATH, O_RDONLY);
    return sensor_ctx->fd < 0 ? -1 : 0;
}

int mq2_sensor_init(void **ctx)
{
    mq2_sensor_ctx_t *sensor_ctx;

    if (ctx == NULL) {
        return -1;
    }
    sensor_ctx = calloc(1, sizeof(*sensor_ctx));
    if (sensor_ctx == NULL) {
        return -1;
    }
    sensor_ctx->fd = -1;
    *ctx = sensor_ctx;
    return mq2_reopen(sensor_ctx);
}

int mq2_sensor_read(void *ctx, data_t *sample)
{
    mq2_sensor_ctx_t *sensor_ctx = ctx;
    ssize_t bytes;

    if (sensor_ctx == NULL || sample == NULL) {
        return -1;
    }
    if (sensor_ctx->fd < 0 && mq2_reopen(sensor_ctx) != 0) {
        return -1;
    }
    bytes = read(sensor_ctx->fd, &sample->smoke_raw, sizeof(sample->smoke_raw));
    if (bytes != (ssize_t)sizeof(sample->smoke_raw)) {
        close(sensor_ctx->fd);
        sensor_ctx->fd = -1;
        return -1;
    }
    sample->smoke_valid = 1;
    return 0;
}

void mq2_sensor_deinit(void *ctx)
{
    mq2_sensor_ctx_t *sensor_ctx = ctx;

    if (sensor_ctx == NULL) {
        return;
    }
    if (sensor_ctx->fd >= 0) {
        close(sensor_ctx->fd);
    }
    free(sensor_ctx);
}
