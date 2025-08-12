#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gopuzzle.h"

#define EXAMPLE_PROJECT_FOLDER "../../../moon-example-project/cache/"

#define TILE_WIDTH              24
#define TILE_HEIGHT             11
#define HALF_TILE_WIDTH         12
#define HALF_TILE_HEIGHT        6
#define TILE_HIGHLIGHT_COLOR    VGA_COLOR_LIGHT_BLUE

LunaMap* LunaMap_CreateTest() {
    LunaMap*        pMap;
    LunaSprite*     pSprite;
    int             w = 20, h = 20;
    int             numTilemap = 6;
    int             mapByteSize = w * h * sizeof(TileIndex);

    pMap = (LunaMap *)malloc(sizeof(LunaMap));
    pMap->w = w;
    pMap->h = h;
    pMap->numTilemap = numTilemap;
    pMap->tilemap = (LunaSprite **)malloc(numTilemap * sizeof(LunaSprite*));
    pMap->terrain = (TileIndex *)malloc(mapByteSize);
    pMap->loStruct = (TileIndex *)malloc(mapByteSize);
    pMap->hiStruct = (TileIndex *)malloc(mapByteSize);

    memset(pMap->terrain, 1, mapByteSize);
    memset(pMap->loStruct, 0, mapByteSize);
    memset(pMap->hiStruct, 0, mapByteSize);

    MapElement(pMap, pMap->terrain, 1, 0) = 0;
    MapElement(pMap, pMap->terrain, 1, 1) = 0;
    MapElement(pMap, pMap->terrain, 0, 1) = 0;

    MapElement(pMap, pMap->loStruct, 0, 3) = 2;

    MapElement(pMap, pMap->loStruct, 4, 0) = 4;
    MapElement(pMap, pMap->loStruct, 5, 0) = 4;
    MapElement(pMap, pMap->loStruct, 6, 0) = 4;
    MapElement(pMap, pMap->loStruct, 7, 0) = 4;
    MapElement(pMap, pMap->loStruct, 8, 0) = 4;

    MapElement(pMap, pMap->loStruct, 8, 3) = 5;

    pMap->tilemap[0] = NULL; /* tilemap[0] 用永远是 0 */

    Modi_LoadLunaSprite(EXAMPLE_PROJECT_FOLDER "inn_wdnflr0", &pSprite);
    pMap->tilemap[1] = pSprite;
    Modi_LoadLunaSprite(EXAMPLE_PROJECT_FOLDER "inn_screen0", &pSprite);
    pMap->tilemap[2] = pSprite;
    Modi_LoadLunaSprite(EXAMPLE_PROJECT_FOLDER "inn_screen1", &pSprite);
    pMap->tilemap[3] = pSprite;
    Modi_LoadLunaSprite(EXAMPLE_PROJECT_FOLDER "inn_wood0", &pSprite);
    pMap->tilemap[4] = pSprite;
    Modi_LoadLunaSprite(EXAMPLE_PROJECT_FOLDER "inn_chair0", &pSprite);
    pMap->tilemap[5] = pSprite;

    return pMap;
}

void LunaMap_Dispose(LunaMap* pMap) {
    int i;
    /* 跳过第一个，释放其他 tile sprite */
    for (i = 1; i < pMap->numTilemap; ++i) {
        Modi_DisposeLunaSprite(pMap->tilemap[i]);
    }
    free(pMap->tilemap);
    free(pMap->terrain);
    free(pMap->loStruct);
    free(pMap->hiStruct);
    free(pMap);
}

void LunaMap_DrawLayer(LunaMap *pMap, TileIndex* pLayer, int startX, int startY) {
    int x, y, dx, dy;
    LunaSprite* pSprite;
    TileIndex tileIndex;
    for (y = 0; y < pMap->h; ++y) {
        for (x = 0; x < pMap->w; ++x) {
            tileIndex = MapElement(pMap, pLayer, x, y);
            if (tileIndex == 0) continue;
            pSprite = pMap->tilemap[tileIndex];
            if (NULL == pSprite) continue;
            dx = startX + x * HALF_TILE_WIDTH - y * HALF_TILE_WIDTH - (pSprite->width - TILE_WIDTH) / 2;
            dy = startY + x * HALF_TILE_HEIGHT + y * HALF_TILE_HEIGHT + TILE_HEIGHT - pSprite->height;
            Modi_DrawLunaSprite(pSprite, dx, dy, SPRITE_MODE_NORMAL);
        }
    }
}

void LunaMap_Draw(LunaMap* pMap, int cursorX, int cursorY) {
    int screenWidth, screenHeight;
    int screenHalfWidth, screenHalfHeight;
    int startX, startY;
    int curOffsetX, curOffsetY;

    Gongshu_GetResolution(&screenWidth, &screenHeight);

    screenHalfWidth = screenWidth / 2;
    screenHalfHeight = screenHeight / 2;
    startX = screenHalfWidth - HALF_TILE_WIDTH - (cursorX * HALF_TILE_WIDTH - cursorY * HALF_TILE_WIDTH);
    startY = screenHalfHeight - HALF_TILE_HEIGHT - (cursorX * HALF_TILE_HEIGHT + cursorY * HALF_TILE_HEIGHT);

    LunaMap_DrawLayer(pMap, pMap->terrain, startX, startY);
    LunaMap_DrawLayer(pMap, pMap->loStruct, startX, startY);
    LunaMap_DrawLayer(pMap, pMap->hiStruct, startX, startY);

    /* 绘制光标菱形 */
    curOffsetX = screenHalfWidth - 1;
    curOffsetY = screenHalfHeight - 1;
    Modi_PlotLine(curOffsetX, curOffsetY - HALF_TILE_HEIGHT, curOffsetX - HALF_TILE_WIDTH, curOffsetY, TILE_HIGHLIGHT_COLOR);
    Modi_PlotLine(curOffsetX, curOffsetY + HALF_TILE_HEIGHT, curOffsetX - HALF_TILE_WIDTH, curOffsetY, TILE_HIGHLIGHT_COLOR);
    curOffsetX = screenHalfWidth;
    curOffsetY = screenHalfHeight - 1;
    Modi_PlotLine(curOffsetX, curOffsetY + HALF_TILE_HEIGHT, curOffsetX + HALF_TILE_WIDTH, curOffsetY, TILE_HIGHLIGHT_COLOR);
    Modi_PlotLine(curOffsetX, curOffsetY - HALF_TILE_HEIGHT, curOffsetX + HALF_TILE_WIDTH, curOffsetY, TILE_HIGHLIGHT_COLOR);

}

int Gopuzzle_Main() {
    int         keyCode;
    LunaMap*    pLunaMap;
    int         cursorX = 0, cursorY = 0;
    int         screenWidth, screenHeight;
    char        szBuf[100];

    if (!Gongshu_Init()) {
        fprintf(stderr, "Failed to initialize Graphics\n");
        return -1;
    }

    Gongshu_GetResolution(&screenWidth, &screenHeight);

    pLunaMap = LunaMap_CreateTest();

    while (Modi_GetRunningFlag()) {
        Gongshu_ClearCanvas();
        LunaMap_Draw(pLunaMap, cursorX, cursorY);
        Modi_Print6x8("Map Editor Demo", 0, 0, FALSE, VGA_COLOR_CYAN);
        sprintf(szBuf, "Cursor (%03d,%03d)", cursorX, cursorY);
        Modi_Print6x8(szBuf, 0, screenHeight - 8, FALSE, VGA_COLOR_CYAN);
        keyCode = Modi_WaitKey();
        if (keyCode == GSE_KEY_CODE_UP) {
            cursorY--;
        }
        else if (keyCode == GSE_KEY_CODE_DOWN) {
            cursorY++;
        }
        else if (keyCode == GSE_KEY_CODE_LEFT) {
            cursorX--;
        }
        else if (keyCode == GSE_KEY_CODE_RIGHT) {
            cursorX++;
        }
    }
    Gongshu_Dispose();
    LunaMap_Dispose(pLunaMap);

    return 0;
}