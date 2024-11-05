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

  //  I2C devices like the BME280 have multiple registers, each serving different purposes (e.g.,
  //  storing temperature data, humidity data, configuration settings, etc.). To read from a
  //  specific register, you first need to tell the device which register you're interested in. This
  //  is done by writing the address of that register to the device. Once the device knows which
  //  register you want to read, it prepares that data to be sent over the I2C bus when a read
  //  request is made. It is similar to opening a book on a specific page to read.
  write(file, &regAddress, 1);

  // Reads first 3 bytes of temperature data starting from &regAddress. The temperature data in the
  // BME280 sensor is stored across three registers. These are typically labeled as
  // - MSB (Most Significant Byte) holds the upper part of the measurement data. Contains the upper
  // 8 bits.
  // - LSB (Least Significant Byte) holds the middle part of the measurement data. Contains the
  // upper 8 bits.
  // - XLSB (Extended Least Significant Byte) contains the lower bits of the measurement data.
  // Contains the lower 4 bits used and 4 bits of padding or unused data.
  read(file, buf, 3);

  // Combine the bytes to create the raw data value.
  // The first byte should be shifter relates to the second byte and contains 8 bits.
  // The second byte should be shifter relates to the second byte and contains 8 bits.
  // The third byte should be right-shifter because only 4 bits used and 4 bits of padding or unused
  // data.
  int32_t data = (buf[0] << 12) | (buf[1] << 4) | buf[2] >> 4;

  return data;
}

int main()
{
  while (1)
  {
    // In Linux, hardware devices are often represented as files in the filesystem.
    // open() function is used here to open the I2C device file (/dev/i2c-1).
    // /dev/i2c-0, /dev/i2c-1, etc., are device files that represent different I2C buses (different
    // I2C set of pins on the board). The number after i2c- signifies the bus number. This file
    // represents the I2C bus to which the BME280 is connected. O_RDWR indicates that the file is
    // opened for both reading and writing.
    int file = open(I2C_DEVICE, O_RDWR);
    if (file < 0)
    {
      cerr << "Error opening I2C device" << endl;
      return -1;
    }

    // I2C (Inter-Integrated Circuit) is a multi-master, multi-slave, packet-switched, single-ended,
    // serial communication bus. This means that multiple devices (slaves) can be connected to the
    // same bus and controlled by a master (in this case, the Raspberry Pi).
    // Each device (slave) on the bus is assigned a unique address. This address is used by the
    // master to communicate with a specific slave device (BME280).
    // In this case, we installed `i2cdetect`
    // > sudo apt-get update
    // > sudo apt-get install i2c-tools g++
    // Run the command:
    // > i2cdetect -y 1
    // This will display a grid of addresses with the address of your BME280 sensor highlighted if
    // connected properly. In fact, for the current case BME280_I2C_ADDR = 0x76 After this call, any
    // I2C operations performed on this file descriptor will communicate with the BME280 sensor.
    // The I2C_SLAVE constant is used to indicate that you want to set the slave address for
    // subsequent I2C operations.
    if (ioctl(file, I2C_SLAVE, BME280_I2C_ADDR) < 0)
    {
      cerr << "Error setting I2C address" << endl;
      close(file);
      return -1;
    }

    // The initialization code sets the BME280 sensor to a specific operational mode with defined
    // oversampling settings for pressure and temperature measurements. This setup is crucial for
    // preparing the sensor to perform measurements according to the desired accuracy and power
    // consumption requirements.
    // 0x27 is hex value. Translate it to binary format: 0x27 = 001 001 11
    // (https://www.rapidtables.com/convert/number/hex-to-binary.html)
    // * Bits 7-5 (temperature oversampling): 001 (x1)
    // * Bits 4-2 (pressure oversampling): 010 (x1)
    // * Bits 1-0 (mode): 11 (normal mode);
    // ** other possible values: Sleep Mode (00), Forced mode (10 or 01), Normal Mode (11).
    //
    // How Oversampling Works:
    // * Multiple Readings: The sensor takes multiple readings of the temperature over a short
    // period.
    // * Averaging: These multiple readings are then mathematically averaged or otherwise processed
    // to produce a single reading.
    // * Result: The final temperature value reported is smoother and more stable, as random noise
    // and fluctuations in individual readings tend to cancel each other out.
    uint8_t config[2] = { BME280_CTRL_MEAS_REG, 0xFF };
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

    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
  return 0;
}