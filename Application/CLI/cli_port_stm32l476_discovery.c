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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cli.h"
#include "cmsis_os.h"
#include "stdio.h"
#include "stdlib.h"
#include "stm32l4xx_hal.h"
#include "string.h"

#define STDIN_RX_MEM_SIZE 256   //!< STDIN input buffer size
#define STDOUT_TX_LINE_SIZE 256  //!< STDOUT Single line maximum length.
#define STDOUT_TX_QUEUE_SIZE 256 //!< STDOUT Number of lines in queue.
#define STDOUT_TX_MEM_SIZE 4096  //!< STDOUT total memory buffer usage.

#define STDIO_MALLOC(x) malloc(x)
#define STDIO_FREE(x) free(x)

extern UART_HandleTypeDef  huart1;
extern UART_HandleTypeDef  huart2;
extern UART_HandleTypeDef  huart3;
static UART_HandleTypeDef *STDIN_huart                             = NULL; //!< STDIN UART handle
static UART_HandleTypeDef *STDOUT_huart                            = NULL; //!< STDOUT UART handle
static UART_HandleTypeDef *STDERR_huart                            = NULL; //!< STDERR UART handle
static uint8_t             STDIN_RxBuf[STDIN_RX_MEM_SIZE]          = {0};
static uint8_t *           STDOUT_TxQueuePtr[STDOUT_TX_QUEUE_SIZE] = {0};
static uint16_t            STDOUT_TxQueueLen[STDOUT_TX_QUEUE_SIZE] = {0};
static uint16_t            STDOUT_TxQueueHead                      = 0;
static uint16_t            STDOUT_TxQueueTail                      = 0;

/*! External CLI command
 */
extern int cli_reset(int argc, char **argv);
extern int cli_info(int argc, char **argv);
extern int cli_mem(int argc, char **argv);
extern int cli_qspi(int argc, char **argv);
extern int cli_os(int argc, char **argv);
extern int cli_top(int argc, char **argv);

/*!@brief Get STDOUT transmit queue usage.
 *
 * @return Number of transmit already in queue.
 */
uint32_t STDOUT_GetQueueUsage(void)
{
    return (STDOUT_TxQueueHead + STDOUT_TX_QUEUE_SIZE - STDOUT_TxQueueTail) % STDOUT_TX_QUEUE_SIZE;
}

/*!@brief Get STDOUT transmit queue memory usage.
 *
 * @return Number of bytes already in STDOUT output heap.
 */
uint32_t STDOUT_GetMemUsage(void)
{
    uint16_t i   = 0;
    uint32_t sum = 0;
    for (i = 0; i < STDOUT_TX_QUEUE_SIZE; i++)
    {
        sum += STDOUT_TxQueueLen[i];
    }
    return sum;
}

/*!@brief Put a string to the head of the transmit queue.
 *        This function will request heap to buffer the string, and the transmit
 * is handled by DMA & ISR.
 *
 * @param str   Pointer to string to transmit.
 * @param len   Length of the string.
 * @return      Number of bytes is put into the queue head.
 */
uint32_t STDOUT_PushToQueueHead(char *str, uint16_t len)
{
    // Check Queue & Memory usage. Wait here if they reach limit.
    while ((STDOUT_GetQueueUsage() >= STDOUT_TX_QUEUE_SIZE - 1))
    {
        ;
    }
    while (STDOUT_GetMemUsage() >= STDOUT_TX_MEM_SIZE)
    {
        ;
    }

    // Request memory and buffer string. These memory should be set free in
    // Print_TransmitCpltCallBack
    uint8_t *pBuf = NULL;
    do
    {
        pBuf = (uint8_t *)STDIO_MALLOC((size_t)len);
    } while (pBuf == NULL);

    memcpy(pBuf, str, len);

    // Set new Queue Head
    STDOUT_TxQueuePtr[STDOUT_TxQueueHead] = pBuf;
    STDOUT_TxQueueLen[STDOUT_TxQueueHead] = len;
    STDOUT_TxQueueHead =
        (STDOUT_TxQueueHead == STDOUT_TX_QUEUE_SIZE - 1) ? 0 : STDOUT_TxQueueHead + 1;

    return len;
}

/*!@brief Trigger a UART transmit from Queue tail.
 *
 * @param huart     Handle of target UART.
 * @return          Number of bytes is put into DMA.
 */
uint32_t STDOUT_TransmitFromQueueTail(UART_HandleTypeDef *huart)
{
    uint8_t *pData = STDOUT_TxQueuePtr[STDOUT_TxQueueTail];
    uint32_t Len   = STDOUT_TxQueueLen[STDOUT_TxQueueTail];

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
    STDIO_FREE((void *)STDOUT_TxQueuePtr[STDOUT_TxQueueTail]);

    STDOUT_TxQueuePtr[STDOUT_TxQueueTail] = NULL;
    STDOUT_TxQueueLen[STDOUT_TxQueueTail] = 0;
    STDOUT_TxQueueTail =
        (STDOUT_TxQueueTail == STDOUT_TX_QUEUE_SIZE - 1) ? 0 : STDOUT_TxQueueTail + 1;

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
        static int idx = 0;
        char       c   = STDIN_RxBuf[idx];

        if (c == 0)
        {
            *ptr = EOF;
        }
        else
        {
            *ptr++ = c;

            // Clear current buffer and move forward.
            STDIN_RxBuf[idx] = 0;
            idx              = (idx >= sizeof(STDIN_RxBuf) - 1) ? 0 : idx + 1;
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
    if (file == 1)
    {

        STDOUT_PushToQueueHead(ptr, len);
        STDOUT_TransmitFromQueueTail(STDOUT_huart);

        //CDC_Transmit_FS(ptr,len);

        return len;
    }

    // STDERR
    if (file == 2)
    {
        // Blocking transmit mode for STDERR ; Higher Priority than STDOUT.
        HAL_UART_AbortTransmit(STDERR_huart);
        while (HAL_OK != HAL_UART_Transmit(STDERR_huart, (uint8_t *)ptr, len, 1 + len))
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
        HAL_UART_Receive_DMA(STDIN_huart, STDIN_RxBuf, sizeof(STDIN_RxBuf));
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == STDOUT_huart->Instance)
    {
        STDOUT_TransmitCpltCallBack(huart);
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

int cli_port_init()
{
    // Set UART handle pointer
    STDIN_huart  = &huart2; //!< STDIN UART handle
    STDOUT_huart = &huart2; //!< STDOUT UART handle
    STDERR_huart = &huart2; //!< STDERR UART handle

    // Clear buffer
    memset(STDIN_RxBuf, 0, STDIN_RX_MEM_SIZE * sizeof(uint8_t));
    memset(STDOUT_TxQueuePtr, 0, STDOUT_TX_QUEUE_SIZE * sizeof(uint8_t *));
    memset(STDOUT_TxQueueLen, 0, STDOUT_TX_QUEUE_SIZE * sizeof(uint16_t));
    STDOUT_TxQueueHead = 0;
    STDOUT_TxQueueTail = 0;

    // Set STDIO type and buffer size
    setvbuf(stdout, (char *)NULL, _IOLBF, 256);
    setvbuf(stderr, (char *)NULL, _IONBF, 0);
    setvbuf(stdin, (char *)NULL, _IONBF, 0);

    // Trigger UART receivign.
    HAL_UART_Receive_DMA(STDIN_huart, STDIN_RxBuf, sizeof(STDIN_RxBuf));

    // Register board command
    Cli_Register("info", "MCU Information", &cli_info);
    Cli_Register("reset", "MCU Reset", &cli_reset);
    Cli_Register("mem", "Memory write/read", &cli_mem);
    Cli_Register("qspi", "Quad-SPI flash operation", &cli_qspi);
    Cli_Register("os", "RTOS operation", &cli_os);
    Cli_Register("top", "RTOS operation", &cli_top);
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
