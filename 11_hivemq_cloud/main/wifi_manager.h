#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

/* Bits de l'EventGroup — lisibles depuis main.c */
#define WIFI_CONNECTED_BIT   BIT0
#define WIFI_FAIL_BIT        BIT1

/**
 * @brief Initialise le Wi-Fi en mode station et lance la connexion.
 *
 * Bloquant jusqu'à connexion réussie ou échec définitif.
 * Reconnexion automatique avec backoff exponentiel en cas de coupure.
 *
 * @return ESP_OK si connecté, ESP_FAIL si échec après MAX_RETRY tentatives
 */
esp_err_t wifi_manager_init(void);

/**
 * @brief Retourne le handle de l'EventGroup Wi-Fi.
 *
 * Permet aux autres tâches d'attendre WIFI_CONNECTED_BIT
 * avant de lancer des opérations réseau.
 */
EventGroupHandle_t wifi_manager_get_event_group(void);

#endif /* WIFI_MANAGER_H */
