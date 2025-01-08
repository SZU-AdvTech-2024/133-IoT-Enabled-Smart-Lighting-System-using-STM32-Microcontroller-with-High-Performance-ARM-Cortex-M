#pragma once

#include <stdint.h>
#include "esp_log.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_err.h"
#include "driver/gpio.h"

esp_err_t esp_spi_lcd_init_io(esp_lcd_panel_io_handle_t *io_handle);
esp_err_t esp_spi_lcd_panel_init(esp_lcd_panel_handle_t *panel_handle,const esp_lcd_panel_io_handle_t io_handle);
esp_err_t esp_lcd_backlight_init(void);
void esp_lcd_backlight_on(void);
void esp_lcd_backlight_off(void);