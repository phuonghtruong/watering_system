#ifndef LCD_I2C_H
#define LCD_I2C_H

#include "hardware/i2c.h"

// --- Configuration ---
#define LCD_ADDR 0x27      // Default address is 0x27 or 0x3F
#define LCD_I2C_PORT i2c1  // Or i2c1
#define PIN_SDA 6          // GPIO 6 (Pin 9)
#define PIN_SCL 7          // GPIO 7 (Pin 10)

// --- Commands ---
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_FUNCTIONSET 0x20

// --- Flags ---
#define LCD_BACKLIGHT 0x08
#define LCD_ENABLE_BIT 0x04

// Modes for lcd_send_byte
#define LCD_CHARACTER 1
#define LCD_COMMAND 0

// --- Functions ---
void lcd_init();
void lcd_send_byte(uint8_t val, int mode);
void lcd_clear();
void lcd_set_cursor(int line, int position);
void lcd_string(const char* s);

#endif
