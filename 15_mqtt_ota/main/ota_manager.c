#include "ota_manager.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_https_ota.h"
#include "esp_app_format.h"
#include "esp_crt_bundle.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mbedtls/sha256.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "OTA";

/* ------------------------------------------------------------------ */
/*  Utilitaire : convertit un hash binaire (32 octets) en hex string   */
/* ------------------------------------------------------------------ */
static void sha256_to_hex(const uint8_t *hash, char *out_hex)
{
    for (int i = 0; i < 32; i++) {
        sprintf(out_hex + (i * 2), "%02x", hash[i]);
    }
    out_hex[64] = '\0';
}

/* ------------------------------------------------------------------ */
/*  API publique                                                        */
/* ------------------------------------------------------------------ */
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
        .url               = url,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms        = 30000,
        .keep_alive_enable = true,
        .buffer_size       = 4096,
        .buffer_size_tx    = 1024,
    };

    esp_https_ota_config_t ota_cfg = {
        .http_config = &http_cfg,
    };

    /* Initialisation SHA256 si hash attendu */
    mbedtls_sha256_context sha_ctx;
    bool verify_hash = (sha256 && strlen(sha256) == 64);
    if (verify_hash) {
        mbedtls_sha256_init(&sha_ctx);
        mbedtls_sha256_starts(&sha_ctx, 0); /* 0 = SHA256, 1 = SHA224 */
    }

    /* API avancée */
    esp_https_ota_handle_t ota_handle = NULL;
    esp_err_t ret = esp_https_ota_begin(&ota_cfg, &ota_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_https_ota_begin echoue : %s", esp_err_to_name(ret));
        if (verify_hash) mbedtls_sha256_free(&sha_ctx);
        return OTA_RESULT_DOWNLOAD_FAILED;
    }

    /* Téléchargement chunk par chunk avec calcul SHA256 */
    ESP_LOGI(TAG, "Telechargement en cours...");

    //esp_ota_handle_t update_handle = esp_https_ota_get_image_upgrade_ctx(ota_handle);
    //const esp_partition_t *update_part = esp_ota_get_next_update_partition(NULL);
    
    const esp_partition_t *update_part = esp_ota_get_next_update_partition(NULL);
    uint8_t  chunk[4096];
    int32_t  offset = 0;

    while (true) {
        ret = esp_https_ota_perform(ota_handle);

        if (verify_hash) {
            /* Lit les octets fraîchement écrits dans la partition */
            int32_t len = esp_https_ota_get_image_len_read(ota_handle);
            if (len > offset) {
                int32_t to_read = len - offset;
                while (to_read > 0) {
                    int32_t block = (to_read > (int32_t)sizeof(chunk))
                                    ? (int32_t)sizeof(chunk) : to_read;
                    if (esp_partition_read(update_part, offset,
                                           chunk, block) == ESP_OK) {
                        mbedtls_sha256_update(&sha_ctx, chunk, block);
                    }
                    offset   += block;
                    to_read  -= block;
                }
            }
        }

        if (ret == ESP_ERR_HTTPS_OTA_IN_PROGRESS) continue;
        if (ret == ESP_OK) break;

        ESP_LOGE(TAG, "esp_https_ota_perform echoue : %s", esp_err_to_name(ret));
        esp_https_ota_abort(ota_handle);
        if (verify_hash) mbedtls_sha256_free(&sha_ctx);
        return OTA_RESULT_DOWNLOAD_FAILED;
    }

    /* Vérification SHA256 */
    if (verify_hash) {
        uint8_t hash_bin[32];
        mbedtls_sha256_finish(&sha_ctx, hash_bin);
        mbedtls_sha256_free(&sha_ctx);

        char hash_hex[65];
        sha256_to_hex(hash_bin, hash_hex);
        ESP_LOGI(TAG, "SHA256 calcule : %s", hash_hex);
        ESP_LOGI(TAG, "SHA256 attendu : %s", sha256);

        if (strcasecmp(hash_hex, sha256) != 0) {
            ESP_LOGE(TAG, "SHA256 invalide — OTA annule");
            esp_https_ota_abort(ota_handle);
            return OTA_RESULT_INVALID_HASH;
        }
        ESP_LOGI(TAG, "SHA256 OK");
    } else {
        ESP_LOGW(TAG, "Aucun SHA256 fourni — verification ignoree");
    }

    /* Finalisation */
    ret = esp_https_ota_finish(ota_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_https_ota_finish echoue : %s", esp_err_to_name(ret));
        return OTA_RESULT_DOWNLOAD_FAILED;
    }

    ESP_LOGI(TAG, "OTA reussi — reboot dans 1s");
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_restart();
    return OTA_RESULT_SUCCESS;
}
