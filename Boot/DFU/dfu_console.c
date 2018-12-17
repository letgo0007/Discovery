/******************************************************************************
 * @file    dfu_console.c
 * @brief   Mini Console for DFU (Device Firmware Upgrade) function.
 *
 * @author  Nick Yang
 * @date    2018/08/12
 * @version V0.2 Initial Version, support Intel Hex (ihex) format.
 *          V0.3 Using dfu_print / dfu_getchar instead of printf / getchar
 * @ref     < https://en.wikipedia.org/wiki/Intel_HEX#Color_legend >
 *****************************************************************************/

/*! Includes ----------------------------------------------------------------*/

#include "stm32l4xx_hal.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "stdarg.h"

#include "dfu_console.h"
#include "dfu_flash_if.h"

/*! Defines -----------------------------------------------------------------*/

/*! Variables ---------------------------------------------------------------*/
extern UART_HandleTypeDef huart2;

uint8_t *Dfu_InputBuf = NULL;
uint16_t Dfu_InputIdx = 0;
uint8_t *Dfu_OutputBuf = NULL;
uint16_t Dfu_OutputIdx = 0;

/*! Functions ---------------------------------------------------------------*/
/*!@brief Convert 'A' to 0x0A
 *
 * @param   ascii   : 'A'  ASCII character
 * @param   value   : 0x0A Pointer to u8 array
 * @return  [0]     : Success
 *          [-1]    : Fail
 */
uint8_t ascii_to_u4(uint8_t ascii, uint8_t *value)
{
    switch (ascii)
    {
    case '0' ... '9':
        *value = ascii - '0';
        break;
    case 'a' ... 'z':
        *value = ascii - 'a' + 0x0A;
        break;
    case 'A' ... 'Z':
        *value = ascii - 'A' + 0x0A;
        break;
    default:
        return -1;
    }
    return 0;
}

/*!@brief Convert "1234ABCD" to 0x12 0x34 0xAB 0xCD
 *
 * @param   string  : "1234ABCD"
 * @param   len     : Length of "1234ABCD" = 8
 * @param   value   : Pointer to u8 array, [0x12,0x34,0x56,0x78]
 * @return  [0]     : Success
 *          [-1]    : Fail
 */
int ascii_to_u8(char *string, uint16_t len, uint8_t *value)
{
    uint16_t i;
    int status = 0;

    for (i = 0; i < len; i = i + 2)
    {
        uint8_t high_4bit = 0;
        uint8_t low_4bit = 0;

        status = ascii_to_u4(string[i], &high_4bit);
        status += ascii_to_u4(string[i + 1], &low_4bit);

        if (status != 0)
        {
            return -1;
        };

        *value++ = (high_4bit << 4) + low_4bit;
    }
    return 0;
}

int dfu_flush(void)
{
    if (Dfu_OutputBuf == NULL)
    {
        return 0;
    }

    static uint16_t last_head = 0;
    static uint16_t last_len = 0;

    // Flush output buffer only when UART TX is not busy
    HAL_UART_StateTypeDef state = HAL_UART_GetState(&huart2);
    if ((HAL_UART_STATE_BUSY_TX == state) || (HAL_UART_STATE_BUSY_TX_RX == state))
    {
        return 0;
    }

    // Erase last transfer buffers
    if (last_len != 0)
    {
        memset(&Dfu_OutputBuf[last_head], 0, last_len);
        last_head = last_head + last_len;
        if (last_head >= DFU_OUTPUT_BUF_SIZE)
        {
            last_head = 0;
        }
        last_len = 0;
    }

    // Find something in the buffer is not empty
    int head = last_head;
    int len = 0;
    if (Dfu_OutputBuf[head] != 0)
    {
        len = strlen((const char*) &Dfu_OutputBuf[last_head]);
    }

    // Trigger transfer
    HAL_UART_Transmit_DMA(&huart2, &Dfu_OutputBuf[head], len);
    last_head = head;
    last_len = len;

    return len;
}

int dfu_print(char *fmt, ...)
{
    if (Dfu_OutputBuf == NULL)
    {
        return 0;
    }

    // Virtual print to buffer
    va_list argptr;
    va_start(argptr, fmt);
    int cnt;
    char buffer[1024] = { 0 };
    cnt = vsprintf(buffer, fmt, argptr);
    va_end(argptr);

    // Copy to output buffer
    for (int i = 0; i < cnt; i++)
    {
        Dfu_OutputBuf[Dfu_OutputIdx] = buffer[i];
        Dfu_OutputIdx = (Dfu_OutputIdx >= DFU_OUTPUT_BUF_SIZE - 1) ? 0 : Dfu_OutputIdx + 1;
    }

    dfu_flush();

    return (cnt);
}

//Get a single char from input buffer
int dfu_getchar()
{
    if (Dfu_InputBuf == NULL)
    {
        return 0;
    }

    uint8_t c = Dfu_InputBuf[Dfu_InputIdx];
    if (c == 0)
    {
        return '\xff';
    }
    else
    {
        Dfu_InputBuf[Dfu_InputIdx] = 0;
        Dfu_InputIdx = (Dfu_InputIdx >= DFU_INPUT_BUF_SIZE - 1) ? 0 : Dfu_InputIdx + 1;
        return c;
    }
}

//Get a line (end with '\r' or '\n') from input buffer
int dfu_getline(char *line)
{
    static char linebuf[256] = { 0 };
    static int idx = 0;
    int c = 0;

    do
    {
        //Get 1 char and check
        c = dfu_getchar();

        //Handle characters
        switch (c)
        {
        case 0x00:  //NULL for ASCII
        case 0xFF:  //EOF for ASCII
        case EOF:   //EOF for STDIN
        case '\n':  //End of a line, Unix/Windows Style.
        {
            break;
        }
        case '\r':  //End of a line, MacOS style
        {
            if (idx < 1) //Empty line with "/n"
            {
                dfu_print("\n]");
            }
            else
            {
                //Copy out line buffer
                memcpy(line, linebuf, idx);

                //Clear buffer and index
                memset(linebuf, 0, idx);
                int ret = idx;
                idx = 0;
                return ret; //return copy data count
            }
            break;
        }
        default:
        {
            //Buffer 1 byte
            linebuf[idx++] = c;

            //echo back if it's not a ihex line.
            if (linebuf[0] != ':')
            {
                dfu_print("%c", c);
            }
            break;
        }
        }
    } while (c != '\xff');

    return 0;
}

void dfu_io_init(void)
{
    Dfu_InputBuf = malloc(DFU_INPUT_BUF_SIZE);
    memset(Dfu_InputBuf, 0, DFU_INPUT_BUF_SIZE);

    Dfu_OutputBuf = malloc(DFU_OUTPUT_BUF_SIZE);
    memset(Dfu_OutputBuf, 0, DFU_INPUT_BUF_SIZE);

    HAL_UART_Receive_DMA(&huart2, Dfu_InputBuf, DFU_INPUT_BUF_SIZE);
}

void dfu_io_deinit(void)
{
    free(Dfu_InputBuf);
    free(Dfu_OutputBuf);
    HAL_UART_AbortTransmit_IT(&huart2);
    HAL_UART_AbortReceive_IT(&huart2);
}

/*!@brief Parse a Intel Hex format line, example is:
 *        :020000040800F2
 *
 *
 * @param   string  : ":020000040800F2" Intel Hex file line, start with ':'
 * @param   len     : Length of string
 * @param   hexline : Pointer to hex line structure.
 * @return  @ref Dfu_RetTypeDef
 */
Dfu_RetTypeDef Hex_ParseLine(char *string, int len, Dfu_HexLineTypeDefine *hexline)
{
    if (string[0] == ':')
    {
        uint8_t u8buf[256] = { 0 };
        uint8_t u8len = 0;
        uint8_t sum = 0;

        /*! Convert ASCII string to uint8_t, 2x ASIIC to 1x u8.
         *
         */
        u8len = (len - 1) / 2;
        if (-1 == ascii_to_u8(&string[1], len - 1, u8buf))
        {
            hexline->ErrorCount++;
            return DFU_BYTE_ERR; //Hex Data convert Error
        }

        /*!Check Format, HEX file line format:
         * https://en.wikipedia.org/wiki/Intel_HEX#Color_legend
         *
         * Header       : ':'
         * Byte[0]      : Data Length [0~0xFF]
         * Byte[1:2]    : Data Offset [0~0xFFFF] ,high byte first
         * Byte[3]      : Data Type, @ref HEX_DATATYPE_DefineGroup
         * Byte[4:N-1]  : Data.
         * Byte[N]      : Check Sum.
         *
         * N = 5 + Data Length
         * Sum(0,1,...,N) & 0xFF = 0x00
         */
        hexline->Header = ':';
        hexline->DataLength = u8buf[0];
        hexline->DataOffset = u8buf[1] * 256 + u8buf[2];
        hexline->DataType = u8buf[3];
        hexline->CheckSum = u8buf[4 + hexline->DataLength];

        /*! Check Data Length */
        if ((5 + hexline->DataLength) != u8len)
        {
            hexline->ErrorCount++;
            return DFU_LENGTH_ERR; //Line Length Error
        }

        /*! Check Sum */
        for (int i = 0; i < u8len; i++)
        {
            sum += u8buf[i];
        }

        if ((sum & 0x00FF) != 0x0000)
        {
            hexline->ErrorCount++;
            return DFU_CHECKSUM_ERR; //CheckSum Error
        }

        /*! Export Data */
        memcpy(hexline->DataBuf, &u8buf[4], hexline->DataLength);
        hexline->LineCount++;
        return DFU_OK;
    }
    else
    {
        return DFU_FORMAT_ERR; //Invalid Header Error
    }
}

/*!@brief Write Flash using Intel hex line.
 *
 * @param hexline   : Pointer to a hexline structure.
 * @return
 */
Dfu_RetTypeDef Hex_ExcuteLine(Dfu_HexLineTypeDefine *hexline)
{
    uint32_t base_addr = 0;
    uint32_t address = 0;

    //Get current boot bank
    uint32_t CurrentBank = Flash_getActiveBank();
    uint32_t OtherBank = FLASH_BANK_2 + FLASH_BANK_1 - CurrentBank;

    switch (hexline->DataType)
    {
    case HEX_DATATYPE_DATA: //!< Write Byte to Flash
        //Calculate Address on backup bank
        address = hexline->BaseAddress + hexline->DataOffset - FLASH_BASE
                + Flash_getAddress(OtherBank, 0);

        //Write Flash
        Flash_program_8bit(address, hexline->DataBuf, hexline->DataLength);

        //Add byte count
        hexline->ByteCount += hexline->DataLength;

        if (hexline->ByteCount % 1024 <= 8)
        {
            dfu_print("\r[%4d kB]", hexline->ByteCount / 1024 + 1);
            //dfu_print("#");
        }

        break;
    case HEX_DATATYPE_END: //!< End of a Hex File
        //End of operation
        dfu_print("\r\n[%03d.%03d]Hex: End of file\n", HAL_GetTick() / 1000, HAL_GetTick() % 1000);
        dfu_print("Hex: LineCount  = %ld\n", hexline->LineCount);
        dfu_print("Hex: ErrorCount = %ld\n", hexline->ErrorCount);
        dfu_print("Hex: ByteCount  = %ld\n", hexline->ByteCount);
        dfu_flush();
        HAL_Delay(10);

        if (hexline->ErrorCount == 0)
        {
            dfu_print("Hex: Reboot to new bank\n");
            dfu_print("========End of DFU===========\n");
            HAL_Delay(10);
            dfu_flush();
            HAL_Delay(10);
            Flash_setActiveBank(OtherBank);
        }
        break;
    case HEX_DATATYPE_EXT_SEG_ADDR:
        //Not Used
        break;
    case HEX_DATATYPE_START_SEG_ADDR:
        //Not Used
        break;
    case HEX_DATATYPE_LINEAR_ADDR: //!< Set Base Address
        //Set Base Address
        base_addr = (uint32_t) (hexline->DataBuf[0] * 256 + hexline->DataBuf[1]) << 16;
        hexline->BaseAddress = base_addr;
        dfu_print("\r\n[%03d.%03d]Hex: Set Base Address @ 0x%08lX\n", HAL_GetTick() / 1000,
                HAL_GetTick() % 1000, base_addr);
        break;
    case HEX_DATATYPE_START_ADDR: //!< Set
        address = (uint32_t) (hexline->DataBuf[0] * 256 + hexline->DataBuf[1]) << 16;
        dfu_print("\r\n[%03d.%03d]Hex: Set Start Address @ 0x%08lX\n", HAL_GetTick() / 1000,
                HAL_GetTick() % 1000, address);
        fflush(stdout);
        HAL_Delay(3);
        break;
    default:
        //Error
        hexline->ErrorCount++;
        dfu_print("DATA Type not support!");
        break;
    }
    return 0;
}

uint32_t Dfu_selectBootBank()
{
    //!< Get current working bank
    Dfu_RetTypeDef ret = DFU_OK;
    uint32_t CurrentBank = Flash_getActiveBank();
    uint32_t OtherBank = FLASH_BANK_2 + FLASH_BANK_1 - CurrentBank;

    dfu_print("%s: Boot from flash bank [%ld]\n", __FILE__, CurrentBank);

    /*! Boot from work bank or dual work bank is selected */
    if (((CurrentBank == DFU_WORK_BANK)) || (DFU_WORK_BANK == FLASH_BANK_2 + FLASH_BANK_1))
    {
        ;
    }
    else /*! Single work bank is selected, and Boot from download bank. */
    {
        //Single work bank is selected, copy and jump to the other bank.
        HAL_Delay(10);
        dfu_print("\e[33m%s: Work bank is [%ld], Try copy bank[%ld]->bank[%ld].\n\e[0m", __FILE__,
        DFU_WORK_BANK, CurrentBank, OtherBank);

        ret = Flash_copyBank(CurrentBank, OtherBank);

        if (DFU_OK == ret)
        {
            dfu_print("\n\e[33mBank Copy Success! Reboot to bank[%ld].\e[0m\n", OtherBank);
            Flash_setActiveBank(OtherBank);
        }

    }

    return CurrentBank;
}

/*!@brief DFU Initial Work Flow:
 *
 * FLASH_BANK_1 is working bank.
 * FLASH_BANK_2 is DFU bank.
 *
 * [Reboot] -> Bank1 ? -> Wait Key for 1s ------No----->             [Normal work]
 *                                        |
 *                                        ------YES---->             [DFU]
 *
 * [Reboot] -> Bank2 ? -> Copy Bank2 to Bank1 -> Set Bank1 active -> [Reboot]
 *
 * [DFU]    -> Parse Hex file -> Write Bank2 -> Set Bank2 active ->  [Reboot]
 *
 */

Dfu_RetTypeDef Bsp_Dfu_Init()
{
    /*! 1. Initialize IO */
    dfu_io_init();
    dfu_print("%s: Bsp_Dfu_init()\n", __FILE__);

    /*! 2. Select Boot bank & DFU bank*/
    uint32_t BootBank = Dfu_selectBootBank();
    uint32_t DfuBank = FLASH_BANK_2 + FLASH_BANK_1 - BootBank;

    /*! 3. Wait Keyboard to enter DFU console. */
    dfu_print("%s: Press [Enter] to enter DFU console, wait %d ms.\n", __FILE__, DFU_BOOT_DELAY);
    uint32_t end_tick = HAL_GetTick() + DFU_BOOT_DELAY;
    while (HAL_GetTick() < end_tick)
    {
        char c = dfu_getchar();
        if (('\r' == c) || ('\n' == c))
        {
            dfu_print("\n========Start of DFU=========\n");

            //Erase target flash
            //Flash_erasePage(DfuBank, 0, DFU_MAX_PAGE);
            Flash_eraseBank(DfuBank);

            // Run DFU console
            Bsp_Dfu_Console();
        }
        HAL_Delay(100);
        dfu_print(".");
    }

    /*! 4. Return to normal boot! */
    dfu_io_deinit();
    return DFU_OK;
}

/*!@brief  Start a mini console for DFU function.
 *         It will check input of UART and write DFU bank flash.
 *
 * @return
 */
Dfu_RetTypeDef Bsp_Dfu_Console()
{
    const char *Dfu_helptext = "\n"
            "\e[1m\e[5m===Mini DFU console===\e[0m\n"
            "FW Download: Transmit the <.hex> file through UART, e.g.\n"
            "             \e[4mcat ./Build/discovery.hex >/dev/cu.usbmodem14203\e[0m\n"
            "quit   | q : Quit DFU mode\n"
            "status | s : Show DFU status.\n"
            "help       : Show this help text.\r\n";

    static int str_len = 0;
    static char str_buf[HEX_MAX_STRING_LENGTH] = { 0 };
    static Dfu_HexLineTypeDefine hexline = { 0 };

    Dfu_RetTypeDef ret = DFU_OK;

    dfu_print("\r\n]");

    while (1)
    {
        //Get Line from UART
        str_len = dfu_getline(str_buf);

        if (str_len > 0)
        {
            //Process command
            if (strcmp(str_buf, "help") == 0)
            {
                dfu_print("\n%s", Dfu_helptext);
            }
            else if ((strcmp(str_buf, "q") == 0) || (strcmp(str_buf, "quit") == 0))
            {
                dfu_print("\nQuit DFU process.\n");
                return -1;
            }
            else if ((strcmp(str_buf, "s") == 0) || (strcmp(str_buf, "status") == 0))
            {
                dfu_print("%s\nDFU status:\n", str_buf);
                dfu_print("HexLineCount  = %ld\n", hexline.LineCount);
                dfu_print("HexErrorCount = %ld\n", hexline.ErrorCount);
                dfu_print("HexByteCount  = %ld\n", hexline.ByteCount);
            }
            else if (str_buf[0] == ':')
            {
                //Parse sting using ihex format
                ret = Hex_ParseLine(str_buf, str_len, &hexline);
                if (ret == DFU_OK)
                {
                    //Process HEX line
                    ret = Hex_ExcuteLine(&hexline);
                    if (ret != DFU_OK)
                    {
                        dfu_print("ERROR: Dfu_runHexLine fail, error code = [%d]\n", ret);
                    }
                }
                else
                {

                    dfu_print("ERROR: Dfu_parseHexLine fail, error code = [%d]\n", ret);
                }

                //Clear buffer after successfully run a command.
                memset(str_buf, 0, sizeof(str_buf));
                str_len = 0;
            }
            else
            {
                dfu_print("\nERROR: Unknown command, try [help].\n");
            }

            //Clear empty line
            memset(str_buf, 0, sizeof(str_buf));
            str_len = 0;
        }

        dfu_flush();
    };

    return DFU_OK;
}
