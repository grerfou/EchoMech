#include "sensor_sim.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_random.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <math.h>
#include <string.h>

static const char *TAG = "SENSOR_SIM";

#define TEMP_BASE           20.0f
#define TEMP_NOISE_SIGMA     0.8f
#define TEMP_DRIFT_MAX       5.0f
#define TEMP_DRIFT_STEP      0.05f

#define PRES_BASE         1013.0f
#define PRES_NOISE_SIGMA     0.5f
#define PRES_DRIFT_MAX      10.0f
#define PRES_DRIFT_STEP      0.02f

#define HUM_BASE            50.0f
#define HUM_NOISE_SIGMA      1.5f
#define HUM_DRIFT_MAX       20.0f
#define HUM_DRIFT_STEP       0.1f

#define LUX_MAX           1000.0f
#define LUX_NOISE_SIGMA     20.0f
#define LUX_CYCLE_PERIOD_S 300.0f

#define DRIFT_UPDATE_EVERY  10

typedef struct {
    float    drift_temp;
    float    drift_pressure;
    float    drift_humidity;
    uint32_t tick;
} sim_state_t;

static sim_state_t       s_state;
static SemaphoreHandle_t s_mutex     = NULL;
static bool              s_initialized = false;

static float rand_uniform(void)
{
    uint32_t r = (esp_random() & 0x7FFFFF) + 1;
    return (float)r / (float)(1 << 23);
}

static float gaussian(float sigma)
{
    float u1 = rand_uniform();
    float u2 = rand_uniform();
    float z  = sqrtf(-2.0f * logf(u1)) * cosf(2.0f * (float)M_PI * u2);
    return sigma * z;
}

static float random_walk(float current, float step, float bound)
{
    float dir  = (esp_random() & 1) ? +1.0f : -1.0f;
    float next = current + dir * step;
    if (next >  bound) next =  bound;
    if (next < -bound) next = -bound;
    return next;
}

esp_err_t sensor_sim_init(void)
{
#ifdef SENSOR_USE_REAL
    ESP_LOGI(TAG, "Mode REEL — init driver hardware (TODO)");
    return ESP_OK;
#endif

    if (s_initialized) {
        ESP_LOGW(TAG, "Deja initialise");
        return ESP_OK;
    }

    s_mutex = xSemaphoreCreateMutex();
    if (!s_mutex) {
        ESP_LOGE(TAG, "Echec creation mutex");
        return ESP_ERR_NO_MEM;
    }

    memset(&s_state, 0, sizeof(s_state));
    s_initialized = true;

    ESP_LOGI(TAG, "Simulateur pret");
    return ESP_OK;
}

esp_err_t sensor_sim_read(sensor_data_t *out)
{
    if (!out) return ESP_ERR_INVALID_ARG;

#ifdef SENSOR_USE_REAL
    return ESP_ERR_NOT_SUPPORTED;
#endif

    xSemaphoreTake(s_mutex, portMAX_DELAY);

    s_state.tick++;

    if (s_state.tick % DRIFT_UPDATE_EVERY == 0) {
        s_state.drift_temp     = random_walk(s_state.drift_temp,
                                             TEMP_DRIFT_STEP, TEMP_DRIFT_MAX);
        s_state.drift_pressure = random_walk(s_state.drift_pressure,
                                             PRES_DRIFT_STEP, PRES_DRIFT_MAX);
        s_state.drift_humidity = random_walk(s_state.drift_humidity,
                                             HUM_DRIFT_STEP, HUM_DRIFT_MAX);
    }

    out->temp     = TEMP_BASE + s_state.drift_temp + gaussian(TEMP_NOISE_SIGMA);

    out->pressure = PRES_BASE + s_state.drift_pressure + gaussian(PRES_NOISE_SIGMA);

    out->humidity = HUM_BASE + s_state.drift_humidity + gaussian(HUM_NOISE_SIGMA);
    if (out->humidity <   0.0f) out->humidity =   0.0f;
    if (out->humidity > 100.0f) out->humidity = 100.0f;

    float uptime_s = (float)(esp_timer_get_time() / 1000000);
    float cycle    = (sinf(2.0f * (float)M_PI * uptime_s / LUX_CYCLE_PERIOD_S) + 1.0f) / 2.0f;
    out->lux       = cycle * LUX_MAX + gaussian(LUX_NOISE_SIGMA);
    if (out->lux <    0.0f) out->lux =    0.0f;
    if (out->lux > LUX_MAX) out->lux = LUX_MAX;

    out->timestamp_us = esp_timer_get_time();

    xSemaphoreGive(s_mutex);
    return ESP_OK;
}
