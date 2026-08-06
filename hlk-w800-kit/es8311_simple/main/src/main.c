/**
 * @file main.c
 *
 * @brief ES8311 audio loop test for the HLK-W800-KIT.
 *
 * Flow: record ~3 s from the on-board mic (ES8311 ADC via I2S RX),
 * play it back on the speaker, then play a synthesized 400-2000 Hz sweep,
 * repeat forever.
 *
 * ES8311 control is bit-banged on PA1(CCLK)/PA4(CDATA) -- the W800 hardware
 * I2C controller cannot reach this bus (see cht8305c_test).
 * The register init is a direct port of the proven Espressif
 * esp_codec_dev es8311 driver (slave, use_mclk, 16 kHz, 16-bit, I2S std,
 * MCLK = 384 * fs = 6.144 MHz).
 *
 * Wiring: ES8311 CCLK=PA1, CDATA=PA4, MCLK=PA7, I2S_BCK=PB8, I2S_LRCK=PB9,
 * I2S_DOUT=PB10, I2S_DIN=PB11.  ANT8108 nShutdown=PB4 (active low).
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "wmsdk_config.h"
#include "wm_dt.h"
#include "wm_drv_i2s.h"
#include "wm_drv_gpio.h"
#include "wm_drv_wdt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LOG_TAG "es8311"
#include "wm_log.h"

/* bit-bang I2C to ES8311 (same bus as CHT8305C) */
#define SCL_PIN   WM_GPIO_NUM_1  /* PA1 = CCLK */
#define SDA_PIN   WM_GPIO_NUM_4  /* PA4 = CDATA */
#define AMP_SHDN  WM_GPIO_NUM_20 /* PB4, ANT8108 nShutdown, active low */

#define ES8311_ADDR 0x18 /* 7-bit, CE=0 */
#define WM_I2S_DEV_NAME "i2s"

#define SAMPLE_RATE  16000
#define MCLK_HZ      (SAMPLE_RATE * 384) /* 6.144 MHz */
#define REC_SECONDS  3
#define SAMPLE_NUM   (SAMPLE_RATE * REC_SECONDS)
#define CHUNK_BYTES  1200

static wm_device_t *s_i2s = NULL;
static volatile int s_tx_done = 0;
static volatile int s_rx_done = 0;

/* ---------------- ES8311 register defines ---------------- */
#define REG_RESET   0x00
#define REG_CLK1    0x01
#define REG_CLK2    0x02
#define REG_CLK3    0x03
#define REG_CLK4    0x04
#define REG_CLK5    0x05
#define REG_CLK6    0x06
#define REG_CLK7    0x07
#define REG_CLK8    0x08
#define REG_SDPIN   0x09
#define REG_SDPOUT  0x0A
#define REG_SYS0B   0x0B
#define REG_SYS0C   0x0C
#define REG_SYS0D   0x0D
#define REG_SYS0E   0x0E
#define REG_SYS10   0x10
#define REG_SYS11   0x11
#define REG_SYS12   0x12
#define REG_SYS13   0x13
#define REG_SYS14   0x14
#define REG_ADC15   0x15
#define REG_ADC16   0x16
#define REG_ADC17   0x17
#define REG_ADC1B   0x1B
#define REG_ADC1C   0x1C
#define REG_DAC31   0x31
#define REG_DAC32   0x32
#define REG_DAC37   0x37
#define REG_GPIO44  0x44
#define REG_GP45    0x45

/* ---------------- bit-bang I2C ---------------- */

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
    scl(1);
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
    ack = sda_in();
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
    sda_out(send_ack ? 0 : 1);
    scl(1);
    scl(0);
    sda_out(1);
    return data;
}

static int es8311_write(uint8_t reg, uint8_t val)
{
    int ret;
    i2c_start();
    ret = i2c_write_byte((uint8_t)(ES8311_ADDR << 1));
    if (ret == 0) {
        ret = i2c_write_byte(reg);
    }
    if (ret == 0) {
        ret = i2c_write_byte(val);
    }
    i2c_stop();
    return ret;
}

static int es8311_read(uint8_t reg, uint8_t *val)
{
    int ret;
    i2c_start();
    ret = i2c_write_byte((uint8_t)(ES8311_ADDR << 1));
    if (ret == 0) {
        ret = i2c_write_byte(reg);
    }
    i2c_start(); /* repeated start */
    if (ret == 0) {
        ret = i2c_write_byte((uint8_t)((ES8311_ADDR << 1) | 1));
    }
    if (ret == 0) {
        *val = i2c_read_byte(0);
    }
    i2c_stop();
    return ret;
}

static int es8311_update(uint8_t reg, uint8_t mask, uint8_t val)
{
    uint8_t cur = 0;
    if (es8311_read(reg, &cur) != 0) {
        return -1;
    }
    cur = (uint8_t)((cur & ~mask) | (val & mask));
    return es8311_write(reg, cur);
}

/* Ported 1:1 from esp_codec_dev es8311 driver init sequence (slave mode,
 * use_mclk=true, mclk_div=384, 16 kHz / 16-bit / I2S std). */
static int es8311_init(void)
{
    uint8_t regv = 0;
    int ret;

    /* open(): */
    es8311_write(REG_GPIO44, 0x08);
    es8311_write(REG_GPIO44, 0x08); /* second write for I2C noise immunity */
    es8311_write(REG_CLK1, 0x30);
    es8311_write(REG_CLK2, 0x00);
    es8311_write(REG_CLK3, 0x10);
    es8311_write(REG_ADC16, 0x24);
    es8311_write(REG_CLK4, 0x10);
    es8311_write(REG_CLK5, 0x00);
    es8311_write(REG_SYS0B, 0x00);
    es8311_write(REG_SYS0C, 0x00);
    es8311_write(REG_SYS10, 0x1F);
    es8311_write(REG_SYS11, 0x7F);
    es8311_write(REG_RESET, 0x80); /* soft reset */
    vTaskDelay(pdMS_TO_TICKS(10));

    ret = es8311_read(REG_RESET, &regv);
    if (ret != 0) {
        wm_log_error("ES8311: no ACK (read reset)");
        return -1;
    }
    regv &= 0xBF; /* slave mode */
    es8311_write(REG_RESET, regv);

    regv = 0x3F; /* use mclk, not inverted */
    es8311_write(REG_CLK1, regv);
    es8311_update(REG_CLK6, 0x20, 0x00); /* sclk not inverted */
    es8311_write(REG_SYS13, 0x10);
    es8311_write(REG_ADC1B, 0x0A);
    es8311_write(REG_ADC1C, 0x6A);
    es8311_write(REG_GPIO44, 0x58); /* internal reference ADCL+DACR */

    /* set_fs: 16-bit, I2S standard */
    es8311_update(REG_SDPIN, 0x1C, 0x0C);
    es8311_update(REG_SDPOUT, 0x1C, 0x0C);
    es8311_update(REG_SDPIN, 0x03, 0x00);
    es8311_update(REG_SDPOUT, 0x03, 0x00);

    /* config_sample(16000, mclk=6144000): coeff pre_div=3, pre_multi=2,
     * adc_div=1, dac_div=1, fs_mode=0, lrck_h=0, lrck_l=0xff,
     * bclk_div=4, adc_osr=0x10, dac_osr=0x20 */
    es8311_update(REG_CLK2, 0x07, 0x00);            /* keep low bits */
    es8311_update(REG_CLK2, 0xE0, 0x40);            /* pre_div 3 */
    es8311_update(REG_CLK2, 0x38, 0x08);            /* pre_multi x2 */
    es8311_write(REG_CLK5, 0x00);                   /* adc_div=1, dac_div=1 */
    es8311_update(REG_CLK3, 0x7F, 0x10);            /* fs_mode=0, adc_osr=0x10 */
    es8311_update(REG_CLK4, 0x7F, 0x20);            /* dac_osr=0x20 */
    es8311_update(REG_CLK7, 0x3F, 0x00);            /* lrck_h=0 */
    es8311_write(REG_CLK8, 0xFF);                   /* lrck_l=0xff */
    es8311_update(REG_CLK6, 0x1F, 0x03);            /* bclk_div 4 */

    /* start(): */
    es8311_write(REG_RESET, 0x80);                  /* regv=0x80, slave */
    es8311_write(REG_CLK1, 0x3F);
    es8311_update(REG_SDPIN, 0x40, 0x00);
    es8311_update(REG_SDPOUT, 0x40, 0x00);
    es8311_write(REG_ADC17, 0xBF);                  /* ADC volume */
    es8311_write(REG_SYS0E, 0x02);
    es8311_write(REG_SYS12, 0x00);                  /* enable DAC */
    es8311_write(REG_SYS14, 0x1A);                  /* analog pga gain */
    es8311_update(REG_SYS14, 0x40, 0x00);           /* analog mic, not dmic */
    es8311_write(REG_SYS0D, 0x01);
    es8311_write(REG_ADC15, 0x40);
    es8311_write(REG_DAC37, 0x08);
    es8311_write(REG_GP45, 0x00);

    /* unmute + volume + mic gain (18 dB) */
    es8311_update(REG_DAC31, 0x60, 0x00);           /* DAC unmute */
    es8311_write(REG_DAC32, 0x80);                  /* DAC volume */
    es8311_write(REG_ADC16, 0x03);                  /* mic gain 18 dB */

    wm_log_info("ES8311 init complete");
    return 0;
}

/* ---------------- I2S ---------------- */

static void tx_cb(wm_device_t *dev, int event)
{
    s_tx_done = 1;
}

static void rx_cb(wm_device_t *dev, int event)
{
    s_rx_done = 1;
}

static void play_buffer(int16_t *buf, int n)
{
    int bytes = (int)(n * sizeof(int16_t));
    int off = 0;

    while (off < bytes) {
        int chunk = (bytes - off) > CHUNK_BYTES ? CHUNK_BYTES : (bytes - off);
        int waits = 0;
        s_tx_done = 0;
        wm_drv_i2s_write_async(s_i2s, (uint8_t *)buf + off, chunk);
        while (!s_tx_done && waits < 100) {
            vTaskDelay(pdMS_TO_TICKS(10));
            waits++;
        }
        if (!s_tx_done) {
            wm_log_warn("tx chunk timeout at %d", off);
            break;
        }
        off += chunk;
    }
    wm_log_info("tx played %d bytes", off);
}

static void play_sine_sweep(void)
{
#define SWEEP_SECONDS 3
#define SINE_LUT_LEN  256
    static const int16_t s_sine_lut[SINE_LUT_LEN] = {
        0, 736, 1472, 2207, 2941, 3672, 4402, 5129,
        5853, 6573, 7289, 8001, 8709, 9410, 10107, 10797,
        11481, 12157, 12827, 13488, 14142, 14787, 15423, 16050,
        16667, 17274, 17871, 18457, 19032, 19595, 20147, 20686,
        21213, 21727, 22229, 22716, 23190, 23650, 24096, 24528,
        24944, 25346, 25732, 26103, 26458, 26797, 27120, 27426,
        27716, 27990, 28246, 28486, 28708, 28913, 29101, 29271,
        29424, 29558, 29675, 29774, 29856, 29919, 29964, 29991,
        30000, 29991, 29964, 29919, 29856, 29774, 29675, 29558,
        29424, 29271, 29101, 28913, 28708, 28486, 28246, 27990,
        27716, 27426, 27120, 26797, 26458, 26103, 25732, 25346,
        24944, 24528, 24096, 23650, 23190, 22716, 22229, 21727,
        21213, 20686, 20147, 19595, 19032, 18457, 17871, 17274,
        16667, 16050, 15423, 14787, 14142, 13488, 12827, 12157,
        11481, 10797, 10107, 9410, 8709, 8001, 7289, 6573,
        5853, 5129, 4402, 3672, 2941, 2207, 1472, 736,
        0, -736, -1472, -2207, -2941, -3672, -4402, -5129,
        -5853, -6573, -7289, -8001, -8709, -9410, -10107, -10797,
        -11481, -12157, -12827, -13488, -14142, -14787, -15423, -16050,
        -16667, -17274, -17871, -18457, -19032, -19595, -20147, -20686,
        -21213, -21727, -22229, -22716, -23190, -23650, -24096, -24528,
        -24944, -25346, -25732, -26103, -26458, -26797, -27120, -27426,
        -27716, -27990, -28246, -28486, -28708, -28913, -29101, -29271,
        -29424, -29558, -29675, -29774, -29856, -29919, -29964, -29991,
        -30000, -29991, -29964, -29919, -29856, -29774, -29675, -29558,
        -29424, -29271, -29101, -28913, -28708, -28486, -28246, -27990,
        -27716, -27426, -27120, -26797, -26458, -26103, -25732, -25346,
        -24944, -24528, -24096, -23650, -23190, -22716, -22229, -21727,
        -21213, -20686, -20147, -19595, -19032, -18457, -17871, -17274,
        -16667, -16050, -15423, -14787, -14142, -13488, -12827, -12157,
        -11481, -10797, -10107, -9410, -8709, -8001, -7289, -6573,
        -5853, -5129, -4402, -3672, -2941, -2207, -1472, -736,
    };
    int total = SWEEP_SECONDS * SAMPLE_RATE;
    int16_t *buf = malloc(total * sizeof(int16_t));
    uint32_t phase = 0;
    int i;

    if (buf == NULL) {
        return;
    }
    for (i = 0; i < total; i++) {
        float f = 400.0f + 1600.0f * (float)i / (total - 1);
        phase += (uint32_t)(f * SINE_LUT_LEN / SAMPLE_RATE * 1024.0f);
        buf[i] = (int16_t)(s_sine_lut[(phase >> 10) & (SINE_LUT_LEN - 1)] / 2);
    }
    wm_log_info("playing sine sweep 400Hz->2000Hz");
    play_buffer(buf, total);
    free(buf);
}

static void audio_task(void *arg)
{
    int16_t *rec = malloc(SAMPLE_NUM * sizeof(int16_t));
    int total_bytes = SAMPLE_NUM * (int)sizeof(int16_t);
    int off, waits;

    if (rec == NULL) {
        wm_log_error("alloc failed");
        vTaskDelete(NULL);
    }

    while (1) {
        /* record in chunks (DMA ring is small) */
        s_rx_done = 0;
        off = 0;
        wm_log_info("recording %d s...", REC_SECONDS);
        while (off < total_bytes) {
            int chunk = (total_bytes - off) > CHUNK_BYTES ? CHUNK_BYTES : (total_bytes - off);
            waits = 0;
            s_rx_done = 0;
            wm_drv_i2s_read_async(s_i2s, (uint8_t *)rec + off, chunk);
            while (!s_rx_done && waits < 100) {
                vTaskDelay(pdMS_TO_TICKS(10));
                waits++;
            }
            if (!s_rx_done) {
                wm_log_warn("rx chunk timeout at %d", off);
                break;
            }
            off += chunk;
        }
        wm_log_info("rx captured %d/%d bytes", off, total_bytes);
        if (off == total_bytes) {
            wm_log_info("playing back recorded sample...");
            play_buffer(rec, SAMPLE_NUM);
        }

        play_sine_sweep();

        vTaskDelay(pdMS_TO_TICKS(500));
    }

    vTaskDelete(NULL);
}

static void heartbeat_task(void *arg)
{
    while (1) {
        wm_log_info("heartbeat: alive (tx_done=%d rx_done=%d)", s_tx_done, s_rx_done);
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
    vTaskDelete(NULL);
}

int main(void)
{
    wm_drv_i2s_cfg_t i2s_cfg = {0};

    wm_device_t *wdt = wm_dt_get_device_by_name("wdt");
    if (wdt != NULL) {
        wm_drv_wdt_deinit(wdt);
    }

    wm_drv_gpio_init("gpio");

    /* bit-bang I2C pins for ES8311 control */
    wm_drv_gpio_iomux_func_sel(SCL_PIN, WM_GPIO_IOMUX_FUN5);
    wm_drv_gpio_iomux_func_sel(SDA_PIN, WM_GPIO_IOMUX_FUN5);
    wm_drv_gpio_set_pullmode(SCL_PIN, WM_GPIO_PULL_UP);
    wm_drv_gpio_set_pullmode(SDA_PIN, WM_GPIO_PULL_UP);
    wm_drv_gpio_set_dir(SCL_PIN, WM_GPIO_DIR_OUTPUT);
    wm_drv_gpio_set_dir(SDA_PIN, WM_GPIO_DIR_OUTPUT);
    wm_drv_gpio_data_set(SCL_PIN);
    wm_drv_gpio_data_set(SDA_PIN);

    /* speaker amp enable (ANT8108 /shutdown) */
    wm_drv_gpio_iomux_func_sel(AMP_SHDN, WM_GPIO_IOMUX_FUN5);
    wm_drv_gpio_set_dir(AMP_SHDN, WM_GPIO_DIR_OUTPUT);
    wm_drv_gpio_data_set(AMP_SHDN);

    i2s_cfg.mode = WM_I2S_MODE_MASTER;
    i2s_cfg.dir  = WM_I2S_DIR_INOUT;
    i2s_cfg.std  = WM_I2S_STD_I2S;
    i2s_cfg.xtype = WM_I2S_XFER_DMA;
    i2s_cfg.rx_pkt_size = CHUNK_BYTES;
    i2s_cfg.rx_pkt_num  = 6;
    i2s_cfg.tx_pkt_num  = 6;
    s_i2s = wm_drv_i2s_init(WM_I2S_DEV_NAME, &i2s_cfg);
    if (s_i2s == NULL) {
        wm_log_error("i2s init failed");
        while (1) {
            vTaskDelay(pdMS_TO_TICKS(5000));
        }
    }

    wm_drv_i2s_set_mclk(s_i2s, MCLK_HZ);
    wm_drv_i2s_set_format(s_i2s, SAMPLE_RATE, WM_I2S_BITS_16BIT, WM_I2S_CHAN_TYPE_MONO_LEFT);
    wm_drv_i2s_register_write_cb(s_i2s, tx_cb);
    wm_drv_i2s_register_read_cb(s_i2s, rx_cb);

    if (es8311_init() != 0) {
        wm_log_error("ES8311 init failed -- check bus / codec (staying alive)");
        while (1) {
            vTaskDelay(pdMS_TO_TICKS(5000));
        }
    }

    wm_log_info("ES8311 audio loop started");
    xTaskCreate(heartbeat_task, "hb", 1024, NULL, 5, NULL);
    xTaskCreate(audio_task, "audio", 2048, NULL, 5, NULL);

    return 0;
}
