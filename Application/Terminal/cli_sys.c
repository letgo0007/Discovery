/******************************************************************************
 * @file    command_sys.c
 *          Command list for STM32 system function.
 *
 * @author  Nick Yang
 * @date    2018/04/20
 * @version V0.1
 *****************************************************************************/
#include <app_terminal.h>
#include "stm32l4xx_hal.h"

#include "stdlib.h"
#include "string.h"
#include "sys/unistd.h"

#include "cli.h"

/*Pointer to certain address*/
#define CHECK_FUNC_RET(status, func) \
    do {\
        int ret = func;\
        if (status != ret)\
        {\
            printf("\e[32mERROR: Return=[%d] "#func"<%s:%d>\n\e[0m", ret, __FILE__, __LINE__);\
            return ret;\
        }\
    } while (0)

#define HWREG32(x)          (*((volatile uint32_t *)((uint32_t)x)))
#define HWREG16(x)          (*((volatile uint16_t *)((uint32_t)x)))
#define HWREG8(x)           (*((volatile uint8_t *)((uint32_t)x)))

extern int str_to_u32(char *str, uint32_t *value);

void sys_dump_u32(uint64_t address, uint32_t len)
{
    fflush(stdout);
    HAL_Delay(3);

    for (int i = 0; i < len; i++)
    {
        printf("0x%08lX ", HWREG32(address + 4 * i));
        if (i % 8 == 7)
        {
            printf("\n");
            fflush(stdout);
            HAL_Delay(5);
        }
    }

    printf("\n");
}

void sys_dump_u8(uint64_t address, uint32_t len)
{
    fflush(stdout);
    HAL_Delay(3);

    for (int i = 0; i < len; i++)
    {
        printf("0x%02X ", HWREG8(address++));

        if (i % 16 == 15)
        {
            printf("\n");
            fflush(stdout);
            HAL_Delay(3);
        }
    }

    printf("\n");
}

uint32_t sys_check_address(uint32_t address)
{
    return address;
}

int cli_reset(int argc, char *argv[])
{
    uint32_t delay = 10;

    if (argc > 0)
    {
        str_to_u32(argv[0], &delay);
    }

    printf("\n\e[33mReset MCU in [%ld] ms!\e[0m\r\n", delay);

    //Run Pre-Reset command, clear log buffer.
    fflush(stderr);
    fflush(stdout);

    //Delay and reset.
    HAL_Delay(delay);
    HAL_NVIC_SystemReset();
    return 0;
}

int cli_repeat(int argc, char *argv[])
{
    if ((argc < 2) || (strcmp(argv[0], "-h") == 0) || (strcmp(argv[0], "--help") == 0))
    {
        printf("repeat [count] [command]\n"
                "\tcount\t:Repeat cound of a command.\n"
                "\tcommand\t:The terminal command to run, e.g. \"test -i 123\"\n");
    }

    int count = strtol(argv[0], NULL, 0);
    int i = 0;

    for (i = 0; i < count; i++)
    {
        char cmd_buf[TERM_STRING_BUF_SIZE] =
        { 0 };
        char *argbuf[TERM_TOKEN_AMOUNT] =
        { 0 };
        int argcount = 0;

        strcpy(cmd_buf, argv[1]);

        printf("%sRepeat [%d/%d] of [%s]\n%s", TERM_BOLD, i + 1, count, cmd_buf, TERM_RESET);
        fflush(stdout);

        Cli_parseString(cmd_buf, &argcount, argbuf);
        Cli_runCommand(argcount, argbuf, gTermCommand);
    }

    return 0;
}

int cli_info(int argc, char *argv[])
{

    printf("HAL VERSION: 0x%08lX\n", HAL_GetHalVersion());
    printf("DEV ID     : 0x%08lX\n", HAL_GetDEVID());
    printf("REV ID     : 0x%08lX\n", HAL_GetREVID());
    printf("UID        : 0x%08lX%08lX%08lX\n", HAL_GetUIDw0(), HAL_GetUIDw1(), HAL_GetUIDw2());

    printf("FLASH Addr : 0x%08lX\n", FLASH_BASE);
    printf("FLASH Size : %d kB\n", *(uint16_t*) FLASHSIZE_BASE);
    printf("SRAM  Addr : 0x%08lX\n", SRAM_BASE);
    printf("SRAM  Addr : 0x%08lX\n", SRAM1_SIZE_MAX + SRAM2_SIZE);

    return 0;
}

const char *mem_helptext = "mem32 / mem8 command usage:\n"
        "\t-w --write [addr] [value] Write a value to memory\n"
        "\t-r --read  [addr] Read a value from memory\n"
        "\t-d --dump  [addr] [len] Dump memory content.\n"
        "\t-i --info  Show Memory info\n"
        "\t-h --help  Show this help text.\n";

int cli_mem32(int argc, char *argv[])
{
    if (argc == 0)
    {
        printf("%s", mem_helptext);
        return 0;
    }

    if ((strcmp(argv[0], "-w") == 0) || (strcmp(argv[0], "--write") == 0))
    {
        uint32_t addr = 0;
        uint32_t value = 0;
        str_to_u32(argv[1], &addr);
        str_to_u32(argv[2], &value);

        if (sys_check_address(addr) == 0)
        {
            printf("ERROR: Invalid address of [0x%08lX], access denied.\n", addr);
            return -1;
        }

        HWREG32(addr) = value;

        printf("Mem write: addr[0x%lX]=%ld(0x%08lX)\n", addr, HWREG32(addr), HWREG32(addr));
    }
    else if ((strcmp(argv[0], "-r") == 0) || (strcmp(argv[0], "--read") == 0))
    {
        uint32_t addr = 0;
        str_to_u32(argv[1], &addr);

        if (sys_check_address(addr) == 0)
        {
            printf("ERROR: Invalid address of [0x%08lX], access denied.\n", addr);
            return -1;
        }

        printf("Mem read: addr[0x%lX]=%ld(0x%08lX)\n", addr, HWREG32(addr), HWREG32(addr));
    }
    else if ((strcmp(argv[0], "-d") == 0) || (strcmp(argv[0], "--dump") == 0))
    {
        uint32_t addr = 0;
        uint32_t length = 0;
        str_to_u32(argv[1], &addr);
        str_to_u32(argv[2], &length);

        sys_dump_u32(addr, length);
    }
    else if ((strcmp(argv[0], "-h") == 0) || (strcmp(argv[0], "--help") == 0))
    {
        printf("%s", mem_helptext);
    }
    else
    {
        printf("Unknown option of [%s], try [-h] for help.\n", argv[0]);
    }

    return 0;
}

int cli_mem8(int argc, char *argv[])
{
    if ((argc == 0) || (argv == NULL))
    {
        printf("%s", mem_helptext);
        return 0;
    }

    if ((strcmp(argv[0], "-w") == 0) || (strcmp(argv[0], "--write") == 0))
    {
        uint32_t addr = 0;
        uint16_t len = argc - 2;
        uint8_t *pdata = malloc(len);

        //Get Address
        CHECK_FUNC_RET(0, str_to_u32(argv[1], &addr));

        //Get Data
        for (int i = 0; i < len; i++)
        {
            CHECK_FUNC_RET(0, str_to_u32(argv[2 + i], pdata + i));
        }

        //Write Data
        for (int i = 0; i < len; i++)
        {
            HWREG8(addr+i) = pdata[i];
        }

        //Print Result
        printf("Mem write: addr=[0x%lX], length=[%d]\n", addr, len);
        sys_dump_u8(pdata, len);
    }
    else if ((strcmp(argv[0], "-r") == 0) || (strcmp(argv[0], "--read") == 0))
    {
        uint32_t addr = 0;
        str_to_u32(argv[1], &addr);

        if (sys_check_address(addr) == 0)
        {
            printf("ERROR: Invalid address of [0x%08lX], access denied.\n", addr);
            return -1;
        }

        printf("Mem read: addr[0x%lX] = %d (0x%02X)\n", addr, HWREG8(addr), HWREG8(addr));
    }
    else if ((strcmp(argv[0], "-d") == 0) || (strcmp(argv[0], "--dump") == 0))
    {
        uint32_t addr = 0;
        uint32_t length = 0;
        str_to_u32(argv[1], &addr);
        str_to_u32(argv[2], &length);

        sys_dump_u8(addr, length);
    }
    else if ((strcmp(argv[0], "-h") == 0) || (strcmp(argv[0], "--help") == 0))
    {
        printf("%s", mem_helptext);
    }
    else
    {
        printf("Unknown option of [%s], try [-h] for help.\n", argv[0]);
    }

    return 0;
}
