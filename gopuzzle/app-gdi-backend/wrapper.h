#ifndef _CE_WRAPPER_LAYER_
#define _CE_WRAPPER_LAYER_

#include <windows.h>

#ifdef VER_PLATFORM_WIN32_CE
#    define IS_WIN_CE 1
#else
#    define IS_WIN_CE 0
#endif

/*==========================================*
  * Platform
 -------------------------------------------

 ===========================================*/

#if !defined(_T)
#	if defined(UNICODE)
#		define _T(x) L##x
#	else
#		define _T(x) x
#	endif
#endif

typedef struct tagDEVICE_INFO {
	OSVERSIONINFO osInfo;
	BITMAP bitmap;
} DEVICE_INFO;

extern DEVICE_INFO DeviceInfo;

void 	  Device_CollectInfo		  (HDC hdc);
void 	  Device_PopupInfo		    ();
char*   w2c                     (const TCHAR *s, char *c);
TCHAR*  c2w                     (const char *c, TCHAR *s);
BOOL    ToogleVisibleTaskbar    (BOOL visible);

/*==========================================*
  * Graphics
 -------------------------------------------

 ===========================================*/

/* Failed */
#define VIDEO_BUFFER_MODE_FAILED    0x00

/* 2Bit grayscale  */
/* For CE1 or CE2 Palm-size */
#define VIDEO_BUFFER_MODE_2BIT      2

/* 8Bit Palette */
/* For CE2 or later / HPC */
#define VIDEO_BUFFER_MODE_8BIT      8

/* Portrait */
#define PORTRAIT_MODE_NO            0
#define PORTRAIT_MODE_LEFT          1
#define PORTRAIT_MODE_RIGHT         2

int     WG_GetVideoBitMode      ();
int     WG_Initialize           (HDC hdc, int bufferWidth, int bufferHeight, int portraitMode, TCHAR* pErrMsg);
void    WG_Close                ();
void    WG_Clear                ();
void    WG_ClearWhite           ();
void    WG_SetPixel             (int x, int y, unsigned char colorIndex);
void    WG_SelectPutDisp        ();

typedef struct {
    int width;
    int height;
    HDC hdcBuffer;
    int ready;
    int portraitMode;
    void (* fpPutDisp)();
} WrapperGraphConfig;

WrapperGraphConfig* WG_GetConfig();

/*==========================================*
  * Graphics
 -------------------------------------------
 
 ===========================================*/

#endif /* _CE_WRAPPER_LAYER_ */