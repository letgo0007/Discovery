#include "cli.h"
#include "stdio.h"
#include "stm32l4xx_hal.h"
#include "stdlib.h"

int cli_info(int argc, char *argv[])
{
    printf("HAL VERSION: 0x%08lX\n", HAL_GetHalVersion());
    printf("DEV ID     : 0x%08lX\n", HAL_GetDEVID());
    printf("REV ID     : 0x%08lX\n", HAL_GetREVID());
    printf("UID        : 0x%08lX%08lX%08lX\n", HAL_GetUIDw0(), HAL_GetUIDw1(), HAL_GetUIDw2());

    printf("FLASH Addr : 0x%08lX\n", FLASH_BASE);
    printf("FLASH Size : %d kB\n", *(uint16_t *)FLASHSIZE_BASE);
    printf("SRAM1 Addr : 0x%08lX\n", SRAM1_BASE);
    printf("SRAM1 Size : %ld kB\n", SRAM1_SIZE_MAX / 1024);
    printf("SRAM2 Addr : 0x%08lX\n", SRAM2_BASE);
    printf("SRAM2 Size : %ld kB\n", SRAM2_SIZE / 1024);

    return 0;
}

int cli_reset(int argc, char *argv[])
{
    uint32_t delay = 10;

    if (argc > 1)
    {
        delay = strtol(argv[0], NULL, 0);
    }

    printf("\n\e[33mReset MCU in [%ld] ms!\e[0m\r\n", delay);

    // Run Pre-Reset command, clear log buffer.
    fflush(stderr);
    fflush(stdout);

    // Delay and reset.
    HAL_Delay(delay);
    HAL_NVIC_SystemReset();
    return 0;
}
