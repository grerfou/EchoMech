#include "wifi_manager.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include <string.h>

static const char *TAG = "WIFI";

/* EventGroup partagé */
static EventGroupHandle_t s_wifi_event_group = NULL;

/* Compteur de tentatives de reconnexion */
static int s_retry_count = 0;

/* Backoff exponentiel : délai en ms entre tentatives */
static int s_backoff_ms  = 1000;
#define BACKOFF_MAX_MS   30000

/* ------------------------------------------------------------------ */
/*  Handler d'événements Wi-Fi et IP                                   */
/* ------------------------------------------------------------------ */
static void event_handler(void *arg, esp_event_base_t event_base,
                           int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "Wi-Fi démarré, connexion en cours...");
        esp_wifi_connect();

    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t *disc =
            (wifi_event_sta_disconnected_t *)event_data;

        ESP_LOGW(TAG, "Déconnecté (reason=%d), tentative %d/%d dans %dms",
                 disc->reason, s_retry_count + 1,
                 CONFIG_WIFI_MAX_RETRY, s_backoff_ms);

        if (s_retry_count < CONFIG_WIFI_MAX_RETRY) {
            /* Backoff exponentiel */
            vTaskDelay(pdMS_TO_TICKS(s_backoff_ms));
            s_backoff_ms = (s_backoff_ms * 2 > BACKOFF_MAX_MS)
                           ? BACKOFF_MAX_MS : s_backoff_ms * 2;
            s_retry_count++;
            esp_wifi_connect();
        } else {
            ESP_LOGE(TAG, "Echec définitif après %d tentatives",
                     CONFIG_WIFI_MAX_RETRY);
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }

    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;

        ESP_LOGI(TAG, "IP obtenue : " IPSTR,
                 IP2STR(&event->ip_info.ip));

        /* Reset du compteur et du backoff pour la prochaine déconnexion */
        s_retry_count = 0;
        s_backoff_ms  = 1000;

        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

/* ------------------------------------------------------------------ */
/*  API publique                                                        */
/* ------------------------------------------------------------------ */
esp_err_t wifi_manager_init(void)
{
    /* NVS requis par le stack Wi-Fi */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS corrompu, effacement...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /* Enregistrement des handlers */
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, NULL));

    /* Configuration SSID / Password depuis menuconfig */
    wifi_config_t wifi_config = {
        .sta = {
            .ssid     = CONFIG_WIFI_SSID,
            .password = CONFIG_WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Connexion à '%s'...", CONFIG_WIFI_SSID);

    /* Attente bloquante : connecté ou échec définitif */
    EventBits_t bits = xEventGroupWaitBits(
        s_wifi_event_group,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE,   /* ne pas effacer les bits */
        pdFALSE,   /* attendre UN des deux bits (OR) */
        portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connecté avec succès !");
        return ESP_OK;
    }

    ESP_LOGE(TAG, "Connexion échouée");
    return ESP_FAIL;
}

EventGroupHandle_t wifi_manager_get_event_group(void)
{
    return s_wifi_event_group;
}
