/*
 * cli_os.c
 *
 *  Created on: Oct 11, 2018
 *      Author: nickyang
 */

#include "cmsis_os.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

const char OS_HELPTEXT[] = "RTOS control commands:\n"
                           "\t-l --list        List RTOS threads.\n"
                           "\t-s --status      Show RTOS running status\n"
                           "\t-h --help        Show this help text";

void Cus_vTaskGetRunTimeStats(char *pcWriteBuffer)
{
    TaskStatus_t *       pxTaskStatusArray;
    volatile UBaseType_t uxArraySize, x;
    uint32_t             ulTotalRunTime, ulStatsAsPercentage;

    // Make sure the write buffer does not contain a string.
    *pcWriteBuffer = 0x00;

    // Take a snapshot of the number of tasks in case it changes while this
    // function is executing.
    uxArraySize = uxTaskGetNumberOfTasks();

    // Allocate a TaskStatus_t structure for each task.  An array could be
    // allocated statically at compile time.
    pxTaskStatusArray = pvPortMalloc(uxArraySize * sizeof(TaskStatus_t));

    if (pxTaskStatusArray != NULL)
    {
        // Generate raw status information about each task.
        uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, &ulTotalRunTime);

        // For percentage calculations.
        ulTotalRunTime /= 100UL;

        // Avoid divide by zero errors.
        if (ulTotalRunTime > 0)
        {
            // For each populated position in the pxTaskStatusArray array,
            // format the raw data as human readable ASCII data
            for (x = 0; x < uxArraySize; x++)
            {
                // What percentage of the total run time has the task used?
                // This will always be rounded down to the nearest integer.
                // ulTotalRunTimeDiv100 has already been divided by 100.
                ulStatsAsPercentage = pxTaskStatusArray[x].ulRunTimeCounter / ulTotalRunTime;

                if (ulStatsAsPercentage > 0UL)
                {
                    sprintf(pcWriteBuffer, "%s\t\t%lu\t\t%lu%%\r\n",
                            pxTaskStatusArray[x].pcTaskName, pxTaskStatusArray[x].ulRunTimeCounter,
                            ulStatsAsPercentage);
                }
                else
                {
                    // If the percentage is zero here then the task has
                    // consumed less than 1% of the total run time.
                    sprintf(pcWriteBuffer, "%s\t\t%lu\t\t<1%%\r\n", pxTaskStatusArray[x].pcTaskName,
                            pxTaskStatusArray[x].ulRunTimeCounter);
                }

                pcWriteBuffer += strlen((char *)pcWriteBuffer);
            }
        }

        // The array is no longer needed, free the memory it consumes.
        vPortFree(pxTaskStatusArray);
    }
}

int cli_os(int argc, char **argv)
{
    argc--;
    argv++;

    if ((argc == 0) || (strcmp(argv[0], "-h") == 0) || (strcmp(argv[0], "--help") == 0))
    {
        printf("%s", OS_HELPTEXT);
        return 0;
    }

    if ((strcmp(argv[0], "-l") == 0) || (strcmp(argv[0], "--list") == 0))
    {
        uint8_t *buf = malloc(1024);
        osThreadList(buf);
        printf("ThreadName      Stat\tPri.\tStack\tId\n");
        printf("%s", buf);
        free(buf);
    }
    else if ((strcmp(argv[0], "-s") == 0) || (strcmp(argv[0], "--status") == 0))
    {

        printf("Total Heap          = %d B\n", configTOTAL_HEAP_SIZE);
        printf("Current Free Heap   = %d B\n", xPortGetFreeHeapSize());
        printf("Lifetime Minimum    = %d B\n", xPortGetMinimumEverFreeHeapSize());

        char *buf = malloc(1024);
        Cus_vTaskGetRunTimeStats(buf);
        printf("ThreadName      TotalRunTime\t%%\n");
        printf("%s", buf);
        free(buf);
    }
    else
    {
        printf("\e[31mERROR: Unknown option of [%s], try [-h] for help.\e[0m\n", argv[0]);
    }

    return 0;
}
