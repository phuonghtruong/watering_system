#include "lcd_i2c.h"

#include "pico/stdlib.h"  // IWYU pragma: keep

// Helper to send a single byte over I2C
static void i2c_write_byte(uint8_t val) { i2c_write_blocking(LCD_I2C_PORT, LCD_ADDR, &val, 1, false); }

// Low-level strobe to "commit" data to the LCD
static void lcd_toggle_enable(uint8_t val) {
  sleep_us(600);
  i2c_write_byte(val | LCD_ENABLE_BIT);
  sleep_us(600);
  i2c_write_byte(val & ~LCD_ENABLE_BIT);
  sleep_us(600);
}

void lcd_send_byte(uint8_t val, int mode) {
  uint8_t high = mode | (val & 0xF0) | LCD_BACKLIGHT;
  uint8_t low = mode | ((val << 4) & 0xF0) | LCD_BACKLIGHT;

  i2c_write_byte(high);
  lcd_toggle_enable(high);
  i2c_write_byte(low);
  lcd_toggle_enable(low);
}

void lcd_init() {
  // I2C Initialisation at 100Khz
  i2c_init(LCD_I2C_PORT, 100 * 1000);
  gpio_set_function(PIN_SDA, GPIO_FUNC_I2C);
  gpio_set_function(PIN_SCL, GPIO_FUNC_I2C);
  gpio_pull_up(PIN_SDA);
  gpio_pull_up(PIN_SCL);

  // Initial sequence to set to 4-bit mode
  lcd_send_byte(0x03, LCD_COMMAND);
  lcd_send_byte(0x03, LCD_COMMAND);
  lcd_send_byte(0x03, LCD_COMMAND);
  lcd_send_byte(0x02, LCD_COMMAND);

  lcd_send_byte(LCD_FUNCTIONSET | 0x08, LCD_COMMAND);     // 2 lines
  lcd_send_byte(LCD_DISPLAYCONTROL | 0x04, LCD_COMMAND);  // Display on
  lcd_clear();
}

void lcd_clear() {
  lcd_send_byte(LCD_CLEARDISPLAY, LCD_COMMAND);
  sleep_ms(2);
}

void lcd_set_cursor(int line, int position) {
  int val = (line == 0) ? 0x80 + position : 0xC0 + position;
  lcd_send_byte(val, LCD_COMMAND);
}

void lcd_string(const char* s) {
  while (*s) {
    lcd_send_byte(*s++, LCD_CHARACTER);
  }
}
