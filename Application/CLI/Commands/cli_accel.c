#include "accelero.h"
#include "stm32l476g_discovery_compass.h"
#include "string.h"

extern ACCELERO_DrvTypeDef *AccelerometerDrv;

/**@brief Command line interface for Accel
 *
 * @param argc
 * @param argv
 */
int cli_accel(int argc, char **argv)
{

    const char ACCEL_HELPTEXT[] = "Accelerometer command usage:\n"
                                  "\t-o --on   Turn on accelerometer\n"
                                  "\t-f --off  Turn off accelerometer\n"
                                  "\t-r --read Read accelerometer value\n"
                                  "\t-i --info Show accelerometer information\n"
                                  "\t-h --help Show this help text.\n";

    if ((argc == 0) || (strcmp(argv[0], "-h") == 0) || (strcmp(argv[0], "--help") == 0))
    {
        printf("%s", ACCEL_HELPTEXT);
        return 0;
    }

    if ((strcmp(argv[0], "-o") == 0) || (strcmp(argv[0], "--on") == 0))
    {
        BSP_COMPASS_Init();
        return 0;
    }

    if (AccelerometerDrv == NULL)
    {
        printf("\e[31mERROR: No Acceleration Meter driver available!\n\e[0m");
        return 0;
    }

    if ((strcmp(argv[0], "-f") == 0) || (strcmp(argv[0], "--off") == 0))
    {
        BSP_COMPASS_LowPower();
    }
    else if ((strcmp(argv[0], "-r") == 0) || (strcmp(argv[0], "--read") == 0))
    {
        if (AccelerometerDrv->GetXYZ != NULL)
        {
            int16_t pDataXYZ[3] = {0};
            AccelerometerDrv->GetXYZ(pDataXYZ);

            printf("Acceleration Meter\nX=[%d]\nY=[%d]\nZ=[%d]\n", pDataXYZ[0], pDataXYZ[1],
                   pDataXYZ[2]);
        }
    }
    else if ((strcmp(argv[0], "-i") == 0) || (strcmp(argv[0], "--info") == 0))
    {
        if (AccelerometerDrv->ReadID != NULL)
        {
            uint8_t id = 0;
            id         = AccelerometerDrv->ReadID();

            printf("Acceleration Meter, Device ID = [0x%X]\n", id);
        }
    }
    else
    {
        printf("Unknown option of [%s], try [-h] for help.\n", argv[0]);
    }
    return 0;
}
