#include "task_sensor.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_random.h"

static const char *TAG = "SENSOR";

void task_sensor(void *pvParameters){
    
    sensor_data_t data;
    uint32_t lecture_count = 0;

    ESP_LOGI(TAG, "task sensor start");

    while(1){

        // Simumlation donnée capteur
        data.temp = 20.0f + (float)(esp_random() % 100) / 10.0f;
        data.humidity = 40.0f + (float)(esp_random() % 400) / 10.0f;
        //data.timestamp_ms = 40.0f + (uint64_t)(esp_random() * 1000);
        data.timestamp_ms = (uint64_t)xTaskGetTickCount() * portTICK_PERIOD_MS;

        lecture_count++;
        ESP_LOGI(TAG, "Lecture #%lu : temp=%.1f°C hum=%.1f%%",
                lecture_count,
                data.temp,
                data.humidity);

        // Envoie dans la queu - 0 = sans attente de peine
        if (xQueueSend(xQueueSensor, &data, 0) != pdTRUE){
            ESP_LOGW(TAG, "Queue pleine, donnee perdue !");
        }

        // Lecture  toutes les 5 secondes
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
