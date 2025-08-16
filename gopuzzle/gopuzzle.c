#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gopuzzle.h"

#define UI_STATE_MAP_EDIT       0
#define UI_STATE_TILE_SELECT    1
#define UI_STATE_SETTING        2

#define UI_TILESET_START_X      8
#define UI_TILESET_START_Y      16
#define UI_TILESET_CLIENT_W     160
#define UI_TILESET_CLIENT_H     120
#define UI_TILE_CELL_W          28
#define UI_TILE_CELL_H          24

#define UI_MENU_FILL            1
#define UI_MENU_SAVE            2
#define UI_MENU_SETTINGS        3
#define UI_MENU_QUIT            4


static const char*  MapLayerName[] = { "Terrain", "LoStruct", "HiStruct", "View Only" };

static const struct {
    int menuActionId;
    char* menuText;
} MapMenuItems[] = {
    { UI_MENU_FILL,     "Fill Area" },
    { UI_MENU_SAVE,     "Save" },
    { UI_MENU_SETTINGS, "Settings"},
    { UI_MENU_QUIT,     "Quit" },
};

static const int NumMapMenuItems = sizeof(MapMenuItems) / sizeof(MapMenuItems[0]);

struct {
    int state;
    struct {
        int layer;
        int tileIndex;
        BOOL isMenuOpened;
        int menuIndex;
        struct {
            int x;
            int y;
        } cursor;
    } map;
    struct {
        int tileIndex;
        int colSize;
        int rowSize;
    } tile;
} ui = {
    /* state    */ UI_STATE_MAP_EDIT,
    /* map      */ {
        /* layer        */ LAYER_VIEW_ONLY,
        /* tileIndex    */ 0,
        /* isMenuOpend  */ FALSE,
        /* menuIndex    */ 0,
        /* cursor       */ { 0, 0 }
    },
    /* tile     */ {
        /* tileIndex    */ 0,
        /* colSize      */ UI_TILESET_CLIENT_W / UI_TILE_CELL_W,
        /* rowSize      */ UI_TILESET_CLIENT_H / UI_TILE_CELL_H
    }
};
int         keyCode;
LunaMap*    pLunaMap;
int         screenWidth, screenHeight;

ModiLoadResult TestLoadSprite(const char* path, LunaSprite** ppSprite) {
    char fullPath[200];
    sprintf(fullPath, "%s\\cache\\%s", Gongshu_GetAppPath(), path);
    return Modi_LoadLunaSprite(fullPath, ppSprite);
}

LunaMap* LunaMap_CreateTest() {
    LunaMap*        pMap;
    LunaSprite*     pSprite;
    int             w = 20, h = 20;
    int             numTilemap = 6;
    int             mapByteSize = w * h * sizeof(TileIndex);

    pMap = (LunaMap *)malloc(sizeof(LunaMap));
    pMap->w = w;
    pMap->h = h;
    pMap->numTilemap                = numTilemap;
    pMap->tilemap                   = (LunaSprite **)malloc(numTilemap * sizeof(LunaSprite*));
    pMap->layer[LAYER_TERRAIN]      = (TileIndex *)malloc(mapByteSize);
    pMap->layer[LAYER_LO_STRUCT]    = (TileIndex *)malloc(mapByteSize);
    pMap->layer[LAYER_HI_STRUCT]    = (TileIndex *)malloc(mapByteSize);

    memset(pMap->layer[LAYER_TERRAIN], 1, mapByteSize);
    memset(pMap->layer[LAYER_LO_STRUCT], 0, mapByteSize);
    memset(pMap->layer[LAYER_HI_STRUCT], 0, mapByteSize);

    MapElement(pMap, LAYER_TERRAIN, 1, 0) = 0;
    MapElement(pMap, LAYER_TERRAIN, 1, 1) = 0;
    MapElement(pMap, LAYER_TERRAIN, 0, 1) = 0;

    MapElement(pMap, LAYER_LO_STRUCT, 0, 3) = 2;
    MapElement(pMap, LAYER_HI_STRUCT, 0, 3) = 3;

    MapElement(pMap, LAYER_LO_STRUCT, 4, 0) = 4;
    MapElement(pMap, LAYER_LO_STRUCT, 5, 0) = 4;
    MapElement(pMap, LAYER_LO_STRUCT, 6, 0) = 4;
    MapElement(pMap, LAYER_LO_STRUCT, 7, 0) = 4;
    MapElement(pMap, LAYER_LO_STRUCT, 8, 0) = 4;

    MapElement(pMap, LAYER_LO_STRUCT, 8, 3) = 5;

    pMap->tilemap[0] = NULL; /* tilemap[0] 用永远是 0 */

    TestLoadSprite("inn_wdnflr0", &pSprite);
    pMap->tilemap[1] = pSprite;
    TestLoadSprite("inn_screen0", &pSprite);
    pMap->tilemap[2] = pSprite;
    TestLoadSprite("inn_screen1", &pSprite);
    pMap->tilemap[3] = pSprite;
    TestLoadSprite("inn_wood0", &pSprite);
    pMap->tilemap[4] = pSprite;
    TestLoadSprite("inn_chair0", &pSprite);
    pMap->tilemap[5] = pSprite;

    return pMap;
}

void LunaMap_Dispose(LunaMap* pMap) {
    int i;
    /* 跳过第一个，释放其他 tile sprite */
    for (i = 1; i < pMap->numTilemap; ++i) {
        LunaSprite* pSprite = pMap->tilemap[i];
        if (pSprite) {
            Modi_DisposeLunaSprite(pSprite);
        }
    }
    free(pMap->tilemap);
    /* 释放所有layer */
    for (i = LAYER_TERRAIN; i <= LAYER_HI_STRUCT; ++i) {
         free(pMap->layer[i]);
    }
    free(pMap);
}

void LunaMap_DrawLayer(const LunaMap *pMap, int layerIndex, int startX, int startY, int maskLevel) {
    int x, y, dx, dy;
    LunaSprite* pSprite;
    TileIndex tileIndex;
    for (y = 0; y < pMap->h; ++y) {
        for (x = 0; x < pMap->w; ++x) {
            tileIndex = MapElement(pMap, layerIndex, x, y);
            if (tileIndex == 0) continue;
            pSprite = pMap->tilemap[tileIndex];
            if (NULL == pSprite) continue;
            dx = startX + x * HALF_TILE_WIDTH - y * HALF_TILE_WIDTH - (pSprite->width - TILE_WIDTH) / 2;
            dy = startY + x * HALF_TILE_HEIGHT + y * HALF_TILE_HEIGHT + TILE_HEIGHT - pSprite->height;
            Modi_DrawLunaSpriteWithMask(pSprite, dx, dy, SPRITE_MODE_NORMAL, maskLevel);
        }
    }
}

void LunaMap_Draw(const LunaMap* pMap) {
    int screenWidth, screenHeight;
    int screenHalfWidth, screenHalfHeight;
    int startX, startY;
    int curOffsetX, curOffsetY;

    Gongshu_GetResolution(&screenWidth, &screenHeight);

    screenHalfWidth = screenWidth / 2;
    screenHalfHeight = screenHeight / 2;
    startX = screenHalfWidth - HALF_TILE_WIDTH - (ui.map.cursor.x * HALF_TILE_WIDTH - ui.map.cursor.y * HALF_TILE_WIDTH);
    startY = screenHalfHeight - HALF_TILE_HEIGHT - (ui.map.cursor.x * HALF_TILE_HEIGHT + ui.map.cursor.y * HALF_TILE_HEIGHT);

    switch (ui.map.layer) {
    case LAYER_TERRAIN:
        LunaMap_DrawLayer(pMap, LAYER_TERRAIN, startX, startY, DARK_MASK_LEVEL_FULL);
        LunaMap_DrawLayer(pMap, LAYER_LO_STRUCT, startX, startY, DARK_MASK_LEVEL_MEDIUM);
        LunaMap_DrawLayer(pMap, LAYER_HI_STRUCT, startX, startY, DARK_MASK_LEVEL_MEDIUM);
        break;
    case LAYER_LO_STRUCT:
        LunaMap_DrawLayer(pMap, LAYER_TERRAIN, startX, startY, DARK_MASK_LEVEL_FULL);
        Modi_ApplyDarkMask(DARK_MASK_LEVEL_MEDIUM, VGA_COLOR_BLACK);
        LunaMap_DrawLayer(pMap, LAYER_LO_STRUCT, startX, startY, DARK_MASK_LEVEL_FULL);
        LunaMap_DrawLayer(pMap, LAYER_HI_STRUCT, startX, startY, DARK_MASK_LEVEL_MEDIUM);
        break;
    case LAYER_HI_STRUCT:
        LunaMap_DrawLayer(pMap, LAYER_TERRAIN, startX, startY, DARK_MASK_LEVEL_FULL);
        LunaMap_DrawLayer(pMap, LAYER_LO_STRUCT, startX, startY, DARK_MASK_LEVEL_FULL);
        Modi_ApplyDarkMask(DARK_MASK_LEVEL_MEDIUM, VGA_COLOR_BLACK);
        LunaMap_DrawLayer(pMap, LAYER_HI_STRUCT, startX, startY, DARK_MASK_LEVEL_FULL);
        break;
    case LAYER_VIEW_ONLY:
        LunaMap_DrawLayer(pMap, LAYER_TERRAIN, startX, startY, DARK_MASK_LEVEL_FULL);
        LunaMap_DrawLayer(pMap, LAYER_LO_STRUCT, startX, startY, DARK_MASK_LEVEL_FULL);
        LunaMap_DrawLayer(pMap, LAYER_HI_STRUCT, startX, startY, DARK_MASK_LEVEL_FULL);
        break;
    }

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

void Luna_DrawMapMenu() {
    int startX = 8, startY = 16;
    int i;
    static const char StrMapHeader[] = "---Map Menu---";
    static const unsigned char ColorPlain = VGA_COLOR_BRIGHT_WHITE;
    static const unsigned char ColorHighlight = VGA_COLOR_GREEN;
    static const int MenuWidth = (sizeof(StrMapHeader) - 1) / sizeof(StrMapHeader[0]) * 6;

    if (!ui.map.isMenuOpened) {
        return;
    }

    Modi_FillRect(
        startX, startY,
        MenuWidth, (NumMapMenuItems + 1) * 8,
        ColorPlain
    );

    Modi_Print6x8(StrMapHeader, startX, startY, TRUE, ColorHighlight);

    for (i = 0; i < NumMapMenuItems; ++i) {
        int y = startY + (i + 1) * 8;
        if (i == ui.map.menuIndex) {
            Modi_FillRect(startX, y, MenuWidth, 8, ColorHighlight);
        }
        Modi_Print6x8(
            MapMenuItems[i].menuText,
            startX, y,
            FALSE,
            i == ui.map.menuIndex ? ColorPlain : ColorHighlight
        );
    }
}

void Luna_DrawTileSet(const LunaMap* pLunaMap) {
    int x, y;
    int i;
    int startRow, endRow;
    int startIndex, endIndex;
    int offset;
    int currentRow = ui.tile.tileIndex / ui.tile.colSize;
    char szBuf[100];
    
    startRow = currentRow - ui.tile.rowSize / 2;
    endRow = currentRow + ui.tile.rowSize;

    if (startRow < 0) startRow = 0;
    if (endRow - startRow > ui.tile.rowSize) endRow = ui.tile.rowSize + startRow;

    startIndex = startRow * ui.tile.colSize;
    endIndex = endRow * ui.tile.colSize - 1;

    if (startIndex < 0) startIndex = 0;
    if (endIndex >= pLunaMap->numTilemap) endIndex = pLunaMap->numTilemap - 1;

#if 0
    Modi_PlotLine(UI_TILESET_START_X, UI_TILESET_START_Y, UI_TILESET_START_X + UI_TILESET_CLIENT_W, UI_TILESET_START_Y, VGA_COLOR_LIGHT_BLUE);
    Modi_PlotLine(UI_TILESET_START_X + UI_TILESET_CLIENT_W, UI_TILESET_START_Y, UI_TILESET_START_X + UI_TILESET_CLIENT_W, UI_TILESET_START_Y + UI_TILESET_CLIENT_H, VGA_COLOR_LIGHT_BLUE);
    Modi_PlotLine(UI_TILESET_START_X + UI_TILESET_CLIENT_W, UI_TILESET_START_Y + UI_TILESET_CLIENT_H, UI_TILESET_START_X, UI_TILESET_START_Y + UI_TILESET_CLIENT_H, VGA_COLOR_LIGHT_BLUE);
    Modi_PlotLine(UI_TILESET_START_X, UI_TILESET_START_Y + UI_TILESET_CLIENT_H, UI_TILESET_START_X, UI_TILESET_START_Y, VGA_COLOR_LIGHT_BLUE);
#endif

    offset = ui.tile.tileIndex - startIndex;
    x = UI_TILESET_START_X + (offset % ui.tile.colSize) * UI_TILE_CELL_W;
    y = UI_TILESET_START_Y + (offset / ui.tile.colSize) * UI_TILE_CELL_H;

    Modi_PlotLine(x, y, x + UI_TILE_CELL_W, y, VGA_COLOR_CYAN);
    Modi_PlotLine(x + UI_TILE_CELL_W, y, x + UI_TILE_CELL_W, y + UI_TILE_CELL_H, VGA_COLOR_CYAN);
    Modi_PlotLine(x + UI_TILE_CELL_W, y + UI_TILE_CELL_H, x, y + UI_TILE_CELL_H, VGA_COLOR_CYAN);
    Modi_PlotLine(x, y + UI_TILE_CELL_H, x, y, VGA_COLOR_CYAN);

    for (i = startIndex; i <= endIndex; ++i) {
        offset = i - startIndex;
        sprintf(szBuf, "%03d", i);
        x = UI_TILESET_START_X + (offset % ui.tile.colSize) * UI_TILE_CELL_W;
        y = UI_TILESET_START_Y + (offset / ui.tile.colSize) * UI_TILE_CELL_H;
        Modi_Print4x6(szBuf, x + 1, y + 1, TRUE, VGA_COLOR_CYAN);
        if (i == 0) continue;
        Modi_DrawLunaSpriteInRange(
            pLunaMap->tilemap[i],
            x + 1, y + 7,
            SPRITE_MODE_NORMAL,
            0, 2, 0, 1
        );
    }

    if (ui.tile.tileIndex != 0) {
        sprintf(szBuf, "Tile #%d", ui.tile.tileIndex);
        x = UI_TILESET_START_X + UI_TILESET_CLIENT_W;
        y = UI_TILESET_START_Y;
        Modi_Print6x8(szBuf, x, y, FALSE, VGA_COLOR_BRIGHT_WHITE);
        Modi_DrawLunaSprite(
            pLunaMap->tilemap[ui.tile.tileIndex],
            x + 2, y + 10,
            SPRITE_MODE_NORMAL
        );
    }
    else {
        Modi_Print6x8("No Tile", UI_TILESET_START_X + UI_TILESET_CLIENT_W, UI_TILESET_START_Y, TRUE, VGA_COLOR_BRIGHT_WHITE);
    }
}

void Gopuzzle_Redraw() {
    char szBuf[100];
    
    Gongshu_ClearCanvas();

    switch (ui.state) {
    case UI_STATE_MAP_EDIT: 
        LunaMap_Draw(pLunaMap);
        Modi_FillRect(0, 0, screenWidth, 8, VGA_COLOR_CYAN);
        Modi_FillRect(0, screenHeight - 8, screenWidth, 8, VGA_COLOR_CYAN);
        sprintf(szBuf, "Map Editor | Tile #%d | Layer [%s]", ui.map.tileIndex, MapLayerName[ui.map.layer]);
        Modi_Print6x8(szBuf, 0, 0, FALSE, VGA_COLOR_BRIGHT_WHITE);
        Modi_Print4x6("A: Set / B: Menu / X: Switch Layer / Y: Select Tile", 104, screenHeight - 7, FALSE, VGA_COLOR_BRIGHT_WHITE);
        sprintf(szBuf, "Cursor (%03d,%03d)", ui.map.cursor.x, ui.map.cursor.y);
        Modi_Print6x8(szBuf, 0, screenHeight - 8, FALSE, VGA_COLOR_BRIGHT_WHITE);
        Luna_DrawMapMenu();
        break;
    case UI_STATE_TILE_SELECT:
        Modi_FillRect(0, 0, screenWidth, 8, VGA_COLOR_CYAN);
        Modi_FillRect(0, screenHeight - 8, screenWidth, 8, VGA_COLOR_CYAN);
        Modi_Print6x8("Select Tile", 0, 0, FALSE, VGA_COLOR_BRIGHT_WHITE);
        sprintf(szBuf, "Set Tile (%03d)", ui.tile.tileIndex);
        Modi_Print6x8(szBuf, 0, screenHeight - 8, FALSE, VGA_COLOR_BRIGHT_WHITE);
        Modi_Print4x6("A: Confirm / B: Map Editor", 120, screenHeight - 7, FALSE, VGA_COLOR_BRIGHT_WHITE);
        Luna_DrawTileSet(pLunaMap);
        break;
    case UI_STATE_SETTING:
        break; 
    }
}

void LunaMap_RecursiveFilling(LunaMap* pLunaMap, int layer, int x, int y, TileIndex originalIndex, TileIndex replaceIndex) {
    TileIndex idx;

    if (x < 0 || y < 0 || x >= pLunaMap->w || y >= pLunaMap->h) {
        return;
    }

    idx = MapElement(pLunaMap, layer, x, y);
    if (idx != originalIndex || idx == replaceIndex) {
        return;
    }

    MapElement(pLunaMap, layer, x, y) = replaceIndex;
    LunaMap_RecursiveFilling(pLunaMap, layer, x - 1, y, originalIndex, replaceIndex);
    LunaMap_RecursiveFilling(pLunaMap, layer, x, y -1, originalIndex, replaceIndex);
    LunaMap_RecursiveFilling(pLunaMap, layer, x + 1, y, originalIndex, replaceIndex);
    LunaMap_RecursiveFilling(pLunaMap, layer, x, y + 1, originalIndex, replaceIndex);
}

void Gopuzzle_HandleInput() {
    keyCode = Modi_WaitKey();
    switch (ui.state) {
    case UI_STATE_MAP_EDIT: 
        if (ui.map.isMenuOpened) {
            switch (keyCode) {
            case GSE_KEY_CODE_UP:       ui.map.menuIndex--; break;
            case GSE_KEY_CODE_DOWN:     ui.map.menuIndex++; break;
            case GSE_KEY_CODE_A: {
                int actionId = MapMenuItems[ui.map.menuIndex].menuActionId;
                switch (actionId) {
                case UI_MENU_FILL: {
                    if (ui.map.layer < LAYER_VIEW_ONLY) {
                        int x = ui.map.cursor.x;
                        int y = ui.map.cursor.y;
                        int layer = ui.map.layer;
                        TileIndex originalIndex = MapElement(pLunaMap, layer, x, y);
                        TileIndex replaceIndex = ui.map.tileIndex;
                        printf("layer %d x = %d, y = %d\n", layer, x, y);
                        LunaMap_RecursiveFilling(pLunaMap, layer, x, y, originalIndex, replaceIndex);
                        ui.map.isMenuOpened = FALSE;
                    }
                    break;
                }
                case UI_MENU_SETTINGS:
                    break;
                case UI_MENU_SAVE:
                    break;
                case UI_MENU_QUIT:
                    Modi_SetRunningFlag(FALSE);
                    break;
                }
                break;
            }
            case GSE_KEY_CODE_B:
                ui.map.isMenuOpened = FALSE;
                return;
            }
            if (ui.map.menuIndex < 0) ui.map.menuIndex = NumMapMenuItems - 1;
            else if (ui.map.menuIndex >= NumMapMenuItems) ui.map.menuIndex = 0;
        }
        else {
            switch (keyCode) {
            case GSE_KEY_CODE_UP:       ui.map.cursor.y--; break;
            case GSE_KEY_CODE_DOWN:     ui.map.cursor.y++; break;
            case GSE_KEY_CODE_LEFT:     ui.map.cursor.x--; break;
            case GSE_KEY_CODE_RIGHT:    ui.map.cursor.x++; break;
            case GSE_KEY_CODE_Y:
                ui.state = UI_STATE_TILE_SELECT;
                ui.tile.tileIndex = ui.map.tileIndex;
                return;
            case GSE_KEY_CODE_X:
                ui.map.layer++;
                if (ui.map.layer > LAYER_VIEW_ONLY) ui.map.layer = LAYER_TERRAIN;
                return;
            case GSE_KEY_CODE_B:
                ui.map.isMenuOpened = TRUE;
                return;
            case GSE_KEY_CODE_A:
                if (ui.map.layer == LAYER_VIEW_ONLY) {
                    return;
                }
                MapElement(pLunaMap, ui.map.layer, ui.map.cursor.x, ui.map.cursor.y) = ui.map.tileIndex;
                return;
            }
            if (ui.map.cursor.x < 0) ui.map.cursor.x = pLunaMap->w - 1;
            else if (ui.map.cursor.x >= pLunaMap->w) ui.map.cursor.x = 0;
            if (ui.map.cursor.y < 0) ui.map.cursor.y = pLunaMap->h - 1;
            else if (ui.map.cursor.y >= pLunaMap->h) ui.map.cursor.y = 0;
        }
        break;
    case UI_STATE_TILE_SELECT:
        switch (keyCode) {
        case GSE_KEY_CODE_UP:       ui.tile.tileIndex -= ui.tile.colSize; break;
        case GSE_KEY_CODE_DOWN:     ui.tile.tileIndex += ui.tile.colSize; break;
        case GSE_KEY_CODE_LEFT:     ui.tile.tileIndex--; break;
        case GSE_KEY_CODE_RIGHT:    ui.tile.tileIndex++; break;
        case GSE_KEY_CODE_A: {
            ui.map.tileIndex = ui.tile.tileIndex;
            ui.state = UI_STATE_MAP_EDIT;
            return;
        }
        case GSE_KEY_CODE_B:
            ui.state = UI_STATE_MAP_EDIT;
            return;
        }
        if (ui.tile.tileIndex < 0) ui.tile.tileIndex = pLunaMap->numTilemap - 1;
        if (ui.tile.tileIndex >= pLunaMap->numTilemap) ui.tile.tileIndex = 0;
        break;
    case UI_STATE_SETTING:
        break;
    }
}

int Gopuzzle_Main() {
    if (!Gongshu_Init()) {
        fprintf(stderr, "Failed to initialize Graphics\n");
        return -1;
    }

    Gongshu_GetResolution(&screenWidth, &screenHeight);

    pLunaMap = LunaMap_CreateTest();

    while (Modi_GetRunningFlag()) {        
        Gopuzzle_Redraw();
        Gopuzzle_HandleInput();
    }

    Gongshu_Dispose();
    LunaMap_Dispose(pLunaMap);

    return 0;
}