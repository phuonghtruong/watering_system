#include "bme280.h"
#include <stdio.h> // IWYU pragma: keep

// Calibration parameters (Stored on chip)
struct bme280_calib {
  uint16_t dig_T1;
  int16_t dig_T2, dig_T3;
  uint16_t dig_P1;
  int16_t dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
  uint8_t dig_H1, dig_H3;
  int16_t dig_H2, dig_H4, dig_H5;
  int8_t dig_H6;
  uint32_t t_fine;
} cal;

int32_t t_fine;

void bme280_read_calib(i2c_inst_t *i2c) {
  uint8_t buf[32];
  uint8_t reg = 0x88;
  i2c_write_blocking(i2c, BME280_ADDR, &reg, 1, true);
  i2c_read_blocking(i2c, BME280_ADDR, buf, 24, false);

  cal.dig_T1 = buf[0] | (buf[1] << 8);
  cal.dig_T2 = buf[2] | (buf[3] << 8);
  cal.dig_T3 = buf[4] | (buf[5] << 8);
  cal.dig_P1 = buf[6] | (buf[7] << 8);
  cal.dig_P2 = buf[8] | (buf[9] << 8);
  cal.dig_P3 = buf[10] | (buf[11] << 8);
  cal.dig_P4 = buf[12] | (buf[13] << 8);
  cal.dig_P5 = buf[14] | (buf[15] << 8);
  cal.dig_P6 = buf[16] | (buf[17] << 8);
  cal.dig_P7 = buf[18] | (buf[19] << 8);
  cal.dig_P8 = buf[20] | (buf[21] << 8);
  cal.dig_P9 = buf[22] | (buf[23] << 8);

  reg = 0xA1;
  i2c_write_blocking(i2c, BME280_ADDR, &reg, 1, true);
  i2c_read_blocking(i2c, BME280_ADDR, &cal.dig_H1, 1, false);

  reg = 0xE1;
  i2c_write_blocking(i2c, BME280_ADDR, &reg, 1, true);
  i2c_read_blocking(i2c, BME280_ADDR, buf, 7, false);
  cal.dig_H2 = buf[0] | (buf[1] << 8);
  cal.dig_H3 = buf[2];
  cal.dig_H4 = (buf[3] << 4) | (buf[4] & 0xF);
  cal.dig_H5 = (buf[5] << 4) | (buf[4] >> 4);
  cal.dig_H6 = buf[6];
}

void bme280_init(i2c_inst_t *i2c) {
  bme280_read_calib(i2c);
  uint8_t config[] = {REG_CTRL_HUM, 0x01}; // Humidity oversampling x1
  i2c_write_blocking(i2c, BME280_ADDR, config, 2, false);
  uint8_t meas[] = {REG_CTRL_MEAS,
                    0x27}; // Normal mode, temp/press oversampling x1
  i2c_write_blocking(i2c, BME280_ADDR, meas, 2, false);
}

// Compensation math simplified from Bosch Datasheet
float compensate_temp(int32_t adc_T) {
  int32_t var1, var2;
  var1 =
      ((((adc_T >> 3) - ((int32_t)cal.dig_T1 << 1))) * ((int32_t)cal.dig_T2)) >>
      11;
  var2 = (((((adc_T >> 4) - ((int32_t)cal.dig_T1)) *
            ((adc_T >> 4) - ((int32_t)cal.dig_T1))) >>
           12) *
          ((int32_t)cal.dig_T3)) >>
         14;
  cal.t_fine = var1 + var2;
  return (float)((cal.t_fine * 5 + 128) >> 8) / 100.0f;
}

// Add the Humidity Compensation function (Bosch formula)
float compensate_humidity(int32_t adc_H) {
  int32_t v_x1_u32r;

  v_x1_u32r = (cal.t_fine - ((int32_t)76800));
  v_x1_u32r =
      (((((adc_H << 14) - (((int32_t)cal.dig_H4) << 20) -
          (((int32_t)cal.dig_H5) * v_x1_u32r)) +
         ((int32_t)16384)) >>
        15) *
       (((((((v_x1_u32r * ((int32_t)cal.dig_H6)) >> 10) *
            (((v_x1_u32r * ((int32_t)cal.dig_H3)) >> 11) + ((int32_t)32768))) >>
           10) +
          ((int32_t)2097152)) *
             ((int32_t)cal.dig_H2) +
         8192) >>
        14));
  v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) *
                             ((int32_t)cal.dig_H1)) >>
                            4));
  v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
  v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);

  return (float)(v_x1_u32r >> 12) / 1024.0f;
}

void bme280_read_data(i2c_inst_t *i2c, bme280_data_t *data) {
  uint8_t buf[8];
  uint8_t reg = REG_PRESS_MSB;
  i2c_write_blocking(i2c, BME280_ADDR, &reg, 1, true);
  i2c_read_blocking(i2c, BME280_ADDR, buf, 8, false);

  int32_t adc_P = (buf[0] << 12) | (buf[1] << 4) | (buf[2] >> 4);
  int32_t adc_T = (buf[3] << 12) | (buf[4] << 4) | (buf[5] >> 4);
  int32_t adc_H = (buf[6] << 8) | buf[7];

  data->temperature = compensate_temp(adc_T);
  data->humidity = compensate_humidity(adc_H);
  // Note: Pressure and Humidity compensation functions omitted for brevity
  // but follow similar Bosch formulas using t_fine.
}
