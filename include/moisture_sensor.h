#ifndef MOISTURE_SENSOR_H
#define MOISTURE_SENSOR_H

#include <stdint.h>

// Configuration
#define MOISTURE_PIN 26
#define MOISTURE_ADC_CH 0

// Calibration Constants (Adjust these based on your specific sensor)
#define MOISTURE_DRY_VALUE 2660.0f
#define MOISTURE_WET_VALUE 1100.0f

/**
 * Initialize the GPIO and ADC for the moisture sensor.
 * Note: adc_init() should be called in main before this.
 */
void moisture_sensor_init();

/**
 * Returns the raw 12-bit ADC value (0-4095)
 */
uint16_t moisture_sensor_read_raw();

/**
 * Returns the moisture level as a percentage (0.0 to 100.0)
 */
float moisture_sensor_read_percent();

#endif
