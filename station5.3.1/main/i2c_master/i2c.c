#include "i2c.h"
#include "driver/i2c_master.h"
#include <string.h>
#include <stdio.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "soc/gpio_num.h"

#define SCL_IO_PIN GPIO_NUM_1
#define SDA_IO_PIN GPIO_NUM_0
#define PORT_NUMBER -1  //-1 meansauto detect port number

/**
 * @brief 初始化I2C主机总线
 * @param bus_handle I2C总线句柄指针
 * @return esp_err_t 错误码
 */
esp_err_t i2c_master_init(i2c_master_bus_handle_t* bus_handle){ 
    /* I2C总线配置 */
    i2c_master_bus_config_t i2c_bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,      // 使用默认时钟源
        .i2c_port = PORT_NUMBER,                // I2C端口号
        .scl_io_num = SCL_IO_PIN,              // SCL引脚
        .sda_io_num = SDA_IO_PIN,              // SDA引脚
        .glitch_ignore_cnt = 7,                 // 忽略毛刺计数
        .flags.enable_internal_pullup = true,   // 使能内部上拉
    };
    
    /* 创建新的I2C主机总线 */
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_config, bus_handle));
    return ESP_OK;
}