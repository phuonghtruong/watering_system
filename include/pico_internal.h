#ifndef PICO_INTERNAL_H
#define PICO_INTERNAL_H

/**
 * Reads the internal temperature sensor of the RP2040/RP2350.
 * Note: adc_init() and adc_set_temp_sensor_enabled(true) must be called in
 * main.
 */
float get_chip_temperature();

#endif
