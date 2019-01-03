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
#include "cli_port.h"

/*! Functions ---------------------------------------------------------------*/
/*!@brief
 *
 * @param pBuf
 * @param size
 * @return
 */
int RingBuf_Init(RingBuf_TypeDef *pBuf, int size)
{
    if ((pBuf == NULL) || (size == 0))
    {
        return RB_RET_ERR_PARAM;
    }

    if (pBuf->Lock == LOCK)
    {
        return RB_RET_ERR_LOCK;
    }

    pBuf->Lock = LOCK;
    pBuf->pBuf = cli_calloc(size);
    pBuf->Size = size;
    pBuf->PutIndex = 0;
    pBuf->GetIndex = 0;
    pBuf->Lock = UNLOCK;

    return RB_RET_OK;
}

/*!@brief
 *
 * @param   pBuf
 * @return
 */
int RingBuf_DeInit(RingBuf_TypeDef *pBuf)
{
    if ((pBuf == NULL) || (pBuf->pBuf == NULL) || (pBuf->Size == 0))
    {
        return RB_RET_ERR_PARAM;
    }

    pBuf->Lock = LOCK;
    cli_free(pBuf->pBuf);
    pBuf->Size = 0;
    pBuf->PutIndex = 0;
    pBuf->GetIndex = 0;
    pBuf->Lock = UNLOCK;

    return RB_RET_OK;
}

int RingBuf_PutChar(RingBuf_TypeDef *pBuf, char c)
{
    if ((pBuf == NULL) || (pBuf->pBuf == NULL) || (pBuf->Size == 0))
    {
        return RB_RET_ERR_PARAM;
    }

    pBuf->Lock = LOCK;
    pBuf->pBuf[pBuf->PutIndex] = c;
    pBuf->PutIndex = (pBuf->PutIndex >= pBuf->Size - 1) ? 0 : pBuf->PutIndex + 1;
    pBuf->Lock = UNLOCK;

    return RB_RET_OK;
}

int RingBuf_GetChar(RingBuf_TypeDef *pBuf)
{
    if ((pBuf == NULL) || (pBuf->pBuf == NULL) || (pBuf->Size == 0))
    {
        return RB_RET_ERR_PARAM;
    }

    char c = pBuf->pBuf[pBuf->GetIndex];

    if (c == 0)
    {
        return EOF;
    }
    else
    {
        pBuf->Lock = LOCK;
        pBuf->pBuf[pBuf->GetIndex] = 0;
        pBuf->GetIndex = (pBuf->GetIndex >= pBuf->Size - 1) ? 0 : pBuf->GetIndex + 1;
        pBuf->Lock = UNLOCK;
        return c;
    }
}

/*!@brief Initialize a Message Queue structure
 *
 * @param pQueue
 * @param QueueSize
 * @param MemSize
 * @return
 */
int MsgQueue_Init(MsgQueue_TypeDef *pQueue, int QueueSize, int MemSize)
{
    if ((pQueue == NULL) || (QueueSize == 0) || (MemSize == 0))
    {
        return MQ_RET_ERR_PARAM;
    }

    if (pQueue->Lock == LOCK)
    {
        return MQ_RET_ERR_LOCK;
    }

    pQueue->Lock = LOCK;
    pQueue->MsgPtrQueue = cli_calloc(sizeof(char*) * QueueSize);
    pQueue->MsgLenQueue = cli_calloc(sizeof(int) * QueueSize);

    pQueue->QueueSize = QueueSize;
    pQueue->MemorySize = MemSize;
    pQueue->Head = 0;
    pQueue->Tail = 0;

    pQueue->Lock = UNLOCK;
    return MQ_RET_OK;
}

/*!@brief
 *
 * @param pQueue
 * @return
 */
int MsgQueue_DeInit(MsgQueue_TypeDef *pQueue)
{

    if (pQueue->Lock == LOCK)
    {
        return MQ_RET_ERR_LOCK;
    }
    pQueue->Lock = LOCK;
    cli_free(pQueue->MsgPtrQueue);
    cli_free(pQueue->MsgLenQueue);
    pQueue->QueueSize = 0;
    pQueue->Head = 0;
    pQueue->Tail = 0;

    pQueue->Lock = UNLOCK;
    return MQ_RET_OK;
}

/*!@brief   Get available number of messages in a queue
 *
 * @param   pQueue
 * @return  Number of messages still could
 */
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

/*!@brief   Get available number of memory in a queue.
 *
 * @param   pQueue
 * @return  Number of bytes of memory available.
 */
int MsgQueue_GetFreeMemory(MsgQueue_TypeDef *pQueue)
{
    int sum = 0;
    for (int i = 0; i < pQueue->QueueSize; i++)
    {
        sum += pQueue->MsgLenQueue[i];
    }
    return pQueue->MemorySize - sum;
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
        return MQ_RET_ERR_CORRUPT;
    }
    if ((pMsg == NULL) || (Len == 0))
    {
        return MQ_RET_ERR_PARAM;
    }

    int start_time = cli_gettick();
    while ((MsgQueue_GetFreeQueue(pQueue) == 0) || (MsgQueue_GetFreeMemory(pQueue) <= Len))
    {
        if (cli_gettick() > start_time + 100)
        {
            return MQ_RET_ERR_MEM;
        }
    }

    pQueue->Lock = LOCK;
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

    pQueue->Lock = UNLOCK;
    return MQ_RET_OK;
}

/*!@brief Pull a message from queue tail
 * @note  This function does NOT automatically delete the message from queue tail when called.
 *        Use MsgQueue_FreeTail() to delete the message from queue tail and free memory.
 *        Otherwise this will always returns the same tail message.
 *
 * @param pQueue    Pointer to the MsgQueue struct
 * @param ppMsg     Pointer to buffer to save message
 * @param pLen      Pointer to Length of the message
 * @return
 */
int MsgQueue_PullFromTail(MsgQueue_TypeDef *pQueue, char **ppMsg, int *pLen)
{
    if ((pQueue == NULL) || (pQueue->MsgLenQueue == NULL) || (pQueue->QueueSize == 0))
    {
        return MQ_RET_ERR_PARAM;
    }

    *ppMsg = pQueue->MsgPtrQueue[pQueue->Tail];
    *pLen = pQueue->MsgLenQueue[pQueue->Tail];

    return MQ_RET_OK;
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
        return MQ_RET_ERR_PARAM;
    }

    cli_free(pQueue->MsgPtrQueue[pQueue->Tail]);
    pQueue->MsgPtrQueue[pQueue->Tail] = NULL;
    pQueue->MsgLenQueue[pQueue->Tail] = 0;
    pQueue->Tail = (pQueue->Tail == pQueue->QueueSize - 1) ? 0 : pQueue->Tail + 1;

    return MQ_RET_OK;
}
