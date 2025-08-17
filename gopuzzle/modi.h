#ifndef _MODI_LAYER_
#define _MODI_LAYER_

#include "gongshu.h"

typedef struct {
    unsigned char magicByte1;
    unsigned char magicByte2;
    unsigned char hrzCount;
    unsigned char vrtCount;
    unsigned char hrzPadding;
    unsigned char vrtPadding;
    unsigned char colorIndex[3];
    unsigned char spriteRaw[];
} LunaBmpContent;

typedef struct {
    int             hrzCount;
    int             vrtCount;
    int             width;
    int             height;
    unsigned char   colorIndex[3];
    unsigned char   spriteRaw[];
} LunaSprite;

typedef enum {
    MLR_NO_ERROR = 0,
    MLR_FILE_NOT_EXIST,
    MLR_INVALID_HEADER,
    MLR_INVALID_SIZE
} ModiLoadResult;

/* 正常三种颜色 */
#define SPRITE_MODE_NORMAL              0
/* 黑白双色 */
#define SPRITE_MODE_BLACK_WHITE         1
/* 黑白灰度三种颜色 */
#define SPRITE_MODE_BLACK_WHITE_GRAY    2

/* 变暗等级 */
/*  0% 变暗, 0b0000
    --
    -- */
#define DARK_MASK_LEVEL_NONE    0 
/*
    0% 变暗, 0b1000
    o-
    --  */
#define DARK_MASK_LEVEL_LIGHT   0x8
/*  25% 变暗, 0b1001
     o-
     -- */
#define DARK_MASK_LEVEL_MEDIUM  0x9
/*  50% 变暗, 0b1101
    oo
    -o */
#define DARK_MASK_LEVEL_HEAVY   0xD
/*  100% 变暗, 0b1111
    oo
    oo */
#define DARK_MASK_LEVEL_FULL    0xF

typedef unsigned char uchar;

int             Modi_GetRunningFlag         (void);
int             Modi_SetRunningFlag         (BOOL flag);
void            Modi_FillRect               (int left, int top, int width, int height, unsigned char colorIndex);
void            Modi_ApplyDarkMask          (int level, unsigned char colorIndex);
void            Modi_PlotLine               (int x0, int y0, int x1, int y1, unsigned char colorIndex);
void            Modi_DrawLunaSpriteInRange  (const LunaSprite* pSprite, int dx, int dy, int spriteMode, int rangeHrzStart, int rangeHrzEnd, int rangeVrtStart, int rangeVrtEnd);
void            Modi_DrawLunaSprite         (const LunaSprite* pSprite, int dx, int dy, int spriteMode);
void            Modi_DrawLunaSpriteWithMask (const LunaSprite* pSprite, int dx, int dy, int spriteMode, int maskLevel);
void            Modi_Print6x8               (const uchar* str, int dx, int dy, BOOL rev, unsigned char colorIndex);
void            Modi_Print4x6               (const char* str, int dx, int dy, BOOL rev, unsigned char colorIndex);
int             Modi_WaitKey                (void);
ModiLoadResult  Modi_LoadLunaSprite         (const char *filePath, LunaSprite** ppLunaSprite);
void            Modi_DisposeLunaSprite      (LunaSprite* pLunaSprite);

#define Modi_DrawLunaSprite8x8(raw, dx, dy, colorIndex) Modi_DrawOneBpp((raw), (dx), (dy), 8, 8, FALSE, (colorIndex))
#define Modi_DrawLunaSprite8x8WithMask(raw, dx, dy, colorIndex, maskLevel) Modi_DrawOneBppWithMask((raw), (dx), (dy), 8, 8, FALSE, (colorIndex), (maskLevel))

#endif /* _MODI_LAYER */