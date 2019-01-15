/******************************************************************************
 * @file    cli.h
 * @brief   Command Line Interface (CLI) for MCU.
 *
 * @author  Nick Yang
 * @date    2018/11/01
 * @version V1.0
 *****************************************************************************/

#ifndef CLI_H_
#define CLI_H_

/*! Includes ----------------------------------------------------------------*/
#include "cli_port.h"
#include "stdio.h"

/*! Defines -----------------------------------------------------------------*/

/*!@defgroup ANSI flow control Escape sequence define.
 *  Terminal flow control with ANSI escape code.
 *  Refer to <https://en.wikipedia.org/wiki/ANSI_escape_code>
 */
// clang-format off
#define ANSI_CUU        "\e[A"  //!< Cursor Up
#define ANSI_CUD        "\e[B"  //!< Cursor Down
#define ANSI_CUF        "\e[C"  //!< Cursor Forward
#define ANSI_CUB        "\e[D"  //!< Cursor Back
#define ANSI_CNL        "\e[E"  //!< Cursor Next Line
#define ANSI_CPL        "\e[F"  //!< Cursor Previous Line
#define ANSI_SCP        "\e[s"  //!< Save Cursor Position
#define ANSI_RCP        "\e[u"  //!< Restore Cursor Position

#define ANSI_ED         "\e[J"  //!< Erase Display
#define ANSI_EL0        "\e[K"  //!< Erase from curse to line end
#define ANSI_EL1        "\e[1K" //!< Erase from line start to curse
#define ANSI_EL2        "\e[2K" //!< Erase all line

#define ANSI_RESET      "\e[0m"
#define ANSI_BOLD       "\e[1m"
#define ANSI_ITALIC     "\e[3m"
#define ANSI_UNDERLINE  "\e[4m"
#define ANSI_BLINK      "\e[5m"

#define ANSI_BLACK      "\e[30m"
#define ANSI_RED        "\e[31m"
#define ANSI_GREEN      "\e[32m"
#define ANSI_YELLOW     "\e[33m"
#define ANSI_BLUE       "\e[34m"
#define ANSI_MAGENTE    "\e[35m"
#define ANSI_CYAN       "\e[36m"

/*!@defgroup CLI return code defines
 *
 */
#define CLI_OK                  0       //!< General success.
#define CLI_FAIL                -1      //!< General fail.
#define CLI_PROMPT_CHAR         ">"     //!< Prompt string shows at the head of line
#define CLI_PROMPT_LEN          1       //!< Prompt string length
#define CLI_COMMAND_LEN         256     //!< Maximum command length
#define CLI_COMMAND_TOKEN_MAX   32      //!< Maximum arguments in a command
#define CLI_NUM_OF_BUILTIN_CMD  10      //!< Number of built-in commands
#define CLI_NUM_OF_EXTERNAL_CMD 64      //!< Number of external commands
#define CLI_NUM_OF_ALIAS        16      //!< Number of alias
#define CLI_VERSION             "1.0.0" //!< CLI version string

/*!@defgroup CLI history function defines
 *
 */
#define HISTORY_ENABLE          1       //!< Enable history function
#define HISTORY_DEPTH           32      //!< Maximum number of command saved in history
#define HISTORY_MEM_SIZE        256     //!< Maximum RAM usage for history

// clang-format on

// General Print
#define CLI_PRINT(msg, args...)                                                                    \
    if (gCliDebugLevel >= 0)                                                                       \
    {                                                                                              \
        fprintf(stdout, msg, ##args);                                                              \
    }

// Error Message output, with RED color.
#define CLI_ERROR(msg, args...)                                                                    \
    if (gCliDebugLevel >= 1)                                                                       \
    {                                                                                              \
        fprintf(stderr, ANSI_RED "%s <%s:%d> " msg ANSI_RESET, CLI_TimeStampStr(), __FILE__,       \
                __LINE__, ##args);                                                                 \
    }

// Warning Message output, with Yellow color.
#define CLI_WARNING(msg, args...)                                                                  \
    if (gCliDebugLevel >= 2)                                                                       \
    {                                                                                              \
        fprintf(stdout, ANSI_YELLOW "%s <%s:%d> " msg ANSI_RESET, CLI_TimeStampStr(), __FILE__,    \
                __LINE__, ##args);                                                                 \
    }

// Info Message output, with Magente color.
#define CLI_INFO(msg, args...)                                                                     \
    if (gCliDebugLevel >= 3)                                                                       \
    {                                                                                              \
        fprintf(stdout, ANSI_MAGENTE "%s " msg ANSI_RESET, CLI_TimeStampStr(), ##args);            \
    }

/*!@typedef CliCommand_TypeDef
 *          Structure for a CLI command.
 */
typedef struct CliCommand_TypeDef {
    const char *Name;                   //!< Command Name
    const char *Prompt;                 //!< Prompt text
    int (*Func)(int argc, char **argv); //!< Function call
} CliCommand_TypeDef;

/*!@typedef CliOption_TypeDef
 *          Structure for a CLI command options. It's a implement of the
 *          "getopt" & "getopt_long" function.
 * @example see "builtin_test" function
 */
typedef struct CliOption_TypeDef {
    const char  ShortName; //!< Short name work with "-", e.g. 'h'
    const char *LongName;  //!< Long name work with "--", e.g. "help"
    const int   ReturnVal; //!< Return value . Use short name would be the simplest way.
} CliOption_TypeDef;

/*! Variables ---------------------------------------------------------------*/

/*!@def gCliDebugLevel
 *      -1  : Turn off all print
 *      0   : PRINT only, no debug info.
 *      1   : PRINT + ERROR
 *      2   : PRINT + ERROR + WARNING
 *      3   : PRINT + ERROR + WARNING + INFO
 */
extern int gCliDebugLevel;

/*! Functions ---------------------------------------------------------------*/
char *CLI_TimeStampStr(void);
int   CLI_Register(const char *name, const char *prompt, int (*func)(int, char **));
int   CLI_Unregister(const char *name);
int   CLI_ExecuteByArgs(int argcount, char **argbuf);
int   CLI_ExecuteByString(char *cmd);
int   CLI_Init(void);
int   CLI_Run(void);
void  CLI_Task(void const *arguments);

#endif /* CLI_H_ */
