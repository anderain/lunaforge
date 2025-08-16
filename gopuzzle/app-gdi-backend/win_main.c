#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "wrapper.h"
#include "../gongshu.h"
#include "../../artifacts/jasmine89/jasmine.h"
#include "event_queue.h"

#define APP_NAME "Gopuzzle"

void Framework_Main();

/* Global Variables: */
HINSTANCE           hInst;
HWND                hWndMain;
int                 screenWidth,screenHeight;
int                 vBufWidth, vBufHeight;
MSG                 msg;
BOOL                bLoop = TRUE;
extern EventQueue   eventQueue;
WrapperGraphConfig* pWgConfig;

/* Foward declarations of functions included in this code module: */
ATOM                MyRegisterClass         (HINSTANCE hInstance, LPTSTR szWindowClass);
BOOL                InitInstance            (HINSTANCE, int);
LRESULT CALLBACK    WndProc                 (HWND, UINT, WPARAM, LPARAM);
void                Redraw                  ();
void                FlushWindow             (HDC, LPRECT);
int                 Gopuzzle_Main           ();
int                 Modi_SetRunningFlag     (BOOL flag);

struct {
    BOOL landAutoDetect;
    struct {
        unsigned int up;
        unsigned int down;
        unsigned int left;
        unsigned int right;
        unsigned int a, b, x, y;
    } keyCode;
} appWin32Config = {
    TRUE,
    { 'W', 'S', 'A', 'D', 'N', 'M', 'J', 'K' }
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine,int nCmdShow) {
    int     videoMode;
    RECT    clientRect;
    RECT    *pRect = &clientRect;
    HDC     hdc;
    BOOL    portraitMode;
    TCHAR   szErrBuf[200];

    /* Get app path */
    {
        TCHAR szPathBuf[MAX_PATH];
        char szCharBuf[MAX_PATH];
        int i;
        GetModuleFileName(NULL, szPathBuf, MAX_PATH);
        for (i = _tcslen(szPathBuf) - 1; i >= 0; --i) {
            if (szPathBuf[i] == '\\' || szPathBuf[i] == '/') {
                szPathBuf[i] = 0;
                break;
            }
        }
        MessageBox(NULL, szPathBuf, _T("App Path"), MB_OK);
#ifdef UNICODE
        w2c(szPathBuf, szCharBuf);
#else
        strcpy(szCharBuf, szPathBuf);
#endif
        Gongshu_SetAppPath(szCharBuf);
    }
    /* Load json config */
    {
        char* szFileContent;
        int length;
        char szJsonFilePath[200];
        TCHAR errMsg[200];

        sprintf(szJsonFilePath, "%s\\gopuzzle.config.json", Gongshu_GetAppPath());

        if (Gongshu_LoadFile(szJsonFilePath, &szFileContent, &length)) {
            JasmineNode*    rootNode    = NULL;
            JasmineParser*  parser      = NULL;
            char*           szJson      = NULL;

            /* Convert binary to zero-terminated string */
            szJson = (char *)malloc(length + 1);
            memset(szJson, 0, length + 1);
            memcpy(szJson, szFileContent, length);
            if (szFileContent) free(szFileContent);

            rootNode = Jasmine_Parse(szJson, &parser);
            
            if (parser->errorCode == JASMINE_ERROR_NO_ERROR) {
                appWin32Config.landAutoDetect   = JasmineConfig_LoadInteger(rootNode, "win.landAutoDetect", TRUE);
                appWin32Config.keyCode.up       = JasmineConfig_LoadInteger(rootNode, "win.keyCode.up",     'W');
                appWin32Config.keyCode.down     = JasmineConfig_LoadInteger(rootNode, "win.keyCode.down",   'S');
                appWin32Config.keyCode.left     = JasmineConfig_LoadInteger(rootNode, "win.keyCode.left",   'A');
                appWin32Config.keyCode.right    = JasmineConfig_LoadInteger(rootNode, "win.keyCode.right",  'D');
                appWin32Config.keyCode.a        = JasmineConfig_LoadInteger(rootNode, "win.keyCode.a",      'N');
                appWin32Config.keyCode.b        = JasmineConfig_LoadInteger(rootNode, "win.keyCode.b",      'M');
                appWin32Config.keyCode.x        = JasmineConfig_LoadInteger(rootNode, "win.keyCode.x",      'J');
                appWin32Config.keyCode.y        = JasmineConfig_LoadInteger(rootNode, "win.keyCode.y",      'K');
                MessageBox(NULL, _T("JSON config loaded"), _T("Success"), MB_OK);
            }
            else {
#ifdef UNICODE
                c2w(parser->errorString, errMsg);
#else
                strcpy(errMsg, parser->errorString);
#endif
                MessageBox(NULL, errMsg, _T("Error"), MB_OK);
            }

            if (rootNode) JasmineNode_Dispose(rootNode);
            if (parser) JasmineParser_Dispose(parser);
            if (szJson) free(szJson);
        }
        else {
            MessageBox(NULL, _T("Failed to read config file"), _T("Error"), MB_OK);
        }
    }

    /* Initialize random function */
    srand(GetTickCount());

    /* Perform application initialization: */
    if (!InitInstance (hInstance, nCmdShow)) {
        return FALSE;
    }

    hdc = GetDC(hWndMain);

    /* Collect device infomation */
    /* pass hdc for compatible bitmap testing */
    Device_CollectInfo(hdc);

    /* [Debug] Popup device info */
    /* Device_PopupInfo(); */

    /* Detect portrait mode */
    portraitMode = PORTRAIT_MODE_NO;
    if (IS_WIN_CE && vBufHeight > vBufWidth) {
        portraitMode = PORTRAIT_MODE_LEFT;
    }

    if (!appWin32Config.landAutoDetect) {
        portraitMode = PORTRAIT_MODE_NO;
    }

    /* Initialize video buffer */
    videoMode = WG_Initialize(
        hdc,
        vBufWidth,
        vBufHeight,
        portraitMode,
        szErrBuf
    );

    /* Failed to create video buffer */
    if (VIDEO_BUFFER_MODE_FAILED == videoMode) {
        MessageBox(hWndMain, szErrBuf, _T("VIDEO ERROR"), MB_OK);
        ReleaseDC(hWndMain, hdc);
        DestroyWindow(hWndMain);
        return -1;
    }

    pWgConfig = WG_GetConfig();

    WG_ClearWhite();

    {
        int i, j, colorIndex = 0;
        int x, y, cw = pWgConfig->width / 16, ch = pWgConfig->height / 16;

        for (y = 0; y < 16; ++y) {
            for (x = 0; x < 16; ++x, ++colorIndex) {
                for (i = 0; i < ch; ++i) {
                    for (j = 0; j < cw; ++j) {
                        WG_SetPixel(x * cw + j, y * ch + i, (unsigned char)colorIndex);
                    }
                }
            }
        }
    }

    InvalidateRect(hWndMain, NULL, TRUE);
    UpdateWindow(hWndMain);

    Gopuzzle_Main();

    return 0;
}


ATOM MyRegisterClass(HINSTANCE hInstance, LPTSTR szWindowClass) {
    WNDCLASS wc;

    wc.style            = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc      = (WNDPROC) WndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = hInstance;
    wc.hIcon            = NULL; // LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
    wc.hCursor          = 0;
    wc.hbrBackground    = (HBRUSH) GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName     = 0;
    wc.lpszClassName    = szWindowClass;

    return RegisterClass(&wc);
}

void CenterWindowForPC() {
    RECT winRect;

    /* Get windows position */
    GetWindowRect(hWndMain, &winRect);

    /* Move windows to center  */
    SetWindowPos(
        hWndMain,
        HWND_TOP,
        (screenWidth - (winRect.right - winRect.left)) / 2,
        (screenHeight - (winRect.bottom - winRect.top)) / 2,
        0, 0,
        SWP_NOSIZE
    );
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
    TCHAR   szWindowClass[] = _T(APP_NAME);
    RECT    clientRect;
    int     winWidth, winHeight;
    DWORD   windowStyle;

    hInst = hInstance;

    MyRegisterClass(hInstance, szWindowClass);

    screenWidth = GetSystemMetrics(SM_CXSCREEN);
    screenHeight = GetSystemMetrics(SM_CYSCREEN);

#if IS_WIN_CE
    vBufWidth = screenWidth;
    vBufHeight = screenHeight;
#else
    vBufWidth = 640;
    vBufHeight = 480;
#endif

    clientRect.left  = 0;
    clientRect.top  = 0;

#if !(IS_WIN_CE)
    /* Note: */
    /* `AdjustWindowRect` not exist in CE */
    clientRect.right    = vBufWidth;
    clientRect.bottom   = vBufHeight;
    windowStyle         = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    AdjustWindowRect(&clientRect, windowStyle, FALSE);
#else
    clientRect.right    = screenWidth;
    clientRect.bottom   = screenHeight;
    windowStyle         = WS_VISIBLE;
#endif

    winWidth    = clientRect.right - clientRect.left;
    winHeight   = clientRect.bottom - clientRect.top;

    hWndMain = CreateWindow(szWindowClass, _T(APP_NAME), windowStyle, 0, 0, winWidth, winHeight, NULL, NULL, hInstance, NULL);

    if (!hWndMain) {
        return FALSE;
    }

#if (IS_WIN_CE)
    ShowWindow(hWndMain, nCmdShow);
    ToogleVisibleTaskbar(FALSE);
    UpdateWindow(hWndMain);
    SetWindowPos(hWndMain, HWND_TOP, 0, 0, screenWidth, screenHeight, SWP_SHOWWINDOW);
#else
    ShowWindow(hWndMain, nCmdShow);
    UpdateWindow(hWndMain);
    CenterWindowForPC();
#endif

    return TRUE;
}

void FlushWindow(HDC hdc, LPRECT pRect) {
    if (!pWgConfig || !pWgConfig->ready) {
        return;
    }
    BitBlt(
        hdc,
        pRect->left, pRect->top, pRect->right, pRect->bottom,
        pWgConfig->hdcBuffer,
        0, 0, SRCCOPY
    );
}

void ConfirmExit() {
    if (IDYES == MessageBox(hWndMain, _T("Are you sure to quit?"), _T("Quit"), MB_YESNO)) {
        DestroyWindow(hWndMain);
    }
    else {
        InvalidateRect(hWndMain, NULL, TRUE);
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    HDC hdc;
    int wmId, wmEvent;
    PAINTSTRUCT ps;

    switch (message)  {
    case WM_COMMAND:
        wmId    = LOWORD(wParam); 
        wmEvent = HIWORD(wParam); 
        /* Parse the menu selections: */
        switch (wmId) {
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
    case WM_LBUTTONDOWN: {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);
        break;
    }
    case WM_LBUTTONUP: {
        break;
    }
    case WM_KEYDOWN:
        /* Force Exit */
        if (wParam == VK_TAB || wParam == VK_ESCAPE) {
            ConfirmExit();
        }
        /* Toggle portrait mode */
        else if (wParam == VK_DOWN) {
            if (pWgConfig->portraitMode == PORTRAIT_MODE_LEFT) {
                pWgConfig->portraitMode = PORTRAIT_MODE_RIGHT;
                WG_SelectPutDisp();
                InvalidateRect(hWnd, NULL, TRUE);
            }
            else if (pWgConfig->portraitMode == PORTRAIT_MODE_RIGHT) {
                pWgConfig->portraitMode = PORTRAIT_MODE_LEFT;
                WG_SelectPutDisp();
                InvalidateRect(hWnd, NULL, TRUE);
            }
        }
        else if (wParam == appWin32Config.keyCode.up) {
            EventQueue_Enqueue(&eventQueue, GSE_TYPE_KEY_DOWN, GSE_KEY_CODE_UP, 0);
        }
        else if (wParam == appWin32Config.keyCode.down) {
            EventQueue_Enqueue(&eventQueue, GSE_TYPE_KEY_DOWN, GSE_KEY_CODE_DOWN, 0);
        }
        else if (wParam == appWin32Config.keyCode.left) {
            EventQueue_Enqueue(&eventQueue, GSE_TYPE_KEY_DOWN, GSE_KEY_CODE_LEFT, 0);
        }
        else if (wParam == appWin32Config.keyCode.right) {
            EventQueue_Enqueue(&eventQueue, GSE_TYPE_KEY_DOWN, GSE_KEY_CODE_RIGHT, 0);
        }
        else if (wParam == appWin32Config.keyCode.a) {
            EventQueue_Enqueue(&eventQueue, GSE_TYPE_KEY_DOWN, GSE_KEY_CODE_A, 0);
        }
        else if (wParam == appWin32Config.keyCode.b) {
            EventQueue_Enqueue(&eventQueue, GSE_TYPE_KEY_DOWN, GSE_KEY_CODE_B, 0);
        }
        else if (wParam == appWin32Config.keyCode.x) {
            EventQueue_Enqueue(&eventQueue, GSE_TYPE_KEY_DOWN, GSE_KEY_CODE_X, 0);
        }
        else if (wParam == appWin32Config.keyCode.y) {
            EventQueue_Enqueue(&eventQueue, GSE_TYPE_KEY_DOWN, GSE_KEY_CODE_Y, 0);
        }
        break;
    case WM_KEYUP:
        if (wParam == appWin32Config.keyCode.up) {
            EventQueue_Enqueue(&eventQueue, GSE_TYPE_KEY_UP, GSE_KEY_CODE_UP, 0);
        }
        else if (wParam == appWin32Config.keyCode.down) {
            EventQueue_Enqueue(&eventQueue, GSE_TYPE_KEY_UP, GSE_KEY_CODE_DOWN, 0);
        }
        else if (wParam == appWin32Config.keyCode.left) {
            EventQueue_Enqueue(&eventQueue, GSE_TYPE_KEY_UP, GSE_KEY_CODE_LEFT, 0);
        }
        else if (wParam == appWin32Config.keyCode.right) {
            EventQueue_Enqueue(&eventQueue, GSE_TYPE_KEY_UP, GSE_KEY_CODE_RIGHT, 0);
        }
        else if (wParam == appWin32Config.keyCode.a) {
            EventQueue_Enqueue(&eventQueue, GSE_TYPE_KEY_UP, GSE_KEY_CODE_A, 0);
        }
        else if (wParam == appWin32Config.keyCode.b) {
            EventQueue_Enqueue(&eventQueue, GSE_TYPE_KEY_UP, GSE_KEY_CODE_B, 0);
        }
        else if (wParam == appWin32Config.keyCode.x) {
            EventQueue_Enqueue(&eventQueue, GSE_TYPE_KEY_UP, GSE_KEY_CODE_X, 0);
        }
        else if (wParam == appWin32Config.keyCode.y) {
            EventQueue_Enqueue(&eventQueue, GSE_TYPE_KEY_UP, GSE_KEY_CODE_Y, 0);
        }
        break;
    case WM_CREATE: 
        break;
    case WM_ERASEBKGND:
        return TRUE;
    case WM_PAINT: {
        RECT rt;
        hdc = BeginPaint(hWnd, &ps);
        GetClientRect(hWnd, &rt);
        if (pWgConfig && pWgConfig->ready) {
            pWgConfig->fpPutDisp();
        }
        FlushWindow(hdc, &rt);
        EndPaint(hWnd, &ps);
        break;
    }
    case WM_CLOSE:
        ConfirmExit();
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        WG_Close();
        ToogleVisibleTaskbar(TRUE);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
