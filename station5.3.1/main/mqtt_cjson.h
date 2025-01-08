#ifndef MQTT_CJSON_H
#define MQTT_CJSON_H

#include <stdio.h>

#include "cJSON.h"

typedef struct mqtt_cjson {
    cJSON *root;
    char* str;
}mqtt_cjson_t;
extern mqtt_cjson_t mqtt_cjson;

void mqtt_cjson_init(mqtt_cjson_t *mqtt_cjson);
// void mqtt_cjson_add(mqtt_cjson_t *mqtt_cjson, const char *key, const char *value);
// cJSON_PrintUnformatted

#endif