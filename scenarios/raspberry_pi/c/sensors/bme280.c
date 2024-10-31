#include "bme280.h"
#include <stdio.h>
#include <stdlib.h>

// /sys/bus/iio/devices/iio:device0 is link to -> /sys/devices/platform/soc/fe804000.i2c/i2c-1/1-0076/iio:device0
#define HUMIDITY "/sys/bus/iio/devices/iio:device0/in_humidityrelative_input"
#define PRESSURE "/sys/bus/iio/devices/iio:device0/in_pressure_input"
#define TEMPERATURE "/sys/bus/iio/devices/iio:device0/in_temp_input"

static int readGauge(float *value, const char *filePath) {
    FILE *inputStream = fopen(filePath, "r");
    if (inputStream == NULL) {
        perror("Failed to open file");
        return EXIT_FAILURE;
    }

    if (fscanf(inputStream, "%f", value) != 1) {
        perror("Failed to read file");
        fclose(inputStream);
        return EXIT_FAILURE;
    }

    fclose(inputStream);
    return EXIT_SUCCESS;
}

Bme280Data readBME280() {
    Bme280Data readings;
    float rawHumidity = 0.0f;
    float rawPressure = 0.0f;
    float rawTemperature = 0.0f;

    if (readGauge(&rawHumidity, HUMIDITY) == EXIT_FAILURE ||
        readGauge(&rawPressure, PRESSURE) == EXIT_FAILURE ||
        readGauge(&rawTemperature, TEMPERATURE) == EXIT_FAILURE) {
        fprintf(stderr, "Error reading sensors\n");
        return readings;
    }

    readings.humidity = rawHumidity;
    readings.pressure = rawPressure * 10.0f;
    readings.temperature = rawTemperature / 1000.0f;

    printf("Humidity: %.2f\n", readings.humidity);
    printf("Pressure: %.2f hPa\n", readings.pressure);
    printf("Temperature: %.2f°C\n", readings.temperature);

    return readings;
}