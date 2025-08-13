#include <windows.h>
#include "wrapper.h"

char * w2c(const TCHAR *s, char *c) {
    int i;
    for (i = 0; s[i]; ++i) {
        c[i] = (char)s[i];
    }
    c[i] = 0;
    return c;
}

TCHAR * c2w(const char *c, TCHAR *s) {
    int i;
    for (i = 0; c[i]; ++i) {
        s[i] = c[i];
    }
    s[i] = 0;
    return s;
}

BOOL ToogleVisibleTaskbar(BOOL visible) {
#if (IS_WIN_CE)
	HWND hWndTaskbar = FindWindow(_T("HHTaskBar"), NULL);
	if (!visible) {
		SetWindowPos(hWndTaskbar, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_HIDEWINDOW);
	} else {
		SetWindowPos(hWndTaskbar, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
	}
#endif
	return visible;
}

DEVICE_INFO DeviceInfo;

static void Device_CollectCompatibleBitmapInfo(HDC hdc) {
    HBITMAP hBitmap;

    // Create a temporary bitmap
    hBitmap = CreateCompatibleBitmap(hdc, 8, 8);

    // Fetch info
    GetObject(hBitmap, sizeof(BITMAP), &DeviceInfo.bitmap);

    // Release bitmap
    DeleteObject(hBitmap);
}

void Device_CollectInfo(HDC hdc) {
    GetVersionEx(&DeviceInfo.osInfo);
    Device_CollectCompatibleBitmapInfo(hdc);
}

void Device_PopupInfo() {
    TCHAR   szBuf[300]  = _T("");
    TCHAR   szTitle[]   = _T("Collected Device Info");
    TCHAR   szFormat[]  = _T("Platform Id : %d\n")
                          _T("Major Version : %d\n")
                          _T("Version : %s\n")
                          _T("BM Type: %d\n")
                          _T("BM Width Bytes: %d\n")
                          _T("BM BitsPixel: %d\n");
    // Format infomation
    wsprintf(
        szBuf,
        szFormat,
        DeviceInfo.osInfo.dwPlatformId,
        DeviceInfo.osInfo.dwMajorVersion,
        DeviceInfo.osInfo.szCSDVersion,
        DeviceInfo.bitmap.bmType,
        DeviceInfo.bitmap.bmWidthBytes,
        (int)DeviceInfo.bitmap.bmBitsPixel
    );

    MessageBox(NULL, szBuf, szTitle, MB_OK);
}
