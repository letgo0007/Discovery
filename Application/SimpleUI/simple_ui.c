#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "main.h"
#include "stdio.h"

#include "stm32l476g_discovery_glass_lcd.h"

void SimpleUI_Task(void const *arguments)
{
    BSP_LCD_GLASS_Init();
    for (;;)
    {
        static uint32_t bar_id = 0;
        static char string[8] = {0};
        sprintf(string,"0x%X",bar_id);
        BSP_LCD_GLASS_Clear();
        BSP_LCD_GLASS_DisplayString(string);
        BSP_LCD_GLASS_DisplayBar(bar_id);
        bar_id ++;
        osDelay(1000);
    }
}
