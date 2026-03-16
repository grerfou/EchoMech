#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "wifi_manager.h"
#include "sensor_sim.h"

static const char *TAG = "MAIN";

void task_sensor(void *pvParameters)
{
    sensor_data_t data;
    uint32_t count       = 0;
    TickType_t last_wake = xTaskGetTickCount();

    /* Attente que le Wi-Fi soit connecté avant de démarrer */
    EventGroupHandle_t eg = wifi_manager_get_event_group();
    xEventGroupWaitBits(eg, WIFI_CONNECTED_BIT,
                        pdFALSE, pdTRUE, portMAX_DELAY);

    ESP_LOGI(TAG, "Wi-Fi OK — démarrage lectures capteur");

    while (1) {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(5000));

        sensor_sim_read(&data);
        count++;

        ESP_LOGI(TAG, "#%04lu | temp=%.2f°C | hum=%.1f%% | "
                 "pres=%.1f hPa | lux=%.0f lx",
                 count, data.temp, data.humidity,
                 data.pressure, data.lux);
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== EchoMesh v0.5 - WiFi Connect ===");
    ESP_LOGI(TAG, "Heap libre : %lu bytes", esp_get_free_heap_size());

    ESP_ERROR_CHECK(sensor_sim_init());

    /* Connexion Wi-Fi — bloquant jusqu'à IP obtenue */
    esp_err_t ret = wifi_manager_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Wi-Fi KO — arrêt");
        return;
    }

    xTaskCreate(task_sensor, "task_sensor", 4096, NULL, 1, NULL);

    ESP_LOGI(TAG, "Système opérationnel");
}
