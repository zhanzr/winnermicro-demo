/**
 * @file main.c
 *
 * @brief blink_hello for the HLK-W800-KIT board.
 *
 * Cycles the on-board RGB LED (PB0=Green, PB1=Red, PB2=Blue) through colours
 * and prints the system CPU frequency on the log UART (UART0 @ 115200).
 * The W800 has the full Wi-Fi/BLE RF stack populated.
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "wm_dt.h"
#include "wm_drv_gpio.h"
#include "wm_drv_rcc.h"
#include "wm_drv_wdt.h"

#define LOG_TAG "blink_hello"
#include "wm_log.h"

/* W800 GPIO number -> pin name: 16 = PB0, 17 = PB1, 18 = PB2 */
#define LED_RED   WM_GPIO_NUM_17 /* PB1 */
#define LED_GREEN WM_GPIO_NUM_16 /* PB0 */
#define LED_BLUE  WM_GPIO_NUM_18 /* PB2 */

/*
 * RGB LED polarity. Default assumes common-anode (GPIO low = LED on).
 * If the LED colours look inverted, flip LED_ON/LED_OFF.
 */
#define LED_ON  0
#define LED_OFF 1

static const wm_gpio_num_t s_rgb[3] = { LED_GREEN, LED_RED, LED_BLUE };

static void rgb_set(int r, int g, int b)
{
    if (r) {
        wm_drv_gpio_data_reset(LED_RED);
    } else {
        wm_drv_gpio_data_set(LED_RED);
    }
    if (g) {
        wm_drv_gpio_data_reset(LED_GREEN);
    } else {
        wm_drv_gpio_data_set(LED_GREEN);
    }
    if (b) {
        wm_drv_gpio_data_reset(LED_BLUE);
    } else {
        wm_drv_gpio_data_set(LED_BLUE);
    }
}

static void blink_task(void *arg)
{
    while (1) {
        rgb_set(1, 0, 0); /* Red   */
        wm_log_info("RGB: RED");
        vTaskDelay(pdMS_TO_TICKS(400));

        rgb_set(0, 1, 0); /* Green */
        wm_log_info("RGB: GREEN");
        vTaskDelay(pdMS_TO_TICKS(400));

        rgb_set(0, 0, 1); /* Blue  */
        wm_log_info("RGB: BLUE");
        vTaskDelay(pdMS_TO_TICKS(400));

        rgb_set(1, 1, 1); /* White */
        wm_log_info("RGB: WHITE");
        vTaskDelay(pdMS_TO_TICKS(400));

        rgb_set(0, 0, 0); /* Off   */
        wm_log_info("RGB: OFF");
        vTaskDelay(pdMS_TO_TICKS(400));
    }

    vTaskDelete(NULL);
}

int main(void)
{
    wm_device_t *wdt = wm_dt_get_device_by_name("wdt");
    if (wdt != NULL) {
        wm_drv_wdt_deinit(wdt); /* watchdog not needed for this demo */
    }

    /* Read back the configured system frequency. */
    wm_device_t *rcc = wm_drv_clock_init("rcc");
    int cpu_mhz      = 0;
    if (rcc != NULL) {
        cpu_mhz = wm_drv_rcc_get_config_clock(rcc, WM_RCC_TYPE_CPU);
    }
    wm_log_info("System frequency: %d MHz (%d Hz)", cpu_mhz, cpu_mhz * 1000000);

    /* Configure the RGB LED pins as push-pull outputs. */
    wm_drv_gpio_init("gpio");
    for (int i = 0; i < 3; i++) {
        wm_drv_gpio_iomux_func_sel(s_rgb[i], WM_GPIO_IOMUX_FUN5);
        wm_drv_gpio_set_dir(s_rgb[i], WM_GPIO_DIR_OUTPUT);
        wm_drv_gpio_data_set(s_rgb[i]); /* start LED off */
    }

    xTaskCreate(blink_task, "blink", 512, NULL, 5, NULL);

    return 0;
}
