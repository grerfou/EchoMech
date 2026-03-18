#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "cJSON.h"
#include "wifi_manager.h"
#include "ntp_manager.h"
#include "mqtt_manager.h"
#include "sensor_sim.h"

static const char *TAG = "MAIN";

/* ------------------------------------------------------------------ */
/*  RTC memory — survit au deep sleep                                  */
/* ------------------------------------------------------------------ */
RTC_DATA_ATTR static uint32_t s_boot_count = 0;
RTC_DATA_ATTR static float    s_last_temp  = 0.0f;

/* ------------------------------------------------------------------ */
/*  Construction payload JSON                                           */
/* ------------------------------------------------------------------ */
static char *build_json(const sensor_data_t *data,
                        const char *timestamp,
                        const char *device_id,
                        uint32_t boot_count)
{
    cJSON *root = cJSON_CreateObject();
    if (!root) return NULL;

    cJSON_AddNumberToObject(root, "temp",       (double)data->temp);
    cJSON_AddNumberToObject(root, "humidity",   (double)data->humidity);
    cJSON_AddNumberToObject(root, "pressure",   (double)data->pressure);
    cJSON_AddNumberToObject(root, "lux",        (double)data->lux);
    cJSON_AddStringToObject(root, "ts",         timestamp);
    cJSON_AddStringToObject(root, "device",     device_id);
    cJSON_AddNumberToObject(root, "boot_count", (double)boot_count);

    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json;
}

/* ------------------------------------------------------------------ */
/*  app_main — exécuté à chaque réveil                                 */
/* ------------------------------------------------------------------ */
void app_main(void)
{
    int64_t t_start = esp_timer_get_time();

    s_boot_count++;

    /* Cause du réveil */
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    if (cause == ESP_SLEEP_WAKEUP_TIMER) {
        ESP_LOGI(TAG, "=== Réveil timer #%lu ===", s_boot_count);
    } else {
        ESP_LOGI(TAG, "=== Premier démarrage #%lu ===", s_boot_count);
    }

    ESP_LOGI(TAG, "Heap libre : %lu bytes", esp_get_free_heap_size());

    /* Device ID basé sur MAC */
    uint8_t mac[6];
    char device_id[20];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(device_id, sizeof(device_id),
             "echomesh-%02X%02X%02X", mac[3], mac[4], mac[5]);

    /* Lecture capteur */
    ESP_ERROR_CHECK(sensor_sim_init());
    sensor_data_t data;
    sensor_sim_read(&data);
    s_last_temp = data.temp;

    ESP_LOGI(TAG, "Capteur : temp=%.2f°C | hum=%.1f%% | "
             "pres=%.1f hPa | lux=%.0f lx",
             data.temp, data.humidity, data.pressure, data.lux);

    /* Connexion Wi-Fi */
    esp_err_t ret = wifi_manager_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Wi-Fi KO — on dort quand meme");
        goto sleep;
    }

    /* NTP — on accepte le fallback uptime si timeout */
    ntp_manager_init();

    /* MQTT */
    ret = mqtt_manager_init();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "MQTT KO — on dort quand meme");
        goto sleep;
    }

    {
        char timestamp[32];
        ntp_manager_get_timestamp(timestamp, sizeof(timestamp));

        char *json = build_json(&data, timestamp, device_id, s_boot_count);
        if (json) {
            mqtt_manager_publish("sensors/data", json, 1);

            /* Attente PUBACK QoS1 — 500ms max */
            vTaskDelay(pdMS_TO_TICKS(500));
            free(json);
        }
    }

    {
        int64_t t_elapsed = (esp_timer_get_time() - t_start) / 1000;
        ESP_LOGI(TAG, "Sequence complete en %lld ms", t_elapsed);
    }

sleep:
    ESP_LOGI(TAG, "Deep sleep %d secondes...",
             CONFIG_SLEEP_DURATION_SEC);
    esp_deep_sleep(CONFIG_SLEEP_DURATION_SEC * 1000000ULL);
}
