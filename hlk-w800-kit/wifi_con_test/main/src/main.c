/**
 * @file main.c
 *
 * @brief WiFi connection test for the HLK-W800-KIT board (W800 chip).
 *
 * Ported from the ESP-IDF `wifi_con_test` example: scans for access points,
 * connects to the AP from wifi_config.h (with WPA2 -> WPA/WPA2 retry), prints
 * the assigned IP, then reports WiFi status in a loop while blinking the RGB
 * LED's red channel (PB1).
 */

#include <stdio.h>
#include <string.h>
#include "wmsdk_config.h"
#include "wm_event.h"
#include "wm_wifi.h"
#include "wm_dt.h"
#include "wm_drv_gpio.h"
#include "wm_drv_wdt.h"
#include "lwip/ip_addr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "wifi_config.h"

#define LOG_TAG "wifi_con"
#include "wm_log.h"

#ifndef DEFAULT_AP
#error "wifi_config.h is missing DEFAULT_AP. Copy wifi_config.h.example to main/src/wifi_config.h and fill in your credentials."
#endif

/* RGB LED red channel = PB1 = WM_GPIO_NUM_17. Polarity TBD; active-low
 * (common-anode) assumed. */
#define LED_GPIO     WM_GPIO_NUM_17
#define LED_ON       0
#define LED_OFF      1
#define MAX_AP_COUNT 20

static EventGroupHandle_t s_wifi_event_group;

#define WIFI_CONNECTED_BIT (1 << 0)
#define WIFI_FAIL_BIT      (1 << 1)

static char s_last_ip[16] = "0.0.0.0";

static const char *auth_mode_str(wm_wifi_auth_mode_t mode)
{
    switch (mode) {
        case WM_WIFI_AUTH_OPEN:
            return "Open";
        case WM_WIFI_AUTH_WEP:
            return "WEP";
        case WM_WIFI_AUTH_WPA_PSK:
            return "WPA";
        case WM_WIFI_AUTH_WPA2_PSK:
            return "WPA2";
        case WM_WIFI_AUTH_WPA_WPA2_PSK:
            return "WPA/WPA2";
        case WM_WIFI_AUTH_WPA3_PSK:
            return "WPA3";
        case WM_WIFI_AUTH_WPA2_WPA3_PSK:
            return "WPA2/WPA3";
        default:
            return "?";
    }
}

static void event_handler(wm_event_group_t group, int event, void *data, void *priv)
{
    if (group == WM_WIFI_EV) {
        if (event == WM_EVENT_WIFI_STA_CONNECTED) {
            wm_log_info("WiFi: connected to AP");
        } else if (event == WM_EVENT_WIFI_STA_DISCONNECTED) {
            wm_log_warn("WiFi: disconnected");
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
    } else if (group == WM_LWIP_EV) {
        if (event == WM_EVENT_WIFI_STA_GOT_IP) {
            wm_lwip_event_data_t *d = (wm_lwip_event_data_t *)data;
            ipaddr_ntoa_r((ip_addr_t *)&d->sta_got_ip_info.ip, s_last_ip, sizeof(s_last_ip));
            wm_log_info("WiFi: got IP %s", s_last_ip);
            xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        }
    }
}

static int scan_networks(void)
{
    uint16_t number     = MAX_AP_COUNT;
    uint16_t ap_count   = 0;
    int err             = WM_ERR_SUCCESS;
    wm_wifi_ap_record_t ap_info[MAX_AP_COUNT];

    wm_log_info("--- Scanning for available APs ---");

    err = wm_wifi_scan_start(NULL, true);
    if (err != WM_ERR_SUCCESS) {
        wm_log_error("Scan failed: %d", err);
        return err;
    }

    memset(ap_info, 0, sizeof(ap_info));
    err = wm_wifi_scan_get_ap_records(&number, ap_info);
    if (err != WM_ERR_SUCCESS) {
        wm_log_error("Get AP records failed: %d", err);
        return err;
    }
    err = wm_wifi_scan_get_ap_num(&ap_count);
    if (err != WM_ERR_SUCCESS) {
        wm_log_error("Get AP count failed: %d", err);
        return err;
    }

    wm_log_info("  Found %u AP(s)", ap_count);
    for (int i = 0; (i < number) && (i < ap_count); i++) {
        wm_log_info("  [%02d] %-32s  %4d dBm  ch%2u  %s",
                    i, ap_info[i].ssid, ap_info[i].rssi, ap_info[i].primary,
                    auth_mode_str(ap_info[i].authmode));
    }

    return WM_ERR_SUCCESS;
}

static int connect_with_retry(const char *auth_name, wm_wifi_auth_mode_t auth, uint32_t timeout_ms)
{
    wm_wifi_config_t wifi_config;
    memset(&wifi_config, 0, sizeof(wifi_config));

    wifi_config.sta.threshold.authmode = auth;
    strncpy((char *)wifi_config.sta.ssid, DEFAULT_AP, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char *)wifi_config.sta.password, DEFAULT_PASSWD, sizeof(wifi_config.sta.password) - 1);

    wm_log_info("  Connecting (%s, %u ms)...", auth_name, timeout_ms);

    int err = wm_wifi_set_config(WM_WIFI_IF_STA, &wifi_config);
    if (err != WM_ERR_SUCCESS) {
        wm_log_error("  Set config failed: %d", err);
        return err;
    }

    err = wm_wifi_connect();
    if (err != WM_ERR_SUCCESS) {
        wm_log_error("  Connect failed: %d", err);
        return err;
    }

    xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdTRUE, pdFALSE, pdMS_TO_TICKS(timeout_ms));

    if (bits & WIFI_CONNECTED_BIT) {
        wm_log_info("  LINK_UP");
        return WM_ERR_SUCCESS;
    }

    if (bits & WIFI_FAIL_BIT) {
        wm_log_info("  FAILED");
    } else {
        wm_log_info("  TIMEOUT");
    }
    return WM_ERR_FAILED;
}

static void wifi_con_test_task(void *arg)
{
    int err;

    err = wm_wifi_init();
    if (err != WM_ERR_SUCCESS) {
        wm_log_error("wm_wifi_init failed: %d", err);
        vTaskDelete(NULL);
        return;
    }

    scan_networks();

    wm_log_info("--- Connecting to \"%s\" (WPA2, 30s timeout) ---", DEFAULT_AP);
    if (connect_with_retry("WPA2", WM_WIFI_AUTH_WPA2_PSK, 30000) == WM_ERR_SUCCESS) {
        wm_log_info("Connected successfully");
        wm_log_info("  IP: %s", s_last_ip);
    } else {
        wm_log_info("WPA2 failed. Trying WPA/WPA2 mixed mode...");
        if (connect_with_retry("WPA/WPA2", WM_WIFI_AUTH_WPA_WPA2_PSK, 20000) == WM_ERR_SUCCESS) {
            wm_log_info("Connected successfully (WPA/WPA2 mixed)");
            wm_log_info("  IP: %s", s_last_ip);
        } else {
            wm_log_warn("Connection failed. Check credentials in wifi_config.h");
        }
    }

    wm_log_info("--- Entering main loop ---");
    uint32_t loop_count = 0;

    while (1) {
        if (loop_count % 30 == 0) {
            wm_wifi_ap_record_t ap_info;
            if (wm_wifi_sta_get_ap_info(&ap_info) == WM_ERR_SUCCESS) {
                wm_log_info("--- WiFi status: UP (\"%s\", RSSI: %d dBm) ---",
                            ap_info.ssid, ap_info.rssi);
                wm_log_info("  IP: %s", s_last_ip);
            } else {
                wm_log_warn("--- WiFi status: DOWN ---");
            }
        }

        /* LED1 blinks; low-active so on == low. */
        if (loop_count & 1) {
            wm_drv_gpio_data_reset(LED_GPIO);
        } else {
            wm_drv_gpio_data_set(LED_GPIO);
        }
        loop_count++;
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

int main(void)
{
    wm_device_t *wdt = wm_dt_get_device_by_name("wdt");
    if (wdt != NULL) {
        wm_drv_wdt_deinit(wdt); /* watchdog not needed for this demo */
    }

    wm_drv_gpio_init("gpio");
    wm_drv_gpio_iomux_func_sel(LED_GPIO, WM_GPIO_IOMUX_FUN5);
    wm_drv_gpio_set_dir(LED_GPIO, WM_GPIO_DIR_OUTPUT);
    wm_drv_gpio_data_set(LED_GPIO); /* LED off */

    s_wifi_event_group = xEventGroupCreate();
    wm_event_add_callback(WM_WIFI_EV, WM_EVENT_ANY_TYPE, event_handler, NULL);
    wm_event_add_callback(WM_LWIP_EV, WM_EVENT_ANY_TYPE, event_handler, NULL);

    xTaskCreate(wifi_con_test_task, "wifi_con", 4096, NULL, 5, NULL);

    return 0;
}
