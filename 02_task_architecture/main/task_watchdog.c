#include "task_watchdog.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "WATCHDOG";

void task_watchdog(void *pvParameters)
{
    ESP_LOGI(TAG, "task_watchdog demarree");

    while (1) {
        /* Rapport système toutes les 30 secondes */
        ESP_LOGI(TAG, "--- Rapport système ---");
        ESP_LOGI(TAG, "Heap libre    : %lu bytes",
                 esp_get_free_heap_size());
        ESP_LOGI(TAG, "Heap min ever : %lu bytes",
                 esp_get_minimum_free_heap_size());

        vTaskDelay(pdMS_TO_TICKS(30000));
    }
}
