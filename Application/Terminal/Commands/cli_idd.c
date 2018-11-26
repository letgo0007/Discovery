/*
 * cli_idd.c
 *
 *  Created on: Oct 10, 2018
 *      Author: nickyang
 */

#include "stdlib.h"
#include "string.h"

#include "stm32l476g_discovery_idd.h"

#define CHECK_FUNC_EXIT(status, func)                                                              \
    do                                                                                             \
    {                                                                                              \
        int ret = func;                                                                            \
        if (status != ret)                                                                         \
        {                                                                                          \
            printf("\e[31mERROR: Return=[%d] " #func "<%s:%d>\n\e[0m", ret, __FILE__, __LINE__);   \
            goto exit;                                                                             \
        }                                                                                          \
    } while (0)

extern int  str_to_u8(char *str, uint8_t *value);
extern int  str_to_u32(char *str, uint32_t *value);
extern void print_u8(uint32_t address, uint32_t len);

int cli_idd_selftest()
{
    return 0;
}

/**@brief Command line interface for Accel
 *
 * @param argc
 * @param argv
 */
int cli_idd(int argc, char **argv)
{
    const char IDD_HELPTEXT[] = "Idd current sensor commands:\n"
                                "\t-i --init        Idd initialize\n"
                                "\t-p --property    Idd info \n"
                                "\t-s --selftest    Run Idd self test.\n"
                                "\t-r --read        Read Idd Current Value in 10nA\n"
                                "\t-h --help        Show this help text.\n";

    if ((argc == 0) || (strcmp(argv[0], "-h") == 0) || (strcmp(argv[0], "--help") == 0))
    {
        printf("%s", IDD_HELPTEXT);
        return 0;
    }

    if ((strcmp(argv[0], "-i") == 0) || (strcmp(argv[0], "--init") == 0))
    {
        CHECK_FUNC_EXIT(IDD_OK, BSP_IDD_Init());
        // BSP_IDD_Reset();
        printf("IDD Initialize OK!\n");
    }
    else if ((strcmp(argv[0], "-r") == 0) || (strcmp(argv[0], "--read") == 0))
    {
        BSP_IDD_StartMeasure();
        HAL_Delay(1000);
        uint32_t value = 0;
        BSP_IDD_GetValue(&value);

        printf("IDD = %8ld nA\n", value * 10);
    }
    else
    {
        printf("\e[31mERROR: Unknown option of [%s], try [-h] for help.\e[0m\n", argv[0]);
    }
exit:

    return 0;
}
