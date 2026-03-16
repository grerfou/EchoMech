#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include "esp_err.h"
#include <stdbool.h>

/**
 * @brief Initialise et connecte le client MQTT au broker.
 *
 * Doit être appelé après la connexion Wi-Fi.
 * Bloquant jusqu'à CONNACK reçu ou timeout.
 *
 * @return ESP_OK si connecté, ESP_FAIL sinon
 */
esp_err_t mqtt_manager_init(void);

/**
 * @brief Publie un message sur un topic.
 *
 * @param topic   topic MQTT (ex: "sensors/temperature")
 * @param payload message à publier
 * @param qos     0 ou 1
 * @return ESP_OK si publié, ESP_FAIL sinon
 */
esp_err_t mqtt_manager_publish(const char *topic,
                                const char *payload,
                                int qos);

/**
 * @brief Retourne true si le client est connecté au broker.
 */
bool mqtt_manager_is_connected(void);

#endif /* MQTT_MANAGER_H */
