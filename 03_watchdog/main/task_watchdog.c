#include "task_watchdog.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "WATCHDOG";

void task_watchdog(void *pvParameters)
{
    esp_err_t err;

    ESP_LOGI(TAG, "task_watchdog demarree");

    /* Enregistrement de cette tâche auprès du TWDT */
    err = esp_task_wdt_add(NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Erreur enregistrement TWDT : %s",
                 esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "Tache enregistree dans le TWDT");
    }

    while (1) {
        /* Reset du watchdog — preuve que la tâche est vivante */
        esp_task_wdt_reset();

        /* Rapport système toutes les 30 secondes */
        ESP_LOGI(TAG, "--- Rapport système ---");
        ESP_LOGI(TAG, "Heap libre    : %lu bytes",
                 esp_get_free_heap_size());
        ESP_LOGI(TAG, "Heap min ever : %lu bytes",
                 esp_get_minimum_free_heap_size());

        /* Sleep par tranches de 5s avec reset WDT */
        for (int i = 0; i < 6; i++) {
            esp_task_wdt_reset();
            vTaskDelay(pdMS_TO_TICKS(5000));
        }
    }
}
