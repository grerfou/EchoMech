#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "cJSON.h"
#include "wifi_manager.h"
#include "ntp_manager.h"
#include "mqtt_manager.h"
#include "sensor_sim.h"

static const char *TAG = "MAIN";

static char *build_json_payload(const sensor_data_t *data,
                                 const char *timestamp,
                                 const char *device_id)
{
    cJSON *root = cJSON_CreateObject();
    if (!root) return NULL;

    cJSON_AddNumberToObject(root, "temp",     (double)data->temp);
    cJSON_AddNumberToObject(root, "humidity", (double)data->humidity);
    cJSON_AddNumberToObject(root, "pressure", (double)data->pressure);
    cJSON_AddNumberToObject(root, "lux",      (double)data->lux);
    cJSON_AddStringToObject(root, "ts",       timestamp);
    cJSON_AddStringToObject(root, "device",   device_id);

    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json_str;
}

void task_sensor(void *pvParameters)
{
    sensor_data_t data;
    uint32_t count       = 0;
    char timestamp[32];
    char device_id[20];
    TickType_t last_wake = xTaskGetTickCount();

    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(device_id, sizeof(device_id),
             "echomesh-%02X%02X%02X", mac[3], mac[4], mac[5]);

    EventGroupHandle_t eg = wifi_manager_get_event_group();
    xEventGroupWaitBits(eg, WIFI_CONNECTED_BIT,
                        pdFALSE, pdTRUE, portMAX_DELAY);

    ESP_LOGI(TAG, "Device ID : %s", device_id);

    while (1) {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(5000));

        sensor_sim_read(&data);
        ntp_manager_get_timestamp(timestamp, sizeof(timestamp));
        count++;

        ESP_LOGI(TAG, "#%04lu | temp=%.2f°C | hum=%.1f%% | "
                 "pres=%.1f hPa | lux=%.0f lx",
                 count, data.temp, data.humidity,
                 data.pressure, data.lux);

        char *json = build_json_payload(&data, timestamp, device_id);
        if (!json) {
            ESP_LOGE(TAG, "Echec allocation JSON");
            continue;
        }

        ESP_LOGI(TAG, "Payload : %s", json);

        /* Toujours publier — mqtt_manager gère le buffer offline */
        mqtt_manager_publish("sensors/data", json, 1);

        free(json);
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== EchoMesh v0.9 - MQTT Reconnect ===");
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
        ESP_LOGW(TAG, "MQTT KO — buffer offline actif des le debut");
    }

    xTaskCreate(task_sensor, "task_sensor", 4096, NULL, 1, NULL);

    ESP_LOGI(TAG, "Systeme operationnel");
}
