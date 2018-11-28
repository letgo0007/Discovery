/******************************************************************************
 * @file    cli.c
 * @brief   A simple Command Line Interface (CLI) for MCU.
 *
 * @author  Nick Yang
 * @date    2018/11/01
 * @version V1.0
 *****************************************************************************/
/** Includes ----------------------------------------------------------------*/

#include "cli.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
/** Private defines ---------------------------------------------------------*/

/** Private function prototypes ---------------------------------------------*/
extern void         cli_sleep(int ms);
extern unsigned int cli_gettick(void);
extern void *       cli_malloc(size_t size);
extern void         cli_free(void *ptr);
extern int          cli_port_init(void);
extern void         cli_port_deinit(void);
extern int          cli_port_getc(void);
extern int          cli_getopt(int argc, char **args, char **data_ptr, CliOption_TypeDef options[]);

/** Variables ---------------------------------------------------------------*/
char *              StringPtr        = NULL; // Command String buffer pointer
unsigned int        StringIdx        = 0;    // Command string index
char **             HistoryPtr       = NULL; // History pointer buffer pointer
unsigned int        HistoryQueueHead = 0;    // History queue head
unsigned int        HistoryQueueTail = 0;    // History queue tail
unsigned int        HistoryPullDepth = 0;    // History pull depth
unsigned int        HistoryMemUsage  = 0;    // History total memory usage
CliCommand_TypeDef *CliCommandList   = NULL; // CLI commands list pointer
unsigned int        CliNumOfBuiltin  = 0;    // Number of built-in commands
unsigned int        CliNumOfCommands = 0;    // Number of commands

/** Functions ---------------------------------------------------------------*/
/*!@brief Insert a char to a position of a string.
 *
 * @param string    Pointer to the old string
 * @param c         Char to insert
 * @param pos       Position to insert
 * @return          Pointer to the new string or NULL when it fails.
 */
char *insert_char(char *string, char c, int pos)
{
    if ((string == NULL) || (pos > strlen(string)))
    {
        return NULL;
    }

    // right shift buffer from end to insert point
    for (int i = strlen(string) + 1; i > pos; i--)
    {
        string[i] = string[i - 1];
    }

    // insert value
    string[pos] = c;

    return string;
}

/*!@brief Insert a char to a position of a string.
 *
 * @param string    Pointer to the old string
 * @param pos       Position to delete
 * @return          Pointer to the new string or NULL when it fails.
 */
char *delete_char(char *string, int pos)
{
    if ((string == NULL) || (pos > strlen(string)))
    {
        return NULL;
    }

    // left shift buffer from index to end
    for (int i = pos; i < strlen(string) + 1; i++)
    {
        string[i - 1] = string[i];
    }

    return string;
}

/*!@brief Print a new line on terminal and set the curse to a position.
 *
 * @param string    String to print.
 * @param pos       Position to put the curse
 */
void print_newline(char *string, int pos)
{
    // Erase terminal line, print new buffer string and Move cursor
    printf("%s\r%s%s", ANSI_ERASE_LINE, CLI_PROMPT_CHAR, string);
    printf("\e[%luG", (uint32_t)pos + strlen(CLI_PROMPT_CHAR) + 1);
}

/*!@brief Clear history buffer & heap.
 *
 */
void history_clear(void)
{
    if (HistoryPtr != NULL)
    {
        // Try free string heap if used.
        for (int i = 0; i < HISTORY_DEPTH; i++)
        {
            if (HistoryPtr[i] != NULL)
            {
                cli_free(HistoryPtr[i]);
                HistoryPtr[i] = NULL;
            }
        }

        // Reset index
        HistoryQueueHead = 0;
        HistoryQueueTail = 0;
        HistoryPullDepth = 0;
        HistoryMemUsage  = 0;
    }
}

/*!@brief Get the number of commands stored in history heap.
 *
 * @return
 */
int history_getdepth(void)
{
    return HistoryQueueHead - HistoryQueueTail;
}

/*!@brief Get the number of bytes heap memory used to store history string.
 *
 * @return
 */
int history_getmem(void)
{
    return HistoryMemUsage;
}

/*!@brief Push a string to history queue head.
 *
 * @param string    String to put to history
 * @return Pointer to where the history is stored.
 */
char *history_push(char *string)
{
    if ((string == NULL) || (HistoryPtr == NULL))
    {
        return NULL;
    }

    // Request memory & copy command
    unsigned int len = strlen(string) + 1;
    char *       ptr = cli_malloc(len);
    memcpy(ptr, string, len);

    // Save new history queue pointer & queue head.
    HistoryPtr[HistoryQueueHead % HISTORY_DEPTH] = ptr;
    HistoryQueueHead++;
    HistoryMemUsage += len;

    // Release History buffer if number or memory usage out of limit
    while ((history_getdepth() >= HISTORY_DEPTH) || (history_getmem() >= HISTORY_MEM_SIZE))
    {
        // Release from Queue Tail
        int idx = HistoryQueueTail % HISTORY_DEPTH;

        if (HistoryPtr[idx] != NULL)
        {
            HistoryMemUsage -= strlen(HistoryPtr[idx]) + 1;
            cli_free(HistoryPtr[idx]);
            HistoryPtr[idx] = NULL;
        }

        HistoryQueueTail++;
    }

    return ptr;
}

/*!@brief Pull a string from history buffer at certain depth.
 *
 * @param depth     The depth of history to pull.
 *                  1 means you are pulling the newest, larger value means
 * older.
 * @return          Pointer to pulled history buffer or NULL for failure.
 */
char *history_pull(int depth)
{
    if (HistoryPtr == NULL)
    {
        return NULL;
    }

    // Calculate where to pull the history.
    unsigned int pull_idx = (HistoryQueueHead - depth) % HISTORY_DEPTH;

    if (HistoryPtr[pull_idx] != NULL)
    {
        // Pull out history to string buffer
        memset(StringPtr, 0, CLI_STR_BUF_SIZE);
        strcpy(StringPtr, HistoryPtr[pull_idx]);
        StringIdx = strlen(StringPtr);

        // Print new line on console
        print_newline(StringPtr, StringIdx);
    }
    else
    {
        // Put string buffer to empty if no history
        memset(StringPtr, 0, CLI_STR_BUF_SIZE);
        StringIdx = 0;
        // Print new line on console
        print_newline(StringPtr, StringIdx);
    }

    return HistoryPtr[pull_idx];
}

/*!@brief Handle specail key from key board.
 *        Check if a string is part of ANSI escape sequence.
 *        Refer to <https://en.wikipedia.org/wiki/ANSI_escape_code>
 *        Loop put the character from a string to this function.
 *        This function give terminal the ability to response to some multi-byte
 * Keyboard keys.
 *
 * @param  c    Character to check.
 * @retval 0    The character is part of escape sequence.
 * @retval c    The character is not part
 */
int handle_special_key(char c)
{
    static char EscBuf[8] = {0};
    static int  EscIdx    = 0;
    static char EscFlag   = 0;

    // Start of ESC flow control
    if (c == '\e')
    {
        EscFlag = 1;
        EscIdx  = 0;
        memset(EscBuf, 0, 8);
    }

    // Return the character unchanged if not Escape sequence.
    if (EscFlag == 0)
    {
        return c;
    }
    else
    {
        // Put character to Escape sequence buffer
        EscBuf[EscIdx++] = c;

        if (strcmp(EscBuf, ANSI_CURSOR_UP) == 0) //!< Up Arrow
        {
            if (HistoryPullDepth < history_getdepth())
            {
                HistoryPullDepth++;
            }
            history_pull(HistoryPullDepth);
        }
        else if (strcmp(EscBuf, ANSI_CURSOR_DOWN) == 0) //!< Down Arrow
        {
            if (HistoryPullDepth > 0)
            {
                HistoryPullDepth--;
            }
            history_pull(HistoryPullDepth);
        }
        else if (strcmp(EscBuf, ANSI_CURSOR_RIGHT) == 0) //!< Right arrow
        {
            if (StringPtr[StringIdx] != 0)
            {
                StringIdx++;
                printf("%s", ANSI_CURSOR_RIGHT);
            }
        }
        else if (strcmp(EscBuf, ANSI_CURSOR_LEFT) == 0) //!< Left arrow
        {
            if (StringIdx > 0)
            {
                StringIdx--;
                printf("%s", ANSI_CURSOR_LEFT);
            }
        }

        // Escape Sequence is ended by a Letter, clear buffer and flag for next
        // new operation.
        if (((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')))
        {
            EscFlag = 0;
            memset(EscBuf, 0, 8);
            EscIdx = 0;
        }

        return 0;
    }

    return 0;
}

/*!@brief Built-in command of "help"
 *
 */
int builtin_help(int argc, char **argv)
{
    printf("\r\nBuilt-in Commands [%d]:\n", CliNumOfBuiltin);
    printf("-------------------------------------------\n");
    for (int i = 0; i < CliNumOfBuiltin; i++)
    {
        if ((CliCommandList[i].Name != NULL) && (CliCommandList[i].Prompt != NULL))
        {
            printf("%-12s%s\n", CliCommandList[i].Name, CliCommandList[i].Prompt);
        }
    }

    printf("\r\nRegistered Commands [%d]: \n", CliNumOfCommands - CliNumOfBuiltin);
    printf("-------------------------------------------\n");
    for (int i = CliNumOfBuiltin; i < CLI_COMMAND_SIZE; i++)
    {
        if ((CliCommandList[i].Name != NULL) && (CliCommandList[i].Prompt != NULL))
        {
            printf("%-12s%s\n", CliCommandList[i].Name, CliCommandList[i].Prompt);
        }
    }
    printf("\n");
    return 0;
}

int builtin_version(int argc, char **argv)
{
    printf("\r\n---------------------------------\n");
    printf("Command Line Interface %s\n", CLI_VERSION);
    printf("Compiler      = %s\n", __VERSION__);
    printf("Date/Time     = %s %s\n", __DATE__, __TIME__);
    printf("---------------------------------\n");
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
    printf("History is function disabled.\n");
    return -1;
#endif

    if ((argc < 2) || (args[argc - 1] == NULL))
    {
        printf("%s", helptext);
        return -1;
    }

    if ((strcmp("-d", args[1]) == 0) || (strcmp("--dump", args[1]) == 0))
    {
        printf("History Mem Usage = %d\n", HistoryMemUsage);
        printf("History dump:\n");
        printf("Index  Address    Command\n");
        printf("-------------------------\n");
        for (int i = HistoryQueueTail; i < HistoryQueueHead; i++)
        {
            int j = i % HISTORY_DEPTH;
            printf("%-6d 0x%08X %s\n", i, (int)HistoryPtr[j],
                   (HistoryPtr[j] == NULL) ? "NULL" : HistoryPtr[j]);
        }
    }
    else if ((strcmp("-c", args[1]) == 0) || (strcmp("--clear", args[1]) == 0))
    {
        printf("History clear!\n");
        history_clear();
    }
    else if ((strcmp("-h", args[1]) == 0) || (strcmp("--help", args[1]) == 0))
    {
        printf("%s", helptext);
    }
    else
    {
        printf("\%sERROR: invalid option of [%s]%s\n", ANSI_RED, args[1], ANSI_RESET);
    }

    return 0;
}

/*!@brief Built-in command of "test"
 *
 */
int builtin_test(int argc, char **args)
{
    printf("Argc = %d\n", argc);
    for (int i = 0; i < argc; i++)
    {
        printf("Args[%d] = %s\n", i, args[i] == NULL ? "NULL" : args[i]);
    }

    static CliOption_TypeDef options[] = {{'i', "integer", 'i'},
                                          {'s', "string", 's'},
                                          {0, "bool", 'b'},
                                          {'h', "help", 'h'},
                                          {0, "", 0}};

    int   c       = 0;
    char *data[1] = {0};

    do
    {
        c = cli_getopt(argc, args, data, options);

        switch (c)
        {
        case 'i':
        {
            if (*data != NULL)
            {
                printf("Get Integer value of [%s]\n", *data);
            }
            break;
        }
        case 's':
        {
            if (*data != NULL)
            {
                printf("Get String value of [%s]\n", *data);
            }
            break;
        }
        case 'b':
        {
            printf("Bool flag is set\n");
            break;
        }
        case 'h':
        {
            printf("help text here!");
            break;
        }
        case '?':
        default:
        {
            printf("%sERROR: invalid option of [%s]%s\n", ANSI_RED,
                   (*data == NULL ? "NULL" : *data), ANSI_RESET);
            return -1;
        }
        }

    } while (c != -1);

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
        printf("%s", helptext);
        return -1;
    }

    unsigned int count = strtol(args[1], NULL, 0);

    for (unsigned int i = 1; i <= count; i++)
    {

        printf("-----\n%sRepeat %d/%d: [%s] %s\n", ANSI_BOLD, i, count, args[2], ANSI_RESET);
        int ret = Cli_RunByString(args[2]);
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
        printf("%s", helptext);
        return -1;
    }

    float sec = strtof(args[1], NULL);
    cli_sleep(sec * 1000);

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
        printf("%s", helptext);
        return -1;
    }

    unsigned int start = cli_gettick();
    int          ret   = Cli_RunByArgs(argc - 1, args + 1);
    unsigned int stop  = cli_gettick();

    printf("time: %d.%03d s\n", (stop - start) / 1000, (stop - start) % 1000);

    return ret;
}

/*!@brief   Get options from arguments.
 *          This is a implement for "getopt" & "getopt_long" in standard C++
 * liberary. This function check all the arguments and return the index of
 * option if the argument has a format of
 *          "-x" or "--xxxxx", and it matches short name or long name in the
 * options list.. Generally this function should be called in loop until it
 * returns '0'.
 *
 * @example Refer to builtin_test as an example. a simple example as below:
 *          int ret = 0;
 *          CliOption_TypeDef options[] = {{'a',"aaa",'a'}};
 *          do {
 *              ret = cli_getopt(argc, args, data, options);
 *              switch (ret) {
 *              case 'a':
 *                  ...; break;
 *              case '?':
 *                  printf("Unknown option!");
 *                  ...; break;
 *              }
 *          }while(ret != -1)
 *
 * @param   argc        Argument count
 * @param   args        Argument vector
 * @param   data_ptr    Pointer to data argument of current
 * @param   options     Option list, refer to @typedef CliOption_TypeDef
 * @retval  -1          End of operation, all arguments processed.
 *          '?'         Get an unknown option that is not in the options list.
 *          others      ReturnVal in the options list that matches current
 * argument.
 */
int cli_getopt(int argc, char **args, char **data_ptr, CliOption_TypeDef options[])
{
    static int    op_argc = 0;
    static char **op_args = NULL;
    static int    op_idx  = 0;
    static int    op_ret  = '?';

    if ((op_argc != argc) || (op_args != args))
    {
        op_argc = argc;
        op_args = args;
        op_idx  = 1; // ignore the 1st argument, it's the command name.
        op_ret  = '?';
    }

    if ((op_argc > 0) && (op_idx < op_argc) && (op_args[op_idx] != NULL))
    {
        // Long options with "--"
        if ((args[op_idx][0] == '-') && (args[op_idx][1] == '-'))
        {
            *data_ptr = NULL;

            int i = 0;
            while (options[i].ReturnVal != 0)
            {
                if (options[i].LongName[0] != 0)
                {
                    if (strcmp(&args[op_idx][2], options[i].LongName) == 0)
                    {
                        op_ret = options[i].ReturnVal;
                        goto exit;
                    }
                }
                i++;
            }
            *data_ptr = args[op_idx];
            op_ret    = '?';
            goto exit;
        }
        // Short Options with "-"
        else if (args[op_idx][0] == '-')
        {
            *data_ptr = NULL;

            int i = 0;
            while (options[i].ReturnVal != 0)
            {
                if ((args[op_idx][1] == options[i].ShortName) && (options[i].ShortName != 0))
                {
                    op_ret = options[i].ReturnVal;
                    goto exit;
                }
                i++;
            }
            *data_ptr = args[op_idx];
            op_ret    = '?';
            goto exit;
        }
        // Data options
        else
        {
            *data_ptr = args[op_idx];
            goto exit;
        }
    }
    else
    {
        return -1;
    }

exit:
    op_idx++;
    return op_ret;
}

/*!@brief Get a line for CLI.
 *        This function will check input from cli_port_getc() function.
 *        Put them to buffer until get a new line "\n".
 *
 * @return Pointer to the line or NULL for no line is get.
 */
char *cli_getline(void)
{
    if (StringPtr == NULL)
    {
        return 0;
    }

    char c = 0;

    do
    {
        // Get 1 char and check
        c = cli_port_getc();

        // Handle characters
        switch (c)
        {
        case '\x0':  // NULL
        case '\xff': // EOF
        {
            break;
        }
        case '\x7f': // Delete for MacOs keyboard
        case '\b':   // Backspace PC keyboard
        {
            if (StringIdx > 0)
            {
                // Delete 1 byte from buffer.
                delete_char(StringPtr, StringIdx);
                StringIdx--;
                // Print New line
                print_newline(StringPtr, StringIdx);
            }
            break;
        }
        case '\r': // CR
        case '\n': // LF
        {
            // Push to history without \'n'
            if (StringIdx > 0)
            {
                history_push(StringPtr);
            }

            // Echo back
            strcat(StringPtr, "\n");
            printf("\n");

            // Return pointer and length
            StringIdx        = 0;
            HistoryPullDepth = 0;
            return StringPtr;
        }
        default:
        {
            // Handle special keys first
            if (handle_special_key(c) == 0)
            {
                return 0;
            }
            else
            {
                if (strlen(StringPtr) < CLI_STR_BUF_SIZE - 2)
                {
                    // Insert 1 byte to buffer
                    insert_char(StringPtr, c, StringIdx);
                    StringIdx++;

                    // Loop back a char or line
                    (StringPtr[StringIdx] == 0) ? printf("%c", c)
                                                : print_newline(StringPtr, StringIdx);
                }
            }
            break;
        }
        }
    } while ((c != 0xFF) && (c != EOF));

    return NULL;
}

/*!@brief   String to Arguments
 *
 * @param str   Input string
 * @param argc  Output argument count
 * @param argv  Output argument vector
 * @return      Pointer to the tail of the string is not processed, or NULL for
 * all string is processed.
 */
char *cli_strtoarg(char *str, int *argc, char **argv)
{
    if ((str == NULL) || (argc == NULL) || (argv == NULL))
    {
        return NULL;
    }

    *argc              = 0;
    char flag_quote    = 0; // Flags for inside 2x quote mark ""
    char flag_arg_head = 0; // Flags to mark argument head

    for (int i = 0; str[i] != 0; i++)
    {
        switch (str[i])
        {
        case '#':
        {
            // Ignore comment lines
            return NULL;
        }
        case '"':
        {
            // Set ignore flag up. string inside "" will not be processed.
            flag_quote = !flag_quote;
            str[i]     = 0;
            break;
        }
        case ';':
        {
            // Command separator, return tail commands for next process.
            if (flag_quote == 0)
            {
                str[i]     = 0;
                char *tail = str + i + 1;

                return (*tail == 0) ? NULL : tail;
            }
            break;
        }
        case '\t':
        case ' ':
        case '\r':
        case '\n':
        {
            // Separators
            if (flag_quote == 0)
            {
                str[i]        = 0;
                flag_arg_head = 0;
            }
            break;
        }
        default:
        {
            // Normal characters, save argument pointer.
            if ((flag_arg_head == 0) && (*argc < CLI_ARGC_MAX))
            {
                argv[*argc] = &str[i];
                (*argc)++;
            }
            flag_arg_head = 1;
            break;
        }
        }
    }

    return NULL;
}

/*!@brief   Register a command to CLI.
 * @example Cli_Register("help","show help text",&builtin_help);
 *
 * @param   name      Command name
 * @param   prompt    Command prompt text
 * @param   func      Pointer to function to run when the command is called.
 *
 * @retval  index    The index of the command is inserted in the command list.
 * @retval  -1       Command register fail.
 */
int Cli_Register(const char *name, const char *prompt, int (*func)(int, char **))
{
    if ((name == NULL) || (prompt == NULL) || func == NULL)
    {
        return CLI_FAIL;
    }

    for (int i = 0; i < CLI_COMMAND_SIZE; i++)
    {
        // Find a empty slot to save the command.
        if ((CliCommandList[i].Name == NULL) && (CliCommandList[i].Func == NULL))
        {
            CliCommandList[i].Name   = name;
            CliCommandList[i].Prompt = prompt;
            CliCommandList[i].Func   = func;

            CliNumOfCommands++;
            return i;
        }
    }

    return CLI_FAIL;
}

int Cli_Unregister(const char *name)
{
    if ((name == NULL) || (name[0] == 0))
    {
        return CLI_FAIL;
    }

    for (int i = 0; i < CLI_COMMAND_SIZE; i++)
    {
        // Delete the command
        if (strcmp(CliCommandList[i].Name, name) == 0)
        {
            CliCommandList[i].Name   = NULL;
            CliCommandList[i].Prompt = NULL;
            CliCommandList[i].Func   = NULL;

            CliNumOfCommands--;
            return i;
        }
    }
    return CLI_FAIL;
}

/*!@brief   Run the CLI by given arguments.
 *
 * @param   argc
 * @param   args
 * @return  -1      Run command fail.
 */
int Cli_RunByArgs(int argc, char **args)
{
    if ((argc == 0) || (args == NULL))
    {
        return CLI_FAIL;
    }

    for (int i = 0; i < CLI_COMMAND_SIZE; i++)
    {
        if ((CliCommandList[i].Name != NULL) && (CliCommandList[i].Func != NULL))
        {
            if (strcmp(CliCommandList[i].Name, args[0]) == 0)
            {
                int ret = CliCommandList[i].Func(argc, args);
                printf("%s\n", ret ? "FAIL" : "OK");
                return ret;
            }
        }
    }

    printf("%sERROR: Unknown command of [%s], try [help].%s\n", ANSI_RED, args[0], ANSI_RESET);
    return CLI_FAIL;
}

/*!@brief   Run the CLI by given string.
 *          This function will conver the command string to arguments and run
 * Cli_RunByArgs.
 *
 * @param   cmd     Command string, e.g. "test -i 123"
 * @return
 */
int Cli_RunByString(char *cmd)
{
    if ((cmd == NULL) || (*cmd == 0))
    {
        return CLI_FAIL;
    }

    // Buffer string
    char *sub_cmd = cli_malloc(strlen(cmd) + 1);
    strcpy(sub_cmd, cmd);

    // Loop until all string is processed.
    do
    {
        // String to arguments
        int    argc = 0;
        char **argv = cli_malloc(sizeof(char *) * CLI_ARGC_MAX);
        sub_cmd     = cli_strtoarg(sub_cmd, &argc, argv);

        // Run by arguments
        Cli_RunByArgs(argc, argv);
        cli_free(argv);

    } while (sub_cmd != NULL);

    cli_free(sub_cmd);
    return CLI_OK;
}

int Cli_Init(void)
{
    // Clear operation buffers
    StringIdx      = 0;
    StringPtr      = cli_malloc(sizeof(char) * CLI_STR_BUF_SIZE);
    CliCommandList = cli_malloc(sizeof(CliCommand_TypeDef) * CLI_COMMAND_SIZE);

#if HISTORY_ENABLE
    HistoryPtr = cli_malloc(sizeof(char *) * HISTORY_DEPTH);
    history_clear();
#endif

    // Register built-in commands.
    Cli_Register("help", "Show list of commands & prompt.", &builtin_help);
    Cli_Register("history", "Show command history", &builtin_history);
    Cli_Register("test", "CLI argument parse example", &builtin_test);
    Cli_Register("repeat", "Repeat excution of a command", &builtin_repeat);
    Cli_Register("sleep", "Suspend execution for an interval of time", &builtin_sleep);
    Cli_Register("time", "Time command execution", &builtin_time);
    Cli_Register("version", "Show CLI version", &builtin_version);
    CliNumOfBuiltin = 7;
    // Initialize IO port
    cli_port_init();

    builtin_version(0, NULL);
    printf("%s", CLI_PROMPT_CHAR);
    return CLI_OK;
}

int Cli_Deinit(void)
{
    cli_port_deinit();

    history_clear();

    StringIdx = 0;
    cli_free(StringPtr);
    cli_free(HistoryPtr);
    cli_free(CliCommandList);

    return CLI_OK;
}

int Cli_Run(void)
{
    char *str = cli_getline();

    if (str != NULL)
    {
        int len = strlen(str);
        if (len >= 1)
        {
            Cli_RunByString(str);
        }
        memset(str, 0, len + 1);
        printf("%s", CLI_PROMPT_CHAR);
        fflush(stdout);
    }

    return CLI_OK;
}

void Cli_Task(void const *arguments)
{
    Cli_Init();
    /* Infinite loop */
    for (;;)
    {
        Cli_Run();
        cli_sleep(10);
    }
}