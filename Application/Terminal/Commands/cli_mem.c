/******************************************************************************
 * @file    command_sys.c
 *          Command list for STM32 system function.
 *
 * @author  Nick Yang
 * @date    2018/04/20
 * @version V0.1
 *****************************************************************************/

/** Includes ----------------------------------------------------------------*/
#include "stdlib.h"
#include "string.h"

#include "cli.h"
#include "stm32l4xx_hal.h"
/** Private defines ---------------------------------------------------------*/
#define ADDR_OFFSET 0

/** Private function prototypes ---------------------------------------------*/
extern void *cli_malloc(size_t size);
extern void  cli_free(void *ptr);

/** Functions ---------------------------------------------------------------*/
#define CHECK_FUNC_RET(status, func)                                                               \
    do                                                                                             \
    {                                                                                              \
        int ret = func;                                                                            \
        if (status != ret)                                                                         \
        {                                                                                          \
            printf("\e[32mERROR: Return=[%d] " #func "<%s:%d>\n\e[0m", ret, __FILE__, __LINE__);   \
            return ret;                                                                            \
        }                                                                                          \
    } while (0)

void print_u32(uint32_t *ptr, uint32_t len)
{
    for (int i = 0; i < len; i++)
    {
        printf("0x%08lX ", ptr[i]);
        if (i % 8 == 7)
        {
            printf("\n");
        }
    }

    printf("\n");
}

void print_u8(uint8_t *ptr, uint32_t len)
{
    for (int i = 0; i < len; i++)
    {
        printf("0x%02X ", ptr[i]);

        if (i % 16 == 15)
        {
            printf("\n");
        }
    }
    printf("\n");
}

int cli_mem(int argc, char **argv)
{
    const char *helptext = "mem command usage:\n"
                           "\t-w --write [addr] [value ...] Write a value to memory\n"
                           "\t-r --read  [addr] [len]       Read a value from memory\n"
                           "\t-h --help  Show this help text.\n";

    char *tail[1] = {0};

    if ((argc <= 1) || (strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "--help") == 0))
    {
        printf("%s", helptext);
    }
    else if ((strcmp(argv[1], "-w") == 0) || (strcmp(argv[1], "--write") == 0))
    {

        // Check syntax
        if ((argc <= 3) || (argv[2] == NULL) || (argv[3] == NULL))
        {
            goto syntax_error;
        }

        // Build buffer
        uint32_t  len  = argc - 3;
        uint32_t  addr = 0;
        uint32_t *data = cli_malloc(len * sizeof(uint32_t));

        // Get Parameters
        addr = strtoul(argv[2], tail, 0);
        if (**tail != 0)
        {
            goto param_error;
        }

        for (int i = 0; i < len; i++)
        {
            *(data + i) = strtoul(argv[3 + i], tail, 0);
            if (**tail != 0)
            {
                goto param_error;
            }
        }

        // Write memory
        memcpy((uint32_t *)(addr + ADDR_OFFSET), data, len * sizeof(uint32_t));
        printf("Memory Write @ 0x%08lX length = %ld\n", addr, len);
        print_u32(data, len);
        cli_free(data);
    }
    else if ((strcmp(argv[1], "-r") == 0) || (strcmp(argv[1], "--read") == 0))
    {

        // Check syntax
        if ((argc <= 3) || (argv[2] == NULL) || (argv[3] == NULL))
        {
            goto syntax_error;
        }
        // Get Parameters
        uint32_t len  = 0;
        uint32_t addr = 0;

        addr = strtoul(argv[2], tail, 0);
        if (**tail != 0)
        {
            goto param_error;
        }
        len = strtoul(argv[3], tail, 0);
        if (**tail != 0)
        {
            goto param_error;
        }

        // Print results
        printf("Memory Read @ 0x%08lX length = %ld\n", addr, len);
        print_u32((uint32_t *)(addr + ADDR_OFFSET), len);
    }
    else
    {
        goto syntax_error;
    }

    return 0;

param_error:
    printf("\e[31mERROR: can't convert [%s] to integer value.\e[0m\n", *tail);
    return -1;

syntax_error:
    printf("\e[31mERROR: Invalid command syntax, try use --help.\e[0m\n");
    return -1;
}
