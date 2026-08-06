/**
 * @file main.c
 *
 * @brief blink_hello for the HLK-W806-KIT board (W806-C200).
 *
 * Blinks the three on-board LEDs on PB0 / PB1 / PB2 and prints the system
 * CPU frequency on the log UART (UART0 @ 115200). The W806 is a pure MCU
 * (no wireless), 1 MB flash, 288 KB RAM; the on-board PSRAM is not
 * populated.
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

/* W806 GPIO number -> pin name: 16 = PB0, 17 = PB1, 18 = PB2 */
#define LED_PB0 WM_GPIO_NUM_16
#define LED_PB1 WM_GPIO_NUM_17
#define LED_PB2 WM_GPIO_NUM_18

/* LEDs on this board are wired low-active. */
#define LED_ON  0
#define LED_OFF 1

static const wm_gpio_num_t s_leds[3] = { LED_PB0, LED_PB1, LED_PB2 };

static void leds_write(int value)
{
    for (int i = 0; i < 3; i++) {
        if (value) {
            wm_drv_gpio_data_set(s_leds[i]);
        } else {
            wm_drv_gpio_data_reset(s_leds[i]);
        }
    }
}

static void blink_task(void *arg)
{
    int idx = 0;

    while (1) {
        leds_write(LED_OFF);
        wm_drv_gpio_data_reset(s_leds[idx]); /* turn this LED on */
        wm_log_info("LED on: PB%d", idx);

        vTaskDelay(pdMS_TO_TICKS(250));

        idx = (idx + 1) % 3;
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

    /* Configure PB0 / PB1 / PB2 as GPIO outputs. */
    wm_drv_gpio_init("gpio");
    for (int i = 0; i < 3; i++) {
        wm_drv_gpio_iomux_func_sel(s_leds[i], WM_GPIO_IOMUX_FUN5);
        wm_drv_gpio_set_dir(s_leds[i], WM_GPIO_DIR_OUTPUT);
        wm_drv_gpio_data_set(s_leds[i]); /* start with LED off */
    }

    xTaskCreate(blink_task, "blink", 512, NULL, 5, NULL);

    return 0;
}
