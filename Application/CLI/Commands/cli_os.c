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

const char *TaskStateStr[] = {"Running", "Ready", "Blocked", "Suspend", "Deleted", "Invalid"};

osThreadId get_threadid_by_name(char *name)
{
    TaskStatus_t *       pxTaskStatusArray;
    volatile UBaseType_t uxArraySize, x;
    uint32_t             ulTotalRunTime;

    uxArraySize       = uxTaskGetNumberOfTasks();
    pxTaskStatusArray = pvPortMalloc(uxArraySize * sizeof(TaskStatus_t));

    if (pxTaskStatusArray != NULL)
    {
        uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, &ulTotalRunTime);
        for (x = 0; x < uxArraySize; x++)
        {
            if (strcmp(name, pxTaskStatusArray[x].pcTaskName) == 0)
            {
                vPortFree(pxTaskStatusArray);
                return pxTaskStatusArray[x].xHandle;
            }
        }
    }

    vPortFree(pxTaskStatusArray);
    return 0;
}

osThreadId get_threadid_by_num(int num)
{
    TaskStatus_t *       pxTaskStatusArray;
    volatile UBaseType_t uxArraySize, x;
    uint32_t             ulTotalRunTime;

    uxArraySize       = uxTaskGetNumberOfTasks();
    pxTaskStatusArray = pvPortMalloc(uxArraySize * sizeof(TaskStatus_t));

    if (pxTaskStatusArray != NULL)
    {
        uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, &ulTotalRunTime);
        for (x = 0; x < uxArraySize; x++)
        {
            if (pxTaskStatusArray[x].xTaskNumber == num)
            {
                vPortFree(pxTaskStatusArray);
                return pxTaskStatusArray[x].xHandle;
            }
        }
    }

    vPortFree(pxTaskStatusArray);
    return 0;
}

int cli_top(int argc, char **argv)
{
    TaskStatus_t *       pxTaskStatusArray;
    volatile UBaseType_t uxArraySize, x;
    uint32_t             ulTotalRunTime;

    printf("Total Heap      = %d B\n", configTOTAL_HEAP_SIZE);
    printf("Free Heap       = %d B\n", xPortGetFreeHeapSize());
    printf("Low Watermark   = %d B\n", xPortGetMinimumEverFreeHeapSize());
    printf("---------------------------------------------------\n");


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
            printf("ID   Name         State    RunTime  CPU%% Pri  Stack\n");

            // For each populated position in the pxTaskStatusArray array,
            // format the raw data as human readable ASCII data
            for (x = 0; x < uxArraySize; x++)
            {
                printf("%-4ld %-12s %-8s %-8ld %3ld%% %-4ld %-6d\r\n",
                       pxTaskStatusArray[x].xTaskNumber, // Task Number
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

    return 0;
}

int cli_os(int argc, char **argv)
{
    argc--;
    argv++;

    const char *OS_HELPTEXT = "RTOS control commands:\n"
                              "\t-l --list      List RTOS threads\n"
                              "\t-s --suspend [id|name] Suspend a thread by ID or Name\n"
                              "\t-r --resume  [id|name] Resume a thread by ID or Name\n"
                              "\t-v --version   Show RTOS version\n"
                              "\t-h --help      Show this help text";

    if ((argc == 0) || (strcmp(argv[0], "-h") == 0) || (strcmp(argv[0], "--help") == 0))
    {
        printf("%s", OS_HELPTEXT);
        return 0;
    }

    if ((strcmp(argv[0], "-l") == 0) || (strcmp(argv[0], "--list") == 0))
    {
        cli_top(argc, argv);
    }
    else if ((strcmp(argv[0], "-s") == 0) || (strcmp(argv[0], "--suspend") == 0))
    {
        if ((argc < 2) || (argv[1] == NULL))
        {
            goto syntax_error;
        }
        osThreadId thread_id = 0;
        char *     tail[1]   = {0};
        uint32_t   num       = strtol(argv[1], tail, 0);

        if (**tail != 0)
        {
            thread_id = get_threadid_by_name(argv[1]);
        }
        else
        {
            thread_id = get_threadid_by_num(num);
        }

        if (thread_id == 0)
        {
            goto invalid_thread;
        }
        else
        {
            osThreadSuspend(thread_id);
            printf("Suspend Thread [ %s ]\n", argv[1]);
        }
    }
    else if ((strcmp(argv[0], "-r") == 0) || (strcmp(argv[0], "--resume") == 0))
    {
        if ((argc < 2) || (argv[1] == NULL))
        {
            goto syntax_error;
        }
        osThreadId thread_id = 0;
        char *     tail[1]   = {0};
        uint32_t   num       = strtol(argv[1], tail, 0);

        if (**tail != 0)
        {
            thread_id = get_threadid_by_name(argv[1]);
        }
        else
        {
            thread_id = get_threadid_by_num(num);
        }

        if (thread_id == 0)
        {
            goto invalid_thread;
        }
        else
        {
            osThreadResume(thread_id);
            printf("Resume Thread [ %s ]\n", argv[1]);
        }
    }
    else if ((strcmp(argv[0], "-v") == 0) || (strcmp(argv[0], "--version") == 0))
    {
        printf("FreeRTOS    :%s\n", tskKERNEL_VERSION_NUMBER);
        printf("CMSIS Kernel:0x%X\n", osCMSIS_KERNEL);
        printf("CMSIS API   :0x%X\n", osCMSIS);
    }
    else
    {
        printf("\e[31mERROR: Unknown option of [%s], try [-h] for help.\e[0m\n", argv[0]);
    }

    return 0;

syntax_error:
    printf("\e[31mERROR: Command Syntax error, try [--help].\e[0m\n");
    return -1;

invalid_thread:
    printf("\e[31mERROR: Can't find thread [%s], try [--list].\e[0m\n", argv[1]);
    return -1;
}
