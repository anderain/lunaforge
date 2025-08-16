#include "wrapper.h"
#include "../gongshu.h"
#include "event_queue.h"

extern HWND     hWndMain;
EventQueue      eventQueue;
extern MSG      msg;
static char     szAppPath[200] = "";

int Gongshu_Init(void) {
	EventQueue_Init(&eventQueue);
	return 1;
}

void Gongshu_SetAppPath(const char *path) {
    strcpy(szAppPath, path);
}

const char* Gongshu_GetAppPath(void) {
    return szAppPath;
}

void Gongshu_GetResolution(int* pWidth, int *pHeight) {
	WrapperGraphConfig *pWgConfig = WG_GetConfig();
	if (pWidth) *pWidth = pWgConfig->width;
	if (pHeight) *pHeight = pWgConfig->height;
}

void Gongshu_Flip(void) {
	InvalidateRect(hWndMain, NULL, TRUE);
	UpdateWindow(hWndMain);
}

void Gongshu_Dispose(void) {
	EventQueue_ClearAll(&eventQueue);
    DestroyWindow(hWndMain);
	return;
}

void Gongshu_SetPixel(int x, int y, unsigned char colorIndex) {
	WG_SetPixel(x, y, colorIndex);
}

void Gongshu_ClearCanvas(void) {
	WG_Clear();
}

unsigned int Gongshu_GetTicks(void) {
	return 0;
}

void Gongshu_Delay(unsigned int ms) {
	return;
}

BOOL Gongshu_FetchEvent(Gongshu_Event* pEvent) {
    int ret = GetMessage(&msg, NULL, 0, 0);
    if (!ret) {
        pEvent->type = GSE_TYPE_QUIT;
        return TRUE;
    }
    else {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
	if (EventQueue_GetLength(&eventQueue) > 0) {
		EventQueue_Dequeue(&eventQueue, pEvent);
		return TRUE;
	}
	return FALSE;
}

BOOL Gongshu_LoadFile(const char* filePath, unsigned char** pRaw, int* pLength) {
    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD dwFileSize = 0;
    DWORD dwBytesRead = 0;
    unsigned char* buffer = NULL;
#ifdef UNICODE
    TCHAR wFilePath[100];
    c2w(filePath, wFilePath);
#endif

    *pRaw = NULL;
    *pLength = 0;

    /* 打开文件 */
    hFile = CreateFile(
#ifdef UNICODE
        wFilePath,            /* 文件路径 */
#else
        filePath,
#endif
        GENERIC_READ,         /* 只读访问 */
        FILE_SHARE_READ,      /* 共享读取 */
        NULL,                 /* 安全属性（默认） */
        OPEN_EXISTING,        /* 只打开存在的文件 */
        FILE_ATTRIBUTE_NORMAL,/* 普通文件 */
        NULL                  /* 无模板文件 */
    );
    
    if (hFile == INVALID_HANDLE_VALUE) {
        return FALSE;  /* 打开文件失败 */
    }
    
    /* 获取文件大小 */
    dwFileSize = GetFileSize(hFile, NULL);
    if (dwFileSize == INVALID_FILE_SIZE) {
        CloseHandle(hFile);
        return FALSE;  /* 获取文件大小失败 */
    }
    
    /* 分配内存 */
    buffer = (unsigned char*)malloc(dwFileSize);
    if (buffer == NULL) {
        CloseHandle(hFile);
        return FALSE;  /* 内存分配失败 */
    }
    
    /* 读取文件内容 */
    if (!ReadFile(hFile, buffer, dwFileSize, &dwBytesRead, NULL) ||
        dwBytesRead != dwFileSize) {
        free(buffer);
        CloseHandle(hFile);
        return FALSE;  /* 读取文件失败或读取字节数不匹配 */
    }
    
    /* 关闭文件句柄 */
    CloseHandle(hFile);
    
    /* 设置输出参数 */
    *pRaw = buffer;
    *pLength = (int)dwFileSize;
    
    return TRUE;
}