#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "main.h"
#include "stdio.h"
#include "bsp_nvram.h"

void BoardDriver_Task(void const *arguments)
{
    //Wait 200ms for external device power stable.
    osDelay(200);
    Bsp_Nvram_init();
    
    for (;;)
    {
        osDelay(1000);
    }
}
