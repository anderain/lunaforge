#include <stdlib.h>
#include <string.h>
#include "event_queue.h"

/* 初始化队列 */
void EventQueue_Init(EventQueue* queue) {
    memset(queue, 0, sizeof(EventQueue)); /* 置零整个结构 */
    queue->front = 0;
    queue->rear = 0;
    queue->length = 0;
}

/* 获取队列长度 */
int EventQueue_GetLength(EventQueue* queue) {
    return queue->length;
}

/* 清空队列 */
void EventQueue_ClearAll(EventQueue* queue) {
    queue->front = 0;
    queue->rear = 0;
    queue->length = 0;
}

/* 入队操作 */
void EventQueue_Enqueue(EventQueue* queue, int type, int param1, int param2) {
    Gongshu_Event event;
    /* 检查队列是否已满 */
    if (queue->length == QUEUE_MAX_SIZE) {
        return; /* 队列满则放弃 */
    }
    
    /* 创建新事件 */
    event.type = type;
    
    /* 根据事件类型填充参数 */
    switch (type) {
        case GSE_TYPE_KEY_DOWN:
        case GSE_TYPE_KEY_UP:
            event.param.keyCode = param1;
            break;
            
        case GSE_TYPE_MOUSE_DOWN:
        case GSE_TYPE_MOUSE_UP:
            event.param.mouseButtonCode = param1;
            break;
            
        case GSE_TYPE_MOUSE_MOVE:
            event.param.mousePosition.x = param1;
            event.param.mousePosition.y = param2;
            break;
        
        /* GSE_TYPE_QUIT 和其他类型不需要参数 */
        default:
            break;
    }
    
    /* 将事件加入队列 */
    memcpy(&queue->data[queue->rear], &event, sizeof(Gongshu_Event));
    queue->rear = (queue->rear + 1) % QUEUE_MAX_SIZE;
    queue->length++;
}

/* 出队操作 */
void EventQueue_Dequeue(EventQueue* queue, Gongshu_Event* pEvent) {
    /* 检查队列是否为空 */
    if (queue->length == 0) {
        return; /* 队列为空则不写入 */
    }
    
    /* 复制最旧的事件 */
    if (pEvent != NULL) {
        *pEvent = queue->data[queue->front];
    }
    
    /* 更新队列状态 */
    queue->front = (queue->front + 1) % QUEUE_MAX_SIZE;
    queue->length--;
}