#pragma once


#include <stdint.h>
#include "driver/spi_master.h"
#include "esp_err.h"

esp_err_t spi_bus_master_init(void);