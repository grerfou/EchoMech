#ifndef TASK_SENSOR8H
#define TASK_SENSOR8H

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

/* Structure de données capteur partagée entre les taches */
typedef struct {
    float temp;
    float humidity;
    uint64_t timestamp_ms; // Uptime
} sensor_data_t;

/* Handle de la queu partagée - def dans main.c  */
extern QueueHandle_t xQueueSensor;

/* Proto tache  */
void task_sensor(void *pvParameters);

#endif /* Task_SENSOR_H  */
