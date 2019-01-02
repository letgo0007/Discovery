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
#include "cli_builtin.h"
#include "cli_port.h"

#include "stdarg.h"
#include "stdlib.h"
#include "string.h"

/** Private defines ---------------------------------------------------------*/

/** Private function prototypes ---------------------------------------------*/
extern int cli_getopt(int argc, char **args, char **data_ptr, CliOption_TypeDef options[]);

/** Variables ---------------------------------------------------------------*/
int gCliDebugLevel = 3;             // Global debug level
char * pCmdStrBuf = NULL;           // Command String buffer pointer
unsigned int CmdStrIdx = 0; // Command String operation index. This give support for delete & insert function.
char ** pHistoryPtr = NULL;         // History pointer buffer pointer
unsigned int HistoryQueueHead = 0;  // History queue head
unsigned int HistoryQueueTail = 0;  // History queue tail
unsigned int HistoryPullDepth = 0;  // History pull depth
unsigned int HistoryMemUsage = 0;   // History total memory usage

CliCommand_TypeDef *pCmdList_Builtin = NULL;
CliCommand_TypeDef *pCmdList_External = NULL;
CliCommand_TypeDef *pCmdList_Alias = NULL;

/** Functions ---------------------------------------------------------------*/
/*!@brief Insert a char to a position of a string.
 *
 * @param string    Pointer to the old string
 * @param c         Char to insert
 * @param pos       Position to insert
 * @return          Pointer to the new string or NULL when it fails.
 */
char *StrInsert(char *string, char c, int pos)
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
char *StrDelete(char *string, int pos)
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
void StrDump(char *string, int pos)
{
    // Erase terminal line, print new buffer string and Move cursor
    CLI_PRINT("%s\r%s%s", ANSI_ERASE_LINE, CLI_PROMPT_CHAR, string);
    CLI_PRINT("\e[%luG", (uint32_t)pos + strlen(CLI_PROMPT_CHAR) + 1);
}

/*!@brief Clear history buffer & heap.
 *
 */
void history_clear(void)
{
    if (pHistoryPtr != NULL)
    {
        // Try free string heap if used.
        for (int i = 0; i < HISTORY_DEPTH; i++)
        {
            if (pHistoryPtr[i] != NULL)
            {
                cli_free(pHistoryPtr[i]);
                pHistoryPtr[i] = NULL;
            }
        }

        // Reset index
        HistoryQueueHead = 0;
        HistoryQueueTail = 0;
        HistoryPullDepth = 0;
        HistoryMemUsage = 0;
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
    if ((string == NULL) || (pHistoryPtr == NULL))
    {
        return NULL;
    }

    // Request memory & copy command
    unsigned int len = strlen(string) + 1;
    char * ptr = cli_calloc(len);
    memcpy(ptr, string, len);

    // Save new history queue pointer & queue head.
    pHistoryPtr[HistoryQueueHead % HISTORY_DEPTH] = ptr;
    HistoryQueueHead++;
    HistoryMemUsage += len;

    // Release History buffer if number or memory usage out of limit
    while ((history_getdepth() >= HISTORY_DEPTH) || (history_getmem() >= HISTORY_MEM_SIZE))
    {
        // Release from Queue Tail
        int idx = HistoryQueueTail % HISTORY_DEPTH;

        if (pHistoryPtr[idx] != NULL)
        {
            HistoryMemUsage -= strlen(pHistoryPtr[idx]) + 1;
            cli_free(pHistoryPtr[idx]);
            pHistoryPtr[idx] = NULL;
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
    if (pHistoryPtr == NULL)
    {
        return NULL;
    }

    // Calculate where to pull the history.
    unsigned int pull_idx = (HistoryQueueHead - depth) % HISTORY_DEPTH;

    if (pHistoryPtr[pull_idx] != NULL)
    {
        // Pull out history to string buffer
        memset(pCmdStrBuf, 0, CLI_COMMAND_LEN);
        strcpy(pCmdStrBuf, pHistoryPtr[pull_idx]);
        CmdStrIdx = strlen(pCmdStrBuf);

        // Print new line on console
        StrDump(pCmdStrBuf, CmdStrIdx);
    }
    else
    {
        // Put string buffer to empty if no history
        memset(pCmdStrBuf, 0, CLI_COMMAND_LEN);
        CmdStrIdx = 0;
        // Print new line on console
        StrDump(pCmdStrBuf, CmdStrIdx);
    }

    return pHistoryPtr[pull_idx];
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
    static char EscBuf[8] = { 0 };
    static int EscIdx = 0;
    static char EscFlag = 0;

    // Start of ESC flow control
    if (c == '\e')
    {
        EscFlag = 1;
        EscIdx = 0;
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
            if (pCmdStrBuf[CmdStrIdx] != 0)
            {
                CmdStrIdx++;
                CLI_PRINT("%s", ANSI_CURSOR_RIGHT);
            }
        }
        else if (strcmp(EscBuf, ANSI_CURSOR_LEFT) == 0) //!< Left arrow
        {
            if (CmdStrIdx > 0)
            {
                CmdStrIdx--;
                CLI_PRINT("%s", ANSI_CURSOR_LEFT);
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
 *                  CLI_PRINT("Unknown option!");
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
    static int op_argc = 0;
    static char **op_args = NULL;
    static int op_idx = 0;
    static int op_ret = '?';

    if ((op_argc != argc) || (op_args != args))
    {
        op_argc = argc;
        op_args = args;
        op_idx = 1; // ignore the 1st argument, it's the command name.
        op_ret = '?';
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
            op_ret = '?';
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
            op_ret = '?';
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

    exit: op_idx++;
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
    if (pCmdStrBuf == NULL)
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
            if (CmdStrIdx > 0)
            {
                // Delete 1 byte from buffer.
                StrDelete(pCmdStrBuf, CmdStrIdx);
                CmdStrIdx--;
                // Print New line
                StrDump(pCmdStrBuf, CmdStrIdx);
            }
            break;
        }
        case '\r': // CR
        case '\n': // LF
        {
            // Push to history without \'n'
            if (CmdStrIdx > 0)
            {
                history_push(pCmdStrBuf);
            }

            // Echo back
            strcat(pCmdStrBuf, "\n");
            CLI_PRINT("\n");

            // Return pointer and length
            CmdStrIdx = 0;
            HistoryPullDepth = 0;
            return pCmdStrBuf;
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
                if (strlen(pCmdStrBuf) < CLI_COMMAND_LEN - 2)
                {
                    // Insert 1 byte to buffer
                    StrInsert(pCmdStrBuf, c, CmdStrIdx);
                    CmdStrIdx++;

                    // Loop back a char or line
                    if (pCmdStrBuf[CmdStrIdx] == 0)
                    {
                        CLI_PRINT("%c", c)
                    }
                    else
                    {
                        StrDump(pCmdStrBuf, CmdStrIdx);
                    }
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

    *argc = 0;
    char flag_quote = 0; // Flags for inside 2x quote mark ""
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
            str[i] = 0;
            break;
        }
        case ';':
        {
            // Command separator, return tail commands for next process.
            if (flag_quote == 0)
            {
                str[i] = 0;
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
                str[i] = 0;
                flag_arg_head = 0;
            }
            break;
        }
        default:
        {
            // Normal characters, save argument pointer.
            if ((flag_arg_head == 0) && (*argc < CLI_COMMAND_TOKEN_MAX))
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

int cli_excute(int argc, char **args, CliCommand_TypeDef *pCmdList)
{
    if ((argc == 0) || (args == NULL))
    {
        return CLI_FAIL;
    }

    for (int i = 0; pCmdList[i].Name != NULL; i++)
    {
        if ((pCmdList[i].Name != NULL) && (pCmdList[i].Func != NULL))
        {
            if (strcmp(pCmdList[i].Name, args[0]) == 0)
            {
                int ret = pCmdList[i].Func(argc, args);
                CLI_PRINT("%s\n", ret ? "FAIL" : "OK");
                return CLI_OK;
            }
        }
    }

    return CLI_FAIL;
}

char *CLI_TimeStampStr(void)
{
    static char timestamp[16] = { 0 };

    uint32_t tick = cli_gettick();

    sprintf(timestamp, "[%03ld.%03ld]", tick / 1000, tick % 1000);

    return timestamp;
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
int CLI_Register(const char *name, const char *prompt, int (*func)(int, char **))
{
    if ((name == NULL) || (prompt == NULL) || func == NULL)
    {
        return CLI_FAIL;
    }

    for (int i = 0; i < CLI_NUM_OF_EXTERNAL_CMD; i++)
    {
        // Find a empty slot to save the command.
        if ((pCmdList_External[i].Name == NULL) && (pCmdList_External[i].Func == NULL))
        {
            pCmdList_External[i].Name = name;
            pCmdList_External[i].Prompt = prompt;
            pCmdList_External[i].Func = func;

            return i;
        }
    }

    CLI_WARNING("Warning: Command [%s] register Fail, list is full!\n", name);
    return CLI_FAIL;
}

int CLI_Unregister(const char *name)
{
    if ((name == NULL) || (name[0] == 0))
    {
        return CLI_FAIL;
    }

    for (int i = 0; i < CLI_NUM_OF_EXTERNAL_CMD; i++)
    {
        // Delete the command
        if (strcmp(pCmdList_External[i].Name, name) == 0)
        {
            pCmdList_External[i].Name = NULL;
            pCmdList_External[i].Prompt = NULL;
            pCmdList_External[i].Func = NULL;

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
int CLI_ExecuteByArgs(int argc, char **args)
{
    if ((argc == 0) || (args == NULL))
    {
        return CLI_FAIL;
    }

    if (pCmdList_Builtin != NULL)
    {
        if (CLI_OK == cli_excute(argc, args, pCmdList_Builtin))
        {
            return CLI_OK;
        }
    }
    if (pCmdList_External != NULL)
    {
        if (CLI_OK == cli_excute(argc, args, pCmdList_External))
        {
            return CLI_OK;
        }
    }

    CLI_ERROR("ERROR: Unknown command of [%s], try [help].\n", args[0]);
    return CLI_FAIL;

}

/*!@brief   Run the CLI by given string.
 *          This function will conver the command string to arguments and run
 * Cli_RunByArgs.
 *
 * @param   cmd     Command string, e.g. "test -i 123"
 * @return
 */
int CLI_ExecuteByString(char *cmd)
{
    if ((cmd == NULL) || (*cmd == 0))
    {
        return CLI_FAIL;
    }

    // Buffer string before run command
    char *cmd_buf = cli_calloc(strlen(cmd) + 1);
    strcpy(cmd_buf, cmd);

    // Loop until all string is processed.
    char *sub_cmd = cmd_buf;
    do
    {
        // String to arguments
        int argc = 0;
        char **argv = cli_calloc(sizeof(char *) * CLI_COMMAND_TOKEN_MAX);
        sub_cmd = cli_strtoarg(sub_cmd, &argc, argv);

        // Run by arguments
        CLI_ExecuteByArgs(argc, argv);
        cli_free(argv);

    } while (sub_cmd != NULL);

    cli_free(cmd_buf);
    return CLI_OK;
}

int CLI_Init(void)
{
    // Initialize operation buffers
    CmdStrIdx = 0;
    pCmdStrBuf = cli_calloc(sizeof(char) * CLI_COMMAND_LEN);
    pCmdList_Builtin = (CliCommand_TypeDef*) gConstBuiltinCmdList;
    pCmdList_External = cli_calloc(sizeof(CliCommand_TypeDef) * 32);
    pCmdList_Alias = cli_calloc(sizeof(CliCommand_TypeDef) * 32);

#if HISTORY_ENABLE
    pHistoryPtr = cli_calloc(sizeof(char *) * HISTORY_DEPTH);
    history_clear();
#endif

    // Initialize IO port
    cli_port_init();

    // Show Version
    builtin_version(0, NULL);
    return CLI_OK;
}

int CLI_Deinit(void)
{
    cli_port_deinit();

    history_clear();

    CmdStrIdx = 0;
    cli_free(pCmdStrBuf);
    cli_free(pHistoryPtr);
    cli_free(pCmdList_External);
    cli_free(pCmdList_Alias);

    return CLI_OK;
}

int CLI_Run(void)
{
    char *str = cli_getline();

    if (str != NULL)
    {
        int len = strlen(str);
        if (len >= 1)
        {
            CLI_ExecuteByString(str);
        }
        memset(str, 0, len + 1);
        CLI_PRINT("%s", CLI_PROMPT_CHAR);
        fflush(stdout);
    }

    return CLI_OK;
}

void CLI_Task(void const *arguments)
{
    /* Initialize */
    cli_sleep(10); // Wait 10ms for Hardware to settle
    CLI_Init();
    CLI_INFO("%s: Initialize Finish\n", __FUNCTION__);
    cli_sleep(1000); // Wait 1s to start CLI
    CLI_PRINT(CLI_PROMPT_CHAR);

    /* Infinite loop */
    for (;;)
    {
        CLI_Run();
        cli_sleep(10);
    }
}
