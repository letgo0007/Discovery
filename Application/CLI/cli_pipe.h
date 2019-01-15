/******************************************************************************
 * @file    cli_pipe.h
 * @brief   I/O pipe for CLI.
 *          Provide function implement of ring buffer & message queue.
 *
 * @author  Nick Yang
 * @date    2019/01/02
 * @version V0.1
 *****************************************************************************/

#ifndef CLI_PIPE_H_
#define CLI_PIPE_H_

#define RB_RET_OK 0
#define RB_RET_ERR_CORRUPT -1
#define RB_RET_ERR_PARAM -2
#define RB_RET_ERR_MEM -3
#define RB_RET_ERR_LOCK -4

#define MQ_RET_OK 0
#define MQ_RET_ERR_CORRUPT -1
#define MQ_RET_ERR_PARAM -2
#define MQ_RET_ERR_MEM -3
#define MQ_RET_ERR_LOCK -4

typedef enum PIPE_LOCK {
    UNLOCK = 0,
    LOCK   = 1,
} PIPE_LOCK;

typedef struct RingBuf_TypeDef {
    char *    pBuf;
    int       Size;
    int       PutIndex;
    int       GetIndex;
    PIPE_LOCK Lock;
} RingBuf_TypeDef;

typedef struct MsgQueue_TypeDef {
    char **   MsgPtrQueue;
    int *     MsgLenQueue;
    int       QueueSize;
    int       MemorySize;
    int       Head;
    int       Tail;
    PIPE_LOCK Lock;
} MsgQueue_TypeDef;

int RingBuf_Init(RingBuf_TypeDef *pBuf, int size);
int RingBuf_DeInit(RingBuf_TypeDef *pBuf);
int RingBuf_PutChar(RingBuf_TypeDef *pBuf, char c);
int RingBuf_GetChar(RingBuf_TypeDef *pBuf);

int MsgQueue_Init(MsgQueue_TypeDef *pQueue, int QueueSize, int MemSize);
int MsgQueue_DeInit(MsgQueue_TypeDef *pQueue);
int MsgQueue_GetFreeQueue(MsgQueue_TypeDef *pQueue);
int MsgQueue_GetFreeMem(MsgQueue_TypeDef *pQueue);
int MsgQueue_PushToHead(MsgQueue_TypeDef *pQueue, char *pMsg, int Len);
int MsgQueue_PullFromTail(MsgQueue_TypeDef *pQueue, char **ppMsg, int *pLen);
int MsgQueue_FreeFromTail(MsgQueue_TypeDef *pQueue);

#endif /* CLI_PIPE_H_ */
