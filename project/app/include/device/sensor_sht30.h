#ifndef SENSOR_SHT30_H
#define SENSOR_SHT30_H

#include "core/linkqueue.h"

int sht30_sensor_init(void **ctx);
int sht30_sensor_read(void *ctx, data_t *sample);
void sht30_sensor_deinit(void *ctx);

#endif
