/******************************************************************************
 * @file    dfu_console.h
 * @brief   Mini Console for DFU (Device Firmware Upgrade) function.
 *
 * @author  Nick Yang
 * @date    2018/08/12
 * @version V0.2 Initial Version, support Intel Hex (ihex) format.
 *          V0.3 Using dfu_print / dfu_getchar instead of STDIO.
 *
 *****************************************************************************/

#ifndef DFU_CONSOLE_H_
#define DFU_CONSOLE_H_

/*! Includes ----------------------------------------------------------------*/
#include "stdint.h"

/*! Defines -----------------------------------------------------------------*/
// clang-format off
/*!@defgroup    HEX_DATATYPE Define Group
 *              Defines for Intel Hex file format
 * @ref         < https://en.wikipedia.org/wiki/Intel_HEX#Color_legend >
 */
#define HEX_DATATYPE_DATA               0x00
#define HEX_DATATYPE_END                0x01
#define HEX_DATATYPE_EXT_SEG_ADDR       0x02
#define HEX_DATATYPE_START_SEG_ADDR     0x03
#define HEX_DATATYPE_LINEAR_ADDR        0x04
#define HEX_DATATYPE_START_ADDR         0x05
#define HEX_MAX_STRING_LENGTH           64      //!< Maximum ihex file line length
#define HEX_MAX_DATA_LENGTH             16      //!< Maximum ihex data length in a single line

#define DFU_ENTERANCE_CHAR              '\r'    //!< Character to enter DFU mode
#define DFU_PROMPT_CHAR                 "]"     //!< Character as line prompt in DFU mode
#define DFU_BOOT_DELAY                  1000    //!< Delay time for wait keyboard input to enter DFU
#define DFU_INPUT_BUF_SIZE              2048    //!< IO input buffer size
#define DFU_OUTPUT_BUF_SIZE             2048    //!< IO output buffer size

//!< [1]: Force erase flash page.

/*!@def DFU_WORK_BANK       Working Flash Bank, could select FLASH_BANK_1 FLASH_BANK_2 or both.
 * @def DFU_DOWNLOAD_BANK   Download Flash Bank, could select FLASH_BANK_1 FLASH_BANK_2 or both.
 *
 *      FLASH_BANK_1 : Always work on bank1.
 *      FLASH_BANK_2 : Always work on bank2.
 *                     @note when boot from the other bank, it will try copy FW to work bank and jump back to work banks.
 *
 *      FLASH_BANK_1 | FLASH_BANK_2 :   Work on either bank.
 *                     @note automatic download content to another bank.
 */
#define DFU_WORK_BANK                  FLASH_BANK_1//(FLASH_BANK_1 | FLASH_BANK_2)
// clang-format on

/*!@struct Line structure of a line in .hex file
 *
 */
typedef struct Dfu_HexLineTypeDefine {
    uint8_t  Header;
    uint8_t  DataLength;
    uint16_t DataOffset;
    uint8_t  DataType;
    uint8_t  DataBuf[256];
    uint8_t  CheckSum;
    uint32_t BaseAddress;
    uint32_t LineCount;
    uint32_t ErrorCount;
    uint32_t ByteCount;
} Dfu_HexLineTypeDefine;

typedef enum DFU_RET {
    DFU_OK           = 0,   //!< General OK
    DFU_BUSY         = 1,   //!< DFU process is busy
    DFU_END          = 2,   //!< DFU process is end
    DFU_FLASH_ERROR  = -10, //!< Flash operation error
    DFU_FORMAT_ERR   = -5,  //!< Command format error
    DFU_BYTE_ERR     = -4,  //!< Get unknown character
    DFU_CHECKSUM_ERR = -3,  //!< Check Sum Error
    DFU_LENGTH_ERR   = -2,  //!< Data Length Error
    DFU_ERROR        = -1   //!< General Error
} DFU_RET;

/*! Functions ---------------------------------------------------------------*/
DFU_RET Bsp_Dfu_Init();
DFU_RET Bsp_Dfu_Console();

#endif /* DFU_CONSOLE_H_ */
