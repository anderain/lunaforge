#ifndef _GONGSHU_LAYER_
#define _GONGSHU_LAYER_

#include "../artifacts/color-palette/vga_index.h"

#ifndef BOOL
#define BOOL int
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define GSE_TYPE_NONE           0x00
#define GSE_TYPE_QUIT           0x11
#define GSE_TYPE_KEY_DOWN       0x12
#define GSE_TYPE_KEY_UP         0x13
#define GSE_TYPE_MOUSE_DOWN     0x14
#define GSE_TYPE_MOUSE_UP       0x15
#define GSE_TYPE_MOUSE_MOVE     0x16

#define GSE_KEY_CODE_NONE       0x21
#define GSE_KEY_CODE_UP         0x22
#define GSE_KEY_CODE_DOWN       0x23
#define GSE_KEY_CODE_LEFT       0x24
#define GSE_KEY_CODE_RIGHT      0x25
#define GSE_KEY_CODE_A          0x26
#define GSE_KEY_CODE_B          0x27
#define GSE_KEY_CODE_X          0x28
#define GSE_KEY_CODE_Y          0x29

#define GSE_MOUSE_BUTTON_NONE   0x40
#define GSE_MOUSE_BUTTON_L      0x41
#define GSE_MOUSE_BUTTON_R      0x42
#define GSE_MOUSE_BUTTON_M      0x43

#define GS_DEFAULT_DELAY        33

typedef struct {
    int type;
    union {
        int keyCode;
        int mouseButtonCode;
        struct {
            int x;
            int y;
        } mousePosition;
    } param;
} Gongshu_Event;

int             Gongshu_Init            (void);
void            Gongshu_GetResolution   (int* pWidth, int *pHeight);
void            Gongshu_Flip            (void);
void            Gongshu_Dispose         (void);
void            Gongshu_SetPixel        (int x, int y, unsigned char colorIndex);
void            Gongshu_ClearCanvas     (void);
unsigned int    Gongshu_GetTicks        (void);
void            Gongshu_Delay           (unsigned int ms);
BOOL            Gongshu_FetchEvent      (Gongshu_Event* pEvent);
BOOL            Gongshu_LoadFile        (const char* filePath, unsigned char** pRaw, int* pLength);

#endif /* _GONGSHU_LAYER_ */