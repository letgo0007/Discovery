/******************************************************************************
 * @file    cli_nvram.c
 * @brief   Command Line Interface for NVRAM (Non-Volatile Random Access Memory)
 *
 *
 * @author  Nick Yang
 * @date    2018/04/27
 * @version V0.2
 *****************************************************************************/

#include "cli.h"
#include "stdlib.h"
#include <bsp_nvram.h>
#include <stdio.h>
#include <string.h>
#include <sys/_stdint.h>

#define CHECK_FUNC_RET(status, func)                                                               \
    do                                                                                             \
    {                                                                                              \
        int ret = func;                                                                            \
        if (status != ret)                                                                         \
        {                                                                                          \
            printf("\e[31mERROR: Return=[%d] " #func "<%s:%d>\e[0m\n", ret, __FILE__, __LINE__);   \
            return ret;                                                                            \
        }                                                                                          \
    } while (0)

const char *Nvram_helptext = "Nvram command usage:\n"
                             "\t-w --write [addr] [value ...]   Write a value to Nvram\n"
                             "\t-r --read  [addr] [len]         Read a value from Nvram\n"
                             "\t-e --erase Erase Nvram content on flash bank.\n"
                             "\t-d --dump  Dump Nvram content.\n"
                             "\t-i --info  [debug] Show Nvram info\n"
                             "\t-h --help  Show this help text.\n";

int cli_nvram(int argc, char *argv[])
{
    argc--;
    argv++;
    char *tail[1] = {0};

    if ((argc == 0) || (strcmp(argv[0], "-h") == 0) || (strcmp(argv[0], "--help") == 0))
    {
        printf("%s", Nvram_helptext);
        return 0;
    }
    else if ((strcmp(argv[0], "-w") == 0) || (strcmp(argv[0], "--write") == 0))
    {
        // Check syntax
        if ((argc < 3) || (argv[1] == NULL) || (argv[2] == NULL))
        {
            goto syntax_error;
        }

        // Get Parameters
        uint32_t addr = strtoul(argv[1], tail, 0);
        if (**tail != 0)
        {
            goto param_error;
        }
        uint32_t data = strtoul(argv[2], tail, 0);
        if (**tail != 0)
        {
            goto param_error;
        }

        // Write NVRAM
        CHECK_FUNC_RET(0, Bsp_Nvram_write(addr, data));

        // Print Result
        printf("NVRAM Write Reg[0x%04lX] = 0x%lX\n", addr, data);
    }
    else if ((strcmp(argv[0], "-r") == 0) || (strcmp(argv[0], "--read") == 0))
    {
        // Check syntax
        if ((argc < 3) || (argv[1] == NULL) || (argv[2] == NULL))
        {
            goto syntax_error;
        }
        // Get Parameters
        uint32_t data = 0;
        uint32_t addr = strtoul(argv[1], tail, 0);
        if (**tail != 0)
        {
            goto param_error;
        }

        // Read NVRAM
        CHECK_FUNC_RET(0, Bsp_Nvram_read(addr, &data));

        // Print Result
        printf("NVRAM Read Reg[0x%04lX] = 0x%lX\n", addr, data);
    }
    else if ((strcmp(argv[0], "-d") == 0) || (strcmp(argv[0], "--dump") == 0))
    {
        int      status = 0;
        uint32_t val    = 0;
        printf("NVRAM dump:\n");
        for (int i = 0; i < NVRAM_DATA_SIZE; i++)
        {
            status = Bsp_Nvram_read(i, &val);
            if (status == 0)
            {
                printf("Reg[0x%04X]== 0x%lX\n", i, val);
            }
        }
    }
    else if ((strcmp(argv[0], "-e") == 0) || (strcmp(argv[0], "--erase") == 0))
    {
        static uint8_t count = 0;

        if (count == 0)
        {
            printf("\e[33mWARNING: Will Erase all NVRAM content, can't be undo.\e[0m\n");
            printf("Please use \"nvram -e confirm\" to confirm NVRAM mass erase.\n");
        }
        else if (count >= 1)
        {
            if (strcmp(argv[1], "confirm") == 0)
            {
                printf("NVRAM Mass Erase start...\n");
                CHECK_FUNC_RET(0, Bsp_Nvram_erase());
                printf("NVRAM Mass Erase done...\n");
            }
            else
            {
                count = 0;
            }
        }

        count++;
    }
    else
    {
        printf("Unknown option of [%s], try [-h] for help.\n", argv[0]);
    }

    return 0;

param_error:
    printf("\e[31mERROR: can't convert [%s] to integer value.\e[0m\n", *tail);
    return -1;

syntax_error:
    printf("\e[31mERROR: Invalid command syntax, try use --help.\e[0m\n");
    return -1;
}
