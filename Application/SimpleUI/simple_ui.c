#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "main.h"
#include "stdio.h"

#include "stm32l476g_discovery_glass_lcd.h"

void SimpleUI_Task(void const *arguments)
{
    osDelay(200);
    printf("[%8ld]Init Start:\t%s\t%s:%d\n", HAL_GetTick(), __FUNCTION__, __FILE__, __LINE__);
    BSP_LCD_GLASS_Init();
    // osDelay(100);
    printf("[%8ld]Init Finish:\t%s\t%s:%d\n", HAL_GetTick(), __FUNCTION__, __FILE__, __LINE__);

    for (;;)
    {
        static uint32_t bar_id    = 0;
        static char     string[8] = {0};
        sprintf(string, "0x%X", bar_id);
        BSP_LCD_GLASS_Clear();
        BSP_LCD_GLASS_DisplayString(string);
        BSP_LCD_GLASS_DisplayBar(bar_id);
        bar_id++;
        osDelay(1000);
    }
}
