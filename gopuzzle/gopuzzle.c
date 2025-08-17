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
#define UI_MENU_LAYER           2
#define UI_MENU_TILE            3
#define UI_MENU_SPRITE_MODE     4
#define UI_MENU_SAVE            5
#define UI_MENU_SETTINGS        6
#define UI_MENU_QUIT            7

static const char* MapLayerName[] = { "Terrain", "LoStruct", "HiStruct", "View Only", "Passability" };
static const char* SpriteModeName[] = { "VGA Color", "Monochrome", "Grayscale" };
static const struct {
    int menuActionId;
    char* menuText;
} MapMenuItems[] = {
    { UI_MENU_FILL,         "Fill Area" },
    { UI_MENU_LAYER,        "Layer"     },
    { UI_MENU_TILE,         "Tile"      },
    { UI_MENU_SPRITE_MODE,  "Sprite"    } ,
    { UI_MENU_SAVE,         "Save"      },
    { UI_MENU_SETTINGS,     "Settings"  },
    { UI_MENU_QUIT,         "Quit"      }
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
        int spriteMode;
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
        /* cursor       */ { 0, 0 },
        /* spriteMode   */ SPRITE_MODE_NORMAL
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
    int             numTile = 8;
    int             mapByteSize = w * h * sizeof(TileIndex);

    pMap = (LunaMap *)malloc(sizeof(LunaMap));
    pMap->w = w;
    pMap->h = h;
    pMap->numTile                   = numTile;
    pMap->tile                      = (LunaSprite **)malloc(numTile * sizeof(LunaSprite*));
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

    pMap->tile[0] = NULL; /* tile[0] 用永远是 0 */

    TestLoadSprite("inn_wdnflr0", &pSprite);
    pMap->tile[1] = pSprite;
    TestLoadSprite("inn_screen0", &pSprite);
    pMap->tile[2] = pSprite;
    TestLoadSprite("inn_screen1", &pSprite);
    pMap->tile[3] = pSprite;
    TestLoadSprite("inn_wood0", &pSprite);
    pMap->tile[4] = pSprite;
    TestLoadSprite("inn_chair0", &pSprite);
    pMap->tile[5] = pSprite;
    TestLoadSprite("inn_screen2", &pSprite);
    pMap->tile[6] = pSprite;
    TestLoadSprite("inn_bed0", &pSprite);
    pMap->tile[7] = pSprite;

    return pMap;
}

void LunaMap_Dispose(LunaMap* pMap) {
    int i;
    /* 跳过第一个，释放其他 tile sprite */
    for (i = 1; i < pMap->numTile; ++i) {
        LunaSprite* pSprite = pMap->tile[i];
        if (pSprite) {
            Modi_DisposeLunaSprite(pSprite);
        }
    }
    free(pMap->tile);
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
            pSprite = pMap->tile[tileIndex];
            if (NULL == pSprite) continue;
            dx = startX + x * HALF_TILE_WIDTH - y * HALF_TILE_WIDTH - (pSprite->width - TILE_WIDTH) / 2;
            dy = startY + x * HALF_TILE_HEIGHT + y * HALF_TILE_HEIGHT + TILE_HEIGHT - pSprite->height;
            Modi_DrawLunaSpriteWithMask(pSprite, dx, dy, ui.map.spriteMode, maskLevel);
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
    case LAYER_PASSABILITY:
        LunaMap_DrawLayer(pMap, LAYER_TERRAIN, startX, startY, DARK_MASK_LEVEL_FULL);
        LunaMap_DrawLayer(pMap, LAYER_LO_STRUCT, startX, startY, DARK_MASK_LEVEL_FULL);
        LunaMap_DrawLayer(pMap, LAYER_HI_STRUCT, startX, startY, DARK_MASK_LEVEL_FULL);
        /* TODO: Draw passability here */
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
    static const char StrMapHeader[] = "       Map Menu       ";
    static const uchar ColorPlain = VGA_COLOR_CYAN;
    static const uchar ColorHighlight = VGA_COLOR_BRIGHT_WHITE;
    static const int itemHeight = 12;
    static const int MenuWidth = (sizeof(StrMapHeader) - 1) / sizeof(StrMapHeader[0]) * 6;
    int startX = (screenWidth - MenuWidth) / 2, startY = 16, i, actionId;
    char szBuf[50];

    if (!ui.map.isMenuOpened) {
        return;
    }

    /* Shadow and box */
    Modi_FillRect(startX + 2, startY + 2, MenuWidth, (NumMapMenuItems + 1) * itemHeight, 0xc3);
    Modi_FillRect(startX, startY, MenuWidth, (NumMapMenuItems + 1) * itemHeight, ColorPlain);

    /* Header */
    Modi_FillRect(startX, startY, MenuWidth, itemHeight, VGA_COLOR_LIGHT_CYAN);
    Modi_Print6x8((const uchar *)StrMapHeader, startX, startY + 2, FALSE, VGA_COLOR_BLACK);

    for (i = 0; i < NumMapMenuItems; ++i) {
        int top = startY + (i + 1) * itemHeight;
        int left = startX;
        actionId = MapMenuItems[i].menuActionId;
        if (i == ui.map.menuIndex) {
            Modi_FillRect(left, top, MenuWidth, itemHeight, ColorHighlight);
        }
        top += 2;
        left += 3;
        switch (actionId) {
        case UI_MENU_LAYER:
            sprintf(szBuf, "%s \x11[%11s]\x10", MapMenuItems[i].menuText, MapLayerName[ui.map.layer]);
            Modi_Print6x8(
                (const uchar *)szBuf,
                left, top,
                FALSE,
                i == ui.map.menuIndex ? ColorPlain : ColorHighlight
            );
            break;
        case UI_MENU_TILE:
            sprintf(szBuf, "%s          \x11[%03d]\x10", MapMenuItems[i].menuText, ui.map.tileIndex);
            Modi_Print6x8(
                (const uchar *)szBuf,
                left, top,
                FALSE,
                i == ui.map.menuIndex ? ColorPlain : ColorHighlight
            );
            break;
        case UI_MENU_SPRITE_MODE:
            sprintf(szBuf, "%s \x11[%10s]\x10", MapMenuItems[i].menuText, SpriteModeName[ui.map.spriteMode]);
            Modi_Print6x8(
                (const uchar *)szBuf,
                left, top,
                FALSE,
                i == ui.map.menuIndex ? ColorPlain : ColorHighlight
            );
            break;
        default:
            Modi_Print6x8(
                (const uchar *)MapMenuItems[i].menuText,
                left, top,
                FALSE,
                i == ui.map.menuIndex ? ColorPlain : ColorHighlight
            );
        }
    }

    actionId = MapMenuItems[ui.map.menuIndex].menuActionId;
    if (actionId == UI_MENU_TILE) {
        int left = startX + MenuWidth + 4;
        int top = startY;
        if (ui.map.tileIndex == 0) {
            Modi_Print6x8((const uchar*) "No Tile", left, top, TRUE, VGA_COLOR_BRIGHT_WHITE);
        }
        else {
            LunaSprite* pSprite = pLunaMap->tile[ui.map.tileIndex];
            Modi_FillRect(left, top, pSprite->width + 2, pSprite->height + 2, VGA_COLOR_BRIGHT_WHITE);
            Modi_FillRectWithMask(left, top, pSprite->width + 2, pSprite->height + 2, VGA_COLOR_WHITE, DARK_MASK_LEVEL_MEDIUM);
            Modi_DrawLunaSprite(
                pSprite,
                left + 1, top + 1,
                SPRITE_MODE_NORMAL
            );
        }
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
    if (endIndex >= pLunaMap->numTile) endIndex = pLunaMap->numTile - 1;

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
            pLunaMap->tile[i],
            x + 1, y + 7,
            SPRITE_MODE_NORMAL,
            0, 2, 0, 1
        );
    }

    if (ui.tile.tileIndex != 0) {
        LunaSprite* pSprite = pLunaMap->tile[ui.tile.tileIndex];
        sprintf(szBuf, "Tile #%d", ui.tile.tileIndex);
        x = UI_TILESET_START_X + UI_TILESET_CLIENT_W;
        y = UI_TILESET_START_Y;
        Modi_Print6x8((const uchar *)szBuf, x, y, FALSE, VGA_COLOR_BRIGHT_WHITE);
        x += 2;
        y += 10;
        Modi_FillRect(x, y, pSprite->width + 2, pSprite->height + 2, VGA_COLOR_BRIGHT_WHITE);
        Modi_FillRectWithMask(x, y, pSprite->width + 2, pSprite->height + 2, VGA_COLOR_WHITE, DARK_MASK_LEVEL_MEDIUM);
        Modi_DrawLunaSprite(pSprite, x + 1, y + 1, SPRITE_MODE_NORMAL);
    }
    else {
        Modi_Print6x8((const uchar *)"No Tile", UI_TILESET_START_X + UI_TILESET_CLIENT_W, UI_TILESET_START_Y, TRUE, VGA_COLOR_BRIGHT_WHITE);
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
        Modi_Print6x8((const uchar *)szBuf, 0, 0, FALSE, VGA_COLOR_BRIGHT_WHITE);
        Modi_Print4x6("A: Set / B: Menu / X: Switch Layer / Y: Select Tile", 104, screenHeight - 7, FALSE, VGA_COLOR_BRIGHT_WHITE);
        sprintf(szBuf, "Cursor (%03d,%03d)", ui.map.cursor.x, ui.map.cursor.y);
        Modi_Print6x8((const uchar *)szBuf, 0, screenHeight - 8, FALSE, VGA_COLOR_BRIGHT_WHITE);
        Luna_DrawMapMenu();
        break;
    case UI_STATE_TILE_SELECT:
        Modi_FillRect(0, 0, screenWidth, 8, VGA_COLOR_CYAN);
        Modi_FillRect(0, screenHeight - 8, screenWidth, 8, VGA_COLOR_CYAN);
        Modi_Print6x8((const uchar *)"Select Tile", 0, 0, FALSE, VGA_COLOR_BRIGHT_WHITE);
        sprintf(szBuf, "Set Tile (%03d)", ui.tile.tileIndex);
        Modi_Print6x8((const uchar *)szBuf, 0, screenHeight - 8, FALSE, VGA_COLOR_BRIGHT_WHITE);
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
    LunaMap_RecursiveFilling(pLunaMap, layer, x, y - 1, originalIndex, replaceIndex);
    LunaMap_RecursiveFilling(pLunaMap, layer, x + 1, y, originalIndex, replaceIndex);
    LunaMap_RecursiveFilling(pLunaMap, layer, x, y + 1, originalIndex, replaceIndex);
}

void Gopuzzle_HandleInput() {
    keyCode = Modi_WaitKey();
    switch (ui.state) {
    case UI_STATE_MAP_EDIT: 
        if (ui.map.isMenuOpened) {
            int actionId = MapMenuItems[ui.map.menuIndex].menuActionId;
            switch (keyCode) {
            case GSE_KEY_CODE_UP:       ui.map.menuIndex--; break;
            case GSE_KEY_CODE_DOWN:     ui.map.menuIndex++; break;
            case GSE_KEY_CODE_LEFT:
                if (actionId == UI_MENU_LAYER) {
                    ui.map.layer--;
                    if (ui.map.layer < LAYER_LOOP_MIN) ui.map.layer = LAYER_LOOP_MAX;
                }
                else if (actionId == UI_MENU_TILE) {
                    ui.map.tileIndex--;
                    if (ui.map.tileIndex < 0) {
                        ui.map.tileIndex = pLunaMap->numTile - 1;
                    }
                }
                else if (actionId == UI_MENU_SPRITE_MODE) {
                    ui.map.spriteMode--;
                    if (ui.map.spriteMode < SPRITE_MODE_NORMAL) ui.map.spriteMode = SPRITE_MODE_BLACK_WHITE_GRAY;
                }
                break;
            case GSE_KEY_CODE_RIGHT:
                if (actionId == UI_MENU_LAYER) {
                    ui.map.layer++;
                    if (ui.map.layer > LAYER_LOOP_MAX) ui.map.layer = LAYER_LOOP_MIN;
                }
                else if (actionId == UI_MENU_TILE) {
                    ui.map.tileIndex++;
                    if (ui.map.tileIndex >= pLunaMap->numTile) {
                        ui.map.tileIndex = 0;
                    }
                }
                else if (actionId == UI_MENU_SPRITE_MODE) {
                    ui.map.spriteMode++;
                    if (ui.map.spriteMode > SPRITE_MODE_BLACK_WHITE_GRAY) ui.map.spriteMode = SPRITE_MODE_NORMAL;
                }
                break;
            case GSE_KEY_CODE_A: {
                switch (actionId) {
                case UI_MENU_FILL: {
                    if (isLayerEditable(ui.map.layer)) {
                        int x = ui.map.cursor.x;
                        int y = ui.map.cursor.y;
                        int layer = ui.map.layer;
                        TileIndex originalIndex = MapElement(pLunaMap, layer, x, y);
                        TileIndex replaceIndex = ui.map.tileIndex;
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
                if (ui.map.layer > LAYER_LOOP_MAX) ui.map.layer = LAYER_LOOP_MIN;
                return;
            case GSE_KEY_CODE_B:
                ui.map.isMenuOpened = TRUE;
                return;
            case GSE_KEY_CODE_A:
                if (isLayerEditable(ui.map.layer)) {
                    MapElement(pLunaMap, ui.map.layer, ui.map.cursor.x, ui.map.cursor.y) = ui.map.tileIndex;
                }
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
        if (ui.tile.tileIndex < 0) ui.tile.tileIndex = pLunaMap->numTile - 1;
        if (ui.tile.tileIndex >= pLunaMap->numTile) ui.tile.tileIndex = 0;
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