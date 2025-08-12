#ifndef _GOPUZZLE_
#define _GOPUZZLE_

#include "modi.h"
#include "../artifacts/jasmine89/jasmine.h"

typedef unsigned char TileIndex;

typedef struct {
    int             w;
    int             h;
    int             numTilemap;
    LunaSprite**    tilemap;
    TileIndex*      terrain;
    TileIndex*      loStruct;
    TileIndex*      hiStruct;
} LunaMap;

#define MapElement(pMap, layer, x, y) (layer[x + y * (pMap)->w])

#endif /* _GOPUZZLE_ */