#ifndef _GOPUZZLE_
#define _GOPUZZLE_

#include "modi.h"
#include "../artifacts/jasmine89/jasmine.h"

typedef unsigned char TileIndex;

#define LAYER_TERRAIN       0
#define LAYER_LO_STRUCT     1
#define LAYER_HI_STRUCT     2
#define LAYER_VIEW_ONLY     3
#define LAYER_PASSABILITY   4
#define LAYER_LOOP_MIN      LAYER_TERRAIN
#define LAYER_LOOP_MAX      LAYER_PASSABILITY

#define isLayerEditable(layer)  ((layer) == LAYER_TERRAIN || (layer) == LAYER_LO_STRUCT || (layer) == LAYER_HI_STRUCT)

typedef struct {
    int             w;
    int             h;
    int             numTile;
    LunaSprite**    tile;
    TileIndex*      layer[3];
} LunaMap;

#define MapElement(pMap, layerIndex, x, y) ((pMap)->layer[layerIndex][x + y * (pMap)->w])

#define TILE_WIDTH              24
#define TILE_HEIGHT             11
#define HALF_TILE_WIDTH         12
#define HALF_TILE_HEIGHT        6
#define TILE_HIGHLIGHT_COLOR    VGA_COLOR_LIGHT_BLUE

#endif /* _GOPUZZLE_ */