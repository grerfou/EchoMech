idf_component_register(
    SRCS "main.c" "wifi_manager.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES esp_wifi nvs_flash esp_netif sensor_sim
)
