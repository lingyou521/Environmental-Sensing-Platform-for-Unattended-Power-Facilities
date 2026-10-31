#ifndef SENSOR_GPIO_H
#define SENSOR_GPIO_H

#include "core/linkqueue.h"

int door_sensor_init(void **ctx);
int door_sensor_read(void *ctx, data_t *sample);
void door_sensor_deinit(void *ctx);
int pir_sensor_init(void **ctx);
int pir_sensor_read(void *ctx, data_t *sample);
void pir_sensor_deinit(void *ctx);

#endif
