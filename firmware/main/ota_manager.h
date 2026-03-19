#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include "esp_err.h"
#include <stdbool.h>

/**
 * @brief Résultat d'un OTA
 */
typedef enum {
    OTA_RESULT_SUCCESS = 0,
    OTA_RESULT_SAME_VERSION,
    OTA_RESULT_DOWNLOAD_FAILED,
    OTA_RESULT_INVALID_HASH,
} ota_result_t;

/**
 * @brief Lance le téléchargement et l'installation du firmware.
 *
 * Bloquant — peut prendre plusieurs dizaines de secondes.
 * En cas de succès, redémarre automatiquement sur le nouveau firmware.
 * En cas d'échec, retourne sans redémarrer (rollback automatique).
 *
 * @param url       URL HTTPS du firmware .bin
 * @param version   Version attendue (pour éviter de flasher la même)
 * @param sha256    Hash SHA256 attendu (peut être NULL pour ignorer)
 * @return ota_result_t
 */
ota_result_t ota_manager_start(const char *url,
                                const char *version,
                                const char *sha256);

#endif /* OTA_MANAGER_H */
