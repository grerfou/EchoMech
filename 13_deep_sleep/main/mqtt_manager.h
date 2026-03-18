#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include "esp_err.h"
#include <stdbool.h>

#define MQTT_OFFLINE_BUFFER_SIZE  10

esp_err_t mqtt_manager_init(void);
esp_err_t mqtt_manager_publish(const char *topic,
                                const char *payload,
                                int qos);
bool mqtt_manager_is_connected(void);

#endif /* MQTT_MANAGER_H */
