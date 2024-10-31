#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <linux/i2c-dev.h>
#include <sstream>
#include <string>
#include <sys/ioctl.h>
#include <unistd.h>
#include <thread>
#include <chrono>

// I2C device path
#define I2C_DEVICE "/dev/i2c-1"

// BME280 default I2C address
#define BME280_I2C_ADDR 0x76

// BME280 registers
#define TEMP_REG 0xFA
#define HUM_REG 0xFD
#define PRES_REG 0xF7

using namespace std;

int readI2CRegister(int addr, int reg)
{
  // Open the I2C device
  int file = open(I2C_DEVICE, O_RDWR);
  if (file < 0)
  {
    cerr << "Error opening I2C device" << endl;
    return -1;
  }

  // Set the I2C slave address
  if (ioctl(file, I2C_SLAVE, addr) < 0)
  {
    cerr << "Error setting I2C address" << endl;
    close(file);
    return -1;
  }

  // Write the register address
  char buf[1] = { static_cast<char>(reg) };
  if (write(file, buf, 1) != 1)
  {
    cerr << "Error writing to I2C device" << endl;
    close(file);
    return -1;
  }

  // Read the register value
  if (read(file, buf, 1) != 1)
  {
    cerr << "Error reading from I2C device" << endl;
    close(file);
    return -1;
  }

  close(file);
  return static_cast<int>(buf[0]);
}

int main()
{
  while (1)
  {

    int temp = readI2CRegister(BME280_I2C_ADDR, TEMP_REG);
    int hum = readI2CRegister(BME280_I2C_ADDR, HUM_REG);
    int pres = readI2CRegister(BME280_I2C_ADDR, PRES_REG);

    cout << "Temperature: " << temp << " C" << endl;
    cout << "Humidity: " << hum << " %" << endl;
    cout << "Pressure: " << pres << " hPa" << endl;

    std::this_thread::sleep_for(std::chrono::seconds(5));
  }
  return 0;
}