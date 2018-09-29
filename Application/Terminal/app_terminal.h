/******************************************************************************
 * @file    app_terminal.h
 *          An application of Terminal command line interface.
 *          Handle the IO stream, provide support for keyboard operation like
 *          - Up/down Key for History function
 *          - Left/Right/Delete for character insert and delete.
 *
 * @author  Nick Yang
 * @date    2018/09/03
 * @version V0.2
 *****************************************************************************/
#ifndef TERMINAL_H_
#define TERMINAL_H_
/* Includes ------------------------------------------------------------------*/
#include "cli.h"

/* Defines -------------------------------------------------------------------*/

/*!@defgroup TERM_ANSI_DEF ANSI escape code define.
 * Terminal flow control with ANSI escape code.
 * Refer to <https://en.wikipedia.org/wiki/ANSI_escape_code>
 */

#define TERM_CURSOR_UP              "\e[A"
#define TERM_CURSOR_DOWN            "\e[B"
#define TERM_CURSOR_RIGHT           "\e[C"
#define TERM_CURSOR_LEFT            "\e[D"
#define TERM_CURSOR_NEXT_LINE       "\e[E"
#define TERM_CURSOR_PREVIOUS_LINE   "\e[F"
#define TERM_CURSOR_SAVE_POSITION   "\e[s"
#define TERM_CURSOR_RESTORE_POSITION "\e[u"

#define TERM_ERASE_DISPLAY          "\e[J"
#define TERM_ERASE_LINE_START       "\e[1K"
#define TERM_ERASE_LINE_END         "\e[K"
#define TERM_ERASE_LINE             "\e[2K"

#define TERM_RESET                  "\e[0m"
#define TERM_BOLD                   "\e[1m"
#define TERM_ITALIC                 "\e[3m"
#define TERM_UNDERLINE              "\e[4m"
#define TERM_BLINK                  "\e[5m"

#define TERM_BLACK                  "\e[30m"
#define TERM_RED                    "\e[31m"
#define TERM_GREEN                  "\e[32m"
#define TERM_YELLOW                 "\e[33m"
#define TERM_BLUE                   "\e[34m"

#define TERM_STDOUT_BUF_SIZE        256    //!< Maximum STDOUT buffer queue length
#define TERM_STDERR_BUF_SIZE        0
#define TERM_STDIN_BUF_SIZE         0
#define TERM_STRING_BUF_SIZE        256     //!< Maximum terminal command length.
#define TERM_TOKEN_AMOUNT           64      //!< Maximum tokens in a command.
#define TERM_HISTORY_DEPTH          64      //!< History depth.
#define TERM_LOOP_BACK_EN           1       //!< Enable loop back function for terminal.

#define TERM_VERSION                "V1.0"  //!< Terminal Application Version
#define TERM_PROMPT_CHAR            ">"     //!< Terminal prompt string
#define TERM_PROMPT_LEN             1       //!< Terminal prompt string length
#define TERM_COMMAND_MAX            64      //!< Maximum Command amount supported.

typedef struct
{
    char StrBuf[TERM_STRING_BUF_SIZE];      //!< Command string buffer
    int StrIndex;                           //!< Command string operastion index
    int EscFlag;                            //!< Flags to handle ANSI ESC sequence (up/down/left/right)
    char EnableLoopBack;                    //!< Enable Loopback function for terminal
#if TERM_HISTORY_DEPTH
    char *HistoryBuf[TERM_HISTORY_DEPTH]; //!< Buffer to store history command
    int HistoryPushIndex;                   //!< History push index
    int HistoryPullIndex;                   //!< History pull index
#endif
} Term_HandleTypeDef;

/* Variables ------------------------------------------------------------------*/
const Cli_CommandTypeDef gTermCommand[TERM_COMMAND_MAX];
Term_HandleTypeDef gTermHandle;

/* Functions ------------------------------------------------------------------*/
int App_Terminal_init(void);
int App_Terminal_run(void);

#endif /* TERMINAL_H_ */
