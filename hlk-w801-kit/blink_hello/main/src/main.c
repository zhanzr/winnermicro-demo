/**
 * @file main.c
 *
 * @brief blink_hello for the HLK-W801-KIT board.
 *
 * Blinks the seven on-board LEDs and prints the system CPU frequency on the
 * log UART (UART0 @ 115200). The W801's Wi-Fi/BLE RF is populated on this
 * board, so it is a full-featured wireless MCU.
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

/*
 * W801S GPIO numbers (the W801 silicon exposed by the SDK as "W801S"):
 *   PB25 = 41, PB26 = 42, PB18 = 34, PB16 = 32, PB17 = 33, PB11 = 27, PB5 = 21
 */
#define LED1_PB25 WM_GPIO_NUM_41
#define LED2_PB26 WM_GPIO_NUM_42
#define LED3_PB18 WM_GPIO_NUM_34
#define LED4_PB16 WM_GPIO_NUM_32
#define LED5_PB17 WM_GPIO_NUM_33
#define LED6_PB11 WM_GPIO_NUM_27
#define LED7_PB5  WM_GPIO_NUM_21

/* LEDs on this board are wired low-active. */
#define LED_ON  0
#define LED_OFF 1

typedef struct {
    wm_gpio_num_t pin;
    const char   *name;
} led_t;

static const led_t s_leds[7] = {
    { LED1_PB25, "LED1 (PB25)" },
    { LED2_PB26, "LED2 (PB26)" },
    { LED3_PB18, "LED3 (PB18)" },
    { LED4_PB16, "LED4 (PB16)" },
    { LED5_PB17, "LED5 (PB17)" },
    { LED6_PB11, "LED6 (PB11)" },
    { LED7_PB5,  "LED7 (PB5)"  },
};

static void leds_all(int value)
{
    for (int i = 0; i < 7; i++) {
        if (value) {
            wm_drv_gpio_data_set(s_leds[i].pin);   /* off (high) */
        } else {
            wm_drv_gpio_data_reset(s_leds[i].pin); /* on (low)  */
        }
    }
}

static void blink_task(void *arg)
{
    int idx = 0;

    while (1) {
        leds_all(LED_OFF);
        wm_drv_gpio_data_reset(s_leds[idx].pin); /* turn this LED on */
        wm_log_info("LED on: %s", s_leds[idx].name);

        vTaskDelay(pdMS_TO_TICKS(150));

        idx = (idx + 1) % 7;
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

    /* Configure the seven LED GPIOs as push-pull outputs. */
    wm_drv_gpio_init("gpio");
    for (int i = 0; i < 7; i++) {
        wm_drv_gpio_iomux_func_sel(s_leds[i].pin, WM_GPIO_IOMUX_FUN5);
        wm_drv_gpio_set_dir(s_leds[i].pin, WM_GPIO_DIR_OUTPUT);
        wm_drv_gpio_data_set(s_leds[i].pin); /* start with LED off */
    }

    xTaskCreate(blink_task, "blink", 512, NULL, 5, NULL);

    return 0;
}
