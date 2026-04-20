#pragma once

#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_err.h"

/* ---------------- Configuration ---------------- */
#define WIFI_SSID           "iPhone"
#define WIFI_PASS           "datdeptrai"

#define GROUND_STATION_IP   "255.255.255.255"
#define GROUND_STATION_PORT 5005

#define WIFI_CONNECTED_BIT  BIT0

/* ---------------- Main functions ---------------- */

esp_err_t wifi_telem_init(void);
bool wifi_telem_is_connected(void);
esp_err_t wifi_telem_send(const char *message);
void wifi_telem_deinit(void);
