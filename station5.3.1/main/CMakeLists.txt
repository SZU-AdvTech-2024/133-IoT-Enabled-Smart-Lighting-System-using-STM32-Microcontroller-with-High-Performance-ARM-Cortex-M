# 使用GLOB_RECURSE命令获取ui目录下的所有.c文件，并将其存储在变量ui_srcs中
file(GLOB_RECURSE ui_srcs 
        ./ui/custom/*.c
        ./ui/generated/*.c 
        ./ui/generated/guider_customer_fonts/*.c 
        ./ui/generated/guider_fonts/*.c
        ./ui/generated/images/*.c )

# 使用list命令将ui目录及其子目录添加到include_ui_dirs列表中，以便在编译时包含这些目录
list(APPEND include_ui_dirs
    ui/generated
    ui/custom
    ui/generated/guider_customer_fonts
    ui/generated/guider_fonts
    ui/generated/images
    ui
)

# 使用idf_component_register命令注册组件，指定源文件和包含目录
idf_component_register(
    SRCS
        "station_example_main.c"  # 主程序源文件
        "mqtt_cjson.c"            # MQTT和cJSON相关的源文件
        "i2c_master/i2c.c"        # I2C主控相关的源文件
        "i2c_GXHTC3/i2c_gxhtc3.c" # I2C GXHTC3传感器相关的源文件
        "spi_master/spi.c"        # SPI主控相关的源文件
        "esp_spi_lcd/esp_spi_lcd.c" # ESP SPI LCD相关的源文件
        "esp_lvgl_port_init/esp_lvgl_port_init.c" # ESP LVGL初始化相关的源文件
        ${ui_srcs}                # 包含所有ui目录下的.c文件
    INCLUDE_DIRS
        .                        # 当前目录
        i2c_master               # I2C主控相关的头文件目录
        i2c_GXHTC3               # I2C GXHTC3传感器相关的头文件目录
        spi_master               # SPI主控相关的头文件目录
        esp_spi_lcd              # ESP SPI LCD相关的头文件目录
        esp_lvgl_port_init       # ESP LVGL初始化相关的头文件目录
        ${include_ui_dirs}       # 包含所有ui目录及其子目录
)
