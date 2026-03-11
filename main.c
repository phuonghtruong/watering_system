#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/watchdog.h"
#include "pico/stdlib.h"

// Project Headers
#include "lcd_i2c.h"
#include "moisture_sensor.h"
#include "pico_internal.h"
#include "pico_rtc.h"

// --- CONFIGURATION ---
#define I2C_PORT i2c0
#define SDA_PIN 4
#define SCL_PIN 5
#define RELAY_PIN 15
#define BUTTON_PIN 14

// Thresholds
#define MOIST_DRY_THRESHOLD 30.0f
#define MOIST_WET_THRESHOLD 60.0f
#define SENSOR_MIN_HEALTHY 0.5f  // Below this, we assume the wire is unplugged

// Timers (ms)
#define MAX_PUMP_RUN_TIME_MS 20000  // 20 sec safety cutoff
#define SOAK_COOLDOWN_MS 15000      // 15 sec wait for water to sink in
#define DISPLAY_UPDATE_MS 500       // Refresh screen 2x per second
#define PAGE_SWITCH_MS 4000         // Switch LCD pages every 4 seconds
#define WATCHDOG_TIMEOUT_MS 5000    // Reboot if frozen for 5 seconds

// --- SYSTEM STATES ---
typedef enum {
  SYSTEM_IDLE,
  SYSTEM_WATERING_AUTO,
  SYSTEM_WATERING_MANUAL,
  SYSTEM_SOAKING,
  SYSTEM_ERROR_TIMEOUT,
  SYSTEM_ERROR_SENSOR
} system_state_t;

// --- GLOBALS ---
system_state_t current_state = SYSTEM_IDLE;
uint32_t pump_start_time = 0;
uint32_t last_pump_end_time = 0;

// Rolling Average Buffer (Filters out electronic noise)
#define AVG_SAMPLES 20
float moisture_buffer[AVG_SAMPLES];
int buf_idx = 0;

float get_stable_moisture(float new_sample) {
  moisture_buffer[buf_idx] = new_sample;
  buf_idx = (buf_idx + 1) % AVG_SAMPLES;
  float sum = 0;
  for (int i = 0; i < AVG_SAMPLES; i++) sum += moisture_buffer[i];
  return sum / (float)AVG_SAMPLES;
}

const char* get_status_str(system_state_t state) {
  switch (state) {
    case SYSTEM_IDLE:
      return "Status: Ready  ";
    case SYSTEM_WATERING_AUTO:
      return "Mode: Pumping  ";
    case SYSTEM_WATERING_MANUAL:
      return "Mode: Manual   ";
    case SYSTEM_SOAKING:
      return "Status: Soaking";
    case SYSTEM_ERROR_TIMEOUT:
      return "ERR: TIMEOUT   ";
    case SYSTEM_ERROR_SENSOR:
      return "ERR: NO SENSOR ";
    default:
      return "Unknown        ";
  }
}

int main() {
  // 1. Initialize Standard I/O
  stdio_init_all();

  // 2. Safety: Enable Hardware Watchdog
  // If the loop freezes for more than 5 seconds, the Pico hardware reboots
  watchdog_enable(WATCHDOG_TIMEOUT_MS, 1);

  // Wait for USB Serial connection
  sleep_ms(2000);

  printf("[SYSTEM] Smart Watering v3.3 Booting...\n");

  // NEW CLEAN CALL: No arguments needed!
  internal_rtc_init_from_compiler();

  adc_init();
  adc_set_temp_sensor_enabled(true);
  moisture_sensor_init();

  // Relay (Pump) Init
  gpio_init(RELAY_PIN);
  gpio_set_dir(RELAY_PIN, GPIO_OUT);
  gpio_put(RELAY_PIN, 0);

  // Button Init
  gpio_init(BUTTON_PIN);
  gpio_set_dir(BUTTON_PIN, GPIO_IN);
  gpio_pull_up(BUTTON_PIN);

  // I2C & LCD Init
  i2c_init(I2C_PORT, 100 * 1000);
  gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
  gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
  gpio_pull_up(SDA_PIN);
  gpio_pull_up(SCL_PIN);
  lcd_init();

  // Pre-fill buffer with initial reading
  float initial_read = moisture_sensor_read_percent(moisture_sensor_read_raw());
  for (int i = 0; i < AVG_SAMPLES; i++) moisture_buffer[i] = initial_read;

  char line1[17], line2[17], time_str[64];
  uint32_t last_display_time = 0;
  uint32_t last_page_switch = 0;
  int display_page = 0;

  while (true) {
    // Feed the watchdog to prove the code is still running
    watchdog_update();

    uint32_t now = to_ms_since_boot(get_absolute_time());

    // --- STEP A: SENSORS (Non-blocking) ---
    float raw_val = moisture_sensor_read_percent(moisture_sensor_read_raw());
    float moisture_per = get_stable_moisture(raw_val);

    adc_select_input(4);
    float chip_temp = get_chip_temperature();
    bool btn = !gpio_get(BUTTON_PIN);  // Button is Active Low

    // --- STEP B: LOGIC STATE MACHINE ---

    // Sensor Health Check: If moisture is exactly 0.0%, assume hardware failure
    if (moisture_per < SENSOR_MIN_HEALTHY && current_state != SYSTEM_ERROR_TIMEOUT) {
      current_state = SYSTEM_ERROR_SENSOR;
    }

    switch (current_state) {
      case SYSTEM_IDLE:
        gpio_put(RELAY_PIN, 0);
        if (btn) {
          current_state = SYSTEM_WATERING_MANUAL;
          pump_start_time = now;
        } else if (moisture_per < MOIST_DRY_THRESHOLD) {
          // Check if we are still in the soak/cooldown period
          if (now - last_pump_end_time > SOAK_COOLDOWN_MS) {
            current_state = SYSTEM_WATERING_AUTO;
            pump_start_time = now;
          } else {
            current_state = SYSTEM_SOAKING;
          }
        }
        break;

      case SYSTEM_SOAKING:
        gpio_put(RELAY_PIN, 0);
        if (now - last_pump_end_time > SOAK_COOLDOWN_MS) current_state = SYSTEM_IDLE;
        if (btn) {
          current_state = SYSTEM_WATERING_MANUAL;
          pump_start_time = now;
        }
        break;

      case SYSTEM_WATERING_AUTO:
        gpio_put(RELAY_PIN, 1);
        // SUCCESS: Watered until wet threshold
        if (moisture_per > MOIST_WET_THRESHOLD) {
          current_state = SYSTEM_IDLE;
          last_pump_end_time = now;
        }
        // SAFETY: Stop if it takes too long
        else if (now - pump_start_time > MAX_PUMP_RUN_TIME_MS) {
          current_state = SYSTEM_ERROR_TIMEOUT;
          last_pump_end_time = now;
        }
        break;

      case SYSTEM_WATERING_MANUAL:
        gpio_put(RELAY_PIN, 1);
        if (!btn) {
          current_state = SYSTEM_IDLE;
          last_pump_end_time = now;
        }
        if (now - pump_start_time > MAX_PUMP_RUN_TIME_MS) {
          current_state = SYSTEM_ERROR_TIMEOUT;
          last_pump_end_time = now;
        }
        break;

      case SYSTEM_ERROR_TIMEOUT:
      case SYSTEM_ERROR_SENSOR:
        gpio_put(RELAY_PIN, 0);
        if (btn) {        // A button press resets the error
          sleep_ms(500);  // Debounce
          current_state = SYSTEM_IDLE;
          last_pump_end_time = now;
        }
        break;
    }

    // --- STEP C: OUTPUT (LCD & SERIAL) ---
    if (now - last_display_time > DISPLAY_UPDATE_MS) {
      last_display_time = now;

      // Switch LCD pages every X seconds
      if (now - last_page_switch > PAGE_SWITCH_MS) {
        display_page = (display_page + 1) % 2;
        last_page_switch = now;
        lcd_clear();  // Clear to prevent artifacts
      }

      if (display_page == 0) {
        // PAGE 0: Moisture and System Status
        snprintf(line1, sizeof(line1), "Moisture: %.1f%% ", moisture_per);
        snprintf(line2, sizeof(line2), "%-16s", get_status_str(current_state));
      } else {
        // PAGE 1: Temperature and Time
        get_timestamp(time_str, sizeof(time_str));

        // Robust Time Extraction (HH:MM:SS)
        char* first_colon = strchr(time_str, ':');
        char* display_time = (first_colon) ? (first_colon - 2) : "--:--:--";

        snprintf(line1, sizeof(line1), "Temp: %.1f C   ", chip_temp);
        snprintf(line2, sizeof(line2), "Time: %.8s   ", display_time);
      }

      lcd_set_cursor(0, 0);
      lcd_string(line1);
      lcd_set_cursor(1, 0);
      lcd_string(line2);

      // Serial Monitor Logging
      printf("[%s] Moist:%.1f%% State:%s Temp:%.1fC\n", time_str, moisture_per, get_status_str(current_state), chip_temp);
    }

    // 20ms delay makes the button feel very responsive
    sleep_ms(20);
  }
}
