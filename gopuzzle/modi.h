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

int             Modi_GetRunningFlag     (void);
int             Modi_SetRunningFlag     (BOOL flag);
void            Modi_PlotLine           (int x0, int y0, int x1, int y1, unsigned char colorIndex);
void            Modi_DrawOneBpp         (const unsigned char * raw, int dx, int dy, int w, int h, BOOL rev, unsigned char colorIndex);
void            Modi_DrawLunaSprite     (const LunaSprite* pSprite, int dx, int dy, int spriteMode);
void            Modi_Print6x8           (const char* str, int dx, int dy, BOOL rev, unsigned char colorIndex);
void            Modi_Print4x6           (const char* str, int dx, int dy, BOOL rev, unsigned char colorIndex);
int             Modi_WaitKey            (void);
ModiLoadResult  Modi_LoadLunaSprite     (const char *filePath, LunaSprite** ppLunaSprite);
void            Modi_DisposeLunaSprite  (LunaSprite* pLunaSprite);

#define Modi_DrawLunaSprite8x8(raw, dx, dy, colorIndex) Modi_DrawOneBpp((raw), (dx), (dy), 8, 8, FALSE, (colorIndex))

#endif /* _MODI_LAYER */