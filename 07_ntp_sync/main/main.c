#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "wifi_manager.h"
#include "ntp_manager.h"
#include "sensor_sim.h"

static const char *TAG = "MAIN";

void task_sensor(void *pvParameters)
{
    sensor_data_t data;
    uint32_t count       = 0;
    char timestamp[32];
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
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== EchoMesh v0.6 - NTP Sync ===");
    ESP_LOGI(TAG, "Heap libre : %lu bytes", esp_get_free_heap_size());

    ESP_ERROR_CHECK(sensor_sim_init());

    /* Wi-Fi */
    esp_err_t ret = wifi_manager_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Wi-Fi KO — arret");
        return;
    }

    /* NTP — non bloquant pour le reste du système */
    ntp_manager_init();

    xTaskCreate(task_sensor, "task_sensor", 4096, NULL, 1, NULL);

    ESP_LOGI(TAG, "Systeme operationnel");
}
