#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include "esp_err.h"
#include <stdbool.h>

/* Taille du ring buffer offline (nombre de messages max) */
#define MQTT_OFFLINE_BUFFER_SIZE  10

esp_err_t mqtt_manager_init(void);

/**
 * @brief Publie un message.
 *
 * Si non connecté, le message est mis en buffer offline.
 * A la reconnexion, le buffer est vidé automatiquement.
 */
esp_err_t mqtt_manager_publish(const char *topic,
                                const char *payload,
                                int qos);

bool mqtt_manager_is_connected(void);

#endif /* MQTT_MANAGER_H */
