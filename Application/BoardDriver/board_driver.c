#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "main.h"
#include "stdio.h"

void BoardDriver_Task(void const *arguments)
{
    for (;;)
    {
        osDelay(1000);
    }
}
