/**
 * @file main.c
 *
 * @brief Dhrystone 2.1 benchmark for the W806-nano (Air103-nano) board
 *        running at the maximum CPU frequency (240 MHz) with aggressive
 *        optimization flags.
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "wm_dt.h"
#include "wm_drv_rcc.h"
#include "wm_drv_wdt.h"
#include "custom_def.h"
#include "dhry.h"

static void bench_task(void *arg)
{
    wm_device_t *rcc = wm_drv_clock_init("rcc");
    uint32_t cpu_hz  = 0;

    if (rcc != NULL) {
        cpu_hz = (uint32_t)wm_drv_rcc_get_config_clock(rcc, WM_RCC_TYPE_CPU) * 1000000UL;
    }

    while (1) {
        PRINTF("\n--- Dhrystone 2.1 on W806 (W806-nano) @ %u Hz (%u MHz) ---\n", cpu_hz, cpu_hz / 1000000);
        PRINTF("Compiler: %s\n", COMPILER_NAME);
        PRINTF("Flags   : %s\n", COMPILER_FLAGS);

        dhry_main(cpu_hz);

        PRINTF("--- Dhrystone complete ---\n\n");
        vTaskDelay(pdMS_TO_TICKS(10000));
    }

    vTaskDelete(NULL);
}

int main(void)
{
    wm_device_t *wdt = wm_dt_get_device_by_name("wdt");
    if (wdt != NULL) {
        wm_drv_wdt_deinit(wdt); /* disable watchdog for the long benchmark run */
    }

    xTaskCreate(bench_task, "dhry", 2048, NULL, configMAX_PRIORITIES - 1, NULL);

    return 0;
}
