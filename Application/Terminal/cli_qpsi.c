#include "string.h"

#include "stm32l476g_discovery_qspi.h"

const char QSPI_HELPTEXT[] = "Quad SPI flash command usage:\n"
        "\t-i --init    Initialize Quad-SPI\n"
        "\t-m --mount   Mount SPI Flash, it could be accessed at 0x00000000. \n"
        "\t-r --read    [addr] [len]   Read QSPI flash.\n"
        "\t-w --write   [addr] [value] Write QSPI flash.\n"
        "\t-e --erase   Erase QSPI flash.\n"
        "\t-p --property   Show QSPI Flash info \n"
        "\t-h --help    Show this help text.\n";

extern void sys_dump_u8(uint64_t address, uint32_t len);

/**@brief Command line interface for Accel
 *
 * @param argc
 * @param argv
 */
int cli_qspi(int argc, char **argv)
{

    if ((argc == 0) || (strcmp(argv[0], "-h") == 0) || (strcmp(argv[0], "--help") == 0))
    {
        printf("%s", QSPI_HELPTEXT);
        return 0;
    }

    if ((strcmp(argv[0], "-i") == 0) || (strcmp(argv[0], "--init") == 0))
    {
        BSP_QSPI_Init();
    }
    else if ((strcmp(argv[0], "-m") == 0) || (strcmp(argv[0], "--mount") == 0))
    {
        BSP_QSPI_Init();
        BSP_QSPI_EnableMemoryMappedMode();
        __HAL_SYSCFG_REMAPMEMORY_QUADSPI();
    }
    else if ((strcmp(argv[0], "-r") == 0) || (strcmp(argv[0], "--read") == 0))
    {
        uint32_t addr = 0;
        uint32_t size = 0;
        str_to_u32(argv[1], &addr);
        str_to_u32(argv[2], &size);

        uint8_t *readbuf = malloc(size);

        if (readbuf == NULL)
        {
            return -1;
        }

        BSP_QSPI_Read(readbuf, addr, size);

        printf("Read QSPI @ addr[0x%X],size=[%d]\n", addr, size);
        sys_dump_u8(&readbuf[0], size);

        printf("\n");
        free(readbuf);
    }
    else if ((strcmp(argv[0], "-w") == 0) || (strcmp(argv[0], "--write") == 0))
    {
        uint32_t addr = 0;
        uint16_t data_len = argc - 2;
        uint8_t* pdata = malloc(data_len);

        str_to_u32(argv[1], &addr);
        for (int i = 0; i < data_len; i++)
        {
            int ret = str_to_u32(argv[2 + i], pdata + i);
            if (ret == -1)
            {
                printf("ERROR: Can't get number from [%s]\n", argv[2 + i]);
                return -1;
            }
        }

        BSP_QSPI_Write(pdata, addr, data_len);
        printf("Write QSPI @ addr[0x%X], length=[%d]\n", addr, data_len);
        sys_dump_u8(pdata, data_len);
    }

    else if ((strcmp(argv[0], "-e") == 0) || (strcmp(argv[0], "--erase") == 0))
    {
        BSP_QSPI_Erase_Chip();
    }
    else if ((strcmp(argv[0], "-p") == 0) || (strcmp(argv[0], "--property") == 0))
    {
        QSPI_Info info;
        BSP_QSPI_GetInfo(&info);

        printf("QSPI info:\n");
        printf("FlashSize  = %d kB\n", info.FlashSize / 1024);
        printf("SectorSzie = %d kB\n", info.EraseSectorSize / 1024);
        printf("SectorNum  = %d\n", info.EraseSectorsNumber);
        printf("PageSize   = %d Byte\n", info.ProgPageSize);
        printf("PageNumber = %d\n", info.ProgPagesNumber);

    }
    else
    {
        printf("Unknown option of [%s], try [-h] for help.\n", argv[0]);
    }
    return 0;

}
