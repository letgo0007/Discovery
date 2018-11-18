/******************************************************************************
 * @file    command_sys.c
 *          Command list for STM32 system function.
 *
 * @author  Nick Yang
 * @date    2018/04/20
 * @version V0.1
 *****************************************************************************/

#include "stdlib.h"
#include "string.h"
#include "sys/unistd.h"

#include "cli.h"
#include "stm32l4xx_hal.h"

/*Pointer to certain address*/
#define CHECK_FUNC_RET(status, func)                                           \
    do {                                                                       \
        int ret = func;                                                        \
        if (status != ret) {                                                   \
            printf("\e[32mERROR: Return=[%d] " #func "<%s:%d>\n\e[0m", ret,    \
                   __FILE__, __LINE__);                                        \
            return ret;                                                        \
        }                                                                      \
    } while (0)

#define HWREG32(x) (*((volatile uint32_t *)((uint32_t)x)))
#define HWREG16(x) (*((volatile uint16_t *)((uint32_t)x)))
#define HWREG8(x) (*((volatile uint8_t *)((uint32_t)x)))

void print_u32(uint32_t *ptr, uint32_t len) {
    for (int i = 0; i < len; i++) {
        printf("0x%08lX ", ptr[i]);
        if (i % 8 == 7) {
            printf("\n");
        }
    }

    printf("\n");
}

void print_u8(uint8_t *ptr, uint32_t len) {
    for (int i = 0; i < len; i++) {
        printf("0x%02X ", ptr[i]);

        if (i % 16 == 15) {
            printf("\n");
        }
    }
    printf("\n");
}

const char *mem_helptext =
    "mem command usage:\n"
    "\t-w --write [addr] [value] Write a value to memory\n"
    "\t-r --read  [addr] Read a value from memory\n"
    "\t-d --dump  [addr] [len] Dump memory content.\n"
    "\t-i --info  Show Memory info\n"
    "\t-h --help  Show this help text.\n";

int cli_mem32(int argc, char *argv[]) {

    uint32_t *pdata = NULL;

    if ((strcmp(argv[0], "-w") == 0) || (strcmp(argv[0], "--write") == 0)) {
        if ((argc < 3) || argv[1] == NULL) {
            return -1;
        }

        // Get Parameters
        uint32_t addr = 0;
        uint16_t size = (argc - 2);
        str_to_u32(argv[1], &addr);

        // Prepare buffer and parse data
        pdata = (uint32_t *)malloc(size * 4);
        if (pdata == NULL) {
            return -1;
        }

        for (int i = 0; i < size; i++) {
            int ret = str_to_u32(argv[2 + i], pdata + i);
            if (ret == -1) {
                printf("\e[31mERROR: Can't get number from [%s]\e[0m\n",
                       argv[2 + i]);
                return -1;
            }
        }

        // Write Data
        memcpy(addr, pdata, size * 4);

        // Print Result
        printf("Memory Write @ addr[0x%lX], length=[%d]\n", addr, size);
        print_u32(pdata, size);
    } else if ((strcmp(argv[0], "-r") == 0) ||
               (strcmp(argv[0], "--read") == 0)) {
        uint32_t addr = 0;
        str_to_u32(argv[1], &addr);

        printf("Memory Read @ addr[0x%lX]=%ld(0x%08lX)\n", addr, HWREG32(addr),
               HWREG32(addr));
    } else if ((strcmp(argv[0], "-d") == 0) ||
               (strcmp(argv[0], "--dump") == 0)) {
        uint32_t addr   = 0;
        uint32_t length = 0;
        str_to_u32(argv[1], &addr);
        str_to_u32(argv[2], &length);

        print_u32(addr, length);
    } else if ((strcmp(argv[0], "-h") == 0) ||
               (strcmp(argv[0], "--help") == 0)) {
        printf("%s", mem_helptext);
    } else {
        printf("Unknown option of [%s], try [-h] for help.\n", argv[0]);
    }

exit:
    if (pdata != NULL) {
        free(pdata);
    }
    return 0;
}

int cli_mem8(int argc, char *argv[]) {
    uint8_t *pdata = NULL;

    if ((argc == 0) || (argv == NULL)) {
        printf("%s", mem_helptext);
        return 0;
    }

    if ((strcmp(argv[0], "-w") == 0) || (strcmp(argv[0], "--write") == 0)) {
        if ((argc < 3) || argv[1] == NULL) {
            return -1;
        }

        // Get Parameters
        uint32_t addr = 0;
        uint16_t size = argc - 2;
        str_to_u32(argv[1], &addr);

        // Prepare buffer and parse data
        pdata = (uint8_t *)malloc(size);
        if (pdata == NULL) {
            return -1;
        }

        for (int i = 0; i < size; i++) {
            int ret = str_to_u8(argv[2 + i], pdata + i);
            if (ret == -1) {
                printf("\e[31mERROR: Can't get number from [%s]\e[0m\n",
                       argv[2 + i]);
                return -1;
            }
        }

        // Write Data
        memcpy(addr, pdata, size);

        // Print Result
        printf("Memory Write @ addr[0x%lX], length=[%d]\n", addr, size);
        print_u8(pdata, size);
    } else if ((strcmp(argv[0], "-r") == 0) ||
               (strcmp(argv[0], "--read") == 0)) {
        uint32_t addr = 0;
        str_to_u32(argv[1], &addr);

        printf("Memory Read @ addr[0x%lX] = %d (0x%02X)\n", addr, HWREG8(addr),
               HWREG8(addr));
    } else if ((strcmp(argv[0], "-d") == 0) ||
               (strcmp(argv[0], "--dump") == 0)) {
        if ((argc < 3) || argv[1] == NULL) {
            return -1;
        }

        uint32_t addr   = 0;
        uint32_t length = 0;
        str_to_u32(argv[1], &addr);
        str_to_u32(argv[2], &length);

        print_u8(addr, length);
    } else if ((strcmp(argv[0], "-h") == 0) ||
               (strcmp(argv[0], "--help") == 0)) {
        printf("%s", mem_helptext);
    } else {
        printf("Unknown option of [%s], try [-h] for help.\n", argv[0]);
    }

exit:
    if (pdata != NULL) {
        free(pdata);
    }
    return 0;
}
