#include "stdio.h"
#include "stdlib.h"
#include "stm32l476g_discovery_qspi.h"
#include "string.h"

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

extern void *cli_calloc(size_t size);
extern void  cli_free(void *ptr);
extern void  print_u8(uint8_t *ptr, uint32_t len);

void *cli_qspi_malloc(int size)
{
    return cli_calloc(size);
}

void cli_qspi_free(void *ptr)
{
    return cli_free(ptr);
}

int cli_qspi_selftest()
{
    return 0;
}

/**@brief Command line interface for Accel
 *
 * @param argc
 * @param argv
 */
int cli_qspi(int argc, char **argv)
{
    const char *QSPI_HELPTEXT = "Quad-SPI Flash commands:\n"
                                "\t-i --init        QSPI Flash initialize\n"
                                "\t-m --mount       QSPI Flash mount to system address 0x00.\n"
                                "\t-p --property    Show QSPI Flash info \n"
                                "\t-s --selftest    Run QSPI self test.\n"
                                "\t-r --read  [addr] [len]\n"
                                "\t                 Read QSPI flash.\n"
                                "\t-w --write [addr] [value]...[value]\n"
                                "\t                 Write QSPI flash. Maximum a page (256 Byte)\n"
                                "\t-e --erase [addr]\n"
                                "\t                 Erase a sub-sector (4kB) at certain address\n"
                                "\t   --erase_sector [start_sec] [sec_num]\n"
                                "\t                 Erase sectors (64kB) of QSPI Flash.\n"
                                "\t   --erase_all   Erase entire QSPI Flash.\n"
                                "\t-c --copy  [src_addr] [dst_addr] [size]\n"
                                "\t                 Copy data from system memory to QSPI flash.\n"
                                "\t-h --help        Show this help text.\n";

    uint8_t *pdata   = NULL;
    char *   tail[1] = {0};

    argc--;
    argv++;

    if ((argc == 0) || (strcmp(argv[0], "-h") == 0) || (strcmp(argv[0], "--help") == 0))
    {
        printf("%s", QSPI_HELPTEXT);
        return 0;
    }

    if ((strcmp(argv[0], "-i") == 0) || (strcmp(argv[0], "--init") == 0))
    {
        CHECK_FUNC_EXIT(QSPI_OK, BSP_QSPI_Init());

        printf("QSPI Initialize OK!\n");
    }
    else if ((strcmp(argv[0], "-m") == 0) || (strcmp(argv[0], "--mount") == 0))
    {
        CHECK_FUNC_EXIT(QSPI_OK, BSP_QSPI_EnableMemoryMappedMode());
        __HAL_SYSCFG_REMAPMEMORY_QUADSPI();

        printf("QSPI Mounted @ 0x00000000\n");
    }
    else if ((strcmp(argv[0], "-r") == 0) || (strcmp(argv[0], "--read") == 0))
    {
        if ((argc < 2) || argv[1] == NULL || argv[2] == NULL)
        {
            goto syntax_error;
        }

        // Get Parameters
        uint32_t addr = strtoul(argv[1], tail, 0);
        uint32_t size = strtoul(argv[2], tail, 0);

        // Prepare Buffer
        if (size == 0)
        {
            goto syntax_error;
        }
        pdata = (uint8_t *)cli_qspi_malloc(size);

        // Read Flash
        CHECK_FUNC_EXIT(QSPI_OK, BSP_QSPI_Read(pdata, addr, size));

        // Print Results & free buffer
        printf("Read QSPI @ addr[0x%lX], size=[%ld]\n", addr, size);
        print_u8(pdata, size);
    }
    else if ((strcmp(argv[0], "-w") == 0) || (strcmp(argv[0], "--write") == 0))
    {
        if ((argc < 3) || argv[1] == NULL)
        {
            goto syntax_error;
        }

        // Get Parameters
        uint32_t addr = strtoul(argv[1], tail, 0);
        uint16_t size = argc - 2;

        // Prepare buffer and parse data
        pdata = (uint8_t *)cli_qspi_malloc(size);

        for (int i = 0; i < size; i++)
        {
            *(pdata + i) = strtoul(argv[2 + i], tail, 0);
        }

        // Write Flash
        CHECK_FUNC_EXIT(QSPI_OK, BSP_QSPI_Write(pdata, addr, size));

        // Print Result
        printf("Write QSPI @ addr[0x%lX], length=[%d]\n", addr, size);
        print_u8(pdata, size);
    }
    else if (strcmp(argv[0], "--erase_all") == 0)
    {
        QSPI_Info info;
        BSP_QSPI_GetInfo(&info);

        printf("QSPI Chip erase start.\n");
        for (uint32_t i = 0; i < info.SectorNumber; i++)
        {
            CHECK_FUNC_EXIT(QSPI_OK, BSP_QSPI_Erase_Sector(i));
            printf("\rErasing Sector [%4ld], Erased = [%8ld kB]", i, (i + 1) * (info.SectorSize));
        }
        printf("\nQSPI Chip erase OK!\n");
    }
    else if (strcmp(argv[0], "--erase_sector") == 0)
    {
        QSPI_Info info;
        BSP_QSPI_GetInfo(&info);

        // Get Sector ID
        uint32_t sector_start = strtoul(argv[1], tail, 0);
        uint32_t sector_num   = strtoul(argv[2], tail, 0);

        printf("QSPI Erase Sector [0x%lX] ~ [0x%lX]\n", sector_start, sector_start + sector_num);
        for (uint32_t i = 0; i < sector_num; i++)
        {
            CHECK_FUNC_EXIT(QSPI_OK, BSP_QSPI_Erase_Sector(sector_start + i));
            printf("\nErasing Sector [%4ld], Erased = [%8ld kB]", sector_start + i,
                   (i + 1) * (info.SectorSize));
        }

        printf("\nQSPI Erase Sector OK!\n");
    }
    else if ((strcmp(argv[0], "-e") == 0) || (strcmp(argv[0], "--erase") == 0))
    {
        if ((argc < 2) || argv[1] == NULL)
        {
            goto syntax_error;
        }

        // Get Sector ID
        uint32_t address = strtoul(argv[1], tail, 0);

        // Erase Page
        printf("QSPI Erase @ [0x%lX]\n", address);
        CHECK_FUNC_EXIT(QSPI_OK, BSP_QSPI_Erase_Block(address));
    }
    else if ((strcmp(argv[0], "-c") == 0) || (strcmp(argv[0], "--copy") == 0))
    {
        if ((argc < 3) || (argv[1] == NULL) || (argv[2] == NULL) || (argv[3] == NULL))
        {
            goto syntax_error;
        }

        // Get Parameters
        uint32_t src_addr = strtoul(argv[1], tail, 0);
        uint32_t dst_addr = strtoul(argv[2], tail, 0);
        uint32_t size     = strtoul(argv[3], tail, 0);

        // Write QSPI
        CHECK_FUNC_EXIT(QSPI_OK, BSP_QSPI_Write((uint8_t *)src_addr, dst_addr, size));

        // Print Results
        printf("Copy data [0x%lX] -> [0x%lX], size = %ld\n", src_addr, dst_addr, size);
    }
    else if ((strcmp(argv[0], "-s") == 0) || (strcmp(argv[0], "--selftest") == 0))
    {
        printf("QSPI Self Test. TBD...\n");
    }
    else if ((strcmp(argv[0], "-p") == 0) || (strcmp(argv[0], "--property") == 0))
    {
        QSPI_Info info;
        BSP_QSPI_GetInfo(&info);

        printf("QSPI info:\n");
        printf("FlashSize       = %ld kB\n", info.FlashSize / 1024);
        printf("SectorSize      = %ld kB\n", info.SectorSize / 1024);
        printf("SectorNum       = %ld\n", info.SectorNumber);
        printf("EraseSectorSize = %ld kB\n", info.EraseSectorSize / 1024);
        printf("EraseSectorNum  = %ld\n", info.EraseSectorsNumber);
        printf("ProgPageSize    = %ld Byte\n", info.ProgPageSize);
        printf("ProgPageNumber  = %ld\n", info.ProgPagesNumber);
    }
    else
    {
        printf("\e[31mERROR: Unknown option of [%s], try [-h] for help.\e[0m\n", argv[0]);
    }

exit:
    if (pdata != NULL)
    {
        cli_qspi_free(pdata);
    }
    return 0;

syntax_error:
    printf("\e[31mERROR: Unknown command syntax for [%s], try [-h].\e[0m\n", argv[0]);
    return -1;
}
