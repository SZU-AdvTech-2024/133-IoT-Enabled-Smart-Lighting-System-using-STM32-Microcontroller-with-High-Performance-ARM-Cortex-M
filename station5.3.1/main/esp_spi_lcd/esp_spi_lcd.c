#include "esp_spi_lcd.h"

#define LCD_PIN_NUM_LCD_DC 6
#define LCD_PIN_NUM_LCD_CS 4
#define LCD_LCD_PIXEL_CLOCK_HZ (20 * 1000 * 1000)
#define LCD_LCD_CMD_BITS 8
#define LCD_LCD_PARAM_BITS 8

#define LCD_PIN_NUM_RST -1
#define LCD_HOST SPI2_HOST
#define LCD_LCD_BK_LIGHT_ON_LEVEL 0

#define LCD_PIN_NUM_BK_LIGHT 2

esp_err_t esp_lcd_backlight_init(void)
{
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << LCD_PIN_NUM_BK_LIGHT
    };
    return gpio_config(&bk_gpio_config);
}

void esp_lcd_backlight_on(void)
{
    gpio_set_level(LCD_PIN_NUM_BK_LIGHT, LCD_LCD_BK_LIGHT_ON_LEVEL);
}

void esp_lcd_backlight_off(void)
{
    gpio_set_level(LCD_PIN_NUM_BK_LIGHT, !LCD_LCD_BK_LIGHT_ON_LEVEL);
}

esp_err_t esp_spi_lcd_init_io(esp_lcd_panel_io_handle_t *io_handle)
{   
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = LCD_PIN_NUM_LCD_DC, // 数据/命令选择引脚
        .cs_gpio_num = LCD_PIN_NUM_LCD_CS, // 片选引脚
        .pclk_hz = LCD_LCD_PIXEL_CLOCK_HZ, // 像素时钟频率
        .lcd_cmd_bits = LCD_LCD_CMD_BITS, // 命令位数
        .lcd_param_bits = LCD_LCD_PARAM_BITS, // 参数位数
        .spi_mode = 0,//spi模式CPOL, CPHA
        .trans_queue_depth = 10, // 传输队列深度
    };
    // 将 LCD 连接到 SPI 总线
    return esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, io_handle);
}

esp_err_t esp_spi_lcd_panel_init(esp_lcd_panel_handle_t *panel_handle,const esp_lcd_panel_io_handle_t io_handle)
{
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = LCD_PIN_NUM_RST,     // 复位引脚，设为-1表示不使用硬件复位
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,  // RGB颜色元素的顺序
        .bits_per_pixel = 16,    // 每像素16位色深，即RGB565格式
    };
    // 为 ST7789 创建 LCD 面板句柄，并指定 SPI IO 设备句柄
    return esp_lcd_new_panel_st7789(io_handle, &panel_config, panel_handle);
}