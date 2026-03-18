#include "ota_manager.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_https_ota.h"
#include "esp_app_format.h"
#include "esp_crt_bundle.h"
#include <string.h>

static const char *TAG = "OTA";

ota_result_t ota_manager_start(const char *url,
                                const char *version,
                                const char *sha256)
{
    if (!url || strlen(url) == 0) {
        ESP_LOGE(TAG, "URL invalide");
        return OTA_RESULT_DOWNLOAD_FAILED;
    }

    /* Vérification version courante */
    const esp_app_desc_t *current = esp_app_get_description();
    ESP_LOGI(TAG, "Version courante : %s", current->version);
    ESP_LOGI(TAG, "Version cible    : %s", version ? version : "inconnue");

    if (version && strcmp(current->version, version) == 0) {
        ESP_LOGW(TAG, "Meme version — OTA ignore");
        return OTA_RESULT_SAME_VERSION;
    }

    ESP_LOGI(TAG, "Demarrage OTA depuis : %s", url);

    esp_http_client_config_t http_cfg = {
        .url                  = url,
        .crt_bundle_attach    = esp_crt_bundle_attach,
        .timeout_ms           = 30000,
        .keep_alive_enable    = true,
        .buffer_size          = 4096,
        .buffer_size_tx       = 1024,
    };

    esp_https_ota_config_t ota_cfg = {
        .http_config = &http_cfg,
    };

    ESP_LOGI(TAG, "Telechargement en cours...");

    esp_err_t ret = esp_https_ota(&ota_cfg);

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "OTA reussi — reboot dans 1s");
        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_restart();
        return OTA_RESULT_SUCCESS; /* jamais atteint */
    }

    ESP_LOGE(TAG, "OTA echoue : %s", esp_err_to_name(ret));
    return OTA_RESULT_DOWNLOAD_FAILED;
}
