#include <stdbool.h>
#include <stdio.h>

#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"

// Project Headers
#include "lcd_i2c.h"
#include "moisture_sensor.h"
#include "pico_internal.h"
#include "pico_rtc.h"

// I2C Configuration
#define I2C_PORT i2c0
#define SDA_PIN 4
#define SCL_PIN 5

void run_i2c_scan() {
  printf("\n--- Scanning I2C Bus ---\n");
  printf("   0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");
  for (int addr = 0; addr < (1 << 7); ++addr) {
    if (addr % 16 == 0) printf("%02x ", addr);

    // Skip reserved addresses
    if ((addr & 0x78) == 0 || (addr & 0x78) == 0x78) {
      printf("   ");
      continue;
    }

    uint8_t rxdata;
    // 10ms timeout to prevent hanging
    int ret = i2c_read_timeout_us(I2C_PORT, addr, &rxdata, 1, false, 10000);

    if (ret >= 0)
      printf("%02x ", addr);
    else
      printf("-- ");

    if (addr % 16 == 15) printf("\n");
  }
  printf("--- Scan Finished ---\n\n");
}

int main() {
  // 1. Initialize all standard I/O
  stdio_init_all();

  // 2. CRITICAL: Wait for USB Serial to connect
  // This prevents you from missing the I2C scan results in your terminal
  sleep_ms(2000);
  printf("\n[SYSTEM] Booting Raspberry Pi Pico...\n");

  // 3. Initialize RTC and ADC
  internal_rtc_init(2026, 3, 9, 21, 48);
  adc_init();
  adc_set_temp_sensor_enabled(true);

  // Initialize our custom moisture sensor module
  moisture_sensor_init();

  // 4. Initialize I2C Bus
  i2c_init(I2C_PORT, 100 * 1000);  // Start at 100kHz for stability
  gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
  gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
  gpio_pull_up(SDA_PIN);
  gpio_pull_up(SCL_PIN);

  // 5. Run I2C Scan (Check terminal for the LCD address!)
  run_i2c_scan();

  // 6. Initialize LCD
  lcd_init();

  char lcd_line1_buff[17];
  char lcd_line2_buff[17];
  char datetime_buf[256];

  printf("[SYSTEM] Entering Main Loop...\n");

  while (true) {
    // --- STEP A: READ INTERNAL CHIP TEMPERATURE ---
    adc_select_input(4);
    sleep_us(500);     // Allow mux to settle
    (void)adc_read();  // Discard first reading
    float chip_temp = get_chip_temperature();

    // --- STEP B: READ MOISTURE SENSOR ---
    // moisture_sensor_read_raw() already handles its own settling and averaging
    uint16_t raw_m = moisture_sensor_read_raw();
    float moisture_per = moisture_sensor_read_percent(raw_m);

    // --- STEP C: SERIAL OUTPUT ---
    get_timestamp(datetime_buf, sizeof(datetime_buf));
    printf("[%s] Temp: %.2f C | Moist: %.1f%% (Raw: %d)\n", datetime_buf, chip_temp, moisture_per, raw_m);

    // --- STEP D: LCD OUTPUT ---
    // Line 1: Chip Temperature
    snprintf(lcd_line1_buff, sizeof(lcd_line1_buff), "Chip: %.1f C   ", chip_temp);
    lcd_set_cursor(0, 0);
    lcd_string(lcd_line1_buff);

    // Line 2: Moisture Percentage
    snprintf(lcd_line2_buff, sizeof(lcd_line2_buff), "Moist: %.1f%%   ", moisture_per);
    lcd_set_cursor(1, 0);
    lcd_string(lcd_line2_buff);

    // Wait 1 second before next reading
    sleep_ms(1000);
  }

  return 0;
}
