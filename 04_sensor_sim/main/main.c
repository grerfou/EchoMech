#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "sensor_sim.h"

static const char *TAG = "MAIN";

void task_sensor(void *pvParameters)
{
    sensor_data_t data;
    uint32_t count = 0;
    esp_err_t ret;

    while (1) {
        ret = sensor_sim_read(&data);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Erreur lecture : %s", esp_err_to_name(ret));
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        count++;
        ESP_LOGI(TAG, "#%04lu | temp=%.2f°C | hum=%.1f%% | pres=%.1f hPa | lux=%.0f lx | ts=%lld us",
                 count,
                 data.temp,
                 data.humidity,
                 data.pressure,
                 data.lux,
                 data.timestamp_us);

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== EchoMesh v0.3 - Sensor Sim ===");
    ESP_LOGI(TAG, "Heap libre : %lu bytes", esp_get_free_heap_size());

    ESP_ERROR_CHECK(sensor_sim_init());

    xTaskCreate(task_sensor, "task_sensor", 4096, NULL, 1, NULL);

    ESP_LOGI(TAG, "Tache lancee");
}
