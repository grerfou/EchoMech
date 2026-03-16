#include "mqtt_manager.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "mqtt_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "MQTT";

#define MQTT_CONNECTED_BIT  BIT0
#define MQTT_FAIL_BIT       BIT1
#define MQTT_CONNECT_TIMEOUT_MS 10000

static esp_mqtt_client_handle_t s_client      = NULL;
static EventGroupHandle_t       s_mqtt_eg     = NULL;
static bool                     s_connected   = false;

/* ------------------------------------------------------------------ */
/*  Génération du Client ID unique basé sur l'adresse MAC             */
/* ------------------------------------------------------------------ */
static void get_client_id(char *buf, size_t size)
{
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(buf, size, "echomesh-%02X%02X%02X",
             mac[3], mac[4], mac[5]);
}

/* ------------------------------------------------------------------ */
/*  Handler d'événements MQTT                                          */
/* ------------------------------------------------------------------ */
static void mqtt_event_handler(void *arg,
                                esp_event_base_t base,
                                int32_t event_id,
                                void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch (event->event_id) {

    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "CONNACK recu — connecte au broker");
        s_connected = true;
        xEventGroupSetBits(s_mqtt_eg, MQTT_CONNECTED_BIT);
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "Deconnecte du broker");
        s_connected = false;
        break;

    case MQTT_EVENT_PUBLISHED:
        ESP_LOGD(TAG, "PUBACK recu pour msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "Message recu sur topic=%.*s : %.*s",
                 event->topic_len, event->topic,
                 event->data_len, event->data);
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "Erreur MQTT");
        xEventGroupSetBits(s_mqtt_eg, MQTT_FAIL_BIT);
        break;

    default:
        break;
    }
}

/* ------------------------------------------------------------------ */
/*  API publique                                                        */
/* ------------------------------------------------------------------ */
esp_err_t mqtt_manager_init(void)
{
    char client_id[32];
    char broker_uri[64];

    get_client_id(client_id, sizeof(client_id));
    snprintf(broker_uri, sizeof(broker_uri), "mqtt://%s:%d",
             CONFIG_MQTT_BROKER_IP, CONFIG_MQTT_BROKER_PORT);

    ESP_LOGI(TAG, "Connexion broker : %s", broker_uri);
    ESP_LOGI(TAG, "Client ID        : %s", client_id);

    s_mqtt_eg = xEventGroupCreate();

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri       = broker_uri,
        .credentials.client_id    = client_id,
        .session.keepalive        = 30,
        .network.reconnect_timeout_ms = 5000,
    };

    s_client = esp_mqtt_client_init(&mqtt_cfg);
    if (!s_client) {
        ESP_LOGE(TAG, "Echec init client MQTT");
        return ESP_FAIL;
    }

    esp_mqtt_client_register_event(s_client, ESP_EVENT_ANY_ID,
                                   mqtt_event_handler, NULL);
    esp_mqtt_client_start(s_client);

    /* Attente CONNACK avec timeout */
    EventBits_t bits = xEventGroupWaitBits(
        s_mqtt_eg,
        MQTT_CONNECTED_BIT | MQTT_FAIL_BIT,
        pdFALSE, pdFALSE,
        pdMS_TO_TICKS(MQTT_CONNECT_TIMEOUT_MS));

    if (bits & MQTT_CONNECTED_BIT) {
        return ESP_OK;
    }

    ESP_LOGE(TAG, "Timeout connexion MQTT");
    return ESP_FAIL;
}

esp_err_t mqtt_manager_publish(const char *topic,
                                const char *payload,
                                int qos)
{
    if (!s_connected) {
        ESP_LOGW(TAG, "Publish ignore — non connecte");
        return ESP_FAIL;
    }

    int msg_id = esp_mqtt_client_publish(s_client, topic,
                                          payload, 0, qos, 0);
    if (msg_id < 0) {
        ESP_LOGE(TAG, "Echec publish sur %s", topic);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Publie sur %s (msg_id=%d)", topic, msg_id);
    return ESP_OK;
}

bool mqtt_manager_is_connected(void)
{
    return s_connected;
}
