// In Linux, hardware devices are often represented as files in the filesystem. This abstraction
// allows developers to interact with hardware using familiar file operations such as open, read,
// write, and close. This method simplifies the process of device communication in user space
// without needing to dive into complex kernel space operations.

#include <cstdint>
#include <fcntl.h>
#include <iostream>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <chrono>
#include <thread>

#define I2C_DEVICE "/dev/i2c-1"
#define BME280_I2C_ADDR 0x76

// BME280 Registers
#define BME280_TEMP_MSB_REG 0xFA
#define BME280_CTRL_MEAS_REG 0xF4
#define BME280_CONFIG_REG 0xF5
#define BME280_CALIB00_REG 0x88

using namespace std;

int32_t t_fine; // Global variable used in temperature compensation

// Function to read calibration data
void readCalibrationData(int file, uint16_t& dig_T1, int16_t& dig_T2, int16_t& dig_T3)
{
  uint8_t calib[6];
  uint8_t reg = BME280_CALIB00_REG;
  write(file, &reg, 1);
  read(file, calib, 6);

  dig_T1 = (calib[1] << 8) | calib[0];
  dig_T2 = (calib[3] << 8) | calib[2];
  dig_T3 = (calib[5] << 8) | calib[4];
}

// Function to calculate temperature
double compensateTemperature(int32_t adc_T, uint16_t dig_T1, int16_t dig_T2, int16_t dig_T3)
{
  double var1, var2, T;
  var1 = (((double)adc_T) / 16384.0 - ((double)dig_T1) / 1024.0) * ((double)dig_T2);
  var2 = ((((double)adc_T) / 131072.0 - ((double)dig_T1) / 8192.0)
          * (((double)adc_T) / 131072.0 - ((double)dig_T1) / 8192.0))
      * ((double)dig_T3);
  t_fine = (int32_t)(var1 + var2);
  T = (var1 + var2) / 5120.0;
  return T;
}

// Function to read data from the BME280 sensor
int32_t readBME280Data(int file, uint8_t regAddress)
{
  uint8_t buf[3] = { 0 };
  write(file, &regAddress, 1); // Write the register address to read from
  read(file, buf, 3); // Read 3 bytes of data from the register

  // Combine the bytes to create the raw data value
  int32_t data = (buf[0] << 16) | (buf[1] << 8) | buf[2];
  data >>= 4; // The data is left-aligned so shift right to align it properly

  return data;
}

int main()
{
  while (1)
  {
    // open() function is used here to open the I2C device file (/dev/i2c-1).
    // /dev/i2c-0, /dev/i2c-1, etc., are device files that represent different I2C buses.
    // The number after i2c- signifies the bus number. 
    // This file represents the I2C bus to which the
    // BME280 is connected. O_RDWR indicates that the file is opened for both reading and writing.
    int file = open(I2C_DEVICE, O_RDWR);
    if (file < 0)
    {
      cerr << "Error opening I2C device" << endl;
      return -1;
    }

    if (ioctl(file, I2C_SLAVE, BME280_I2C_ADDR) < 0)
    {
      cerr << "Error setting I2C address" << endl;
      close(file);
      return -1;
    }

    // Initialize sensor
    uint8_t config[2] = { BME280_CTRL_MEAS_REG, 0x27 };
    write(file, config, 2);

    // Read calibration data
    uint16_t dig_T1;
    int16_t dig_T2, dig_T3;
    readCalibrationData(file, dig_T1, dig_T2, dig_T3);

    // Read temperature data
    int32_t adc_T = readBME280Data(file, BME280_TEMP_MSB_REG);
    double temperature = compensateTemperature(adc_T, dig_T1, dig_T2, dig_T3);

    cout << "Temperature: " << temperature << " °C" << endl;

    close(file);

    std::this_thread::sleep_for(std::chrono::seconds(5));
  }
  return 0;
}