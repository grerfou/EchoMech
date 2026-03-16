#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "wifi_manager.h"
#include "ntp_manager.h"
#include "mqtt_manager.h"
#include "sensor_sim.h"

static const char *TAG = "MAIN";

void task_sensor(void *pvParameters)
{
    sensor_data_t data;
    uint32_t count       = 0;
    char timestamp[32];
    char payload[128];
    TickType_t last_wake = xTaskGetTickCount();

    /* Attente Wi-Fi */
    EventGroupHandle_t eg = wifi_manager_get_event_group();
    xEventGroupWaitBits(eg, WIFI_CONNECTED_BIT,
                        pdFALSE, pdTRUE, portMAX_DELAY);

    ESP_LOGI(TAG, "Demarrage lectures capteur");

    while (1) {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(5000));

        sensor_sim_read(&data);
        ntp_manager_get_timestamp(timestamp, sizeof(timestamp));
        count++;

        ESP_LOGI(TAG, "#%04lu | temp=%.2f°C | hum=%.1f%% | "
                 "pres=%.1f hPa | lux=%.0f lx | ts=%s",
                 count, data.temp, data.humidity,
                 data.pressure, data.lux, timestamp);

        /* Publication sur topics séparés */
        if (mqtt_manager_is_connected()) {
            snprintf(payload, sizeof(payload), "%.2f", data.temp);
            mqtt_manager_publish("sensors/temperature", payload, 1);

            snprintf(payload, sizeof(payload), "%.1f", data.pressure);
            mqtt_manager_publish("sensors/pressure", payload, 1);

            snprintf(payload, sizeof(payload), "%.0f", data.lux);
            mqtt_manager_publish("sensors/lux", payload, 1);
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== EchoMesh v0.7 - MQTT Local ===");
    ESP_LOGI(TAG, "Heap libre : %lu bytes", esp_get_free_heap_size());

    ESP_ERROR_CHECK(sensor_sim_init());

    esp_err_t ret = wifi_manager_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Wi-Fi KO — arret");
        return;
    }

    ntp_manager_init();

    ret = mqtt_manager_init();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "MQTT KO — lectures sans publication");
    }

    xTaskCreate(task_sensor, "task_sensor", 4096, NULL, 1, NULL);

    ESP_LOGI(TAG, "Systeme operationnel");
}
