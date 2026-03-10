#include "pico_rtc.h"
#include "pico/stdlib.h" // IWYU pragma: keep
#include <stdio.h>

void internal_rtc_init(int16_t year, int8_t month, int8_t day, int8_t hour,
                       int8_t min) {
  rtc_init();

  // datetime_t structure: year, month, day, dotw (0-6), hour, min, sec
  datetime_t t = {.year = year,
                  .month = month,
                  .day = day,
                  .dotw = 1, // Day of the week (0=Sunday, 1=Monday, etc.)
                  .hour = hour,
                  .min = min,
                  .sec = 00};

  rtc_set_datetime(&t);

  // The RTC needs a tiny moment to stabilize after setting
  sleep_us(64);
}

void get_timestamp(char *buf, size_t len) {
  datetime_t t;
  rtc_get_datetime(&t);
  // Uses the SDK utility to turn the struct into a readable string
  datetime_to_str(buf, len, &t);
}
