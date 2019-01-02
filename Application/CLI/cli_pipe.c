/******************************************************************************
 * @file    cli_pipe.c
 * @brief   I/O pipe for CLI.
 *          Provide function implement of ring buffer & message queue.
 *
 * @author  Nick Yang
 * @date    2019/01/02
 * @version V0.1
 *****************************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "cli_pipe.h"

/*! Functions ---------------------------------------------------------------*/

extern void* cli_calloc(unsigned int);
extern void cli_free(void*);
extern unsigned int cli_gettick(void);

int RingBuf_Init(RingBuf_TypeDef *pBuf, int size)
{
    if ((pBuf == NULL) || (size == 0))
    {
        return -1;
    }

    pBuf->pBuf = cli_calloc(size);
    //memset(pBuf->pBuf, 0, size);
    pBuf->Size = size;
    pBuf->PutIndex = 0;
    pBuf->GetIndex = 0;

    return 0;
}

int RingBuf_DeInit(RingBuf_TypeDef *pBuf)
{
    if ((pBuf == NULL) || (pBuf->pBuf == NULL) || (pBuf->Size == 0))
    {
        return -1;
    }

    cli_free(pBuf->pBuf);
    pBuf->Size = 0;
    pBuf->PutIndex = 0;
    pBuf->GetIndex = 0;

    return 0;
}

int RingBuf_PutChar(RingBuf_TypeDef *pBuf, char c)
{
    if ((pBuf == NULL) || (pBuf->pBuf == NULL) || (pBuf->Size == 0))
    {
        return -1;
    }
    pBuf->pBuf[pBuf->PutIndex] = c;
    pBuf->PutIndex = (pBuf->PutIndex >= pBuf->Size - 1) ? 0 : pBuf->PutIndex + 1;

    return 0;
}

int RingBuf_GetChar(RingBuf_TypeDef *pBuf)
{
    if ((pBuf == NULL) || (pBuf->pBuf == NULL) || (pBuf->Size == 0))
    {
        return -1;
    }

    char c = pBuf->pBuf[pBuf->GetIndex];

    if (c == 0)
    {
        return -1;
    }
    else
    {
        pBuf->pBuf[pBuf->GetIndex] = 0;
        pBuf->GetIndex = (pBuf->GetIndex >= pBuf->Size - 1) ? 0 : pBuf->GetIndex + 1;
        return c;
    }
}

int MsgQueue_Init(MsgQueue_TypeDef *pQueue, int QueueSize, int MemSize)
{
    if ((pQueue == NULL) || (QueueSize == 0) || (MemSize == 0))
    {
        return -1;
    }

    pQueue->MsgPtrQueue = cli_calloc(sizeof(char*) * QueueSize);
    //memset(pQueue->MsgPtrQueue, 0, sizeof(char*) * QueueSize);
    pQueue->MsgLenQueue = cli_calloc(sizeof(int) * QueueSize);
    //memset(pQueue->MsgLenQueue, 0, sizeof(int) * QueueSize);

    pQueue->QueueSize = QueueSize;
    pQueue->MemorySize = MemSize;
    pQueue->Head = 0;
    pQueue->Tail = 0;

    return 0;
}

int MsgQueue_DeInit(MsgQueue_TypeDef *pQueue)
{
    cli_free(pQueue->MsgPtrQueue);
    cli_free(pQueue->MsgLenQueue);
    pQueue->QueueSize = 0;
    pQueue->Head = 0;
    pQueue->Tail = 0;

    return 0;
}

int MsgQueue_GetFreeQueue(MsgQueue_TypeDef *pQueue)
{
    int free = 0;
    for (int i = 0; i < pQueue->QueueSize; i++)
    {
        if (pQueue->MsgPtrQueue[i] == NULL)
        {
            free++;
        }
    }
    return free;
}

int MsgQueue_GetMemUsage(MsgQueue_TypeDef *pQueue)
{
    int sum = 0;
    for (int i = 0; i < pQueue->QueueSize; i++)
    {
        sum += pQueue->MsgLenQueue[i];
    }
    return sum;
}

/*!@brief Push a message to queue head
 *
 * @param pQueue    Pointer to the MsgQueue struct
 * @param pMsg      Pointer to the message
 * @param Len       Length of the message
 * @return
 */
int MsgQueue_PushToHead(MsgQueue_TypeDef *pQueue, char *pMsg, int Len)
{
    if ((pQueue == NULL) || (pQueue->MsgLenQueue == NULL) || (pQueue->QueueSize == 0))
    {
        return -1;
    }

    int timeout = 0;
    while ((MsgQueue_GetFreeQueue(pQueue) == 0)
            || (MsgQueue_GetMemUsage(pQueue) >= pQueue->MemorySize - Len))
    {
        if (timeout++ > 100)
        {
            return -1;
        }
    }

    // Request memory and buffer string
    char *pBuf = NULL;
    do
    {
        pBuf = (char *) cli_calloc((size_t) Len);
    } while (pBuf == NULL);

    memcpy(pBuf, pMsg, Len);

    // Set new Queue Head
    pQueue->MsgPtrQueue[pQueue->Head] = pBuf;
    pQueue->MsgLenQueue[pQueue->Head] = Len;
    pQueue->Head = (pQueue->Head == pQueue->QueueSize - 1) ? 0 : pQueue->Head + 1;

    return 0;
}

/*!@brief Pull a message from queue tail
 * @note  This function does NOT automatically delete the message from queue tail when called.
 *        Use MsgQueue_FreeTail() to delete the message from queue tail and free memory.
 *        Otherwise this will always returns the same tail message.
 *
 * @param pQueue
 * @param ppMsg
 * @param pLen
 * @return
 */
int MsgQueue_PullFromTail(MsgQueue_TypeDef *pQueue, char **ppMsg, int *pLen)
{
    if ((pQueue == NULL) || (pQueue->MsgLenQueue == NULL) || (pQueue->QueueSize == 0))
    {
        return -1;
    }

    *ppMsg = pQueue->MsgPtrQueue[pQueue->Tail];
    *pLen = pQueue->MsgLenQueue[pQueue->Tail];

    return 0;
}

/*!@brief Free a message from queue tail
 *        This will delete the tail message from queue and free the buffer.
 *
 * @param pQueue
 * @return
 */
int MsgQueue_FreeFromTail(MsgQueue_TypeDef *pQueue)
{
    if ((pQueue == NULL) || (pQueue->MsgLenQueue == NULL) || (pQueue->QueueSize == 0))
    {
        return -1;
    }

    cli_free(pQueue->MsgPtrQueue[pQueue->Tail]);
    pQueue->MsgPtrQueue[pQueue->Tail] = NULL;
    pQueue->MsgLenQueue[pQueue->Tail] = 0;
    pQueue->Tail = (pQueue->Tail == pQueue->QueueSize - 1) ? 0 : pQueue->Tail + 1;

    return 0;
}
