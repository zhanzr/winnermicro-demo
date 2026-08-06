#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "utils.h"

uint32_t HAL_GetTick(void)
{
    return (uint32_t)xTaskGetTickCount();
}

void HAL_Delay(uint32_t t)
{
    vTaskDelay(pdMS_TO_TICKS(t));
}
