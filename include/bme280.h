#ifndef BME280_H
#define BME280_H

#include "hardware/i2c.h"
#include "pico/stdlib.h" // IWYU pragma: keep

#define BME280_ADDR _u(0x77)

// Registers
#define REG_ID _u(0xD0)
#define REG_RESET _u(0xE0)
#define REG_CTRL_HUM _u(0xF2)
#define REG_CTRL_MEAS _u(0xF4)
#define REG_CONFIG _u(0xF5)
#define REG_PRESS_MSB _u(0xF7)
#define REG_TEMP_MSB _u(0xFA)
#define REG_HUM_MSB _u(0xFD)

typedef struct {
  float temperature;
  float pressure;
  float humidity;
} bme280_data_t;

void bme280_init(i2c_inst_t *i2c);
void bme280_read_data(i2c_inst_t *i2c, bme280_data_t *data);

#endif
