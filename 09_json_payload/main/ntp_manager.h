#ifndef NTP_MANAGER_H
#define NTP_MANAGER_H

#include "esp_err.h"
#include <stdbool.h>

/**
 * @brief Initialise et synchronise l'heure via SNTP.
 *
 * Bloquant jusqu'à sync réussie ou timeout.
 * Doit être appelé APRES la connexion Wi-Fi.
 *
 * @return ESP_OK si heure synchronisée, ESP_ERR_TIMEOUT sinon
 */
esp_err_t ntp_manager_init(void);

/**
 * @brief Retourne true si l'heure est synchronisée.
 */
bool ntp_manager_is_synced(void);

/**
 * @brief Remplit buf avec l'heure actuelle en ISO8601.
 *
 * Format : "2026-03-16T14:30:00+0100"
 * Fallback si pas de sync : "UPTIME:123456789us"
 *
 * @param buf   buffer destination
 * @param size  taille du buffer (minimum 32 octets)
 */
void ntp_manager_get_timestamp(char *buf, size_t size);

#endif /* NTP_MANAGER_H */
