/* WiFi station Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "aiot_state_api.h"
#include "aiot_sysdep_api.h"
#include "aiot_mqtt_api.h"
#include "mqtt_cjson.h"

#include "i2c.h"
#include "i2c_gxhtc3.h"

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "esp_log.h"

#include "i2c.h"
#include "i2c_gxhtc3.h"

#include "spi.h"
#include "esp_spi_lcd.h"
#include "lvgl.h"

#include "esp_lvgl_port.h"
#include "esp_lvgl_port_init.h"
#include "esp_lcd_touch_ft5x06.h"
#include <string.h>
#include "gui_guider.h"
 #include "custom.h"
lv_ui guider_ui;
/* The examples use WiFi configuration that you can set via project configuration menu

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_ESP_WIFI_SSID      "1133"
#define EXAMPLE_ESP_WIFI_PASS      "ltf12345678"
#define EXAMPLE_ESP_MAXIMUM_RETRY  10

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;
mqtt_cjson_t mqtt_cjson={

    .root=NULL,
    .str=NULL,
};

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *TAG = "wifi station";

static int s_retry_num = 0;

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {

            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler));
    // ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler));
    vEventGroupDelete(s_wifi_event_group);
}
i2c_gxhtc3_handle_t gxhtc3_handle;
float temperature=0;
float humidity=0;
int light=0;

/*
 * 这个例程适用于`Linux`这类支持pthread的POSIX设备, 它演示了用SDK配置MQTT参数并建立连接, 之后创建2个线程
 *
 * + 一个线程用于保活长连接
 * + 一个线程用于接收消息, 并在有消息到达时进入默认的数据回调, 在连接状态变化时进入事件回调
 *
 * 需要用户关注或修改的部分, 已经用 TODO 在注释中标明
 *
 */

/* TODO: 替换为自己设备的三元组 */
char *product_key       = "k22e0WCtkgb";
char *device_name       = "esp32";
char *device_secret     = "82e2705b2a5b3bf37daa3c48bd786769";

/* 位于portfiles/aiot_port文件夹下的系统适配函数集合 */
extern aiot_sysdep_portfile_t g_aiot_sysdep_portfile;

/* 位于external/ali_ca_cert.c中的服务器证书 */
extern const char *ali_ca_cert;

static pthread_t g_mqtt_process_thread;
static pthread_t g_mqtt_recv_thread;
static uint8_t g_mqtt_process_thread_running = 0;
static uint8_t g_mqtt_recv_thread_running = 0;

/* TODO: 如果要关闭日志, 就把这个函数实现为空, 如果要减少日志, 可根据code选择不打印
 *
 * 例如: [1577589489.033][LK-0317] mqtt_basic_demo&a13FN5TplKq
 *
 * 上面这条日志的code就是0317(十六进制), code值的定义见core/aiot_state_api.h
 *
 */

/* 日志回调函数, SDK的日志会从这里输出 */
int32_t demo_state_logcb(int32_t code, char *message)
{
    printf("%s", message);
    return 0;
}

/* MQTT事件回调函数, 当网络连接/重连/断开时被触发, 事件定义见core/aiot_mqtt_api.h */
void demo_mqtt_event_handler(void *handle, const aiot_mqtt_event_t *event, void *userdata)
{
    switch (event->type) {
        /* SDK因为用户调用了aiot_mqtt_connect()接口, 与mqtt服务器建立连接已成功 */
        case AIOT_MQTTEVT_CONNECT: {
            printf("AIOT_MQTTEVT_CONNECT\n");
            /* TODO: 处理SDK建连成功, 不可以在这里调用耗时较长的阻塞函数 */
        }
        break;

        /* SDK因为网络状况被动断连后, 自动发起重连已成功 */
        case AIOT_MQTTEVT_RECONNECT: {
            printf("AIOT_MQTTEVT_RECONNECT\n");
            /* TODO: 处理SDK重连成功, 不可以在这里调用耗时较长的阻塞函数 */
        }
        break;

        /* SDK因为网络的状况而被动断开了连接, network是底层读写失败, heartbeat是没有按预期得到服务端心跳应答 */
        case AIOT_MQTTEVT_DISCONNECT: {
            char *cause = (event->data.disconnect == AIOT_MQTTDISCONNEVT_NETWORK_DISCONNECT) ? ("network disconnect") :
                          ("heartbeat disconnect");
            printf("AIOT_MQTTEVT_DISCONNECT: %s\n", cause);
            /* TODO: 处理SDK被动断连, 不可以在这里调用耗时较长的阻塞函数 */
        }
        break;

        default: {

        }
    }
}

/* MQTT默认消息处理回调, 当SDK从服务器收到MQTT消息时, 且无对应用户回调处理时被调用 */
void demo_mqtt_default_recv_handler(void *handle, const aiot_mqtt_recv_t *packet, void *userdata)
{
    switch (packet->type) {
        case AIOT_MQTTRECV_HEARTBEAT_RESPONSE: {
            printf("heartbeat response\n");
            /* TODO: 处理服务器对心跳的回应, 一般不处理 */
        }
        break;

        case AIOT_MQTTRECV_SUB_ACK: {
            printf("suback, res: -0x%04lX, packet id: %d, max qos: %d\n",
                   -packet->data.sub_ack.res, packet->data.sub_ack.packet_id, packet->data.sub_ack.max_qos);
            /* TODO: 处理服务器对订阅请求的回应, 一般不处理 */
        }
        break;

        case AIOT_MQTTRECV_PUB: {
            printf("pub, qos: %d, topic: %.*s\n", packet->data.pub.qos, packet->data.pub.topic_len, packet->data.pub.topic);
            printf("pub, payload: %.*s\n", (int16_t)packet->data.pub.payload_len, packet->data.pub.payload);
            /* TODO: 处理服务器下发的业务报文 */
            if(strstr((char*)packet->data.pub.topic,"/property/set")){
                if(strstr((char*)packet->data.pub.payload,"{\"LEDSwitch\":1}")){
                    lv_custom_set_light(1);
                    // lv_led_on(guider_ui.screen_1_led_1);
                    ESP_LOGI(TAG, "light is on");
                }
                else{
                    lv_custom_set_light(0);
                    // lv_led_off(guider_ui.screen_1_led_1);
                    ESP_LOGI(TAG, "light is off");
                }
                // cJSON *root = cJSON_Parse((char*)packet->data.pub.payload);
                // if (root) {
                //     ESP_LOGW("AIOT_MQTTRECV_PUB","JSON 解析失败");  
                //     const char *error_ptr = cJSON_GetErrorPtr();
                //     if (error_ptr) {
                //         ESP_LOGW("AIOT_MQTTRECV_PUB","错误发生在位置: %s", error_ptr);
                //     }
                // }
                // else{
                //     // 获取 params 对象
                //     cJSON *params = cJSON_GetObjectItemCaseSensitive(root, "params");
                //     if (!cJSON_IsObject(params)) {
                //         ESP_LOGW("AIOT_MQTTRECV_PUB","params 项目不存在或格式不正确");
                //         cJSON_Delete(root);
                //     }
                //     else{
                //         // 获取 lightswitch 项目的值
                //         cJSON *lightswitch = cJSON_GetObjectItemCaseSensitive(params, "LEDSwitch");
                //         if (cJSON_IsBool(lightswitch)) {
                //         if( cJSON_IsTrue(lightswitch)) {
                //                 lv_custom_set_light(1);
                //                 ESP_LOGI(TAG, "light is on");
                //         }
                //         else {
                //                 lv_custom_set_light(0);
                //                 ESP_LOGI(TAG, "light is off");
                //         }
                //         } 
                //     }
                // }
                // cJSON_Delete(root);
            }
            else ESP_LOGI("AIOT_MQTTRECV_PUB","nO property.set");
        break;
        }
        case AIOT_MQTTRECV_PUB_ACK: {
            printf("puback, packet id: %d\n", packet->data.pub_ack.packet_id);
            /* TODO: 处理服务器对QoS1上报消息的回应, 一般不处理 */
        }
        break;

        default: {

        }
    }
}

/* 执行aiot_mqtt_process的线程, 包含心跳发送和QoS1消息重发 */
void *demo_mqtt_process_thread(void *args)
{
    int32_t res = STATE_SUCCESS;

    while (g_mqtt_process_thread_running) {
        res = aiot_mqtt_process(args);
            {
        // ESP_LOGI(TAG, "hhhhhhhhhhhhhhhhh");
        ESP_ERROR_CHECK(gxhtc3_get_tah(gxhtc3_handle));
        printf("gxhtc3 temp: %f    gxhtc3 humi:%f    \n",gxhtc3_handle->temp,gxhtc3_handle->humi);
        temperature=gxhtc3_handle->temp;
        humidity=gxhtc3_handle->humi;
        lv_custom_set_hum(humidity);
        lv_custom_set_temp(temperature);
        char *pub_topic = "/sys/k22e0WCtkgb/esp32/thing/event/property/post";
        cJSON*param =cJSON_GetObjectItem(mqtt_cjson.root,"params");
        cJSON_ReplaceItemInObject(param,"temperature",cJSON_CreateNumber(temperature));
        cJSON_ReplaceItemInObject(param,"hum", cJSON_CreateNumber(humidity));
        temperature+=0.1;
        humidity+=0.1;
        char*pub_payload = cJSON_PrintUnformatted(mqtt_cjson.root);
        // ESP_LOGI(TAG, "%s", pub_payload);
        res = aiot_mqtt_pub(args, pub_topic, (uint8_t *)pub_payload, strlen(pub_payload), 0);
        if (res < 0) {
            printf("aiot_mqtt_sub failed, res: -0x%04lX\n", -res);
    
        }
        // ESP_LOGI(TAG, "hhhhhhhhhhhhhhhhh");
        cJSON_free(pub_payload);
        // ESP_LOGI(TAG, "xxxxxxxxxxx");
        }   
        if (res == STATE_USER_INPUT_EXEC_DISABLED) {
            break;
        }
        sleep(10);
    }
    return NULL;
}

/* 执行aiot_mqtt_recv的线程, 包含网络自动重连和从服务器收取MQTT消息 */
void *demo_mqtt_recv_thread(void *args)
{
    int32_t res = STATE_SUCCESS;

    while (g_mqtt_recv_thread_running) {
        res = aiot_mqtt_recv(args);
        if (res < STATE_SUCCESS) {
            if (res == STATE_USER_INPUT_EXEC_DISABLED) {
                break;
            }
            sleep(1);
        }
    }
    return NULL;
}

int linkkit_main(void)
{
    int32_t     res = STATE_SUCCESS;
    void       *mqtt_handle = NULL;
    char       *url = "iot-as-mqtt.cn-shanghai.aliyuncs.com"; /* 阿里云平台上海站点的域名后缀 */
    char        host[100] = {0}; /* 用这个数组拼接设备连接的云平台站点全地址, 规则是 ${productKey}.iot-as-mqtt.cn-shanghai.aliyuncs.com */
    uint16_t    port = 1883;      /* 无论设备是否使用TLS连接阿里云平台, 目的端口都是443 */
    aiot_sysdep_network_cred_t cred; /* 安全凭据结构体, 如果要用TLS, 这个结构体中配置CA证书等参数 */

    /* 配置SDK的底层依赖 */
    aiot_sysdep_set_portfile(&g_aiot_sysdep_portfile);
    /* 配置SDK的日志输出 */
    aiot_state_set_logcb(demo_state_logcb);

    /* 创建SDK的安全凭据, 用于建立TLS连接 */
    memset(&cred, 0, sizeof(aiot_sysdep_network_cred_t));
    cred.option = AIOT_SYSDEP_NETWORK_CRED_SVRCERT_CA;  /* 使用RSA证书校验MQTT服务端 */
    cred.max_tls_fragment = 16384; /* 最大的分片长度为16K, 其它可选值还有4K, 2K, 1K, 0.5K */
    cred.sni_enabled = 1;                               /* TLS建连时, 支持Server Name Indicator */
    cred.x509_server_cert = ali_ca_cert;                 /* 用来验证MQTT服务端的RSA根证书 */
    cred.x509_server_cert_len = strlen(ali_ca_cert);     /* 用来验证MQTT服务端的RSA根证书长度 */

    /* 创建1个MQTT客户端实例并内部初始化默认参数 */
    mqtt_handle = aiot_mqtt_init();
    if (mqtt_handle == NULL) {
        printf("aiot_mqtt_init failed\n");
        return -1;
    }

    /* TODO: 如果以下代码不被注释, 则例程会用TCP而不是TLS连接云平台 */
    /*
    {
        memset(&cred, 0, sizeof(aiot_sysdep_network_cred_t));
        cred.option = AIOT_SYSDEP_NETWORK_CRED_NONE;
    }
    */

    snprintf(host, 100, "%s.%s", product_key, url);
    /* 配置MQTT服务器地址 */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_HOST, (void *)host);
    /* 配置MQTT服务器端口 */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_PORT, (void *)&port);
    /* 配置设备productKey */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_PRODUCT_KEY, (void *)product_key);
    /* 配置设备deviceName */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_DEVICE_NAME, (void *)device_name);
    /* 配置设备deviceSecret */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_DEVICE_SECRET, (void *)device_secret);
    /* 配置网络连接的安全凭据, 上面已经创建好了 */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_NETWORK_CRED, (void *)&cred);
    /* 配置MQTT默认消息接收回调函数 */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_RECV_HANDLER, (void *)demo_mqtt_default_recv_handler);
    /* 配置MQTT事件回调函数 */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_EVENT_HANDLER, (void *)demo_mqtt_event_handler);

    /* 与服务器建立MQTT连接 */
    res = aiot_mqtt_connect(mqtt_handle);
    if (res < STATE_SUCCESS) {
        /* 尝试建立连接失败, 销毁MQTT实例, 回收资源 */
        aiot_mqtt_deinit(&mqtt_handle);
        printf("aiot_mqtt_connect failed: -0x%04lX\n", -res);
        return -1;
    }

    /* MQTT 订阅topic功能示例, 请根据自己的业务需求进行使用 */
    {
        char *sub_topic = "/sys/k22e0WCtkgb/esp32/thing/service/property/set";

        res = aiot_mqtt_sub(mqtt_handle, sub_topic, NULL, 1, NULL);
        if (res < 0) {
            printf("aiot_mqtt_sub failed, res: -0x%04lX\n", -res);
            return -1;
        }
    }

    /* MQTT 发布消息功能示例, 请根据自己的业务需求进行使用 */
    // {
    //     char *pub_topic = "/sys/k22e0WCtkgb/esp32/thing/event/property/post";
    //     char *pub_payload = "{\"id\":\"1\",\"version\":\"1.0\",\"params\":{\"LightSwitch\":0}}";

    //     res = aiot_mqtt_pub(mqtt_handle, pub_topic, (uint8_t *)pub_payload, strlen(pub_payload), 0);
    //     if (res < 0) {
    //         printf("aiot_mqtt_sub failed, res: -0x%04lX\n", -res);
    //         return -1;
    //     }
    // }


    /* 创建一个单独的线程, 专用于执行aiot_mqtt_process, 它会自动发送心跳保活, 以及重发QoS1的未应答报文 */
    g_mqtt_process_thread_running = 1;
    res = pthread_create(&g_mqtt_process_thread, NULL, demo_mqtt_process_thread, mqtt_handle);
    if (res < 0) {
        printf("pthread_create demo_mqtt_process_thread failed: %ld\n", res);
        return -1;
    }

    /* 创建一个单独的线程用于执行aiot_mqtt_recv, 它会循环收取服务器下发的MQTT消息, 并在断线时自动重连 */
    g_mqtt_recv_thread_running = 1;
    res = pthread_create(&g_mqtt_recv_thread, NULL, demo_mqtt_recv_thread, mqtt_handle);
    if (res < 0) {
        printf("pthread_create demo_mqtt_recv_thread failed: %ld\n", res);
        return -1;
    }

    /* 主循环进入休眠 */
    while (1) {
        sleep(1);
    }

    /* 断开MQTT连接, 一般不会运行到这里 */
    res = aiot_mqtt_disconnect(mqtt_handle);
    if (res < STATE_SUCCESS) {
        aiot_mqtt_deinit(&mqtt_handle);
        printf("aiot_mqtt_disconnect failed: -0x%04lX\n", -res);
        return -1;
    }

    /* 销毁MQTT实例, 一般不会运行到这里 */
    res = aiot_mqtt_deinit(&mqtt_handle);
    if (res < STATE_SUCCESS) {
        printf("aiot_mqtt_deinit failed: -0x%04lX\n", -res);
        return -1;
    }

    g_mqtt_process_thread_running = 0;
    g_mqtt_recv_thread_running = 0;
    pthread_join(g_mqtt_process_thread, NULL);
    pthread_join(g_mqtt_recv_thread, NULL);

    return 0;
}

void app_main(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    i2c_master_bus_handle_t i2c_master_handle;
    ESP_ERROR_CHECK(i2c_master_init(&i2c_master_handle));
    
    ESP_ERROR_CHECK(gxhtc3_i2c_init(i2c_master_handle,&gxhtc3_handle));

    ESP_ERROR_CHECK(gxhtc3_read_id(gxhtc3_handle));
    printf("gxhtc3 id: %u\n",gxhtc3_handle->id);


    static const char *TAG = "LCD";
    ESP_LOGI(TAG,"spi bus master init");
    ESP_ERROR_CHECK(spi_bus_master_init());

    ESP_LOGI(TAG,"esp lcd init io");
    esp_lcd_panel_io_handle_t io_handle;
    ESP_ERROR_CHECK(esp_spi_lcd_init_io(&io_handle));

    ESP_LOGI(TAG,"esp lcd init panel");
    esp_lcd_panel_handle_t panel_handle;
    ESP_ERROR_CHECK(esp_spi_lcd_panel_init(&panel_handle,io_handle));

    /* 复位LCD面板 */
    ESP_LOGI(TAG,"esp lcd reset");
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));

    /* 初始化LCD面板 */
    ESP_LOGI(TAG,"esp lcd init");
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

    /* 打开LCD显示 */
    ESP_LOGI(TAG,"esp lcd display on");
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
    
    /* 反转LCD颜色 */
    ESP_LOGI(TAG,"esp lcd invert color");
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));

    /* 初始化LCD背光 */
    ESP_LOGI(TAG,"esp lcd backlight init");
    ESP_ERROR_CHECK(esp_lcd_backlight_init());

    /* 打开LCD背光 */
    ESP_LOGI(TAG,"esp lcd backlight on");
    esp_lcd_backlight_on();

    ESP_LOGI(TAG,"esp lcd lvgl port init");

    lv_disp_t *disp_handle;
    ESP_ERROR_CHECK(u_esp_lvgl_port_init(io_handle,panel_handle,&disp_handle));

    lv_disp_set_rotation(disp_handle, LV_DISP_ROT_90);

    /* 配置触摸屏参数 */
    esp_lcd_touch_config_t tp_cfg = {
        .x_max = 320,                    /* X轴最大分辨率 */
        .y_max = 240,                    /* Y轴最大分辨率 */
        .rst_gpio_num = -1,              /* 复位引脚,不使用 */
        .int_gpio_num = -1,              /* 中断引脚,不使用 */
        .levels = {
            .reset = 0,                  /* 复位电平 */
            .interrupt = 0,              /* 中断电平 */
        },
        .flags = {
            .swap_xy = 0,                /* 是否交换XY轴 */
            .mirror_x = 0,               /* 是否镜像X轴 */
            .mirror_y = 0,               /* 是否镜像Y轴 */
        },
    };

    /* 创建触摸屏句柄并初始化FT5x06触摸芯片 */
    esp_lcd_touch_handle_t tp;
    //esp_lcd_panel_io_handle_t io_handle;
    esp_lcd_touch_new_i2c_ft5x06(io_handle, &tp_cfg, &tp,i2c_master_handle);
    
    /* 配置LVGL触摸设备参数 */
    const lvgl_port_touch_cfg_t touch_cfg = {
        .disp = disp_handle,  /* LVGL显示设备句柄 */
        .handle = tp,         /* 触摸屏句柄 */
    };
    /* 添加触摸设备并获取句柄 */
    lv_indev_t* touch_handle = lvgl_port_add_touch(&touch_cfg);


    ESP_LOGI(TAG,"lvgl_port_lock");
    if(lvgl_port_lock(0)) {
        //example_lvgl_demo_ui(disp_handle);

        ESP_LOGI(TAG,"lvgl_port_unlock");
        lvgl_port_unlock();
    }
    setup_ui(&guider_ui);


    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();
    ESP_LOGI(TAG, "CJSON init");
    mqtt_cjson_init(&mqtt_cjson);
    /* start linkkit mqtt */
    ESP_LOGI(TAG, "Start linkkit mqtt");
    linkkit_main();

}

