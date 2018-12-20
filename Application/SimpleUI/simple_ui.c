#include "FreeRTOS.h"
#include "cli.h"
#include "cmsis_os.h"
#include "main.h"
#include "stdio.h"

#include "stm32l476g_discovery_glass_lcd.h"

void SimpleUI_Task(void const *arguments)
{
    osDelay(200);
    CLI_INFO("%s: Initialize Start\n", __FUNCTION__);
    BSP_LCD_GLASS_Init();
    osDelay(100);
    CLI_INFO("%s: Initialize Success\n", __FUNCTION__);

    for (;;)
    {
        static uint32_t bar_id = 0;
        static uint8_t string[8] = { 0 };
        sprintf((char*) string, "0x%lX", bar_id);
        BSP_LCD_GLASS_Clear();
        BSP_LCD_GLASS_DisplayString(string);
        BSP_LCD_GLASS_DisplayBar(bar_id);
        bar_id++;
        osDelay(1000);
    }
}
