/******************************************************************************
 * @file    cli_porting_stm32l476_discovery.c
 * @brief   A simple Command Line Interface (CLI) for MCU.
 *          This is the API porting function for STM32L476 Discovery file.
 *          Build a FIFO for UART in/out to override stdio.
 *
 * @author  Nick Yang
 * @date    2018/11/01
 * @version V1.0
 *****************************************************************************/

/*! Includes ----------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cli.h"
#include "cli_pipe.h"
#include "cmsis_os.h"
#include "stm32l4xx_hal.h"
#include "usbd_cdc_if.h"

/*! Defines -----------------------------------------------------------------*/

#define STDIN_RX_BUF_SIZE       256         //!< STDIN input buffer size
#define STDOUT_TX_LINE_SIZE     256         //!< STDOUT max number of bytes in a line
#define STDOUT_TX_QUEUE_SIZE    256         //!< STDOUT max number of lines in the queue
#define STDOUT_TX_MEM_SIZE      4096        //!< STDOUT max memory usage allowed in heap

/*! Variables ---------------------------------------------------------------*/
extern UART_HandleTypeDef huart2;

UART_HandleTypeDef *STDIN_huart = &huart2;  //!< STDIN UART handle
UART_HandleTypeDef *STDOUT_huart = &huart2; //!< STDOUT UART handle
UART_HandleTypeDef *STDERR_huart = &huart2; //!< STDERR UART handle

MsgQueue_TypeDef stdout_pipe = { 0 };

RingBuf_TypeDef stdin_pipe1 = { 0 };
RingBuf_TypeDef stdin_pipe2 = { 0 };

/*! Functions ---------------------------------------------------------------*/

void cli_sleep(int ms)
{
    osDelay(ms);
}

unsigned int cli_gettick(void)
{
    return HAL_GetTick();
}

/*!@brief   Port API for calloc()
 *          Request buffer
 *
 * @param   size
 */
void *cli_calloc(size_t size)
{
    if (size <= 0)
    {
        return NULL;
    }
    void *ptr = NULL;
    while (ptr == NULL)
    {
        //ptr = pvPortMalloc(size);
        ptr = malloc(size);
    }
    memset(ptr, 0, size);
    return ptr;
}

void cli_free(void *ptr)
{
    //vPortFree(ptr);
    free(ptr);
}

int cli_port_init()
{
    // Set STDIO type and buffer size
    setvbuf(stdout, (char *) NULL, _IOLBF, 256);
    setvbuf(stderr, (char *) NULL, _IONBF, 0);
    setvbuf(stdin, (char *) NULL, _IONBF, 0);

    // Set UART handle pointer
    STDIN_huart = &huart2; //!< STDIN UART handle
    STDOUT_huart = &huart2; //!< STDOUT UART handle
    STDERR_huart = &huart2; //!< STDERR UART handle

    // Setup STDOUT pipe
    MsgQueue_Init(&stdout_pipe, STDOUT_TX_QUEUE_SIZE, STDOUT_TX_MEM_SIZE);

    // Setup STDIN pipe
    RingBuf_Init(&stdin_pipe1, STDIN_RX_BUF_SIZE);
    RingBuf_Init(&stdin_pipe2, STDIN_RX_BUF_SIZE);
    HAL_UART_Receive_DMA(STDIN_huart, (uint8_t*) stdin_pipe1.pBuf, STDIN_RX_BUF_SIZE);

    // Register board command
    CLI_Register("info", "MCU Information", &cli_info);
    CLI_Register("reset", "MCU Reset", &cli_reset);
    CLI_Register("mem", "Memory write/read", &cli_mem);
    CLI_Register("nvram", "Non-Volatile RAM operation", &cli_nvram);
    CLI_Register("qspi", "Quad-SPI flash operation", &cli_qspi);
    CLI_Register("os", "RTOS operation", &cli_os);
    CLI_Register("rtc", "Real Time Clock operation", &cli_rtc);

    return 0;
}

void cli_port_deinit()
{
    ;
}

int cli_port_getc(void)
{
    return getchar();
}

/*!@brief   Override system call of _read, route STDIN to UART RX.
 *          get byte from STDIN stream.
 *
 * @param file  stdin
 * @param ptr   pointer to store read byte
 * @param len   length of read bytes
 * @return      Length of bytes actually read.
 */
int _read(int file, char *ptr, int len)
{

    if ((ptr == NULL) || (len == 0))
    {
        return 0;
    }
    // Loop get character
    for (int i = 0; i < len; i++)
    {
        int c1 = RingBuf_GetChar(&stdin_pipe1);
        if (c1 != EOF)
        {
            *ptr++ = c1;
        }
        else
        {
            int c2 = RingBuf_GetChar(&stdin_pipe2);
            if (c2 != EOF)
            {
                *ptr++ = c2;
            }
            else
            {
                *ptr = 0xFF;
            }

        }
    }

    return len;
}

/*!@brief   Override system call of _write, route STDOUT to UART TX.
 *          Transfer bytes through UART.
 *          STDOUT will be transfered in non-blocking mode.
 *          STDERR will be abort current STDOUT (if there are) and transfered in
 * blocking mode.
 *
 * @param file  STDOUT_FILENO or STDERR_FILENO
 * @param ptr   Pointer to bytes
 * @param len   Length of bytes
 * @return      Length of bytes actually transfered
 */
int _write(int file, char *ptr, int len)
{
    if ((ptr == NULL) || (len == 0))
    {
        return 0;
    }

    if ((file == 1) || (file == 2)) //STDOUT = 1, STDERR =2
    {
        CDC_Transmit_FS((uint8_t*) ptr, len);
        MsgQueue_PushToHead(&stdout_pipe, ptr, len);
        char *trans_ptr[1] = { NULL };
        int trans_len = 0;
        MsgQueue_PullFromTail(&stdout_pipe, trans_ptr, &trans_len);
        if ((trans_ptr[0] != NULL) && (trans_len > 0))
        {
            if (HAL_OK == HAL_UART_Transmit_DMA(STDOUT_huart, (uint8_t*) *trans_ptr, trans_len))
            {
                return trans_len;
            }
        }

        return len;
    }

    return 0;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == STDIN_huart->Instance)
    {
        //HAL_UART_Receive_DMA(STDIN_huart, STDIN_RxBuf, sizeof(STDIN_RxBuf));
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == STDOUT_huart->Instance)
    {
        // Free last transmitted buffer
        MsgQueue_FreeFromTail(&stdout_pipe);

        // Trigger next transmit
        char *trans_ptr[1] = { NULL };
        int trans_len = 0;
        MsgQueue_PullFromTail(&stdout_pipe, trans_ptr, &trans_len);
        if ((*trans_ptr != NULL) && (trans_len > 0))
        {
            HAL_UART_Transmit_DMA(huart, (uint8_t*) *trans_ptr, trans_len);
        }
    }
}

void HAL_UsbCdc_ReceiveCallBack(uint8_t* Buf, uint32_t *Len)
{
    for (int i = 0; i < *Len; i++)
    {
        RingBuf_PutChar(&stdin_pipe2, Buf[i]);
    }
}
