#include "wifi_telem.h"

/* ---------------- Static Globals ---------------- */
static const char *TAG = "wifi_telem";

static EventGroupHandle_t s_wifi_event_group = NULL;
static bool s_wifi_started = false;

static int s_sock = -1;
static bool s_use_broadcast = false;
static struct sockaddr_in s_dest_addr;

/* ---------------- Internal Helpers ---------------- */
static void wifi_telem_event_handler(void *arg,
                                     esp_event_base_t event_base,
                                     int32_t event_id,
                                     void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "Wi-Fi started, connecting...");
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "Wi-Fi disconnected, retrying...");
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);

        if (s_sock >= 0) {
            close(s_sock);
            s_sock = -1;
        }

        esp_wifi_connect();
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static esp_err_t wifi_telem_socket_create(void)
{
    if (s_sock >= 0) {
        return ESP_OK;
    }

    memset(&s_dest_addr, 0, sizeof(s_dest_addr));
    s_dest_addr.sin_family = AF_INET;
    s_dest_addr.sin_port = htons(GROUND_STATION_PORT);

    if (strcmp(GROUND_STATION_IP, "255.255.255.255") == 0) {
        s_use_broadcast = true;
        s_dest_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    } else {
        s_use_broadcast = false;
        if (inet_pton(AF_INET, GROUND_STATION_IP, &s_dest_addr.sin_addr) != 1) {
            ESP_LOGE(TAG, "Invalid GROUND_STATION_IP: %s", GROUND_STATION_IP);
            return ESP_ERR_INVALID_ARG;
        }
    }

    s_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (s_sock < 0) {
        ESP_LOGE(TAG, "Failed to create UDP socket");
        s_sock = -1;
        return ESP_FAIL;
    }

    if (s_use_broadcast) {
        int broadcast_enable = 1;
        if (setsockopt(s_sock, SOL_SOCKET, SO_BROADCAST,
                       &broadcast_enable, sizeof(broadcast_enable)) < 0) {
            ESP_LOGW(TAG, "Failed to enable SO_BROADCAST");
        }
    }

    struct timeval tv = {
        .tv_sec = 2,
        .tv_usec = 0
    };

    if (setsockopt(s_sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0) {
        ESP_LOGW(TAG, "Failed to set socket send timeout");
    }

    ESP_LOGI(TAG, "UDP socket created");
    return ESP_OK;
}

/* ---------------- Public Functions ---------------- */
esp_err_t wifi_telem_init(void)
{
    if (s_wifi_started) {
        return ESP_OK;
    }

    s_wifi_event_group = xEventGroupCreate();
    if (s_wifi_event_group == NULL) {
        ESP_LOGE(TAG, "Failed to create Wi-Fi event group");
        return ESP_ERR_NO_MEM;
    }

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(
        esp_event_handler_register(WIFI_EVENT,
                                   ESP_EVENT_ANY_ID,
                                   &wifi_telem_event_handler,
                                   NULL)
    );

    ESP_ERROR_CHECK(
        esp_event_handler_register(IP_EVENT,
                                   IP_EVENT_STA_GOT_IP,
                                   &wifi_telem_event_handler,
                                   NULL)
    );

    wifi_config_t wifi_config = { 0 };
    strncpy((char *)wifi_config.sta.ssid, WIFI_SSID, sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, WIFI_PASS, sizeof(wifi_config.sta.password));
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    s_wifi_started = true;
    ESP_LOGI(TAG, "Wi-Fi telemetry initialized");

    return ESP_OK;
}

bool wifi_telem_is_connected(void)
{
    if (s_wifi_event_group == NULL) {
        return false;
    }

    EventBits_t bits = xEventGroupGetBits(s_wifi_event_group);
    return ((bits & WIFI_CONNECTED_BIT) != 0);
}

esp_err_t wifi_telem_send(const char *message)
{
    if (message == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!s_wifi_started) {
        ESP_LOGW(TAG, "wifi_telem_send called before init");
        return ESP_ERR_INVALID_STATE;
    }

    if (!wifi_telem_is_connected()) {
        ESP_LOGW(TAG, "Wi-Fi not connected, skipping send");
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = wifi_telem_socket_create();
    if (err != ESP_OK) {
        return err;
    }

    int len = strlen(message);
    int sent = sendto(s_sock,
                      message,
                      len,
                      0,
                      (struct sockaddr *)&s_dest_addr,
                      sizeof(s_dest_addr));

    if (sent < 0) {
        ESP_LOGW(TAG, "sendto failed, closing socket");
        close(s_sock);
        s_sock = -1;
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Sent UDP telemetry: %s", message);
    return ESP_OK;
}

void wifi_telem_deinit(void)
{
    if (s_sock >= 0) {
        close(s_sock);
        s_sock = -1;
    }

    if (s_wifi_started) {
        esp_wifi_stop();
        esp_wifi_deinit();
        s_wifi_started = false;
    }

    ESP_LOGI(TAG, "Wi-Fi telemetry deinitialized");
}