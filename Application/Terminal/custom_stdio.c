/*
 * custom_stdio.c
 *
 *  Created on: Sep 28, 2018
 *      Author: nickyang
 */
#include "stm32l4xx_hal.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#define STDIN_RX_MEM_SIZE           1024
#define STDOUT_TX_LINE_SIZE         256
#define STDOUT_TX_QUEUE_SIZE        256
#define STDOUT_TX_MEM_SIZE          4096

extern UART_HandleTypeDef huart2;

UART_HandleTypeDef *STDIN_huart = &huart2;      //!< STDIN UART handle
UART_HandleTypeDef *STDOUT_huart = &huart2;     //!< STDOUT UART handle

uint8_t STDIN_RxBuf[STDIN_RX_MEM_SIZE] =
{ 0 };

uint8_t *STDOUT_TxQueuePtr[STDOUT_TX_QUEUE_SIZE] =
{ 0 };
uint16_t STDOUT_TxQueueLen[STDOUT_TX_QUEUE_SIZE] =
{ 0 };
uint16_t STDOUT_TxQueueHead = 0;
uint16_t STDOUT_TxQueueTail = 0;
uint16_t STDOUT_TxMemUsage = 0;

uint32_t STDOUT_GetQueueUsage(void)
{
    return (STDOUT_TxQueueHead + STDOUT_TX_QUEUE_SIZE - STDOUT_TxQueueTail) % STDOUT_TX_QUEUE_SIZE;
}

uint32_t STDOUT_GetMemUsage(void)
{
    return STDOUT_TxMemUsage;
}

uint32_t STDOUT_PushToQueueHead(uint8_t *str, uint16_t len)
{
    // Check Queue & Memory usage. Wait here if they reach limit.
    while (STDOUT_GetQueueUsage() >= STDOUT_TX_QUEUE_SIZE - 1)
    {
        //HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
        HAL_Delay(10);
    }

    while (STDOUT_GetMemUsage() >= STDOUT_TX_MEM_SIZE)
    {
        HAL_Delay(10);
    }

    // Request memory and buffer string. These memory should be set free in Print_TransmitCpltCallBack
    uint8_t *pBuf = NULL;
    do
    {
        pBuf = (uint8_t*) malloc((size_t) len);
    } while (pBuf == NULL);

    memcpy(pBuf, str, len);

    // Check if Queue is full
    // Set new Queue Head
    STDOUT_TxQueuePtr[STDOUT_TxQueueHead] = pBuf;
    STDOUT_TxQueueLen[STDOUT_TxQueueHead] = len;
    STDOUT_TxQueueHead = (STDOUT_TxQueueHead == STDOUT_TX_QUEUE_SIZE - 1) ? 0 : STDOUT_TxQueueHead + 1;

    STDOUT_TxMemUsage += len;
}

uint32_t STDOUT_TransmitFromQueueTail(UART_HandleTypeDef *huart)
{
    HAL_StatusTypeDef status;
    uint8_t *pData = STDOUT_TxQueuePtr[STDOUT_TxQueueTail];
    uint32_t Len = STDOUT_TxQueueLen[STDOUT_TxQueueTail];

    //Try transmit data if there are any.
    if ((pData != NULL) && (Len > 0))
    {
        status = HAL_UART_Transmit_DMA(huart, pData, Len);
        if (status == HAL_OK)
        {
            return Len;
        }
    }

    return 0;
}

void STDOUT_TransmitCpltCallBack(UART_HandleTypeDef *huart)
{
    //Free Current Buffer.
    free((void*) STDOUT_TxQueuePtr[STDOUT_TxQueueTail]);
    STDOUT_TxMemUsage = STDOUT_TxMemUsage - STDOUT_TxQueueLen[STDOUT_TxQueueTail];
    STDOUT_TxQueuePtr[STDOUT_TxQueueTail] = NULL;
    STDOUT_TxQueueLen[STDOUT_TxQueueTail] = 0;
    STDOUT_TxQueueTail = (STDOUT_TxQueueTail == STDOUT_TX_QUEUE_SIZE - 1) ? 0 : STDOUT_TxQueueTail + 1;

    //Trigger next transfer if needed
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
    //Initialize stage
    static uint8_t InitFlag = 0;
    if (InitFlag == 0)
    {
        InitFlag = 1;
        HAL_UART_Receive_DMA(STDIN_huart, STDIN_RxBuf, sizeof(STDIN_RxBuf));
    }

    //Loop get character
    for (int i = 0; i < len; i++)
    {
        static int idx = 0;
        char c = STDIN_RxBuf[idx];

        if (c == 0)
        {
            *ptr = EOF;
        }
        else
        {
            *ptr++ = c;

            //Clear current buffer and move forward.
            STDIN_RxBuf[idx] = 0;
            idx = (idx >= sizeof(STDIN_RxBuf) - 1) ? 0 : idx + 1;
        }
    }

    return len;
}

/*!@brief   Override system call of _write, route STDOUT to UART TX.
 *          Transfer bytes through UART.
 *          STDOUT will be transfered in non-blocking mode.
 *          STDERR will be abort current STDOUT (if there are) and transfered in blocking mode.
 *
 * @param file  STDOUT_FILENO or STDERR_FILENO
 * @param ptr   Pointer to bytes
 * @param len   Length of bytes
 * @return      Length of bytes actually transfered
 */
int _write(int file, char *ptr, int len)
{
    //Initialize
    static uint8_t InitFlag = 0;
    if (InitFlag == 0)
    {
        /*! Set STDOUT/STDIN/STDERR Buffer type and size
         *(char*), It could be a pointer to buffer or NULL. When set to NULL it will automatic assign the buffer.
         *[_IOLBF], Line buffer mode, transmit data when get a '\n'
         *[_IONBF], No buffer mode, transmit data byte 1by1
         *[_IOFBF], Full buffer mode, transmit data when buffer is full, or manually
         */
        InitFlag = 1;
        setvbuf(stdout, (char*) NULL, _IOLBF, STDOUT_TX_LINE_SIZE);
        setvbuf(stderr, (char*) NULL, _IONBF, 0);
        setvbuf(stdin, (char*) NULL, _IONBF, 0);
    }

    //STDOUT
    if (file == 1)
    {
        if (len == 1)
        {
            // Blocking transmit mode for single byte
            while (HAL_OK != HAL_UART_Transmit(STDOUT_huart, (uint8_t*) ptr, len, 1 + len))
            {
                ;
            }
        }
        else
        {
            STDOUT_PushToQueueHead(ptr, len);
            STDOUT_TransmitFromQueueTail(STDOUT_huart);
        }
        return len;
    }

    //STDERR
    if (file == 2)
    {
        // Blocking transmit mode for STDERR ; Higher Priority than STDOUT.
        HAL_UART_AbortTransmit(STDOUT_huart);
        while (HAL_OK != HAL_UART_Transmit(STDOUT_huart, (uint8_t*) ptr, len, 1 + len))
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
