#include "task_network.h"
#include "task_sensor.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "NETWORK";

void task_network(void *pvParameters)
{
    sensor_data_t data;

    ESP_LOGI(TAG, "task_network demarree");

    while (1) {
        /* Attente bloquante d'une donnée dans la queue (max 10s) */
        if (xQueueReceive(xQueueSensor, &data, pdMS_TO_TICKS(10000)) == pdTRUE) {

            ESP_LOGI(TAG, "Recu : temp=%.1f°C hum=%.1f%% ts=%llums",
                     data.temp,
                     data.humidity,
                     data.timestamp_ms);

            /* Simulation publication MQTT */
            ESP_LOGI(TAG, "MQTT publish -> sensors/temperature : %.1f",
                     data.temp);
            ESP_LOGI(TAG, "MQTT publish -> sensors/humidity    : %.1f",
                     data.humidity);

        } else {
            ESP_LOGW(TAG, "Timeout: aucune donnee recue depuis 10s");
        }
    }
}
