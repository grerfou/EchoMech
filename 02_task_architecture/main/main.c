#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_system.h"
#include "task_sensor.h"
#include "task_network.h"
#include "task_watchdog.h"

static const char *TAG = "MAIN";

/* Queue partagée entre task_sensor et task_network */
QueueHandle_t xQueueSensor = NULL;

void app_main(void)
{
    ESP_LOGI(TAG, "=== EchoMesh v0.2 - Multi-taches ===");
    ESP_LOGI(TAG, "Heap libre : %lu bytes", esp_get_free_heap_size());

    /* Création de la queue : 5 éléments de type sensor_data_t */
    xQueueSensor = xQueueCreate(5, sizeof(sensor_data_t));
    if (xQueueSensor == NULL) {
        ESP_LOGE(TAG, "Erreur creation queue !");
        return;
    }
    ESP_LOGI(TAG, "Queue creee (5 elements)");

    /* Création des tâches
     * Paramètres xTaskCreate :
     * (fonction, nom, stack_bytes, params, priorité, handle) */
    xTaskCreate(task_sensor,   "task_sensor",   4096, NULL, 1, NULL);
    xTaskCreate(task_network,  "task_network",  4096, NULL, 2, NULL);
    xTaskCreate(task_watchdog, "task_watchdog", 2048, NULL, 3, NULL);

    ESP_LOGI(TAG, "3 taches lancees");
    ESP_LOGI(TAG, "Heap libre apres init : %lu bytes",
             esp_get_free_heap_size());
}
