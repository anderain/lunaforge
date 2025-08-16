#include <windows.h>
#include <stdlib.h>
#include "wrapper.h"
#include "../gongshu.h"
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
extern char         lunaFolderPath[];
WrapperGraphConfig* pWgConfig;

/* Foward declarations of functions included in this code module: */
ATOM                MyRegisterClass         (HINSTANCE hInstance, LPTSTR szWindowClass);
BOOL                InitInstance            (HINSTANCE, int);
LRESULT CALLBACK    WndProc                 (HWND, UINT, WPARAM, LPARAM);
void                Redraw                  ();
void                FlushWindow             (HDC, LPRECT);
int                 Gopuzzle_Main           ();
int                 Modi_SetRunningFlag     (BOOL flag);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine,int nCmdShow) {
    int     videoMode;
    RECT    clientRect;
    RECT    *pRect = &clientRect;
    HDC     hdc;
    BOOL    portraitMode;
    TCHAR   szErrBuf[200];

    if (1) {
        TCHAR szPathBuf[MAX_PATH];
        int i;
        GetModuleFileName(NULL, szPathBuf, MAX_PATH);
        for (i = _tcslen(szPathBuf) - 1; i >= 0; --i) {
            if (szPathBuf[i] == '\\' || szPathBuf[i] == '/') {
                szPathBuf[i] = 0;
                break;
            }
        }
#ifdef UNICODE
        w2c(szPathBuf, lunaFolderPath);
#else
        strcpy(lunaFolderPath, szPathBuf);
#endif
        strcpy(strchr(lunaFolderPath, 0), "\\cache\\");
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
        else if (wParam == 'W') {
            EventQueue_Enqueue(&eventQueue, GSE_TYPE_KEY_DOWN, GSE_KEY_CODE_UP, 0);
        }
        else if (wParam == 'S') {
            EventQueue_Enqueue(&eventQueue, GSE_TYPE_KEY_DOWN, GSE_KEY_CODE_DOWN, 0);
        }
        else if (wParam == 'A') {
            EventQueue_Enqueue(&eventQueue, GSE_TYPE_KEY_DOWN, GSE_KEY_CODE_LEFT, 0);
        }
        else if (wParam == 'D') {
            EventQueue_Enqueue(&eventQueue, GSE_TYPE_KEY_DOWN, GSE_KEY_CODE_RIGHT, 0);
        }
        else if (wParam == 'N') {
            EventQueue_Enqueue(&eventQueue, GSE_TYPE_KEY_DOWN, GSE_KEY_CODE_A, 0);
        }
        else if (wParam == 'M') {
            EventQueue_Enqueue(&eventQueue, GSE_TYPE_KEY_DOWN, GSE_KEY_CODE_B, 0);
        }
        else if (wParam == 'J') {
            EventQueue_Enqueue(&eventQueue, GSE_TYPE_KEY_DOWN, GSE_KEY_CODE_X, 0);
        }
        else if (wParam == 'K') {
            EventQueue_Enqueue(&eventQueue, GSE_TYPE_KEY_DOWN, GSE_KEY_CODE_Y, 0);
        }
        break;
    case WM_KEYUP:
        if (wParam == 'W') {
            EventQueue_Enqueue(&eventQueue, GSE_TYPE_KEY_UP, GSE_KEY_CODE_UP, 0);
        }
        else if (wParam == 'S') {
            EventQueue_Enqueue(&eventQueue, GSE_TYPE_KEY_UP, GSE_KEY_CODE_DOWN, 0);
        }
        else if (wParam == 'A') {
            EventQueue_Enqueue(&eventQueue, GSE_TYPE_KEY_UP, GSE_KEY_CODE_LEFT, 0);
        }
        else if (wParam == 'D') {
            EventQueue_Enqueue(&eventQueue, GSE_TYPE_KEY_UP, GSE_KEY_CODE_RIGHT, 0);
        }
        else if (wParam == 'N') {
            EventQueue_Enqueue(&eventQueue, GSE_TYPE_KEY_UP, GSE_KEY_CODE_A, 0);
        }
        else if (wParam == 'M') {
            EventQueue_Enqueue(&eventQueue, GSE_TYPE_KEY_UP, GSE_KEY_CODE_B, 0);
        }
        else if (wParam == 'J') {
            EventQueue_Enqueue(&eventQueue, GSE_TYPE_KEY_UP, GSE_KEY_CODE_X, 0);
        }
        else if (wParam == 'K') {
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
