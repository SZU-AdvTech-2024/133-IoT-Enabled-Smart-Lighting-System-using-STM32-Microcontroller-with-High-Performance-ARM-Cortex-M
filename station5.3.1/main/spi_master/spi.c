#include "spi.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"

#define PIN_NUM_MISO -1
#define PIN_NUM_MOSI 5
#define PIN_NUM_CLK 3

#define LCD_HOST  SPI2_HOST
#define EXAMPLE_LCD_H_RES 240

esp_err_t spi_bus_master_init(void)
{
    esp_err_t ret;
    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_NUM_MISO,
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = EXAMPLE_LCD_H_RES * 80 * sizeof(uint16_t)
    };
    ret = spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO);
    return ret;
}