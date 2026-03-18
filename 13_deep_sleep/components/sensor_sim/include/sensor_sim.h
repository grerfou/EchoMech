#ifndef SENSOR_SIM_H
#define SENSOR_SIM_H

#include <stdint.h>
#include "esp_err.h"

/**
 * Structure capteur — étendue par rapport aux phases 02/03
 * humidity conservée + pressure et lux ajoutés
 */
typedef struct {
    float   temp;         /* °C  — base 20°C ± 5°C             */
    float   humidity;     /* %   — base 50% ± 20%              */
    float   pressure;     /* hPa — base 1013 ± 10 hPa          */
    float   lux;          /* lux — 0..1000, cycle jour/nuit    */
    int64_t timestamp_us; /* uptime µs (esp_timer_get_time)    */
} sensor_data_t;

/**
 * Initialise le simulateur.
 * Sans effet si SENSOR_USE_REAL est défini (futur vrai driver).
 */
esp_err_t sensor_sim_init(void);

/**
 * Lit une mesure simulée.
 * Thread-safe (mutex interne).
 * @param[out] out  pointeur vers la struct à remplir
 */
esp_err_t sensor_sim_read(sensor_data_t *out);

#endif /* SENSOR_SIM_H */
