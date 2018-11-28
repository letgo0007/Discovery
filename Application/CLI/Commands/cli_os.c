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

const char *TaskStateStr[] = {
    "Running",
    "Ready",
    "Blocked",
    "Suspend",
    "Deleted",
    "Invalid"
};


int cli_top(int argc, char **argv)
{
    TaskStatus_t *       pxTaskStatusArray;
    volatile UBaseType_t uxArraySize, x;
    uint32_t             ulTotalRunTime;

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
            // Print Header
            printf("PID  Task         State    RunTime  CPU%% Pri  Stack\n");

            // For each populated position in the pxTaskStatusArray array,
            // format the raw data as human readable ASCII data
            for (x = 0; x < uxArraySize; x++)
            {
                printf("%-4ld %-12s %-8s %-8ld %3ld%% %-4ld %-6d\r\n",
                       pxTaskStatusArray[x].xTaskNumber, // Task ID
                       pxTaskStatusArray[x].pcTaskName,  // Task Name
                       TaskStateStr[pxTaskStatusArray[x].eCurrentState],
                       pxTaskStatusArray[x].ulRunTimeCounter,                  // Task Run time
                       pxTaskStatusArray[x].ulRunTimeCounter / ulTotalRunTime, // Run time %
                       pxTaskStatusArray[x].uxCurrentPriority,                 // Priority
                       pxTaskStatusArray[x].usStackHighWaterMark); // Stack High Water Mark
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
        vTaskGetRunTimeStats(buf);
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
