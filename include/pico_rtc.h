#ifndef PICO_RTC_H
#define PICO_RTC_H

#include "hardware/rtc.h"       // IWYU pragma: keep
#include "pico/util/datetime.h" // IWYU pragma: keep

// Initializes the RTC and sets a starting time
void internal_rtc_init(int16_t year, int8_t month, int8_t day, int8_t hour,
                       int8_t min);

// Gets a formatted string of the current time
void get_timestamp(char *buf, size_t len);

#endif
