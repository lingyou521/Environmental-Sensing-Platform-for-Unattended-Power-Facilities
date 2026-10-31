#ifndef SENSOR_MQ2_H
#define SENSOR_MQ2_H

#include "core/linkqueue.h"

int mq2_sensor_init(void **ctx);
int mq2_sensor_read(void *ctx, data_t *sample);
void mq2_sensor_deinit(void *ctx);

#endif
