#include "esp_lvgl_port_init.h"
/* 显示屏宽度 */
#define DISP_WIDTH 240
/* 显示屏高度 */
#define DISP_HEIGHT 320
/* LVGL显示设备句柄 */
static lv_disp_t * disp_handle;

/**
 * @brief 初始化LVGL端口
 * @param io_handle LCD面板IO句柄
 * @param panel_handle LCD面板句柄
 * @param _disp_handle LVGL显示设备句柄指针
 * @return esp_err_t 错误码
 */
esp_err_t u_esp_lvgl_port_init(esp_lcd_panel_io_handle_t io_handle,esp_lcd_panel_handle_t panel_handle,lv_disp_t **_disp_handle)
{
    /* 使用默认配置初始化LVGL */
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    esp_err_t err = lvgl_port_init(&lvgl_cfg);
    if (err != ESP_OK) {
        return err;
    }

    /* 配置LVGL显示设备参数 */
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,              /* LCD面板IO句柄 */
        .panel_handle = panel_handle,        /* LCD面板句柄 */
        .buffer_size = 320*sizeof(uint16_t)*14*2, /* 显示缓冲区大小 */
        .double_buffer = true,               /* 使用双缓冲 */
        .hres = DISP_WIDTH,                 /* 水平分辨率 */
        .vres = DISP_HEIGHT,                /* 垂直分辨率 */
        .monochrome = false,                /* 非单色显示 */
        //.color_format = LV_COLOR_FORMAT_RGB565, /* RGB565颜色格式 */
        .rotation = {
            .swap_xy = false,               /* 不交换XY轴 */
            .mirror_x = false,              /* 不镜像X轴 */
            .mirror_y = false,              /* 不镜像Y轴 */
        },
        .flags = {
            .buff_dma = true,              /* 使用DMA传输显示缓冲 */
            //.swap_bytes = false,         /* 不交换字节序 */
        }
    };
    /* 添加显示设备并获取句柄 */
    disp_handle = lvgl_port_add_disp(&disp_cfg);
    *_disp_handle = disp_handle;
    return ESP_OK;
}
