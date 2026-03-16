#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "sensor_sim.h"

static const char *TAG_SENSOR  = "SENSOR";
static const char *TAG_NETWORK = "NETWORK";
static const char *TAG_MAIN    = "MAIN";

/* Intervalle de lecture en millisecondes — modifiable ici */
#define SENSOR_PERIOD_MS  5000

/* Queue partagée : 5 éléments max */
QueueHandle_t xQueueSensor = NULL;

/* ------------------------------------------------------------------ */
/*  task_sensor — lecture périodique précise avec vTaskDelayUntil      */
/* ------------------------------------------------------------------ */
void task_sensor(void *pvParameters)
{
    sensor_data_t data;
    uint32_t count       = 0;
    uint32_t err_count   = 0;
    TickType_t last_wake = xTaskGetTickCount();

    ESP_LOGI(TAG_SENSOR, "Demarree, periode %d ms", SENSOR_PERIOD_MS);

    while (1) {
        /* Attente jusqu'au prochain tick fixe — période exacte */
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(SENSOR_PERIOD_MS));

        esp_err_t ret = sensor_sim_read(&data);
        if (ret != ESP_OK) {
            err_count++;
            ESP_LOGE(TAG_SENSOR, "Erreur lecture #%lu : %s",
                     err_count, esp_err_to_name(ret));
            /* La tâche continue — on ne bloque pas sur une erreur */
            continue;
        }

        count++;
        ESP_LOGI(TAG_SENSOR, "#%04lu | temp=%.2f°C | hum=%.1f%% | "
                 "pres=%.1f hPa | lux=%.0f lx",
                 count, data.temp, data.humidity,
                 data.pressure, data.lux);

        /* Envoi dans la queue sans attente :
         * si pleine on log un warning mais on ne bloque pas la période */
        if (xQueueSend(xQueueSensor, &data, 0) != pdTRUE) {
            ESP_LOGW(TAG_SENSOR, "Queue pleine, donnee perdue !");
        }
    }
}

/* ------------------------------------------------------------------ */
/*  task_network — consommateur de la queue                            */
/* ------------------------------------------------------------------ */
void task_network(void *pvParameters)
{
    sensor_data_t data;

    ESP_LOGI(TAG_NETWORK, "Demarree, attente donnees...");

    while (1) {
        /* Attente bloquante — libère le CPU tant que la queue est vide */
        if (xQueueReceive(xQueueSensor, &data, pdMS_TO_TICKS(10000)) == pdTRUE) {
            ESP_LOGI(TAG_NETWORK, "Recu -> temp=%.2f°C | pres=%.1f hPa | "
                     "lux=%.0f lx | ts=%lld us",
                     data.temp, data.pressure, data.lux, data.timestamp_us);

            /* Placeholder MQTT — sera remplacé en phase 4 */
            ESP_LOGI(TAG_NETWORK, "MQTT publish [simulation] OK");
        } else {
            ESP_LOGW(TAG_NETWORK, "Timeout 10s — aucune donnee recue");
        }
    }
}

/* ------------------------------------------------------------------ */
/*  app_main                                                           */
/* ------------------------------------------------------------------ */
void app_main(void)
{
    ESP_LOGI(TAG_MAIN, "=== EchoMesh v0.4 - Sensor Task ===");
    ESP_LOGI(TAG_MAIN, "Heap libre : %lu bytes", esp_get_free_heap_size());

    ESP_ERROR_CHECK(sensor_sim_init());

    xQueueSensor = xQueueCreate(5, sizeof(sensor_data_t));
    if (xQueueSensor == NULL) {
        ESP_LOGE(TAG_MAIN, "Echec creation queue !");
        return;
    }
    ESP_LOGI(TAG_MAIN, "Queue creee (5 elements)");

    xTaskCreate(task_sensor,  "task_sensor",  4096, NULL, 1, NULL);
    xTaskCreate(task_network, "task_network", 4096, NULL, 2, NULL);

    ESP_LOGI(TAG_MAIN, "2 taches lancees");
    ESP_LOGI(TAG_MAIN, "Heap apres init : %lu bytes",
             esp_get_free_heap_size());
}
