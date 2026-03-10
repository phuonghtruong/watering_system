#include "pico_internal.h"
#include "hardware/adc.h"

float get_chip_temperature() {

  // Inside your while loop
  uint16_t raw = adc_read();
  const float conversion_factor = 3.3f / (1 << 12);
  float voltage = raw * conversion_factor;
  float temp = 27 - (voltage - 0.706) / 0.001721; // Official RP2040 formula

  return temp;
}
