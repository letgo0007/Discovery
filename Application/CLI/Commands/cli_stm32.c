#include "cli.h"
#include "stdio.h"
#include "stdlib.h"
#include "stm32l4xx_hal.h"

#define FLASH_SIZE (*(uint16_t *)FLASHSIZE_BASE * 1024)

int cli_info(int argc, char *argv[])
{
    printf("HAL VERSION: 0x%08lX\n", HAL_GetHalVersion());
    printf("DEV ID     : 0x%08lX\n", HAL_GetDEVID());
    printf("REV ID     : 0x%08lX\n", HAL_GetREVID());
    printf("UID        : 0x%08lX%08lX%08lX\n", HAL_GetUIDw0(), HAL_GetUIDw1(), HAL_GetUIDw2());

    printf("FLASH Addr : 0x%08lX ~ 0x%08lX\n", FLASH_BASE, FLASH_BASE + FLASH_SIZE - 1);
    printf("FLASH Size : %d kB\n", FLASH_SIZE / 1024);
    printf("SRAM1 Addr : 0x%08lX ~ 0x%08lX\n", SRAM1_BASE, SRAM1_BASE + SRAM1_SIZE_MAX - 1);
    printf("SRAM1 Size : %ld kB\n", SRAM1_SIZE_MAX / 1024);
    printf("SRAM2 Addr : 0x%08lX ~ 0x%08lX\n", SRAM2_BASE, SRAM2_BASE + SRAM2_SIZE - 1);
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
