#include "ingest/realtime_cache.h"

#include <stdio.h>
#include <string.h>

static void json_escape_string(const char *input, char *output, size_t output_size)
{
    size_t len = 0;
    if (output == NULL || output_size == 0) return;
    if (input == NULL) input = "";
    for (; *input != '\0' && len + 1 < output_size; input++) {
        const char *escaped = NULL;
        switch ((unsigned char)*input) {
        case '"': escaped = "\\\""; break;
        case '\\': escaped = "\\\\"; break;
        case '\n': escaped = "\\n"; break;
        case '\r': escaped = "\\r"; break;
        default: break;
        }
        if (escaped != NULL) {
            size_t escaped_len = strlen(escaped);
            if (len + escaped_len >= output_size) break;
            memcpy(output + len, escaped, escaped_len);
            len += escaped_len;
        } else if ((unsigned char)*input >= 0x20) {
            output[len++] = *input;
        }
    }
    output[len] = '\0';
}

int realtime_cache_init(realtime_cache_t *cache)
{
    if (cache == NULL) return -1;
    memset(cache, 0, sizeof(*cache));
    return pthread_mutex_init(&cache->lock, NULL);
}

void realtime_cache_destroy(realtime_cache_t *cache)
{
    if (cache != NULL) pthread_mutex_destroy(&cache->lock);
}

void realtime_cache_update(realtime_cache_t *cache, const device_sample_t *sample)
{
    if (cache == NULL || sample == NULL) return;
    pthread_mutex_lock(&cache->lock);
    cache->sample = *sample;
    cache->has_sample = 1;
    pthread_mutex_unlock(&cache->lock);
}

int realtime_cache_copy(realtime_cache_t *cache, device_sample_t *out)
{
    if (cache == NULL || out == NULL) return -1;
    pthread_mutex_lock(&cache->lock);
    if (!cache->has_sample) {
        pthread_mutex_unlock(&cache->lock);
        return -1;
    }
    *out = cache->sample;
    pthread_mutex_unlock(&cache->lock);
    return 0;
}

int realtime_cache_to_json(realtime_cache_t *cache, char *buffer, size_t buffer_size)
{
    device_sample_t s;
    char device[256];

    if (buffer == NULL || buffer_size == 0) return -1;
    if (realtime_cache_copy(cache, &s) != 0) {
        snprintf(buffer, buffer_size, "{\"online\":false,\"message\":\"no telemetry received\"}");
        return 0;
    }
    json_escape_string(s.device, device, sizeof(device));
    snprintf(buffer, buffer_size,
             "{\"timestamp\":%llu,\"device\":\"%s\",\"online\":true,"
             "\"facility\":{\"temperature\":%.2f,\"humidity\":%.2f,"
             "\"smoke_raw\":%d,\"smoke_alarm\":%s,\"door_open\":%s,"
             "\"person_detected\":%s,\"facility_alarm\":%s,\"alarm_level\":%d}}",
             (unsigned long long)s.timestamp, device, s.temperature, s.humidity,
             s.smoke_raw, s.smoke_alarm ? "true" : "false",
             s.door_open ? "true" : "false",
             s.person_detected ? "true" : "false",
             s.facility_alarm ? "true" : "false", s.alarm_level);
    return 0;
}
