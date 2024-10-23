#ifndef BME280_DATA_H
#define BME280_DATA_H

typedef struct Bme280Data
{
  float humidity;
  float pressure;
  float temperature;
} Bme280Data;

Bme280Data readBME280();

#endif