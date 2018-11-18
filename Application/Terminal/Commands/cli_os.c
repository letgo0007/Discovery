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
                           "\t-s --status        Show RTOS running status\n"
                           "\t-h --help        Show this help text";

int cli_os(int argc, char **argv) {

    if ((argc == 0) || (strcmp(argv[0], "-h") == 0) ||
        (strcmp(argv[0], "--help") == 0)) {
        printf("%s", OS_HELPTEXT);
        return 0;
    }

    if ((strcmp(argv[0], "-l") == 0) || (strcmp(argv[0], "--list") == 0)) {
        uint8_t *buf = malloc(1024);
        osThreadList(buf);
        printf("ThreadName      Stat\tPri.\tStack\tId\n");
        printf("%s", buf);
        free(buf);
    } else if ((strcmp(argv[0], "-s") == 0) ||
               (strcmp(argv[0], "--status") == 0)) {

        printf("Total Heap          = %d B\n", configTOTAL_HEAP_SIZE);
        printf("Current Free Heap   = %d B\n", xPortGetFreeHeapSize());
        printf("Lifetime Minimum    = %d B\n",
               xPortGetMinimumEverFreeHeapSize());

        char *buf = malloc(1024);
        vTaskGetRunTimeStats(buf);
        printf("ThreadName      TotalRunTime\t%%\n");
        printf("%s", buf);
        free(buf);
    } else {
        printf("\e[31mERROR: Unknown option of [%s], try [-h] for help.\e[0m\n",
               argv[0]);
    }

    return 0;
}
