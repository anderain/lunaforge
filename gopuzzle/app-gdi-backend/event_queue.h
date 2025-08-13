#ifndef _EVENT_QUEUE_
#define _EVENT_QUEUE_

#include "../gongshu.h"

#define QUEUE_MAX_SIZE 50

/* 队列结构体 */
typedef struct {
    Gongshu_Event data[QUEUE_MAX_SIZE];	/* 事件存储数组 */
    int front;                        	/* 队列头索引 */
    int rear;                        	/* 队列尾索引 */
    int length;                     	/* 当前队列长度 */
} EventQueue;

void 	EventQueue_Init			(EventQueue* queue);
int  	EventQueue_GetLength	(EventQueue* queue);
void 	EventQueue_ClearAll		(EventQueue* queue);
void 	EventQueue_Enqueue		(EventQueue* queue, int type, int param1, int param2);
void 	EventQueue_Dequeue		(EventQueue* queue, Gongshu_Event* pEvent);

#endif /* _EVENT_QUEUE */