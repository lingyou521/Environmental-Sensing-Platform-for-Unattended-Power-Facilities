#ifndef PAYLOAD_PARSER_H
#define PAYLOAD_PARSER_H

#include <stdint.h>

typedef struct device_sample {
    char device[32];
    uint64_t timestamp;
    double temperature;
    double humidity;
    int smoke_raw;
    int smoke_alarm;
    int door_open;
    int person_detected;
    int facility_alarm;
    int alarm_level;
    int alcohol_raw;
    int alcohol_level;
    int alcohol_alarm;
    int accel_x;
    int accel_y;
    int accel_z;
    int motion_state;
    int motion_alarm;
    int flame_status;
    int flame_valid;
    int dht11_temperature;
    int dht11_humidity;
    int dht11_valid;
    double gps_lat;
    double gps_lon;
    float gps_speed;
    int gps_satellites;
    int gps_fence_alarm;
    char raw_payload[2048];
} device_sample_t;

int payload_parse_json(const char *payload, device_sample_t *sample);

#endif
