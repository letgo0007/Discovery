/******************************************************************************
 * @file    app_terminal.c
 *          An application of Terminal command line interface.
 *          Handle the IO stream, provide support for keyboard operation like
 *          - Up/down Key for History function
 *          - Left/Right/Delete for character insert and delete.
 *
 * @author  Nick Yang
 * @date    2018/09/03
 * @version V0.2
 *****************************************************************************/

/* Includes ------------------------------------------------------------------*/

#include <app_terminal.h>
#include <stdio.h>
#include <string.h>

/* Private function prototypes -----------------------------------------------*/
extern int cli_test(int argc, char *args[]);
extern int cli_ver(int argc, char *args[]);
extern int cli_echo(int argc, char *args[]);
extern int cli_history(int argc, char *args[]);
extern int cli_repeat(int argc, char *argv[]);
extern int cli_mem32(int argc, char *argv[]);
extern int cli_mem8(int argc, char *argv[]);

extern int cli_reset(int argc, char *argv[]);
extern int cli_time(int argc, char *argv[]);
extern int cli_wait(int argc, char *argv[]);
extern int cli_info(int argc, char *argv[]);

extern int cli_rtc(int argc, char *argv[]);
extern int cli_adc(int argc, char *argv[]);
//extern int cli_senser(int argc, char *argv[]);

//extern int cli_i2c(int argc, char *argv[]);
//extern int cli_gpio(int argc, char *argv[]);
extern int cli_nvram(int argc, char *argv[]);
extern int cli_otp(int argc, char *argv[]);
extern int cli_flash(int argc, char *argv[]);

extern int cli_accel(int argc, char *argv[]);
extern int cli_qspi(int argc, char *argv[]);

/* Private variables ---------------------------------------------------------*/
/*!@var Terminal Command List.
 *
 */
const Cli_CommandTypeDef gTermCommand[TERM_COMMAND_MAX] =
{
{ "", NULL, "\n💪 Terminal Commands\n------------------------------" },
{ "echo", cli_echo, "Echo back command" },
{ "history", cli_history, "Show command history" },
{ "test", cli_test, "Run a argument parse example." },
{ "repeat", cli_repeat, "Repeat running a command." },
{ "version", cli_ver, "Show Command version" },
{ "mem32", cli_mem32, "STM32 memory write/read." },
{ "mem8", cli_mem8, "STM32 memory write/read in byte mode." },
{ "", NULL, "\n💪 STM32 MCU basic commands\n------------------------------" },
{ "info", cli_info, "Show MCU information." },
{ "reset", cli_reset, "STM32 software reset." },
{ "", NULL, "\n💪 Device driver\n------------------------------" },
{ "accel", cli_accel, "Accelerometer commands" },
{ "qspi", cli_qspi, "Quad-SPI Flash commands." },
{ "", NULL, "\n------------------------------\r\n" },
{ NULL, NULL, NULL } };

/*!@var Global Terminal handle
 *
 */
Term_HandleTypeDef gTermHandle =
{ 0 };

int term_get_array_index(int old_idx, int move, int size)
{
    int new_idx = old_idx + move;
    if (new_idx < 0)
    {
        new_idx = size - 1;
    }
    else if (new_idx >= size)
    {
        new_idx = 0;
    }
    return new_idx;
}

/*!@brief Push a string to history buffer.
 *
 * @param TermHandle
 * @param depth
 * @return
 */
int term_history_push(Term_HandleTypeDef *TermHandle, int depth)
{
#if TERM_HISTORY_DEPTH
    //Request memory
    uint32_t len = strlen(TermHandle->StrBuf) + 1;
    uint8_t *ptr = TermHandle->HistoryBuf[TermHandle->HistoryPushIndex];

    //ptr = pvPortMalloc(len);
    ptr = malloc(len);

    TermHandle->HistoryBuf[TermHandle->HistoryPushIndex] = ptr;

    //Copy Command
    ptr[len - 1] = 0;
    memcpy(TermHandle->HistoryBuf[TermHandle->HistoryPushIndex], TermHandle->StrBuf, len);

    //Set new push/pull index, Calculate next history index.
    TermHandle->HistoryPullIndex = TermHandle->HistoryPushIndex;
    TermHandle->HistoryPushIndex = term_get_array_index(TermHandle->HistoryPushIndex, depth, TERM_HISTORY_DEPTH);

    //vPortFree(TermHandle->HistoryBuf[TermHandle->HistoryPushIndex]);
    free(TermHandle->HistoryBuf[TermHandle->HistoryPushIndex]);
    TermHandle->HistoryBuf[TermHandle->HistoryPushIndex] = NULL;
#endif
    return 0;
}

/*!@brief Pull a history string from history buffer.
 *
 * @param TermHandle
 * @param depth
 * @return
 */
int term_history_pull(Term_HandleTypeDef *TermHandle, int depth)
{
#if TERM_HISTORY_DEPTH
    //Calculate next pull index
    int new_pull_idx = term_get_array_index(TermHandle->HistoryPullIndex, depth, TERM_HISTORY_DEPTH);

    //**Up Arrow**, Print & Move
    if (depth < 0)
    {
        //Copy history info and print the new line
        if (TermHandle->HistoryBuf[TermHandle->HistoryPullIndex] != NULL)
        {
            //Copy string from history
            strcpy(TermHandle->StrBuf, TermHandle->HistoryBuf[TermHandle->HistoryPullIndex]);
            TermHandle->StrIndex = strlen(TermHandle->StrBuf);

            //Print the new string.
            printf("%s\r%s%s", TERM_ERASE_LINE, TERM_PROMPT_CHAR, TermHandle->StrBuf);
        }

        //Move to next pull index
        if (TermHandle->HistoryBuf[new_pull_idx] != NULL)
        {
            TermHandle->HistoryPullIndex = new_pull_idx;
        }

    }
    //**Down Arrow**, Move and print
    else if (depth > 0)
    {
        //Reach the end of history, clear out string.
        if (TermHandle->HistoryBuf[new_pull_idx] == NULL)
        {
            memset(TermHandle->StrBuf, 0, TERM_STRING_BUF_SIZE);
            TermHandle->StrIndex = 0;
            printf("%s\r%s", TERM_ERASE_LINE, TERM_PROMPT_CHAR);
            return 0;
        }
        //Move pull index
        TermHandle->HistoryPullIndex = new_pull_idx;

        //Copy history info and print the new line
        if (TermHandle->HistoryBuf[TermHandle->HistoryPullIndex] != NULL)
        {
            //Copy string from history
            strcpy(TermHandle->StrBuf, TermHandle->HistoryBuf[TermHandle->HistoryPullIndex]);
            TermHandle->StrIndex = strlen(TermHandle->StrBuf);

            //Print the new string.
            printf("%s\r%s%s", TERM_ERASE_LINE, TERM_PROMPT_CHAR, TermHandle->StrBuf);
        }
    }
#endif
    return 0;
}

/*!@brief Handle ANSI escape code characters in Terminal.
 *        This provide support for Up/Down/Left/Right Arrows input from Keyboard.
 *
 * @see   https://en.wikipedia.org/wiki/ANSI_escape_code
 * @param c
 *          Character input, starting from '\e'.
 * @param TermHandle
 *          Terminal handler structure.
 * @return
 */
int term_esc_handle(char c, Term_HandleTypeDef *TermHandle)
{
    static char escbuf[8] =
    { 0 };
    static int escidx = 0;

    if (c == '\e')  //Start of ESC folow control, default length = 2
    {
        TermHandle->EscFlag = 2;
    }
    if (c == '[') //Start of a Cursor control, max length = 4, till get a letter.
    {
        TermHandle->EscFlag += 4;
    }

    escbuf[escidx++] = c;

    if (strcmp(escbuf, "\e[A") == 0)    //Up Arrow
    {
        //Pull previous history
        term_history_pull(TermHandle, -1);
    }
    else if (strcmp(escbuf, "\e[B") == 0) //Down Arrow
    {
        //Pull next history
        term_history_pull(TermHandle, 1);
    }
    else if (strcmp(escbuf, "\e[C") == 0)   //Right arrow
    {

        if (TermHandle->StrBuf[TermHandle->StrIndex] != 0)
        {
            (TermHandle->StrIndex)++;
            printf("%s", TERM_CURSOR_RIGHT);
        }
    }
    else if (strcmp(escbuf, "\e[D") == 0)   //Left arrow
    {
        if (TermHandle->StrIndex > 0)
        {
            (TermHandle->StrIndex)--;
            printf("%s", TERM_CURSOR_LEFT);
        }
    }

    //A letter is the end of a Cursor control sequence.
    if (((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')))
    {
        TermHandle->EscFlag = 0;
        memset(escbuf, 0, 8);
        escidx = 0;
    }

    return 0;

}

/*!@brief Insert a char into a string at certain position.
 *
 * @param string
 * @param c
 * @param idx
 */
int term_insert(char *string, char c, int idx)
{
    int len = strlen(string);

    if (idx > len)
    {
        //can't insert if index out of range.
        return -1;
    }

    //right shift buffer from index to end
    int i = 0;
    for (i = len; i > idx; i--)
    {
        string[i] = string[i - 1];
    }

    //insert value
    string[idx] = c;

    return 0;
}

/*!@brief Delete a char from a string.
 *
 * @param string
 * @param idx
 * @return
 */
int term_delete(char *string, int idx)
{
    int len = strlen(string);

    if (idx > len)
    {
        //can't insert if index out of range.
        return -1;
    }

    //left shift buffer from index to end
    int i = 0;
    for (i = idx; i < len + 1; i++)
    {
        string[i - 1] = string[i];
    }

    return 0;
}

/*!@brief Initialize Terminal I/O
 *
 * @param TermHandle    Terminal Hanle
 * @return
 */
int Term_IO_init(Term_HandleTypeDef *TermHandle)
{
    //Initialize TermHandle
    memset(TermHandle, 0, sizeof(Term_HandleTypeDef));
    TermHandle->EnableLoopBack = TERM_LOOP_BACK_EN;

    char *stdout_buf = malloc(TERM_STDOUT_BUF_SIZE);

    // Set STDOUT/STDIN/STDERR Buffer type and size
    //(char*), It could be a pointer to buffer or NULL. When set to NULL it will automatic assign the buffer.
    //[_IOLBF], Line buffer mode, transmit data when get a '\n'
    //[_IONBF], No buffer mode, transmit data byte 1by1
    //[_IOFBF], Full buffer mode, transmit data when buffer is full, or manually
    setvbuf(stdout, stdout_buf, _IONBF, TERM_STDOUT_BUF_SIZE);
    setvbuf(stderr, (char*) NULL, _IONBF, TERM_STDERR_BUF_SIZE);
    setvbuf(stdin, (char*) NULL, _IONBF, TERM_STDIN_BUF_SIZE);

    return 0;
}

/*!@brief Get a single char from input buffer.
 *
 * @return
 */
int Term_IO_getc(void)
{
    return fgetc(stdin);
}

/*!@brief Get a line from Terminal IO, usually from STDIN.
 *
 * @param dest_str      pointer to destination to store read line.
 * @param TermHandle    Terminal Handle
 * @return      Length of read bytes.
 */
int Term_IO_getline(Term_HandleTypeDef *TermHandle, char *dest_str)
{
    int c = 0;

    do
    {
        //Get 1 char and check
        c = Term_IO_getc();

        //Handle characters
        switch (c)
        {
        case 0x00:      //NULL for ASCII
        case 0xFF:      //EOF for ASCII
        case EOF:       //EOF for STDIN
        {
            break;
        }
        case '\e':      //ESC
        {
            term_esc_handle(c, TermHandle);
            break;
        }
        case '\x7f':    //Delete, MacOS Keyboard
        case '\b':      //Backspace, Windows Keyboard.
        {
            if (TermHandle->StrIndex > 0)
            {
                //Delete 1 byte from buffer.
                term_delete(TermHandle->StrBuf, TermHandle->StrIndex);
                //Erase terminal line and print new buffer string.
                printf("%s\r%s%s", TERM_ERASE_LINE, TERM_PROMPT_CHAR, TermHandle->StrBuf);
                //Move cursor
                printf("\e[%dG", TermHandle->StrIndex + strlen(TERM_PROMPT_CHAR));
                TermHandle->StrIndex--;
            }

            break;
        }
        case '\r': //End of a line, MacOS style
        case '\n': //End of a line, Unix/Windows style
        {
            //Push to history without \'n'
            if (TermHandle->StrIndex > 0)
            {
                term_history_push(TermHandle, 1);
            }

            //Copy out string with '\n'
            strcat(TermHandle->StrBuf, "\r\n");
            strcpy(dest_str, TermHandle->StrBuf);
            printf("\n");

            //Clear buffer
            int ret = strlen(TermHandle->StrBuf);
            memset(TermHandle->StrBuf, 0, TERM_STRING_BUF_SIZE);
            TermHandle->StrIndex = 0;

            return ret; //return copy data count
        }
        default:
        {
            //The letters after a ESC is terminal control characters
            if (TermHandle->EscFlag > 0)
            {
                term_esc_handle(c, TermHandle);
                TermHandle->EscFlag--;
            }
            else
            {
                //Insert 1 byte to buffer
                term_insert(TermHandle->StrBuf, c, TermHandle->StrIndex++);

                //Loop Back function
                if (TermHandle->EnableLoopBack)
                {
                    if (TermHandle->StrBuf[TermHandle->StrIndex] == 0)
                    {
                        //Loop back 1 byte if it's the end of a line.
                        printf("%c", c);
                    }
                    else
                    {
                        //Erase entire line and print new buffer content
                        printf("%s\r%s%s", TERM_ERASE_LINE, TERM_PROMPT_CHAR, TermHandle->StrBuf);
                        //Move cursor
                        printf("\e[%dG", TermHandle->StrIndex + strlen(TERM_PROMPT_CHAR) + 1);
                    }
                }
            }
            break;
        }
        }
    } while (c != '\xff');

    return 0;
}

int App_Terminal_register(const char* name, CliCallBack *callback, const char *helptext)
{
    return 0;
}

int App_Terminal_unregister()
{
    return 0;
}

int App_Terminal_init(void)
{
    Term_IO_init(&gTermHandle);
    printf("\r\n#============================#");
    printf("\r\n#    %s%s🍱 Aha, BentoBox! 🍱%s    #", TERM_BOLD, TERM_BLINK, TERM_RESET);
    printf("\r\n#============================#");
    printf("\r\nTerminal Version=[%s]\nCompile Date=[%s %s]\n%s",
    TERM_VERSION,
    __DATE__,
    __TIME__,
    TERM_PROMPT_CHAR);
    return 0;
}

/* Example of a Mini-Terminal */
int App_Terminal_run(void)
{
    static char sbuf[TERM_STRING_BUF_SIZE] =
    { 0 };
    int scount;

    //Start a Mini-Terminal
    scount = Term_IO_getline(&gTermHandle, sbuf);
    if (scount > 1)
    {
        char *argbuf[TERM_TOKEN_AMOUNT] =
        { 0 };
        int argcount = 0;
        //Convert string to args and run command.
        Cli_parseString(sbuf, &argcount, argbuf);
        Cli_runCommand(argcount, argbuf, gTermCommand);

        //Clear string buffer
        memset(sbuf, 0, scount);

        printf("%s", TERM_PROMPT_CHAR);
    }
    else if (scount == 1)
    {
        printf("%s", TERM_PROMPT_CHAR);
    }

    //Flush buffer
    fflush(stdout);
    fflush(stderr);

    return 0;
}

