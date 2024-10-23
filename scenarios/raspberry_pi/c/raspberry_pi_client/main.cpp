/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "./../sensors/bme280.c"
#include "./../sensors/bme280.h"
#include "logging.h"
#include "mosquitto.h"
#include "mqtt_callbacks.h"
#include "mqtt_setup.h"

#define PUB_TOPIC "devices/rasp"
#define QOS_LEVEL 1
#define MQTT_VERSION MQTT_PROTOCOL_V311

/*
 * This client sends and receives messages to/from the Broker.
 * @return MOSQ_ERR_SUCCESS (0) on success, other enum mosq_err_t on failure
 */
int main(int argc, char* argv[])
{
  char payload[100];
  struct mosquitto* mosq;
  int result = MOSQ_ERR_SUCCESS;

  mqtt_client_obj obj = { 0 };
  obj.mqtt_version = MQTT_VERSION;

  if ((mosq = mqtt_client_init(true, argv[1], on_connect, &obj)) == NULL)
  {
    result = MOSQ_ERR_UNKNOWN;
  }
  else if (
      (result = mosquitto_connect_bind_v5(
           mosq, obj.hostname, obj.tcp_port, obj.keep_alive_in_seconds, NULL, NULL))
      != MOSQ_ERR_SUCCESS)
  {
    LOG_ERROR("Failed to connect: %s", mosquitto_strerror(result));
    result = MOSQ_ERR_UNKNOWN;
  }
  else if ((result = mosquitto_loop_start(mosq)) != MOSQ_ERR_SUCCESS)
  {
    LOG_ERROR("Failure starting mosquitto loop: %s", mosquitto_strerror(result));
    result = MOSQ_ERR_UNKNOWN;
  }
  else
  {
    while (keep_running)
    {
      Bme280Data bme280 = readBME280();

      snprintf(
          payload,
          sizeof(payload),
          "Temp: %.2f°C Pressure: %.2f hPa Humidity: %.2f.",
          bme280.temperature,
          bme280.pressure,
          bme280.humidity);

      result = mosquitto_publish_v5(
          mosq, NULL, PUB_TOPIC, (int)strlen(payload), payload, QOS_LEVEL, false, NULL);

      if (result != MOSQ_ERR_SUCCESS)
      {
        LOG_ERROR("Failure while publishing: %s", mosquitto_strerror(result));
      }

      sleep(5);
    }
  }

  if (mosq != NULL)
  {
    mosquitto_disconnect_v5(mosq, result, NULL);
    mosquitto_loop_stop(mosq, false);
    mosquitto_destroy(mosq);
  }
  mosquitto_lib_cleanup();
  return result;
}
