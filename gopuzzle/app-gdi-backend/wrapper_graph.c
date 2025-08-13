#include <math.h>
#include <stdlib.h>
#include "wrapper.h"
#include "../../artifacts/color-palette/vga_palette.h"
#include "../../artifacts/color-palette/vga_palette_grayscale.h"

HDC         hdcBuffer           = NULL;
HBITMAP     hBitmapBuffer       = NULL;
HBITMAP     hBitmapOld          = NULL;
BYTE *      pBitmapPixels       = NULL;
int         iBitmapPixelsSize   = 0;
BYTE *      pFrameBuffer        = NULL;
int         iFrameBufferSize    = 0;
int         iVideoBitMode       = -1;

static WrapperGraphConfig wgConfig = {
    0,      /* Width */
    0,      /* Height */
    0,      /* HDC */
    0,      /* Is initialized? */
    0,      /* Portrait Mode */
    NULL    /* Function of PutDisp */
};

WrapperGraphConfig* WG_GetConfig() {
    return &wgConfig;
}

static int WG_CreateVideoBuffer(
    HDC hdc, 
    int bufferWidth,
    int bufferHeight,
    int modeToSet,
    int portraitMode,
    TCHAR* pErrMsg
) {
    BITMAPINFO* pbmi                = NULL;
    int         paletteSize         = 0;
    int         bmiSizeWithPalette  = 0;
    BYTE *      pixelBuffer         = NULL;
    int         bitCount;
    int         i;

    wgConfig.width = portraitMode ? bufferHeight : bufferWidth;
    wgConfig.height = portraitMode ? bufferWidth : bufferHeight;
    wgConfig.portraitMode = portraitMode;

    /* Basic infomation */
    switch (modeToSet) {
        /*-----------------------------*/
        /* * 2 BIT MODE                */
        /*-----------------------------*/
        case VIDEO_BUFFER_MODE_2BIT: {

            paletteSize = 4;
            iBitmapPixelsSize = bufferWidth / 4 * bufferHeight;  /* 4 PIXEL = 1 BYTE */
            bitCount = 2;

            /* Width alignment */
            /* if (WidthInBytes % 4 != 0) WidthInBytes = (WidthInBytes / 4 + 1) * 4; */

            break;
        }
        /*-----------------------------*/
        /* * 8 BIT MODE                */
        /*-----------------------------*/
        case VIDEO_BUFFER_MODE_8BIT: {
            paletteSize = 256;
            iBitmapPixelsSize = bufferWidth * 1 * bufferHeight; /* 1 BYTE = 8 BITS */
            bitCount = 8;
 
            break;
        }
        /*-----------------------------*/
        /* * NOT DEFINED, FAILED!      */
        /*-----------------------------*/
        default: {
            return VIDEO_BUFFER_MODE_FAILED;
        }
    }

    /* Allocate framebuffer */
    iFrameBufferSize = bufferHeight * bufferWidth * 1;
    pFrameBuffer = (BYTE *)malloc(iFrameBufferSize);

    /* Failed to allocate */
    if (!pFrameBuffer) {
        MessageBox(NULL, _T("FAILED TO ALLOCATE FRAME BUFFER"), _T("ERROR Frame Buffer"), MB_OK);
        return VIDEO_BUFFER_MODE_FAILED;
    }

    iVideoBitMode = modeToSet;
    
    /* Header size to allocate */
    bmiSizeWithPalette = sizeof(BITMAPINFO) + paletteSize * sizeof(RGBQUAD);

    /* Create memory DC */
    hdcBuffer = CreateCompatibleDC(hdc);
    
    /* Allocate header */
    pbmi = (BITMAPINFO *)malloc(bmiSizeWithPalette);
    memset(pbmi, 0, bmiSizeWithPalette);

    /* Write palette */
    switch (iVideoBitMode) {
        /*-----------------------------*/
        /* * 2 BIT MODE                */
        /*-----------------------------*/
        /* 4 colors in palette         */
        /*-----------------------------*/
        case VIDEO_BUFFER_MODE_2BIT: {
            /* Grayscale color: */
            const BYTE grayscale[] = { 0x00, 0x80, 0xc0, 0xff };
            /* Write palette */
            for (i = 0; i < paletteSize; ++i) {
                pbmi->bmiColors[i].rgbRed       = grayscale[i];
                pbmi->bmiColors[i].rgbGreen     = grayscale[i];
                pbmi->bmiColors[i].rgbBlue      = grayscale[i];
                pbmi->bmiColors[i].rgbReserved  = 0;
            }
            break;
        }
        /*-----------------------------*/
        /* * 8 BIT MODE                */
        /*-----------------------------*/
        /* 256 colors in palette       */
        /*-----------------------------*/
        case VIDEO_BUFFER_MODE_8BIT: {
            /* Write palette */
            for (i = 0; i < paletteSize; ++i) {
                int color = VgaMode13hColorPalette[i];
                int r = (color >> 16) & 0xff;
                int g = (color >> 8) & 0xff;
                int b = (color) & 0xff;

                pbmi->bmiColors[i].rgbRed       = (BYTE)r;
                pbmi->bmiColors[i].rgbGreen     = (BYTE)g;
                pbmi->bmiColors[i].rgbBlue      = (BYTE)b;
                pbmi->bmiColors[i].rgbReserved  = 0;
            }
            break;
        }
    }

    pbmi->bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
    pbmi->bmiHeader.biWidth         = bufferWidth;
    pbmi->bmiHeader.biHeight        = -bufferHeight; /* from top to bottom */
    pbmi->bmiHeader.biPlanes        = 1;
    pbmi->bmiHeader.biBitCount      = bitCount;
    pbmi->bmiHeader.biSizeImage     = 0;
    pbmi->bmiHeader.biCompression   = 0;
    pbmi->bmiHeader.biXPelsPerMeter = 0;
    pbmi->bmiHeader.biYPelsPerMeter = 0;
    pbmi->bmiHeader.biClrImportant  = 0;
    /* Note: when creating 2bit DIBsection */
    /* biClrUsed should be set to 0 */
    pbmi->bmiHeader.biClrUsed       = iVideoBitMode == VIDEO_BUFFER_MODE_2BIT ? 0 : paletteSize;

    hBitmapBuffer = CreateDIBSection(hdcBuffer, pbmi, DIB_RGB_COLORS, (void**)&pixelBuffer, NULL, 0);

    /* FAILED TO CREATE BITMAP */
    if (!hBitmapBuffer || !pixelBuffer) {
        DWORD    errorCode;
        TCHAR    szFormat[] = _T("Error Code = %d.\n")
                              _T("Failed to create %dbit bitmap!\n")
                              _T("Image Size: %dx%d\n")
                              _T("Buffer Size = %d");

        errorCode = GetLastError();
        wsprintf(pErrMsg, szFormat, errorCode, bitCount, bufferWidth, bufferHeight, iBitmapPixelsSize);
        
        /* Delete memory DC */
        DeleteDC(hdcBuffer);
        hdcBuffer = NULL;
        free(pbmi);

        /* Free framebuffer */
        free(pFrameBuffer);

        return VIDEO_BUFFER_MODE_FAILED;
    }
    
    /* Save old bitmap */
    hBitmapOld = SelectObject(hdcBuffer, hBitmapBuffer);

    pBitmapPixels         = pixelBuffer;    
    wgConfig.hdcBuffer    = hdcBuffer;
    WG_SelectPutDisp();

    /* Delete bitmap header */
    free(pbmi);

    /* Set ready flag! */
    wgConfig.ready = TRUE; 

    memset(pFrameBuffer, 0x00, iFrameBufferSize);;
    memset(pBitmapPixels, 0x00, iBitmapPixelsSize);

    return modeToSet;
}

int WG_GetVideoBitMode() {
    return iVideoBitMode;
}

int WG_Initialize(HDC hdc, int vBufWidth, int vBufHeight, int portraitMode, TCHAR* pErrMsg) {
    /* Default video mode: 8bit */
    int modeToSet = VIDEO_BUFFER_MODE_8BIT;

#if (IS_WIN_CE)
    /* Detect CE Version */
    /* WinCE 1.x : FORCE 2bit */
    if (DeviceInfo.osInfo.dwMajorVersion < 2) {
        modeToSet = VIDEO_BUFFER_MODE_2BIT;
    }
    /* * WinCE 2.0 or later */
    /*   Depend on device compatible bitmap's bits of pixel */
    else {
        switch (DeviceInfo.bitmap.bmBitsPixel) {
            case 2:  modeToSet = VIDEO_BUFFER_MODE_2BIT;  break;
            case 8:  
            case 16:
            case 32: modeToSet = VIDEO_BUFFER_MODE_8BIT;  break;
        }
    }
#else
    /* Win32 */
    modeToSet = VIDEO_BUFFER_MODE_8BIT;
#endif

    /* Force 8Bit */
    modeToSet = VIDEO_BUFFER_MODE_8BIT;

    return WG_CreateVideoBuffer(hdc, vBufWidth, vBufHeight, modeToSet, portraitMode, pErrMsg);
}

void WG_Close() {
    if (!hdcBuffer) return;

    wgConfig.ready = 0;

    SelectObject(hdcBuffer, hBitmapOld);
    DeleteObject(hBitmapBuffer);
    DeleteDC(hdcBuffer);

    if (pFrameBuffer) {
        free(pFrameBuffer);
    }
}

void WG_Clear() {
    memset(pFrameBuffer, 0x00, iFrameBufferSize);
}

void WG_ClearWhite() {
    memset(pFrameBuffer, 0x0f, iFrameBufferSize);
}

/* Set pixel */
void WG_SetPixel(int x, int y, unsigned char colorIndex) {
    int offset;

    if (x < 0 || x >= wgConfig.width || y < 0 || y >= wgConfig.height);

    offset = (y * wgConfig.width + x);
    pFrameBuffer[offset] = colorIndex;
}

// 2Bit - Portrait Left
static void IWG_PutDisp2Bit_PL() {
    int x, y, i = 0, t = 0, offsetByte, offsetBit;
    BYTE *pixel8 = pBitmapPixels;
    BYTE colorGray;

    for (y = 0; y < wgConfig.height; ++y) {
        t = wgConfig.height - 1 - y;
        for (x = 0; x < wgConfig.width; ++x, ++i) {
            colorGray = VgaMode13hToGrayscalePalette[pFrameBuffer[i]];
            offsetByte = (x * wgConfig.width / 4) + (t >> 2);
            offsetBit = t & 3;
            pixel8[offsetByte] |= (colorGray << ((3 - offsetBit) << 1));
        }
    }
}

// 2Bit - Portrait Right
static void IWG_PutDisp2Bit_PR() {
    int x, y, i = 0, t = 0, offsetByte, offsetBit;
    BYTE *pixel8 = pBitmapPixels;
    BYTE colorGray;

    for (y = 0; y < wgConfig.height; ++y) {
        offsetBit = y & 3;
        for (x = 0; x < wgConfig.width; ++x, ++i) {
            colorGray = VgaMode13hToGrayscalePalette[pFrameBuffer[i]];
            t = wgConfig.width - 1 - x;
            offsetByte = (t * wgConfig.width / 4) + (y >> 2);
            pixel8[offsetByte] |= (colorGray << ((3 - offsetBit) << 1));
        }
    }
}

// 2Bit - Landscape
static void IWG_PutDisp2Bit_LS() {
    int x, y, i = 0, t = 0, offsetByte, offsetBit;
    BYTE *pixel8 = pBitmapPixels;
    BYTE colorGray;

    for (y = 0; y < wgConfig.height; ++y) {
        t = y * wgConfig.width / 4;
        for (x = 0; x < wgConfig.width; ++x, ++i) {
            colorGray = VgaMode13hToGrayscalePalette[pFrameBuffer[i]];
            offsetByte = t + (x >> 2);
            offsetBit = x & 3;
            pixel8[offsetByte] |= (colorGray << ((3 - offsetBit) << 1));
        }
    }
}

// 8Bit - Portrait Left
static void IWG_PutDisp8Bit_PL() {
    int x, y, i = 0, j = 0;
    BYTE *pixel8 = pBitmapPixels;
    for (y = 0; y < wgConfig.height; ++y) {
        for (x = 0; x < wgConfig.width; ++x, ++i) {
            j = (x * wgConfig.height) + wgConfig.height - 1 - y;

            pixel8[j] = pFrameBuffer[i];
        }
    }
}

// 8Bit - Portrait Right
static void IWG_PutDisp8Bit_PR() {
    int x, y, i = 0, j = 0;
    BYTE *pixel8 = pBitmapPixels;
    int t;
    for (y = 0; y < wgConfig.height; ++y) {
        for (x = 0; x < wgConfig.width; ++x, ++i) {
            t = wgConfig.width - x - 1;
            j = (t * wgConfig.height) + y;
            pixel8[j] = pFrameBuffer[i];
        }
    }
}

// 8Bit - Portrait Left
static void IWG_PutDisp8Bit_LS() {
    int x, y, i = 0, j = 0;
    BYTE *pixel8 = pBitmapPixels;

    for (y = 0; y < wgConfig.height; ++y) {
        for (x = 0; x < wgConfig.width; ++x, ++i, ++j) {
            pixel8[j] = pFrameBuffer[i];
        }
    }
}

/* Select Putdisp function */
void WG_SelectPutDisp() {
    switch (iVideoBitMode) {
        case VIDEO_BUFFER_MODE_2BIT: {
            switch (wgConfig.portraitMode) {
                case PORTRAIT_MODE_LEFT:    wgConfig.fpPutDisp = IWG_PutDisp2Bit_PL;    return;
                case PORTRAIT_MODE_RIGHT:   wgConfig.fpPutDisp = IWG_PutDisp2Bit_PR;    return;
                default:                    wgConfig.fpPutDisp = IWG_PutDisp2Bit_LS;    return;
            }
            return;
        }
        case VIDEO_BUFFER_MODE_8BIT: {
            switch (wgConfig.portraitMode) {
                case PORTRAIT_MODE_LEFT:    wgConfig.fpPutDisp = IWG_PutDisp8Bit_PL;    return;
                case PORTRAIT_MODE_RIGHT:   wgConfig.fpPutDisp = IWG_PutDisp8Bit_PR;    return;
                default:                    wgConfig.fpPutDisp = IWG_PutDisp8Bit_LS;    return;
            }
            return;
        }
    }
}
