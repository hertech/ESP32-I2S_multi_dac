#include <stdint.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "driver/gpio.h"
#include "i2s_multi_dac_main.h"



void app_main(void)
{
    gpio_set_direction(GPIO_NUM_0, GPIO_MODE_OUTPUT);
    gpio_set_direction(DEBUG_PIN, GPIO_MODE_OUTPUT); //debug pin
    gpio_set_level(GPIO_NUM_0, 1); // set HIGH disables reset counter
    gpio_set_level(DEBUG_PIN, 1); // set HIGH disables reset counter

    printf("I2S multi dac start\n---------------------------\n");

   xTaskCreate(i2s_example_write_task, "i2s_example_write_task", 4096, NULL, 5, NULL);
}
