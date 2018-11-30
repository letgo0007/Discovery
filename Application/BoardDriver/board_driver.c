#include "FreeRTOS.h"
#include "bsp_nvram.h"
#include "cmsis_os.h"
#include "main.h"
#include "stdio.h"

void BoardDriver_Task(void const *arguments)
{
    // Wait 50ms for external device power stable.
    osDelay(50);
    printf("[%8ld]Init Start:\t%s\t%s:%d\n", HAL_GetTick(), __FUNCTION__, __FILE__, __LINE__);
    Bsp_Nvram_init();
    printf("[%8ld]Init Finish:\t%s\t%s:%d\n", HAL_GetTick(), __FUNCTION__, __FILE__, __LINE__);

    for (;;)
    {
        osDelay(1000);
    }
}
