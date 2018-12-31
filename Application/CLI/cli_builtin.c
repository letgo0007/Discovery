/******************************************************************************
 * @file    cli_builtin.c
 * @brief   Built-in commands for CLI
 *
 * @author  Nick Yang
 * @date    2018/12/31
 * @version V1.0
 *****************************************************************************/
#include "cli.h"
#include "cli_builtin.h"

#include "stdio.h"
#include "stdlib.h"
#include "string.h"

extern int cli_getopt(int argc, char **args, char **data_ptr, CliOption_TypeDef options[]);
extern CliCommand_TypeDef *pCmdList_Builtin;
extern CliCommand_TypeDef *pCmdList_External;
extern CliCommand_TypeDef *pCmdList_Alias;

extern char ** pHistoryPtr;
extern unsigned int HistoryQueueHead;
extern unsigned int HistoryQueueTail;
extern unsigned int HistoryMemUsage;

int builtin_debug(int argc, char **args)
{
    const char *helptext = "debug usage\n"
            "\t-e --on     Turn on debug\n"
            "\t-d --off    Turn off\n"
            "\t-l --level  Show or set debug level\n"
            "\t-h --help   Show help text\n";

    if ((argc <= 1) || (strcmp("-h", args[1]) == 0) || (strcmp("--help", args[1]) == 0))
    {
        CLI_PRINT("%s", helptext);
    }
    else if ((strcmp("-e", args[1]) == 0) || (strcmp("--on", args[1]) == 0))
    {
        gCliDebugLevel = 3;
        CLI_PRINT("Turn on debug log\n");
    }
    else if ((strcmp("-d", args[1]) == 0) || (strcmp("--off", args[1]) == 0))
    {
        gCliDebugLevel = 0;
        CLI_PRINT("Turn off debug log\n");
    }
    else if ((strcmp("-l", args[1]) == 0) || (strcmp("--level", args[1]) == 0))
    {
        if (argc <= 2)
        {
            CLI_PRINT("Debug level = %d\n", gCliDebugLevel);
        }
        else
        {
            unsigned int level = strtol(args[2], NULL, 0);
            gCliDebugLevel = level;
            CLI_PRINT("Debug level = %d\n", gCliDebugLevel);
        }
    }
    else
    {
        CLI_ERROR("ERROR: invalid option of [%s]\n", args[1]);
    }

    return 0;

}

/*!@brief Built-in command of "help"
 *
 */
int builtin_help(int argc, char **argv)
{
    CLI_PRINT("\nBuilt-in commands:\n-------------------------------\n")

    if (pCmdList_Builtin != NULL)
    {
        for (int i = 0;; i++)
        {
            if ((pCmdList_Builtin[i].Name != NULL) && (pCmdList_Builtin[i].Prompt != NULL))
            {
                CLI_PRINT("%-12s%s\n", pCmdList_Builtin[i].Name, pCmdList_Builtin[i].Prompt);
            }
            else
            {
                break;
            }
        }
    }

    CLI_PRINT("\nExternal commands:\n-------------------------------\n")
    for (int i = 0; i < CLI_NUM_OF_EXTERNAL_CMD; i++)
    {
        if ((pCmdList_External[i].Name != NULL) && (pCmdList_External[i].Prompt != NULL))
        {
            CLI_PRINT("%-12s%s\n", pCmdList_External[i].Name, pCmdList_External[i].Prompt);
        }
    }

    CLI_PRINT("\nAlias\n-------------------------------\n")
    for (int i = 0; i < CLI_NUM_OF_ALIAS; i++)
    {
        if ((pCmdList_Alias[i].Name != NULL) && (pCmdList_Alias[i].Prompt != NULL))
        {
            CLI_PRINT("%-12s%s\n", pCmdList_Alias[i].Name, pCmdList_Alias[i].Prompt);
        }
    }

    CLI_PRINT("\n");
    return 0;
}

/*!@brief Built-in command of "history"
 *
 */
int builtin_history(int argc, char **args)
{
    const char *helptext = "history usage:\n"
            "\t-d --dump  Dump command history.\n"
            "\t-c --clear Clear command history.\n"
            "\t-h --help  Show this help text.\n";

#if HISTORY_ENABLE == 0
    CLI_PRINT("History is function disabled.\n");
    return -1;
#endif

    if ((argc < 2) || (args[argc - 1] == NULL))
    {
        CLI_PRINT("%s", helptext);
        return -1;
    }

    if ((strcmp("-d", args[1]) == 0) || (strcmp("--dump", args[1]) == 0))
    {
        CLI_PRINT("History Mem Usage = %d\n", HistoryMemUsage);
        CLI_PRINT("History dump:\n");
        CLI_PRINT("Index  Address    Command\n");
        CLI_PRINT("-------------------------\n");
        for (int i = HistoryQueueTail; i < HistoryQueueHead; i++)
        {
            int j = i % HISTORY_DEPTH;
            CLI_PRINT("%-6d 0x%08X %s\n", i, (int ) pHistoryPtr[j],
                    (pHistoryPtr[j] == NULL) ? "NULL" : pHistoryPtr[j]);
        }
    }
    else if ((strcmp("-c", args[1]) == 0) || (strcmp("--clear", args[1]) == 0))
    {
        CLI_PRINT("History clear!\n");
        history_clear();
    }
    else if ((strcmp("-h", args[1]) == 0) || (strcmp("--help", args[1]) == 0))
    {
        CLI_PRINT("%s", helptext);
    }
    else
    {
        CLI_ERROR("ERROR: invalid option of [%s]\n", args[1]);
    }

    return 0;
}

/*!@brief Built-in command of "repeat"
 *
 */
int builtin_repeat(int argc, char **args)
{
    const char *helptext = "usage: repeat [num] \"command\"\n";

    if ((argc < 3) || (args == NULL))
    {
        CLI_PRINT("%s", helptext);
        return -1;
    }

    unsigned int count = strtol(args[1], NULL, 0);

    for (unsigned int i = 1; i <= count; i++)
    {

        CLI_INFO("%sRepeat %d/%d: [%s] %s\n", ANSI_BOLD, i, count, args[2], ANSI_RESET);
        int ret = CLI_ExecuteByString(args[2]);
        if (ret != 0)
        {
            return ret;
        }
    }

    return 0;
}

/*!@brief Built-in command of "sleep"
 *
 */
int builtin_sleep(int argc, char **args)
{
    const char *helptext = "usage: sleep [seconds]\n";

    if ((argc <= 1) || (args[1] == NULL))
    {
        CLI_PRINT("%s", helptext);
        return -1;
    }

    float sec = strtof(args[1], NULL);
    cli_sleep(sec * 1000);

    return 0;
}

/*!@brief Built-in command of "test"
 *
 */
int builtin_test(int argc, char **args)
{
    CLI_PRINT("Argc = %d\n", argc);
    for (int i = 0; i < argc; i++)
    {
        CLI_PRINT("Args[%d] = %s\n", i, args[i] == NULL ? "NULL" : args[i]);
    }

    static CliOption_TypeDef options[] = { { 'i', "integer", 'i' }, { 's', "string", 's' }, { 0,
            "bool", 'b' }, { 'h', "help", 'h' }, { 0, "", 0 } };

    int c = 0;
    char *data[1] = { 0 };

    do
    {
        c = cli_getopt(argc, args, data, options);

        switch (c)
        {
        case 'i':
        {
            if (*data != NULL)
            {
                CLI_PRINT("Get Integer value of [%s]\n", *data);
            }
            break;
        }
        case 's':
        {
            if (*data != NULL)
            {
                CLI_PRINT("Get String value of [%s]\n", *data);
            }
            break;
        }
        case 'b':
        {
            CLI_PRINT("Bool flag is set\n");
            break;
        }
        case 'h':
        {
            CLI_PRINT("help text here!");
            break;
        }
        case '?':
        default:
        {
            CLI_ERROR("ERROR: invalid option of [%s]\n", (*data == NULL ? "NULL" : *data));
            return -1;
        }
        }

    } while (c != -1);

    return 0;
}

/*!@brief Built-in command of "time"
 *
 */
int builtin_time(int argc, char **args)
{
    const char *helptext = "usage: time [command]\n";

    if ((argc <= 1) || (args[1] == NULL))
    {
        CLI_PRINT("%s", helptext);
        return -1;
    }

    unsigned int start = cli_gettick();
    int ret = CLI_ExecuteByArgs(argc - 1, args + 1);
    unsigned int stop = cli_gettick();

    CLI_PRINT("time: %d.%03d s\n", (stop - start) / 1000, (stop - start) % 1000);

    return ret;
}

int builtin_version(int argc, char **argv)
{
    CLI_PRINT("\r\n---------------------------------\n");
    CLI_PRINT("Command Line Interface %s\n", CLI_VERSION);
    CLI_PRINT("Compiler      = %s\n", __VERSION__);
    CLI_PRINT("Date/Time     = %s %s\n", __DATE__, __TIME__);
    CLI_PRINT("---------------------------------\n");
    return 0;
}

const CliCommand_TypeDef gConstBuiltinCmdList[CLI_NUM_OF_BUILTIN_CMD] = {
        { "debug", "Set debug level", &builtin_debug, },   //
        { "help", "Show list of commands & prompt.", &builtin_help, },                    //
        { "history", "Show command history", &builtin_history, },                         //
        { "test", "CLI argument parse example", &builtin_test, },                         //
        { "repeat", "Repeat execute a command", &builtin_repeat, },                       //
        { "sleep", "Put CLI to sleep for an interval of time", &builtin_sleep, },         //
        { "time", "Time command execution", &builtin_time, },                             //
        { "version", "Show CLI version", &builtin_version, }

};
