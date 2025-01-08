#include "mqtt_cjson.h"
#include <stdint.h>
#include <esp_log.h>
static uint32_t id_index=0;

void mqtt_cjson_init(mqtt_cjson_t *mqtt_cjson)
{
    if(mqtt_cjson->root == NULL){
        ESP_LOGI("mqtt_cjsoN","mqtt_cjson is NULL");
        mqtt_cjson->root=cJSON_CreateObject();
    }
    char id[15];
    snprintf(id,sizeof(id),"%ld",id_index);
    cJSON_AddStringToObject(mqtt_cjson->root, "id", id);
    cJSON_AddStringToObject(mqtt_cjson->root, "version", "1.0");
    cJSON_AddObjectToObject(mqtt_cjson->root, "params");
    cJSON* param=cJSON_GetObjectItem(mqtt_cjson->root,"params");
    cJSON_AddNumberToObject(param,"temperature",0);
    cJSON_AddNumberToObject(param,"hum",0);
    id_index++;
    cJSON_AddStringToObject(mqtt_cjson->root, "method", "thing.event.property.post");

}



