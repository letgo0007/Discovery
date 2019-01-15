#include "cli.h"
#include "stdio.h"
#include "stdlib.h"
#include "stm32l4xx_hal.h"

int cli_info(int argc, char *argv[])
{
    CLI_PRINT("HAL VERSION: 0x%08lX\n", HAL_GetHalVersion());
    CLI_PRINT("DEV ID     : 0x%08lX\n", HAL_GetDEVID());
    CLI_PRINT("REV ID     : 0x%08lX\n", HAL_GetREVID());
    CLI_PRINT("UID        : 0x%08lX%08lX%08lX\n", HAL_GetUIDw0(), HAL_GetUIDw1(), HAL_GetUIDw2());

    CLI_PRINT("FLASH Addr : 0x%08lX ~ 0x%08lX\n", FLASH_BASE, FLASH_BASE + FLASH_SIZE - 1);
    CLI_PRINT("FLASH Size : %ld kB\n", FLASH_SIZE / 1024);
    CLI_PRINT("SRAM1 Addr : 0x%08lX ~ 0x%08lX\n", SRAM1_BASE, SRAM1_BASE + SRAM1_SIZE_MAX - 1);
    CLI_PRINT("SRAM1 Size : %ld kB\n", SRAM1_SIZE_MAX / 1024);
    CLI_PRINT("SRAM2 Addr : 0x%08lX ~ 0x%08lX\n", SRAM2_BASE, SRAM2_BASE + SRAM2_SIZE - 1);
    CLI_PRINT("SRAM2 Size : %ld kB\n", SRAM2_SIZE / 1024);

    return 0;
}

int cli_reset(int argc, char *argv[])
{
    uint32_t delay = 10;

    if (argc > 1)
    {
        delay = strtol(argv[0], NULL, 0);
    }

    CLI_PRINT("\n\e[33mReset MCU in [%ld] ms!\e[0m\r\n", delay);

    // Run Pre-Reset command, clear log buffer.
    fflush(stderr);
    fflush(stdout);

    // Delay and reset.
    HAL_Delay(delay);
    HAL_NVIC_SystemReset();
    return 0;
}
