/**
 * @file main.c
 *
 * @brief blink_hello for the W806-nano (Air103-nano) board.
 *
 * Blinks the three on-board LEDs on PB24 / PB25 / PB26 and prints the
 * system CPU frequency on the log UART (UART0 @ 115200). The W806 is a
 * pure MCU (no wireless), 1 MB flash, 288 KB RAM.
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

/* W806 GPIO number -> pin name: 40 = PB24, 41 = PB25, 42 = PB26 */
#define LED_PB24 WM_GPIO_NUM_40
#define LED_PB25 WM_GPIO_NUM_41
#define LED_PB26 WM_GPIO_NUM_42

/*
 * LEDs on this board are wired high-active (GPIO -> R -> LED anode,
 * LED cathode -> GND), verified from the KiCad netlist: each LED's
 * Pad1 -> GND, Pad2 -> resistor -> W806 GPIO.
 */
#define LED_ON  1
#define LED_OFF 0

static const wm_gpio_num_t s_leds[3] = { LED_PB24, LED_PB25, LED_PB26 };

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
        wm_drv_gpio_data_set(s_leds[idx]); /* turn this LED on */
        wm_log_info("LED on: PB%d", 24 + idx);

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

    /* Configure PB24 / PB25 / PB26 as GPIO outputs. */
    wm_drv_gpio_init("gpio");
    for (int i = 0; i < 3; i++) {
        wm_drv_gpio_iomux_func_sel(s_leds[i], WM_GPIO_IOMUX_FUN5);
        wm_drv_gpio_set_dir(s_leds[i], WM_GPIO_DIR_OUTPUT);
        wm_drv_gpio_data_reset(s_leds[i]); /* start with LED off */
    }

    xTaskCreate(blink_task, "blink", 512, NULL, 5, NULL);

    return 0;
}
