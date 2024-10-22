// (C) Copyright 2018-2021 Dougie Lawson. All rights reserved.
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <chrono>
#include <thread>

#define PRESSURE "/sys/bus/iio/devices/iio:device0/in_pressure_input"
#define TEMPERATURE "/sys/bus/iio/devices/iio:device0/in_temp_input"

struct SensorData
{
  float temp;
  float pressure;
};

SensorData readBMP180()
{
  SensorData readings;
  std::ifstream inputTemp(TEMPERATURE);
  int temp;
  float pressure;
  inputTemp >> temp;
  readings.temp = floor((((float)temp / 1000.0) * 100) + 0.5) / 100;
  std::ifstream inputPress(PRESSURE);
  inputPress >> pressure;
  // std::cout << "In: " << pressure;
  readings.pressure = pressure * 10.0;
  // std::cout << " Out: " << readings.pressure << std::endl;
  return readings;
}

int main()
{
  while (true)
  {
    SensorData values = readBMP180();
    std::cout << "Temp: " << values.temp <<  "°C" << " Pressure: " << values.pressure << " hPa" << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(5));
  }

  return 0;
}
