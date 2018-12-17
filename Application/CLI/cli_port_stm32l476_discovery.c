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
#include "cmsis_os.h"
#include "stm32l4xx_hal.h"
#include "usbd_cdc_if.h"

/*! Defines -----------------------------------------------------------------*/

#define STDIN_RX_BUF_SIZE       256         //!< STDIN input buffer size
#define STDOUT_TX_LINE_SIZE     256         //!< STDOUT max number of bytes in a line
#define STDOUT_TX_QUEUE_SIZE    256         //!< STDOUT max number of lines in the queue
#define STDOUT_TX_MEM_SIZE      4096        //!< STDOUT max memory usage allowed in heap

#define STDIO_MALLOC(x)         malloc(x)
#define STDIO_FREE(x)           free(x)

typedef struct
{
    uint8_t aBuf[STDIN_RX_BUF_SIZE];
    uint16_t PutIndex;
    uint16_t GetIndex;
} STDIN_RingBufTypeDef;

/*! Variables ---------------------------------------------------------------*/
extern UART_HandleTypeDef huart2;

UART_HandleTypeDef *STDIN_huart = &huart2;          //!< STDIN UART handle
UART_HandleTypeDef *STDOUT_huart = &huart2;         //!< STDOUT UART handle
UART_HandleTypeDef *STDERR_huart = &huart2;         //!< STDERR UART handle

uint8_t *STDOUT_MsgPtr[STDOUT_TX_QUEUE_SIZE] = { 0 };
uint16_t STDOUT_MsgLen[STDOUT_TX_QUEUE_SIZE] = { 0 };
uint16_t STDOUT_QueueHead = 0;
uint16_t STDOUT_QueueTail = 0;

STDIN_RingBufTypeDef STDIN_Uart2Buf = { 0 };
STDIN_RingBufTypeDef STDIN_UsbBuf = { 0 };

/*! Functions ---------------------------------------------------------------*/

uint8_t STDIN_GetChar(STDIN_RingBufTypeDef *rbuf)
{
    uint8_t c = rbuf->aBuf[rbuf->GetIndex];

    if (c == 0)
    {
        return EOF;
    }
    else
    {
        rbuf->aBuf[rbuf->GetIndex] = 0;
        rbuf->GetIndex = (rbuf->GetIndex >= STDIN_RX_BUF_SIZE - 1) ? 0 : rbuf->GetIndex + 1;
        return c;
    }
}

void STDIN_PutChar(STDIN_RingBufTypeDef *rbuf, uint8_t c)
{
    rbuf->aBuf[rbuf->PutIndex] = c;
    rbuf->PutIndex = (rbuf->PutIndex >= STDIN_RX_BUF_SIZE - 1) ? 0 : rbuf->PutIndex + 1;
}

/*!@brief Get free queue amount in STDOUT queue
 *
 * @return Free queue amount
 */
uint32_t STDOUT_GetFreeQueue(void)
{
    uint32_t free = 0;
    for (uint16_t i = 0; i < STDOUT_TX_QUEUE_SIZE; i++)
    {
        if (STDOUT_MsgPtr[i] == NULL)
        {
            free++;
        }
    }
    return free;
}

/*!@brief Get free memory in STDOUT queue
 *
 * @return Number of bytes are allowed to use.
 */
uint32_t STDOUT_GetFreeMem(void)
{
    uint32_t sum = 0;
    for (uint16_t i = 0; i < STDOUT_TX_QUEUE_SIZE; i++)
    {
        sum += STDOUT_MsgLen[i];
    }
    return STDOUT_TX_MEM_SIZE - sum;
}

/*!@brief Put a string to the head of the transmit queue.
 *        This function will request heap to buffer the string, and the transmit
 *        is handled by DMA & ISR.
 *
 * @param str   Pointer to string to transmit.
 * @param len   Length of the string.
 * @return      Number of bytes is put into the queue head.
 */
uint32_t STDOUT_PushToQueueHead(char *str, uint16_t len)
{
    // Check Queue & Memory usage. Wait here if they reach limit.
    while ((STDOUT_GetFreeQueue() == 0) && (STDOUT_GetFreeMem() < len))
    {
        ;
    }

    // Request memory and buffer string. These memory should be set free in
    // STDOUT_TransmitCpltCallBack
    uint8_t *pBuf = NULL;
    do
    {
        pBuf = (uint8_t *) STDIO_MALLOC((size_t )len);
    } while (pBuf == NULL);

    memcpy(pBuf, str, len);

    // Set new Queue Head
    STDOUT_MsgPtr[STDOUT_QueueHead] = pBuf;
    STDOUT_MsgLen[STDOUT_QueueHead] = len;
    STDOUT_QueueHead = (STDOUT_QueueHead == STDOUT_TX_QUEUE_SIZE - 1) ? 0 : STDOUT_QueueHead + 1;

    return len;
}

/*!@brief Trigger a UART transmit from Queue tail.
 *
 * @param huart     Handle of target UART.
 * @return          Number of bytes is put into DMA.
 */
uint32_t STDOUT_TransmitFromQueueTail(UART_HandleTypeDef *huart)
{
    uint8_t *pData = STDOUT_MsgPtr[STDOUT_QueueTail];
    uint32_t Len = STDOUT_MsgLen[STDOUT_QueueTail];

    // Try transmit data if there are any.
    if ((pData != NULL) && (Len > 0))
    {
        if (HAL_OK == HAL_UART_Transmit_DMA(huart, pData, Len))
        {
            return Len;
        }
    }

    return 0;
}

void STDOUT_TransmitCpltCallBack(UART_HandleTypeDef *huart)
{
    // Free Current Buffer.
    STDIO_FREE((void * )STDOUT_MsgPtr[STDOUT_QueueTail]);

    STDOUT_MsgPtr[STDOUT_QueueTail] = NULL;
    STDOUT_MsgLen[STDOUT_QueueTail] = 0;
    STDOUT_QueueTail = (STDOUT_QueueTail == STDOUT_TX_QUEUE_SIZE - 1) ? 0 : STDOUT_QueueTail + 1;

    // Trigger next transfer if needed
    STDOUT_TransmitFromQueueTail(huart);
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
        uint8_t c = STDIN_GetChar(&STDIN_Uart2Buf);

        if (c == 0xFF)
        {
            *ptr = 0xFF;

        }
        else
        {
            *ptr++ = c;
        }

        c = STDIN_GetChar(&STDIN_UsbBuf);
        if (c == 0xFF)
        {
            *ptr = 0xFF;

        }
        else
        {
            *ptr++ = c;
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

    // STDOUT
    if (file >= 1)
    {
        CDC_Transmit_FS((uint8_t*) ptr, len);
        STDOUT_PushToQueueHead(ptr, len);
        STDOUT_TransmitFromQueueTail(STDOUT_huart);

        return len;
    }

    // STDERR
    if (file == 0)
    {
        // Blocking transmit mode for STDERR ; Higher Priority than STDOUT.
        HAL_UART_AbortTransmit(STDERR_huart);
        while (HAL_OK != HAL_UART_Transmit(STDERR_huart, (uint8_t *) ptr, len, 1 + len))
        {
            ;
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
        STDOUT_TransmitCpltCallBack(huart);
    }
}

void HAL_UsbCdc_ReceiveCallBack(uint8_t* Buf, uint32_t *Len)
{
    for (int i = 0; i < *Len; i++)
    {
        STDIN_PutChar(&STDIN_UsbBuf, Buf[i]);
    }
}

void cli_sleep(int ms)
{
    osDelay(ms);
}

unsigned int cli_gettick(void)
{
    return HAL_GetTick();
}

void *cli_malloc(size_t size)
{
    if (size <= 0)
    {
        return NULL;
    }
    void *ptr = NULL;
    while (ptr == NULL)
    {
        ptr = pvPortMalloc(size);
        // ptr = malloc(size);
    }
    memset(ptr, 0, size);
    return ptr;
}

void cli_free(void *ptr)
{
    vPortFree(ptr);
    // free(ptr);
}

/*! External CLI command list
 */
extern int cli_reset(int argc, char **argv);
extern int cli_info(int argc, char **argv);
extern int cli_mem(int argc, char **argv);
extern int cli_qspi(int argc, char **argv);
extern int cli_os(int argc, char **argv);
extern int cli_top(int argc, char **argv);
extern int cli_rtc(int argc, char **argv);
extern int cli_nvram(int argc, char **argv);

int cli_port_init()
{
    // Set UART handle pointer
    STDIN_huart = &huart2; //!< STDIN UART handle
    STDOUT_huart = &huart2; //!< STDOUT UART handle
    STDERR_huart = &huart2; //!< STDERR UART handle

    // Clear buffer
    memset(STDOUT_MsgPtr, 0, STDOUT_TX_QUEUE_SIZE * sizeof(uint8_t *));
    memset(STDOUT_MsgLen, 0, STDOUT_TX_QUEUE_SIZE * sizeof(uint16_t));
    STDOUT_QueueHead = 0;
    STDOUT_QueueTail = 0;

    // Set STDIO type and buffer size
    setvbuf(stdout, (char *) NULL, _IOLBF, 256);
    setvbuf(stderr, (char *) NULL, _IONBF, 0);
    setvbuf(stdin, (char *) NULL, _IONBF, 0);

    // Trigger UART reception.
    HAL_UART_Receive_DMA(STDIN_huart, STDIN_Uart2Buf.aBuf, STDIN_RX_BUF_SIZE);

    // Register board command
    Cli_Register("info", "MCU Information", &cli_info);
    Cli_Register("reset", "MCU Reset", &cli_reset);
    Cli_Register("mem", "Memory write/read", &cli_mem);
    Cli_Register("nvram", "Non-Volatile RAM operation", &cli_nvram);
    Cli_Register("qspi", "Quad-SPI flash operation", &cli_qspi);
    Cli_Register("os", "RTOS operation", &cli_os);
    Cli_Register("top", "RTOS operation", &cli_top);
    Cli_Register("rtc", "Real Time Clock operation", &cli_rtc);

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
