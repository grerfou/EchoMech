idf_component_register(
    SRCS "main.c" "wifi_manager.c" "ntp_manager.c" "mqtt_manager.c" "ota_manager.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES esp_wifi nvs_flash esp_netif sensor_sim lwip esp_timer mqtt json esp-tls app_update esp_https_ota
)
