/******************************************************************************
 * @file    cli_term.c
 *          Command Line Interface for Terminal self debug usage.
 *
 *
 * @author  Nick Yang
 * @date    2018/04/27
 * @version V0.2
 *****************************************************************************/

#include <cli.h>
#include <stdio.h>
#include <string.h>
#include <app_terminal.h>

/*!@brief A simple example of handling un-used args. Just print them.
 *
 * @param argc[in]  Argument Count
 * @param args[in]  Pointer to Argument String
 * @retval 0        Success
 * @retval -1       Failure
 */
int print_args(int argc, char **args)
{
    if ((argc == 0) || (args == 0))
    {
        return -1;
    }

    //Example of a call back to handle un-used args.
    //Pass un-used args back;
    int i;
    printf("Un-used argc = [%d]\nUn-used args = ", argc);

    for (i = 0; i < argc; i++)
    {
        printf("%s ", args[i]);
    }
    printf("\n");
    return 0;
}

/*!@brief Handler for command "test".
 *        An example to get 1x interger value, 1x string value, 1x bool value.
 *
 * @param argc[in]  Argument Count
 * @param args[in]  Pointer to Argument String
 *
 * @retval 0        Success
 * @retval -1       Failure
 */
int cli_test(int argc, char *args[])
{
    //It's Recommended to build a temperory struct to store result.
    struct DataStruct
    {
        int IntData[16];
        char StringData[128];
        _Bool BoolData;
    } Tempdata;

    memset(&Tempdata, 0, sizeof(Tempdata));
    memset(Tempdata.IntData, -1, sizeof(int) * 16);

    //Build the option list for main
    Cli_OptionTypeDef MainOpt[] =
    {
    { OPT_COMMENT, 0, NULL, "Basic Options", NULL },
    { OPT_HELP, 'h', "help", "Show help hints", NULL },
    { OPT_INT, 'i', "int", "Get a Integer value", (void*) Tempdata.IntData, NULL, 1, 16 },
    { OPT_STRING, 's', "string", "Get a String value", (void*) Tempdata.StringData },
    { OPT_BOOL, 'b', "bool", "Get a Boolean value", (void*) &Tempdata.BoolData },
    { OPT_END, 0, NULL, NULL, NULL, print_args } };

    //Run Arguments parse using MainOpt
    Cli_parseArgs(argc, args, MainOpt);

    //Print Result
    printf("\nInt:\t");
    int i = 0;
    while (Tempdata.IntData[i] != -1)
    {
        printf("[%d]", Tempdata.IntData[i]);
        i++;
    }
    printf("\nString:\t[%s]\nBool:\t[%d]\n", Tempdata.StringData, Tempdata.BoolData);

    return 0;
}

/*!@brief Handler for command "time". Print Unix time stamp.
 */
int cli_ver(int argc, char *args[])
{
    printf("Version(compile time): %s %s\n", __DATE__, __TIME__);
    return 0;
}

int cli_echo(int argc, char *args[])
{
    int i = 0;
    char strbuf[256] =
    { 0 };

    while (argc--)
    {
        strcat(strbuf, args[i++]);
        strcat(strbuf, " ");
    }
    strcat(strbuf, "\n");
    printf("%s", strbuf);
    return 0;
}

int cli_history(int argc, char *args[])
{
    int i = 0;

    printf("Command History:\n[Index][Command]\n");
    for (i = 0; i < TERM_HISTORY_DEPTH; i++)
    {
        printf("[%5d][%s]%s%s\n", i,                                    // History index
                gTermHandle.HistoryBuf[i],                              // History string
                (i == gTermHandle.HistoryPushIndex) ? "<-Push" : "",    // Push index
                (i == gTermHandle.HistoryPullIndex) ? "<-Pull" : "");   // Pull index
    }
    return 0;
}
