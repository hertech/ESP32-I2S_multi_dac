#include <stdint.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_pdm.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "sdkconfig.h"
#include "i2s_multi_dac_main.h"

#define I2s_BCLK_IO GPIO_NUM_4   // I2S PDM TX clock io number / BCK
#define I2S_WS_IO GPIO_NUM_5           // I2S PDM TX data out io number / WS
#define I2S_DOUT_IO GPIO_NUM_18 // I2S PDM TX data out io number / DO

#define SAMPLE_RATE (192000) // I2S PDM TX frequency
#define AMPLITUDE (32768.0)  // 1~32767
#define CONST_PI (3.14159265f)
#define EXAMPLE_SINE_WAVE_LEN(tone) (uint32_t)((SAMPLE_RATE / (float)tone) + 0.5) // The sample point number per sine wave to generate the tone
#define EXAMPLE_TONE_LAST_TIME_MS 500
#define EXAMPLE_BYTE_NUM_EVERY_TONE (EXAMPLE_TONE_LAST_TIME_MS * SAMPLE_RATE / 1000)
#define WAVE_FREQ_HZ (100)
#define SAMPLE_PER_CYCLE (SAMPLE_RATE / WAVE_FREQ_HZ)

static void ToggleDebugPin()
{
    gpio_set_level(DEBUG_PIN, 0); // LOW
    gpio_set_level(DEBUG_PIN, 1); // HIGH
}

static IRAM_ATTR bool i2s_tx_queue_sent_callback(i2s_chan_handle_t chan_handle, i2s_event_data_t *event, void *user_ctx )
{
    ToggleDebugPin();
    return false;
}

static i2s_chan_handle_t i2s_example_init_pdm_tx(void)
{
    i2s_chan_handle_t tx_chan; // I2S tx channel handler
    /* Setp 1: Determine the I2S channel configuration and allocate TX channel only
     * The default configuration can be generated by the helper macro,
     * it only requires the I2S controller id and I2S role */
    i2s_chan_config_t tx_chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    tx_chan_cfg.auto_clear = true;
    tx_chan_cfg.dma_desc_num = 8; // optimized for this samplerate and buffer
    tx_chan_cfg.dma_frame_num = 240; // optimized for this samplerate and buffer
    ESP_ERROR_CHECK(i2s_new_channel(&tx_chan_cfg, &tx_chan, NULL));
    // callback toggles the MR on the binary counter to synchronize DACs
    i2s_event_callbacks_t callbacks = {
        .on_recv = NULL,
        .on_recv_q_ovf = NULL,
        .on_sent = i2s_tx_queue_sent_callback,
        .on_send_q_ovf = NULL,
    };

    ESP_ERROR_CHECK(i2s_channel_register_event_callback(tx_chan, &callbacks, NULL));
    /* Step 2: Setting the configurations of PDM TX mode and initialize the TX channel
     * The slot configuration and clock configuration can be generated by the macros
     * These two helper macros is defined in 'i2s_pdm.h' which can only be used in PDM TX mode.
     * They can help to specify the slot and clock configurations for initialization or re-configuring */
    // step 2:
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2s_BCLK_IO,
            .ws = I2S_WS_IO,
            .dout = I2S_DOUT_IO,
            .din = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_chan, &std_cfg));

    /* Step 3: Enable the tx channel before writing data */
    // this will be done in the task itself
    return tx_chan;
}


void i2s_example_write_task(void *args)
{
    int16_t *w_buf = (int16_t *)calloc(1, EXAMPLE_BUFF_SIZE);
    assert(w_buf);

    i2s_chan_handle_t tx_chan = i2s_example_init_pdm_tx();

    /* Assign w_buf */
    // int sin_length = EXAMPLE_BUFF_SIZE / 4;
    // int tone_point = 0;
    //  double samples = sin_length / SAMPLE_PER_CYCLE;
    //  for (int i = 0; i < EXAMPLE_BUFF_SIZE; i += 8)
    //  {
    //      w_buf[i] = (int16_t)((sin(i * 2 * CONST_PI / sin_length)) * AMPLITUDE);
    //      w_buf[i + 1] = (int16_t)((sin(i * 3 * CONST_PI / sin_length)) * AMPLITUDE); // double frequency

    //     // w_buf[i + 1] = sinValue;
    //     w_buf[i + 2] = (int16_t)((sin(i * 4 * CONST_PI / sin_length)) * AMPLITUDE);
    //     w_buf[i + 3] = (int16_t)((sin(i * 5 * CONST_PI / sin_length)) * AMPLITUDE);
    //     w_buf[i + 4] = (int16_t)((sin(i * 6 * CONST_PI / sin_length)) * AMPLITUDE);
    //     w_buf[i + 5] = (int16_t)((sin(i * 7 * CONST_PI / sin_length)) * AMPLITUDE);
    //     w_buf[i + 6] = (int16_t)((sin(i * 8 * CONST_PI / sin_length)) * AMPLITUDE);
    //     w_buf[i + 7] = (int16_t)((sin(i * 9 * CONST_PI / sin_length)) * AMPLITUDE);
    //     tone_point++;
    // }

    size_t w_bytes = EXAMPLE_BUFF_SIZE;
    // size_t w_bytes = 0;
    // int tone_point = EXAMPLE_SINE_WAVE_LEN(440); // 440 Hz
    /* Generate the tone buffer */
    // for (int i = 0; i < tone_point; i++) {
    //     w_buf[i] =  (int16_t)((sin(2 * (float)i * CONST_PI / tone_point)) * AMPLITUDE);
    // }

    for (int i = 0; i < EXAMPLE_BUFF_SIZE; i += 8)
    {
        w_buf[i] = 0xCA01; // min = -32768 = 0x8000
        w_buf[i + 1] = 0xCA02;
        w_buf[i + 2] = 0xCA03;
        w_buf[i + 3] = 0xCA04;
        w_buf[i + 4] = 0xCA05;
        w_buf[i + 5] = 0xCA06;
        w_buf[i + 6] = 0xCA07;
        w_buf[i + 7] = 0xCA08; // max = 32767 = 0x7fff
    }

    /* Enable the TX channel */
    // reset WS and DOUT
    gpio_set_level(I2S_WS_IO, 0);          // reset WS
    gpio_set_level(I2S_DOUT_IO, 0); // DOUT
    gpio_set_level(GPIO_NUM_0, 0);             // reset COUNTER 74HC161
    vTaskDelay(pdMS_TO_TICKS(1));              // 1 msec delay
    // start I2S clock
    ESP_ERROR_CHECK(i2s_channel_enable(tx_chan));
    gpio_set_level(GPIO_NUM_0, 1); // disable reset COUNTER 74HC161

    while (1)
    {
        gpio_set_level(GPIO_NUM_0, 1);  
        if (i2s_channel_write(tx_chan, w_buf, EXAMPLE_BUFF_SIZE, &w_bytes, portMAX_DELAY) == ESP_OK)
        {
            //ToggleDebugPin();
            //vTaskDelay(pdMS_TO_TICKS(1)); // 10 msec delay
            //printf("Write Task: i2s write %d bytes\n", w_bytes);
            gpio_set_level(GPIO_NUM_0, 0);  
        }
    }

    while (1)
    {
        vTaskDelay(100);
    }
    free(w_buf);
    vTaskDelete(NULL);
}
