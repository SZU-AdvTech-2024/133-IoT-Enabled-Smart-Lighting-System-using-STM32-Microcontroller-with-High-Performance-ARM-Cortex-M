#include "i2c_gxhtc3.h"

#include "driver/i2c_master.h"
#include <math.h>
#include <string.h>
#include <stdio.h>
#include "sdkconfig.h"
#include "esp_types.h"
#include "esp_log.h"
#include "esp_check.h"
#include "driver/i2c_master.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define POLYNOMIAL  0x31 // P(x) = x^8 + x^5 + x^4 + 1 = 00110001
#define MASTER_FREQUENCY 100000 // 100kHz
#define GXHTC3_ADDR 0x70
#define GXHTC3_conversion_time_ms 20

static const char TAG[] = "i2c-gxhtc3";

const i2c_device_config_t i2c_dev_gxhtc3_config = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = GXHTC3_ADDR,
    .scl_speed_hz = MASTER_FREQUENCY,
};

i2c_gxhtc3_t i2c_gxhtc3={
    .id = 0,
    .i2c_dev = NULL,
};

esp_err_t gxhtc3_i2c_init(i2c_master_bus_handle_t bus_handle, i2c_gxhtc3_handle_t *gxhtc3_handle)
{
    esp_err_t ret = ESP_OK;
    i2c_gxhtc3_handle_t out_handle=&i2c_gxhtc3;
    
    if (out_handle->i2c_dev == NULL) {
        ESP_GOTO_ON_ERROR(i2c_master_bus_add_device(bus_handle, &i2c_dev_gxhtc3_config, &out_handle->i2c_dev), err, TAG, "i2c new bus failed");
    }

    out_handle->conversion_time_ms = GXHTC3_conversion_time_ms;

    *gxhtc3_handle = out_handle;

    return ESP_OK;

err:
    if (out_handle && out_handle->i2c_dev) {
        i2c_master_bus_rm_device(out_handle->i2c_dev);
    }
    return ret;
}




uint8_t gxhtc3_calc_crc(uint8_t *crcdata, uint8_t len)
{
    uint8_t crc = 0xFF; 
  
    for(uint8_t i = 0; i < len; i++)
    {
        crc ^= (crcdata[i]);
        for(uint8_t j = 8; j > 0; --j)
        {
            if(crc & 0x80) crc = (crc << 1) ^ POLYNOMIAL;
            else           crc = (crc << 1);
        }
    }
    return crc;
}


esp_err_t gxhtc3_read_id(i2c_gxhtc3_handle_t gxhtc3_handle)
{
    esp_err_t ret;

    uint8_t write_buf[2];
    uint8_t read_buf[3];
    write_buf[0] = (uint8_t)((CMD_READ_ID>>8)&0xFF);
    write_buf[1] = (uint8_t)(CMD_READ_ID&0xFF);
    ESP_ERROR_CHECK(i2c_master_transmit_receive(gxhtc3_handle->i2c_dev, write_buf, sizeof(write_buf), read_buf, sizeof(read_buf), -1));

    if(read_buf[2]!=gxhtc3_calc_crc(read_buf,2)){    
        gxhtc3_handle->id = 0;
        ret = ESP_FAIL;
    }else{
        gxhtc3_handle->id = (uint16_t)read_buf[0]<<8 | read_buf[1];
        ret = ESP_OK;
    }
    return ret;
}


esp_err_t gxhtc3_get_tah(i2c_gxhtc3_handle_t gxhtc3_handle)
{
    int ret;
    uint8_t write_buf[2];
    uint8_t read_buf[6];

    write_buf[0] = (uint8_t)((CMD_WAKEUP>>8)&0xFF);
    write_buf[1] = (uint8_t)(CMD_WAKEUP&0xFF);
    ESP_ERROR_CHECK(i2c_master_transmit(gxhtc3_handle->i2c_dev, write_buf, sizeof(write_buf), -1));
    write_buf[0] = (uint8_t)((CMD_CONVERT_CLOCK_STRETCHING_NOMAL>>8)&0xFF);
    write_buf[1] = (uint8_t)(CMD_CONVERT_CLOCK_STRETCHING_NOMAL&0xFF);
    ESP_ERROR_CHECK(i2c_master_transmit(gxhtc3_handle->i2c_dev, write_buf, sizeof(write_buf), -1));
    vTaskDelay(gxhtc3_handle->conversion_time_ms / portTICK_PERIOD_MS);
    ESP_ERROR_CHECK(i2c_master_receive(gxhtc3_handle->i2c_dev,read_buf,6,-1));

    if((read_buf[2]!=gxhtc3_calc_crc(read_buf,2)||(read_buf[5]!=gxhtc3_calc_crc(&read_buf[3],2)))){     
        gxhtc3_handle->humi = 0;
        gxhtc3_handle->temp = 0;
        ret = ESP_FAIL;
    }
    else{
        uint16_t rawValueTemp = (read_buf[0]<<8) | read_buf[1];
        uint16_t rawValueHumi = (read_buf[3]<<8) | read_buf[4];
        gxhtc3_handle->temp = (175.0 * (float)rawValueTemp) / 65535.0 - 45.0; 
        gxhtc3_handle->humi = (100.0 * (float)rawValueHumi) / 65535.0;
        ret = ESP_OK;
    }
    return ret;
}



