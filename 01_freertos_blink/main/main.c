#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"

static const char *TAG = "ECHOMESH";

void task_blink(void *pvParameters){
    
    uint32_t count = 0;

    ESP_LOGI(TAG, "task_blink demarre");

    while(1){
        count++;

        ESP_LOGI(TAG, "LED ON [tick=%lu]", count);
        vTaskDelay(pdMS_TO_TICKS(500));

        ESP_LOGI(TAG, "LED OFF [tick=%lu]", count);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void app_main(void){
    ESP_LOGI(TAG, "=== EchoMesh v0.1 - FreeRTOS Blink ===");
    ESP_LOGI(TAG, "Heap libre : %lu bytes", esp_get_free_heap_size());

    xTaskCreate(
            task_blink, // Fonction tache
            "task_blink", // Nom debug
            2048, // Taille stack (bytes)
            NULL,  // Parametre passée a la tache 
            1, // Priorité (1 = basse)
            NULL
    );

    ESP_LOGI(TAG, "Tache lancée, sheduler actif");
}
