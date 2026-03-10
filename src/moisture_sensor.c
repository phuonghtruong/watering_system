#include "moisture_sensor.h"

#include "hardware/adc.h"
#include "pico/stdlib.h"  // Added for sleep_us

void moisture_sensor_init() { adc_gpio_init(MOISTURE_PIN); }

uint16_t moisture_sensor_read_raw() {
  adc_select_input(MOISTURE_ADC_CH);

  // 1. Small delay for the ADC mux to settle
  sleep_us(100);

  // 2. Discard the first reading
  (void)adc_read();

  // 3. Return the average of a few samples for stability
  uint32_t sum = 0;
  for (int i = 0; i < 5; i++) {
    sum += adc_read();
    sleep_us(50);
  }
  return (uint16_t)(sum / 5);
}

float moisture_sensor_read_percent() {
  uint16_t raw = moisture_sensor_read_raw();

  // Use the values from your calibration
  float dry = MOISTURE_DRY_VALUE;
  float wet = MOISTURE_WET_VALUE;

  float percent = 100.0f * (1.0f - ((float)raw - wet) / (dry - wet));

  if (percent > 100.0f) percent = 100.0f;
  if (percent < 0.0f) percent = 0.0f;

  return percent;
}
