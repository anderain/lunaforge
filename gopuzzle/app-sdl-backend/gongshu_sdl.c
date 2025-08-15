#include <stdio.h>
#include <assert.h>
#include <SDL/SDL.h>
#include "../gongshu.h"
#include "../../artifacts/color-palette/vga_palette.h"

#define WINDOW_COLOR_DEPTH  32
#define WINDOW_ZOOM         2
#define COLOR_BLACK         0x000000
#define CANVAS_WIDTH        320
#define CANVAS_HEIGHT       200

static SDL_Surface* sfScreen = NULL;
static SDL_Surface* sfCanvas = NULL;

static void Intl_CreateCanvas(void) {
    Uint32 rmask, gmask, bmask, amask;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;
#else
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
#endif
    sfCanvas = SDL_CreateRGBSurface(SDL_HWSURFACE, CANVAS_WIDTH, CANVAS_HEIGHT, WINDOW_COLOR_DEPTH, rmask, gmask, bmask, amask);
}

static Uint32 Intl_GetPixelFromSurface(SDL_Surface* sf, int x, int y) {
    unsigned char* row8;
    unsigned short* row16;
    unsigned int* row32;
    int bytesPerPixel = sf->format->BytesPerPixel;
    int pitch = sf->pitch;
    
    if (x < 0 || x >= sf->w || y < 0 || y >= sf->h) {
        return COLOR_BLACK;
    }

    switch (bytesPerPixel) {
    case 1:
        row8 = (unsigned char *)((unsigned char *)sf->pixels + y * pitch + x * bytesPerPixel);
        return *row8;

    case 2:
        row16 = (unsigned short *)((unsigned char *)sf->pixels + y * pitch + x * bytesPerPixel);
        return *row16;

    case 4:
        row32 = (unsigned int *)((unsigned char *)sf->pixels + y * pitch + x * bytesPerPixel);
        return *row32;
    }

    return COLOR_BLACK;
}

static void Intl_SetPixelToSurface(SDL_Surface *sf, int x, int y, Uint32 color) {
    unsigned char *row8;
    unsigned short *row16;
    unsigned int *row32;
    int bytesPerPixel = sf->format->BytesPerPixel;
    int pitch = sf->pitch;
    
    if (x < 0 || x >= sf->w || y < 0 || y >= sf->h) {
        return;
    }

    switch (bytesPerPixel) {
    case 1:
        row8 = (unsigned char *)((unsigned char *)sf->pixels + y * pitch + x * bytesPerPixel);
        *row8 = (unsigned char) color;
        break;

    case 2:
        row16 = (unsigned short *)((unsigned char *)sf->pixels + y * pitch + x * bytesPerPixel);
        *row16 = (unsigned short) color;
        break;

    case 4:
        row32 = (unsigned int *)((unsigned char *)sf->pixels + y * pitch + x * bytesPerPixel);
        *row32 = (unsigned int) color;
        break;
    }
}

void Gongshu_ClearCanvas(void) {
    SDL_FillRect(sfCanvas, NULL, COLOR_BLACK);
}

void Gongshu_SetPixel(int x, int y, unsigned char colorIndex) {
    Intl_SetPixelToSurface(sfCanvas, x, y, VgaMode13hColorPalette[colorIndex]);
}

int Gongshu_Init(void) {
    int winWidth, winHeight;
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        return 0;
    }
    winWidth = WINDOW_ZOOM * CANVAS_WIDTH;
    winHeight = winWidth * 3 / 4;
    sfScreen = SDL_SetVideoMode(winWidth, winHeight, WINDOW_COLOR_DEPTH, SDL_HWSURFACE);
    if (sfScreen == NULL) {
        return 0;
    }
    SDL_WM_SetCaption("Gopuzzle - Lunaforge", NULL);
    Intl_CreateCanvas();
    return 1;
}

void Gongshu_GetResolution (int* pWidth, int *pHeight) {
    if (pWidth) *pWidth = CANVAS_WIDTH;
    if (pHeight) *pHeight = CANVAS_HEIGHT;
}

void Gongshu_Flip(void) {
    int x, y, i, j, left, top;

    SDL_FillRect(sfScreen, NULL, 0x303030);

    left = (sfScreen->w - sfCanvas->w * WINDOW_ZOOM) / 2;
    top = (sfScreen->h - sfCanvas->h * WINDOW_ZOOM) / 2;

    /* Copy canvas to screen */
    for (y = 0; y < sfCanvas->h; ++y) {
        for (x = 0; x < sfCanvas->w; ++x) {
            Uint32 color = Intl_GetPixelFromSurface(sfCanvas, x, y);
            for (i = 0; i < WINDOW_ZOOM; ++i) {
                for (j = 0; j < WINDOW_ZOOM; ++j) {
                    Intl_SetPixelToSurface(
                        sfScreen,
                        left + x * WINDOW_ZOOM + j,
                        top + y * WINDOW_ZOOM + i,
                        color
                    );
                }
            }
        }
    }

    SDL_Flip(sfScreen);
}

void Gongshu_Dispose(void) {
    if (!sfScreen) {
        return;
    }
    SDL_Quit();
}

int Intl_MapKey(SDLKey sym) {
    switch (sym) {
    case SDLK_UP:       return GSE_KEY_CODE_UP;
    case SDLK_DOWN:     return GSE_KEY_CODE_DOWN;
    case SDLK_LEFT:     return GSE_KEY_CODE_LEFT; 
    case SDLK_RIGHT:    return GSE_KEY_CODE_RIGHT;
    case SDLK_z:        return GSE_KEY_CODE_A;
    case SDLK_x:        return GSE_KEY_CODE_B;
    case SDLK_a:        return GSE_KEY_CODE_X;
    case SDLK_s:        return GSE_KEY_CODE_Y;
    default:
        return GSE_KEY_CODE_NONE;
    }
}

int Intl_MapMouseButton(Uint8 button) {
    switch (button) {
    case SDL_BUTTON_LEFT:   return GSE_MOUSE_BUTTON_L;
    case SDL_BUTTON_RIGHT:  return GSE_MOUSE_BUTTON_R;
    case SDL_BUTTON_MIDDLE: return GSE_MOUSE_BUTTON_M; 
    default:
        return GSE_MOUSE_BUTTON_NONE;
    }
}

BOOL Gongshu_FetchEvent(Gongshu_Event* pEvent) {
    SDL_Event sdlEvent;

    if (!SDL_PollEvent(&sdlEvent)) {
        return FALSE;
    }
    
    memset(pEvent, 0, sizeof(Gongshu_Event));

    switch (sdlEvent.type) {
        case SDL_QUIT:
            pEvent->type = GSE_TYPE_QUIT;
            return TRUE;
        case SDL_KEYDOWN: {
            int keyCode = Intl_MapKey(sdlEvent.key.keysym.sym);
            if (keyCode == GSE_KEY_CODE_NONE) {
                return FALSE;
            }
            pEvent->type = GSE_TYPE_KEY_DOWN;
            pEvent->param.keyCode = keyCode;
            return TRUE;
        }
        case SDL_KEYUP: {
            int keyCode = Intl_MapKey(sdlEvent.key.keysym.sym);
            if (keyCode == GSE_KEY_CODE_NONE) {
                return FALSE;
            }
            pEvent->type = GSE_TYPE_KEY_UP;
            pEvent->param.keyCode = Intl_MapKey(sdlEvent.key.keysym.sym);
            return TRUE;
        }
        case SDL_MOUSEMOTION:
            pEvent->type = GSE_TYPE_MOUSE_MOVE;
            pEvent->param.mousePosition.x = sdlEvent.motion.x;
            pEvent->param.mousePosition.y = sdlEvent.motion.y;
            return TRUE;
        case SDL_MOUSEBUTTONDOWN:
            pEvent->type = GSE_TYPE_MOUSE_DOWN;
            pEvent->param.mouseButtonCode = Intl_MapMouseButton(sdlEvent.button.button);
            return TRUE;
        case SDL_MOUSEBUTTONUP:
            pEvent->type = GSE_TYPE_MOUSE_UP;
            pEvent->param.mouseButtonCode = Intl_MapMouseButton(sdlEvent.button.button);
            return TRUE; 
        default:
            return FALSE;
    }

    return FALSE;
}

unsigned int Gongshu_GetTicks(void) {
    return SDL_GetTicks();
}

void Gongshu_Delay(unsigned int ms) {
    SDL_Delay(ms);
}

BOOL Gongshu_LoadFile(const char* filePath, unsigned char** pRaw, int* pLength) {
    FILE* fp;

    assert(pRaw != NULL);
    assert(pLength != NULL);

    fp = fopen(filePath, "rb");

    if (!fp) {
        return FALSE;
    }
    
    if (fseek(fp, 0, SEEK_END)) {
        fclose(fp);
        return FALSE;
    }
    
    long fileSize = ftell(fp);
    if (fileSize <= 0) {
        fclose(fp);
        return FALSE;
    }
    
    rewind(fp);
    
    *pRaw = (unsigned char*)malloc(fileSize);
    if (!*pRaw) {
        fclose(fp);
        return FALSE;
    }
    
    size_t readSize = fread(*pRaw, 1, fileSize, fp);
    if (readSize != (size_t)fileSize) {
        free(*pRaw);
        *pRaw = NULL;
        fclose(fp);
        return FALSE;
    }
    
    *pLength = (int)fileSize;
    fclose(fp);
    return TRUE;
}