#include "pico_rtc.h"

#include <stdio.h>
#include <string.h>

#include "hardware/rtc.h"
#include "pico/util/datetime.h"

/**
 * Helper: Calculates the Day of the Week (0 = Sunday... 6 = Saturday)
 * using Sakamoto's Algorithm.
 */
static int calculate_dotw(int y, int m, int d) {
  static int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  if (m < 3) y -= 1;
  return (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7;
}

/**
 * Automatically initializes the Pico RTC using the time the code was compiled.
 */
void internal_rtc_init_from_compiler() {
  // __DATE__ is "Mar 10 2026"
  // __TIME__ is "12:30:05"
  char month_str[4];
  int day, year, hour, minute, second;
  const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

  // 1. Parse compiler strings
  sscanf(__DATE__, "%s %d %d", month_str, &day, &year);
  sscanf(__TIME__, "%d:%d:%d", &hour, &minute, &second);

  // 2. Convert month string to number (1-12)
  int month = 0;
  for (int i = 0; i < 12; i++) {
    if (strcmp(month_str, months[i]) == 0) {
      month = i + 1;
      break;
    }
  }

  // 3. Initialize Hardware RTC
  rtc_init();

  // 4. Create datetime structure
  datetime_t t = {.year = (int16_t)year,
                  .month = (int8_t)month,
                  .day = (int8_t)day,
                  .dotw = (int8_t)calculate_dotw(year, month, day),
                  .hour = (int8_t)hour,
                  .min = (int8_t)minute,
                  .sec = (int8_t)second};

  // 5. Set the time
  if (rtc_set_datetime(&t)) {
    printf("[RTC] Synced to build: %02d/%02d/%d %02d:%02d:%02d (DOTW:%d)\n", day, month, year, hour, minute, second, t.dotw);
  } else {
    printf("[RTC] Error: Failed to set build time!\n");
  }

  sleep_us(64);  // Stabilization delay
}

/**
 * Standard init if you want to provide values manually.
 */
void internal_rtc_init(int16_t year, int8_t month, int8_t day, int8_t hour, int8_t min) {
  rtc_init();
  datetime_t t = {.year = year,
                  .month = month,
                  .day = day,
                  .dotw = (int8_t)calculate_dotw(year, month, day),
                  .hour = hour,
                  .min = min,
                  .sec = 0};
  rtc_set_datetime(&t);
  sleep_us(64);
}

void get_timestamp(char* buf, size_t len) {
  datetime_t t;
  if (rtc_get_datetime(&t)) {
    // Formats: "Tuesday 10 March 2026 12:30:05"
    datetime_to_str(buf, len, &t);
  } else {
    snprintf(buf, len, "RTC Error");
  }
}
