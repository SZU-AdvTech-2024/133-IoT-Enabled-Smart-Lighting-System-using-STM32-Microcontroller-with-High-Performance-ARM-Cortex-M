#pragma once

#include <stdint.h>
#include "driver/i2c_master.h"
#include "esp_err.h"


esp_err_t i2c_master_init(i2c_master_bus_handle_t* bus_handle);
