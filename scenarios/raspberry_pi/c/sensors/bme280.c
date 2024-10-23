#include "bme280.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define HUMIDITY "/sys/bus/iio/devices/iio:device0/in_humidityrelative_input"
#define HUMIDITY_RATIO "/sys/bus/iio/devices/iio:device0/in_humidityrelative_oversampling_ratio"
#define PRESSURE "/sys/bus/iio/devices/iio:device0/in_pressure_input"
#define PRESSURE_RATIO "/sys/bus/iio/devices/iio:device0/in_pressure_oversampling_ratio"
#define TEMPERATURE "/sys/bus/iio/devices/iio:device0/in_temp_input"
#define TEMPERATURE_RATIO "/sys/bus/iio/devices/iio:device0/in_temp_oversampling_ratio"

int readGaudge(float* rawValue, char* filePath)
{
  FILE* inputStream;
  inputStream = fopen(filePath, "r");
  if (inputStream == NULL)
  {
    perror("Failed to open file");
    return EXIT_FAILURE;
  }

  if (fscanf(inputStream, "%f", rawValue) != 1)
  {
    perror("Failed to read file");
    fclose(inputStream);
    return EXIT_FAILURE;
  }

  fclose(inputStream);
}

Bme280Data readBME280()
{
  Bme280Data readings;

  float rawHumidity;
  float rawPressure;
  float rawTemperature;

  readGaudge(&rawHumidity, HUMIDITY);
  readGaudge(&rawPressure, PRESSURE);
  readGaudge(&rawTemperature, TEMPERATURE);

  readings.humidity = rawHumidity;

  readings.pressure = rawPressure * 10.0;

  readings.temperature = rawTemperature / 1000.0;

  printf("Raw Humidity: %d\n", readings.humidity );
  printf("Raw Pressure: %f\n", readings.pressure );
  printf("Raw Temperature: %f\n", readings.temperature );

  return readings;
}
