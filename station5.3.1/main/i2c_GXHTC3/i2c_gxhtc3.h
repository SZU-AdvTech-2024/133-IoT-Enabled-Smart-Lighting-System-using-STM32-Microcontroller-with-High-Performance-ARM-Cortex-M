#pragma once
#include <stdint.h>
#include "driver/i2c_master.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CMD_READ_ID 0xEFC8
#define CMD_SLEEP 0xB098
#define CMD_WAKEUP 0x3517
#define CMD_CONVERT_CLOCK_STRETCHING_NOMAL 0x7CA2


struct i2c_gxhtc3_t {
    i2c_master_dev_handle_t i2c_dev;      /*!< I2C device handle */
    uint8_t conversion_time_ms;                /*!< I2C gxhtc3 write time(ms)*/
    float temp,humi;
    uint16_t id;
};

typedef struct i2c_gxhtc3_t i2c_gxhtc3_t;

/* handle of gxhtc3 device */
typedef struct i2c_gxhtc3_t *i2c_gxhtc3_handle_t;

esp_err_t gxhtc3_read_id(i2c_gxhtc3_handle_t gxhtc3_handle);
esp_err_t gxhtc3_get_tah(i2c_gxhtc3_handle_t gxhtc3_handle);
esp_err_t gxhtc3_i2c_init(i2c_master_bus_handle_t bus_handle, i2c_gxhtc3_handle_t *gxhtc3_handle);

#ifdef __cplusplus
}
#endif

