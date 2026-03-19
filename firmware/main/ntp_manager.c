#include "ntp_manager.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <time.h>
#include <string.h>
#include <stdio.h>

static const char *TAG = "NTP";

#define NTP_SERVER        "pool.ntp.org"
#define NTP_TIMEOUT_MS    10000
#define NTP_CHECK_DELAY   500

static bool s_synced = false;

/* Callback appelé par le stack SNTP à chaque sync */
static void sntp_sync_callback(struct timeval *tv)
{
    s_synced = true;
    ESP_LOGI(TAG, "Heure synchronisee (epoch=%lld)", (long long)tv->tv_sec);
}

esp_err_t ntp_manager_init(void)
{
    ESP_LOGI(TAG, "Demarrage sync NTP -> %s", NTP_SERVER);

    /* Timezone Europe/Paris */
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
    tzset();

    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, NTP_SERVER);
    sntp_set_time_sync_notification_cb(sntp_sync_callback);
    esp_sntp_init();

    /* Attente sync avec timeout */
    int elapsed = 0;
    while (!s_synced && elapsed < NTP_TIMEOUT_MS) {
        vTaskDelay(pdMS_TO_TICKS(NTP_CHECK_DELAY));
        elapsed += NTP_CHECK_DELAY;
        ESP_LOGI(TAG, "Attente sync... (%d/%d ms)", elapsed, NTP_TIMEOUT_MS);
    }

    if (!s_synced) {
        ESP_LOGW(TAG, "Timeout NTP — fallback uptime actif");
        return ESP_ERR_TIMEOUT;
    }

    /* Affichage heure locale pour vérification */
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &timeinfo);
    ESP_LOGI(TAG, "Heure locale : %s", buf);

    return ESP_OK;
}

bool ntp_manager_is_synced(void)
{
    return s_synced;
}

void ntp_manager_get_timestamp(char *buf, size_t size)
{
    if (!s_synced) {
        /* Fallback : uptime en µs */
        snprintf(buf, size, "UPTIME:%lldus", esp_timer_get_time());
        return;
    }

    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    strftime(buf, size, "%Y-%m-%dT%H:%M:%S%z", &timeinfo);
}
