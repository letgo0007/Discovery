/**
 ******************************************************************************
 * File Name          : USART.c
 * Description        : This file provides code for the configuration
 *                      of the USART instances.
 ******************************************************************************
 * This notice applies to any and all portions of this file
 * that are not between comment pairs USER CODE BEGIN and
 * USER CODE END. Other portions of this file, whether
 * inserted by the user or by software development tools
 * are owned by their respective copyright owners.
 *
 * Copyright (c) 2018 STMicroelectronics International N.V.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted, provided that the following conditions are met:
 *
 * 1. Redistribution of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of STMicroelectronics nor the names of other
 *    contributors to this software may be used to endorse or promote products
 *    derived from this software without specific written permission.
 * 4. This software, including modifications and/or derivative works of this
 *    software, must execute solely and exclusively on microcontroller or
 *    microprocessor devices manufactured by or for STMicroelectronics.
 * 5. Redistribution and use of this software other than as permitted under
 *    this license is void and will automatically terminate your rights under
 *    this license.
 *
 * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
 * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT
 * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "usart.h"

#include "gpio.h"
#include "dma.h"

/* USER CODE BEGIN 0 */
#include "stdio.h"
/* USER CODE END 0 */

UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_rx;
DMA_HandleTypeDef hdma_usart2_tx;

/* USART2 init function */

void MX_USART2_UART_Init(void)
{

    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init(&huart2) != HAL_OK)
    {
        _Error_Handler(__FILE__, __LINE__);
    }

    HAL_UART_Receive_DMA(&huart2, uart2_rxbuf, sizeof(uart2_rxbuf));

}

void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{

    GPIO_InitTypeDef GPIO_InitStruct;
    if (uartHandle->Instance == USART2)
    {
        /* USER CODE BEGIN USART2_MspInit 0 */

        /* USER CODE END USART2_MspInit 0 */
        /* USART2 clock enable */
        __HAL_RCC_USART2_CLK_ENABLE()
        ;

        /**USART2 GPIO Configuration
         PD5     ------> USART2_TX
         PD6     ------> USART2_RX
         */
        GPIO_InitStruct.Pin = USART_TX_Pin | USART_RX_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
        HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

        /* USART2 DMA Init */
        /* USART2_RX Init */
        hdma_usart2_rx.Instance = DMA1_Channel6;
        hdma_usart2_rx.Init.Request = DMA_REQUEST_2;
        hdma_usart2_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
        hdma_usart2_rx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_usart2_rx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_usart2_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_usart2_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_usart2_rx.Init.Mode = DMA_CIRCULAR;
        hdma_usart2_rx.Init.Priority = DMA_PRIORITY_HIGH;
        if (HAL_DMA_Init(&hdma_usart2_rx) != HAL_OK)
        {
            _Error_Handler(__FILE__, __LINE__);
        }

        __HAL_LINKDMA(uartHandle, hdmarx, hdma_usart2_rx);

        /* USART2_TX Init */
        hdma_usart2_tx.Instance = DMA1_Channel7;
        hdma_usart2_tx.Init.Request = DMA_REQUEST_2;
        hdma_usart2_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
        hdma_usart2_tx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_usart2_tx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_usart2_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_usart2_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_usart2_tx.Init.Mode = DMA_NORMAL;
        hdma_usart2_tx.Init.Priority = DMA_PRIORITY_LOW;
        if (HAL_DMA_Init(&hdma_usart2_tx) != HAL_OK)
        {
            _Error_Handler(__FILE__, __LINE__);
        }

        __HAL_LINKDMA(uartHandle, hdmatx, hdma_usart2_tx);

        /* USART2 interrupt Init */
        HAL_NVIC_SetPriority(USART2_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(USART2_IRQn);
        /* USER CODE BEGIN USART2_MspInit 1 */

        /* USER CODE END USART2_MspInit 1 */
    }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{

    if (uartHandle->Instance == USART2)
    {
        /* USER CODE BEGIN USART2_MspDeInit 0 */

        /* USER CODE END USART2_MspDeInit 0 */
        /* Peripheral clock disable */
        __HAL_RCC_USART2_CLK_DISABLE();

        /**USART2 GPIO Configuration
         PD5     ------> USART2_TX
         PD6     ------> USART2_RX
         */
        HAL_GPIO_DeInit(GPIOD, USART_TX_Pin | USART_RX_Pin);

        /* USART2 DMA DeInit */
        HAL_DMA_DeInit(uartHandle->hdmarx);
        HAL_DMA_DeInit(uartHandle->hdmatx);

        /* USART2 interrupt Deinit */
        HAL_NVIC_DisableIRQ(USART2_IRQn);
        /* USER CODE BEGIN USART2_MspDeInit 1 */

        /* USER CODE END USART2_MspDeInit 1 */
    }
}

/* USER CODE BEGIN 1 */

uint8_t uart1_txbuf[UART1_TXBUF_SIZE];
uint8_t uart1_rxbuf[UART1_RXBUF_SIZE];
uint8_t uart2_txbuf[UART2_TXBUF_SIZE];
uint8_t uart2_rxbuf[UART2_RXBUF_SIZE];
uint8_t uart3_txbuf[UART3_TXBUF_SIZE];
uint8_t uart3_rxbuf[UART3_RXBUF_SIZE];

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
    int i;

    for (i = 0; i < len; i++)
    {
        static int idx = 0;
        char c = TERM_UART_RXBUF[idx];

        if (c == 0)
        {
            *ptr = '\xff';
        }
        else
        {
            *ptr++ = c;

            //Clear current buffer and move forward.
            TERM_UART_RXBUF[idx] = 0;
            idx++;
            idx = (idx >= sizeof(TERM_UART_RXBUF)) ? 0 : idx;
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
    if (file == 1) //STDOUT
    {
        if (len == 1)
        {
            // Blocking transmit mode for single byte
            while (HAL_OK != HAL_UART_Transmit(&TERM_UART_HANDLE, (uint8_t*) ptr, len, 1 + len))
            {
                ;
            }
            return len;
        }
        else
        {
            // Check if last transfer completed.
            while (HAL_DMA_STATE_BUSY == HAL_DMA_GetState(TERM_UART_HANDLE.hdmatx))
            {
                ;
            }
            // No-Blocking transmit mode for multiple bytes
            while (HAL_OK != HAL_UART_Transmit_DMA(&TERM_UART_HANDLE, (uint8_t*) ptr, len))
            {
                ;
            }
            return len;
        }
    }
    if (file == 2) //STDERR
    {
        // Blocking transmit mode for STDERR ; Higher Priority than STDOUT.
        HAL_UART_AbortTransmit(&TERM_UART_HANDLE);
        while (HAL_OK != HAL_UART_Transmit(&TERM_UART_HANDLE, (uint8_t*) ptr, len, 1 + len))
        {
            ;
        }
        return len;
    }
    return 0;
}
/* USER CODE END 1 */

/* USER CODE END 1 */

/**
 * @}
 */

/**
 * @}
 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
