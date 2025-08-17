#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "modi.h"
#include "../artifacts/raster-fonts/hybird_6x8.h"
#include "../artifacts/raster-fonts/tom_thumb_4x6.h"

#define ABS(v) ((v) < 0 ? -(v) : (v))

static BOOL modiRunningFlag = TRUE;

int Modi_GetRunningFlag(void) {
    return modiRunningFlag;
}

int Modi_SetRunningFlag(BOOL flag) {
    return modiRunningFlag = flag;
}

void Modi_FillRect(int left, int top, int width, int height, unsigned char colorIndex) {
    int screenWidth;
    int screenHeight;
    int right;
    int bottom;
    int x, y;

    Gongshu_GetResolution(&screenWidth, &screenHeight);
    if (left >= screenWidth) return;
    if (top >= screenHeight) return;

    right = left + width - 1;
    bottom = top + height - 1;
    if (right >= screenWidth) right = screenWidth - 1;
    if (bottom >= screenHeight) bottom = screenHeight - 1;

    for (y = top; y <= bottom; ++y) {
        for (x = left; x <= right; ++x) {
            Gongshu_SetPixel(x, y, colorIndex);
        }
    }
}

void Modi_FillRectWithMask(int left, int top, int width, int height, unsigned char colorIndex, int maskLevel) {
    int screenWidth;
    int screenHeight;
    int right;
    int bottom;
    int x, y;

    Gongshu_GetResolution(&screenWidth, &screenHeight);
    if (left >= screenWidth) return;
    if (top >= screenHeight) return;

    right = left + width - 1;
    bottom = top + height - 1;
    if (right >= screenWidth) right = screenWidth - 1;
    if (bottom >= screenHeight) bottom = screenHeight - 1;

    for (y = top; y <= bottom; ++y) {
        int ybit = (y & 1) << 1;
        for (x = left; x <= right; ++x) {
            int bitIndex = (x & 1) | ybit;
            if (maskLevel & (1 << bitIndex)) {
                Gongshu_SetPixel(x, y, colorIndex);
            }
        }
    }
}

void Modi_ApplyDarkMask(int maskLevel, unsigned char colorIndex) {
    int screenWidth, screenHeight;
    Gongshu_GetResolution(&screenWidth, &screenHeight);
    Modi_FillRectWithMask(0, 0, screenWidth, screenHeight, colorIndex, maskLevel);
}

void Modi_PlotLine(int x0, int y0, int x1, int y1, unsigned char color) {
    int dx = ABS(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -ABS(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    for (;;) {
        Gongshu_SetPixel(x0, y0, color);
        if (x0 == x1 && y0 == y1)
            break;
        e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

static void Modi_DrawOneBpp(const unsigned char *raw, int dx, int dy, int w, int h, BOOL rev, unsigned char colorIndex) {
    int pitch = (w >> 3) + (w % 8 ? 1 : 0);
    int x, y, dot;
    unsigned char eightPixels;
    for (y = 0; y < h; ++y) {
        for (x = 0; x < w; ++x) {
            eightPixels = *(raw + y * pitch + (x >> 3));
            dot = (eightPixels >> (7 - x % 8)) & 1;
            if (dot ^ rev) {
                Gongshu_SetPixel(dx + x, dy + y, colorIndex);
            }
        }
    }
}

static void Modi_DrawOneBppWithMask(const unsigned char *raw, int dx, int dy, int w, int h, BOOL rev, unsigned char colorIndex, int maskLevel) {
    int pitch = (w >> 3) + (w % 8 ? 1 : 0);
    int x, y, dot;
    unsigned char eightPixels;

    for (y = 0; y < h; ++y) {
        int ybit = (y & 1) << 1;
        for (x = 0; x < w; ++x) {
            int bitIndex = (x & 1) | ybit;
            int rx = dx + x;
            int ry = dy + y;
            eightPixels = *(raw + y * pitch + (x >> 3));
            dot = (eightPixels >> (7 - x % 8)) & 1;
            if ((dot ^ rev) && (maskLevel & (1 << bitIndex))) {
                Gongshu_SetPixel(rx, ry, colorIndex);
            }
        }
    }
}

void Modi_DrawLunaSpriteWithMask(const LunaSprite* pSprite, int dx, int dy, int spriteMode, int maskLevel) {
    int hrz, vrt, layer;
    unsigned char colorIndex[3];

    switch(spriteMode) {
    case SPRITE_MODE_BLACK_WHITE:
        colorIndex[0] = VGA_COLOR_BLACK;
        colorIndex[1] = VGA_COLOR_BRIGHT_WHITE;
        colorIndex[2] = VGA_COLOR_BRIGHT_WHITE;
        break;
    case SPRITE_MODE_BLACK_WHITE_GRAY:
        colorIndex[0] = VGA_COLOR_BLACK;
        colorIndex[1] = VGA_COLOR_BRIGHT_WHITE;
        colorIndex[2] = VGA_COLOR_WHITE;
        break;
    case SPRITE_MODE_NORMAL:
    default:
        colorIndex[0] = pSprite->colorIndex[0];
        colorIndex[1] = pSprite->colorIndex[1];
        colorIndex[2] = pSprite->colorIndex[2];
    }

    for (hrz = 0; hrz < pSprite->hrzCount; ++hrz) {
        for (vrt = 0; vrt < pSprite->vrtCount; ++vrt) {
            const unsigned char* raw;
            int x = dx + (hrz << 3);
            int y = dy + (vrt << 3);
            int rawOffset = vrt * pSprite->hrzCount + hrz;
            for (layer = 0; layer < 3; ++layer) {
                raw = pSprite->spriteRaw + (rawOffset << 4) + (rawOffset << 3) + (layer << 3);
                Modi_DrawLunaSprite8x8WithMask(raw, x, y, colorIndex[layer], maskLevel);
            }
        }
    }
}

void Modi_DrawLunaSpriteInRange(
    const LunaSprite* pSprite,
    int dx, int dy,
    int spriteMode,
    int rangeHrzStart, int rangeHrzEnd,
    int rangeVrtStart, int rangeVrtEnd
) {
    int hrz, vrt, layer;
    unsigned char colorIndex[3];

    if (rangeHrzStart < 0) rangeHrzStart = 0;
    if (rangeHrzEnd >= pSprite->hrzCount) rangeHrzEnd = pSprite->hrzCount - 1;
    if (rangeVrtStart < 0) rangeVrtStart = 0;
    if (rangeVrtEnd >= pSprite->vrtCount) rangeVrtEnd = pSprite->vrtCount - 1;

    switch(spriteMode) {
    case SPRITE_MODE_BLACK_WHITE:
        colorIndex[0] = VGA_COLOR_BLACK;
        colorIndex[1] = VGA_COLOR_BRIGHT_WHITE;
        colorIndex[2] = VGA_COLOR_BRIGHT_WHITE;
        break;
    case SPRITE_MODE_BLACK_WHITE_GRAY:
        colorIndex[0] = VGA_COLOR_BLACK;
        colorIndex[1] = VGA_COLOR_BRIGHT_WHITE;
        colorIndex[2] = VGA_COLOR_WHITE;
        break;
    case SPRITE_MODE_NORMAL:
    default:
        colorIndex[0] = pSprite->colorIndex[0];
        colorIndex[1] = pSprite->colorIndex[1];
        colorIndex[2] = pSprite->colorIndex[2];
    }

    for (hrz = rangeHrzStart; hrz <= rangeHrzEnd; ++hrz) {
        for (vrt = rangeVrtStart; vrt <= rangeVrtEnd; ++vrt) {
            const unsigned char* raw;
            int x = dx + (hrz << 3);
            int y = dy + (vrt << 3);
            int rawOffset = vrt * pSprite->hrzCount + hrz;
            for (layer = 0; layer < 3; ++layer) {
                raw = pSprite->spriteRaw + (rawOffset << 4) + (rawOffset << 3) + (layer << 3);
                Modi_DrawLunaSprite8x8(raw, x, y, colorIndex[layer]);
            }
        }
    }
}

void Modi_DrawLunaSprite(const LunaSprite* pSprite, int dx, int dy, int spriteMode) {
    Modi_DrawLunaSpriteInRange(pSprite, dx, dy, spriteMode, 0, pSprite->hrzCount - 1, 0, pSprite->vrtCount - 1);
}

void Modi_Print6x8(const uchar* str, int dx, int dy, BOOL rev, unsigned char colorIndex) {
    int i, offset;
    char c;
    for (i = 0; str[i]; ++i, dx += 6) {
        c = str[i];
        if (c < FONT_HYBIRD_6x8_START || c > FONT_HYBIRD_6x8_END)
            continue;
        offset = c - FONT_HYBIRD_6x8_START;
        Modi_DrawOneBpp(FONT_HYBIRD_6x8 + (offset << 3), dx, dy, 6, 8, rev, colorIndex);
    }
}

void Modi_Print4x6(const char *str, int dx, int dy, BOOL rev, unsigned char colorIndex) {
    int i, offset;
    char c;
    for (i = 0; str[i]; ++i, dx += 4) {
        c = str[i];
        if (c < FONT_TOM_THUMB_4x6_START || c > FONT_TOM_THUMB_4x6_END)
            continue;
        offset = c - FONT_TOM_THUMB_4x6_START;
        Modi_DrawOneBpp(FONT_TOM_THUMB_4x6 + ((offset << 2) + (offset << 1)), dx, dy, 4, 6, rev, colorIndex);
    }
}

int Modi_WaitKey(void) {
    Gongshu_Event event;
    Gongshu_Flip();
    for (;;) {
        while (Gongshu_FetchEvent(&event)) {
            if (event.type == GSE_TYPE_QUIT) {
                modiRunningFlag = FALSE;
                return GSE_KEY_CODE_NONE;
            }
            if (event.type == GSE_TYPE_KEY_UP) {
                return event.param.keyCode;
            }
        }
    }
    return GSE_KEY_CODE_NONE;
}

ModiLoadResult Modi_LoadLunaSprite(const char *filePath, LunaSprite** ppLunaSprite) {
    unsigned char*  fileRaw;
    LunaBmpContent* pFileContent;
    int             fileLength;
    int             spriteRawPartSize;
    LunaSprite*     pSprite;

    *ppLunaSprite = NULL;

    if (!Gongshu_LoadFile(filePath, &fileRaw, &fileLength)) {
        return MLR_FILE_NOT_EXIST;
    }

    pFileContent = (LunaBmpContent *)fileRaw;

    /* 校验文件头是否合法 */
    if (fileLength < sizeof(LunaBmpContent)) {
        return MLR_INVALID_SIZE;
    }
    if (pFileContent->magicByte1 != 'I' || pFileContent->magicByte2 != 'M') {
        return MLR_INVALID_HEADER;
    }
    /* 校验文件长度是不是合法 */
    spriteRawPartSize = 8 * 3 * pFileContent->hrzCount * pFileContent->vrtCount;
    if (spriteRawPartSize + sizeof(LunaBmpContent) != fileLength) {
        return MLR_INVALID_SIZE;
    }

    /* 创建 Sprite */
    pSprite = (LunaSprite *)malloc(sizeof(LunaSprite) + spriteRawPartSize);
    pSprite->hrzCount = pFileContent->hrzCount;
    pSprite->vrtCount = pFileContent->vrtCount;
    pSprite->width = pFileContent->hrzCount * 8 - pFileContent->hrzPadding;
    pSprite->height = pFileContent->vrtCount * 8 - pFileContent->vrtPadding;
    memcpy(pSprite->colorIndex, pFileContent->colorIndex, sizeof(pFileContent->colorIndex));
    memcpy(pSprite->spriteRaw, pFileContent->spriteRaw, spriteRawPartSize);
    *ppLunaSprite = pSprite;
    
    free(fileRaw);

    return MLR_NO_ERROR;
}

void Modi_DisposeLunaSprite (LunaSprite* pLunaSprite) {
    free(pLunaSprite);
}