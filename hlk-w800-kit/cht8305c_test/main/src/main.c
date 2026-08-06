/**
 * @file main.c
 *
 * @brief CHT8305C temperature/humidity sensor test for the HLK-W800-KIT.
 *
 * Bit-bangs I2C on PA4 (SDA) / PA1 (SCL) using the GPIO driver (internal
 * pull-ups enabled). Probes the four possible CHT8305C addresses (AD0) and
 * prints temp / relative humidity once per second. Also reads ALERT (PB5).
 *
 * CHT8305C protocol: write reg 0x00 (trigger), wait ~20ms, read 4 bytes:
 *   temp MSB, temp LSB, humi MSB, humi LSB.
 *   Temp(C) = 165*raw/65535 - 40 ; RH(%) = 100*raw/65535
 */

#include <stdio.h>
#include <string.h>
#include "wmsdk_config.h"
#include "wm_dt.h"
#include "wm_drv_gpio.h"
#include "wm_drv_wdt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LOG_TAG "cht8305c"
#include "wm_log.h"

#define SCL_PIN  WM_GPIO_NUM_1  /* PA1 */
#define SDA_PIN  WM_GPIO_NUM_4  /* PA4 */
#define ALERT_PIN WM_GPIO_NUM_21 /* PB5 */

#define CHT8305C_REG_TEMP 0x00

static void bb_delay(void)
{
    volatile uint32_t n = 60;
    while (n--) {
        __asm__ volatile("nop");
    }
}

static void scl(int level)
{
    if (level) {
        wm_drv_gpio_data_set(SCL_PIN);
    } else {
        wm_drv_gpio_data_reset(SCL_PIN);
    }
    bb_delay();
}

static void sda_out(int level)
{
    wm_drv_gpio_set_dir(SDA_PIN, WM_GPIO_DIR_OUTPUT);
    if (level) {
        wm_drv_gpio_data_set(SDA_PIN);
    } else {
        wm_drv_gpio_data_reset(SDA_PIN);
    }
    bb_delay();
}

static int sda_in(void)
{
    int v;
    wm_drv_gpio_set_dir(SDA_PIN, WM_GPIO_DIR_INPUT);
    bb_delay();
    scl(1); /* sample ACK on SCL rising */
    v = wm_drv_gpio_data_get(SDA_PIN);
    scl(0);
    return v;
}

static void i2c_start(void)
{
    scl(1);
    sda_out(1);
    scl(1);
    sda_out(0);
    scl(0);
}

static void i2c_stop(void)
{
    scl(0);
    sda_out(0);
    scl(1);
    sda_out(1);
}

static int i2c_write_byte(uint8_t data)
{
    int i;
    int ack;

    for (i = 7; i >= 0; i--) {
        scl(0);
        sda_out((data >> i) & 1);
        scl(1);
    }
    scl(0);
    ack = sda_in(); /* SDA released; read ACK */
    sda_out(1);
    scl(0);
    return ack == 0 ? 0 : -1;
}

static uint8_t i2c_read_byte(int send_ack)
{
    uint8_t data = 0;
    int i;

    scl(0);
    sda_out(1);
    wm_drv_gpio_set_dir(SDA_PIN, WM_GPIO_DIR_INPUT);
    for (i = 7; i >= 0; i--) {
        scl(1);
        data = (uint8_t)((data << 1) | (wm_drv_gpio_data_get(SDA_PIN) & 1));
        scl(0);
    }
    scl(0);
    wm_drv_gpio_set_dir(SDA_PIN, WM_GPIO_DIR_OUTPUT);
    sda_out(send_ack ? 0 : 1); /* ACK=0, NACK=1 */
    scl(1);
    scl(0);
    sda_out(1);
    return data;
}

static int i2c_probe(uint8_t addr7)
{
    int ret;
    i2c_start();
    ret = i2c_write_byte((uint8_t)(addr7 << 1));
    i2c_stop();
    return ret;
}

static int cht8305c_read_temp_humi(uint8_t addr7, float *temp_c, float *humi_pct)
{
    uint8_t buf[4];
    uint16_t temp16, humi16;

    i2c_start();
    if (i2c_write_byte((uint8_t)(addr7 << 1)) != 0 ||
        i2c_write_byte(CHT8305C_REG_TEMP) != 0) {
        i2c_stop();
        return -1;
    }
    i2c_stop();

    vTaskDelay(pdMS_TO_TICKS(20));

    i2c_start();
    if (i2c_write_byte((uint8_t)((addr7 << 1) | 1)) != 0) {
        i2c_stop();
        return -1;
    }
    buf[0] = i2c_read_byte(1);
    buf[1] = i2c_read_byte(1);
    buf[2] = i2c_read_byte(1);
    buf[3] = i2c_read_byte(0);
    i2c_stop();

    temp16 = ((uint16_t)buf[0] << 8) | buf[1];
    humi16 = ((uint16_t)buf[2] << 8) | buf[3];

    *temp_c   = 165.0f * (float)temp16 / 65535.0f - 40.0f;
    *humi_pct = 100.0f * (float)humi16 / 65535.0f;

    return 0;
}

static void sensor_task(void *arg)
{
    /* Probe the four AD0-configurable CHT8305C addresses. */
    static const uint8_t addrs[4] = { 0x40, 0x44, 0x48, 0x4C };
    int found = -1;
    int i;
    float temp = 0.0f, humi = 0.0f;

    for (i = 0; i < 4; i++) {
        if (i2c_probe(addrs[i]) == 0) {
            wm_log_info("CHT8305C responding at 0x%02x", addrs[i]);
            found = i;
        }
    }
    if (found < 0) {
        wm_log_error("no CHT8305C ACK at 0x40/0x44/0x48/0x4C");
    }

    while (1) {
        int alert = wm_drv_gpio_data_get(ALERT_PIN);
        if (found >= 0 && cht8305c_read_temp_humi(addrs[found], &temp, &humi) == 0) {
            wm_log_info("temp=%0.2f C, humi=%0.2f %%RH, alert=%d", temp, humi, alert);
        } else {
            for (i = 0; i < 4; i++) {
                if (i2c_probe(addrs[i]) == 0) {
                    found = i;
                }
            }
            wm_log_warn("read failed (bus %s), alert=%d",
                        found >= 0 ? "OK" : "empty", alert);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    vTaskDelete(NULL);
}

int main(void)
{
    wm_device_t *wdt = wm_dt_get_device_by_name("wdt");
    if (wdt != NULL) {
        wm_drv_wdt_deinit(wdt);
    }

    wm_drv_gpio_init("gpio");

    /* I2C pins as plain GPIO with internal pull-ups. */
    wm_drv_gpio_iomux_func_sel(SCL_PIN, WM_GPIO_IOMUX_FUN5);
    wm_drv_gpio_iomux_func_sel(SDA_PIN, WM_GPIO_IOMUX_FUN5);
    wm_drv_gpio_set_pullmode(SCL_PIN, WM_GPIO_PULL_UP);
    wm_drv_gpio_set_pullmode(SDA_PIN, WM_GPIO_PULL_UP);
    wm_drv_gpio_set_dir(SCL_PIN, WM_GPIO_DIR_OUTPUT);
    wm_drv_gpio_set_dir(SDA_PIN, WM_GPIO_DIR_OUTPUT);
    wm_drv_gpio_data_set(SCL_PIN);
    wm_drv_gpio_data_set(SDA_PIN);

    /* ALERT input. */
    wm_drv_gpio_iomux_func_sel(ALERT_PIN, WM_GPIO_IOMUX_FUN5);
    wm_drv_gpio_set_dir(ALERT_PIN, WM_GPIO_DIR_INPUT);
    wm_drv_gpio_set_pullmode(ALERT_PIN, WM_GPIO_PULL_UP);

    wm_log_info("CHT8305C bit-bang test started (SCL=PA1, SDA=PA4)");

    xTaskCreate(sensor_task, "cht8305c", 2048, NULL, 5, NULL);

    return 0;
}
