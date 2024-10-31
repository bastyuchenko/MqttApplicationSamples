#include <iostream>
#include <string>
#include <thread>
#include <chrono>

#include "./../sensors/bme280.c"
#include "./../sensors/bme280.h"
#include "logging.h"
#include "mosquitto.h"
#include "mqtt_callbacks.h"
#include "mqtt_setup.h"

constexpr char PUB_TOPIC[] = "devices/rasp";
constexpr int QOS_LEVEL = 1;
constexpr int MQTT_VERSION = MQTT_PROTOCOL_V311;

int main(int argc, char* argv[])
{
    std::string payload;
    struct mosquitto* mosq;
    int result = MOSQ_ERR_SUCCESS;

    mqtt_client_obj obj = { 0 };
    obj.mqtt_version = MQTT_VERSION;

    try {
        mosq = mqtt_client_init(true, argv[1], on_connect, &obj);
        if (!mosq) {
            throw std::runtime_error("Failed to initialize MQTT client.");
        }

        result = mosquitto_connect_bind_v5(mosq, obj.hostname, obj.tcp_port, obj.keep_alive_in_seconds, nullptr, nullptr);
        if (result != MOSQ_ERR_SUCCESS) {
            throw std::runtime_error("Failed to connect: " + std::string(mosquitto_strerror(result)));
        }

        result = mosquitto_loop_start(mosq);
        if (result != MOSQ_ERR_SUCCESS) {
            throw std::runtime_error("Failure starting mosquitto loop: " + std::string(mosquitto_strerror(result)));
        }

        while (keep_running) {
            Bme280Data bme280 = readBME280();

            payload = "Temp: " + std::to_string(bme280.temperature) + "°C Pressure: " + std::to_string(bme280.pressure) + " hPa Humidity: " + std::to_string(bme280.humidity) + "%.";
            std::cout << payload << std::endl;

            result = mosquitto_publish_v5(mosq, nullptr, PUB_TOPIC, static_cast<int>(payload.length()), payload.c_str(), QOS_LEVEL, false, nullptr);
            if (result != MOSQ_ERR_SUCCESS) {
                throw std::runtime_error("Failure while publishing: " + std::string(mosquitto_strerror(result)));
            }

            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        result = MOSQ_ERR_UNKNOWN;
    }

    if (mosq) {
        mosquitto_disconnect_v5(mosq, result, nullptr);
        mosquitto_loop_stop(mosq, false);
        mosquitto_destroy(mosq);
    }
    mosquitto_lib_cleanup();

    return result;
}