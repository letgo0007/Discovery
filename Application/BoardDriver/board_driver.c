#include "board_driver.h"
#include "FreeRTOS.h"
#include "bsp_nvram.h"
#include "cmsis_os.h"
#include "main.h"
#include "stdio.h"

void BoardDriver_Task(void const *arguments)
{
    // Wait 50ms for external device power stable.
    osDelay(50);
    CLI_INFO("%s: Initialize Start\n", __FUNCTION__);
    Bsp_Nvram_Init();

    CLI_INFO("%s: Initialize Finish\n", __FUNCTION__);

    for (;;)
    {
        osDelay(1000);
    }
}
