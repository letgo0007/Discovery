

#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "rtc.h"
const char *rtc_helptext = "rtc command usage:\n"
                           "\t-s --set [RTC] \tSet RTC value, format: 01/02/03 11:22:33\n"
                           "\t-g --get \tGet RTC value\n"
                           "\t-r --reset\tReset RTC to default 00/01/01 00:00:00.\n"
                           "\t-t --tick\tGet System Tick count.\n"
                           "\t-h --help\tShow this help text.\n";

int cli_rtc(int argc, char *argv[])
{
    argc--;
    argv++;

    RTC_DateTypeDef sDate = {1, 1, 1, 0};
    RTC_TimeTypeDef sTime = {0, 0, 0};

    if ((argc == 0) || (strcmp(argv[0], "-h") == 0) || (strcmp(argv[0], "--help") == 0))
    {
        printf("%s", rtc_helptext);
    }
    else if ((strcmp(argv[0], "-s") == 0) || (strcmp(argv[0], "--set") == 0))
    {
        // Command syntax : rtc -s 01/02/03 11:22:33
        char *tail  = NULL;
        sDate.Year  = strtol(strtok(argv[1], " /:"), &tail, 0);
        sDate.Month = strtol(strtok(NULL, " /:"), &tail, 0);
        sDate.Date  = strtol(strtok(NULL, " /:"), &tail, 0);

        sTime.Hours   = strtol(strtok(argv[2], " /:"), &tail, 0);
        sTime.Minutes = strtol(strtok(NULL, " /:"), &tail, 0);
        sTime.Seconds = strtol(strtok(NULL, " /:"), &tail, 0);

        printf("RTC set to: %02d/%02d/%02d %02d:%02d:%02d\n", sDate.Year, sDate.Month, sDate.Date,
               sTime.Hours, sTime.Minutes, sTime.Seconds);

        HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
        HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    }
    else if ((strcmp(argv[0], "-g") == 0) || (strcmp(argv[0], "--get") == 0))
    {

        HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
        HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
        printf("%02d/%02d/%02d %02d:%02d:%02d\n", sDate.Year, sDate.Month, sDate.Date, sTime.Hours,
               sTime.Minutes, sTime.Seconds);
    }
    else if ((strcmp(argv[0], "-r") == 0) || (strcmp(argv[0], "--reset") == 0))
    {
        printf("RTC reset to: %02d/%02d/%02d %02d:%02d:%02d\n", sDate.Year, sDate.Month, sDate.Date,
               sTime.Hours, sTime.Minutes, sTime.Seconds);
        HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
        HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    }
    else if ((strcmp(argv[0], "-t") == 0) || (strcmp(argv[0], "--tick") == 0))
    {
        printf("Get System Tick Count:[%ld]\n", HAL_GetTick());
    }

    else
    {
        printf("Unknow args of [%s], try [-h] for help.\n", argv[0]);
    }

    return 0;
}
