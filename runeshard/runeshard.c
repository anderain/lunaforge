/**
 * @file    runeshard.c
 * @brief   实现BMP格式文件转换核心功能
 * 
 * @details 本文件实现了24位BMP转为8x8 Sprite图集合的功能
 * 
 * @author  锻月楼主
 * @date    2025-07-17
 * @version 0.1
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* .bmp 文件读取结果 */
#define BMP_SUCCESS             0 /* 读取成功 */
#define BMP_READ_FAILED         1 /* 读文件失败 */
#define BMP_INVALID_HEADER      2 /* 非法文件头 */
#define BMP_INVALID_FORMAT      3 /* 非法颜色格式 */
#define BMP_INVALID_WIDTH       4 /* 非法宽度（非8倍数） */

/* .meta.txt 文件读取结果 */
#define META_SUCCESS            0 /* 读取成功 */
#define META_READ_FAILED        1 /* 读文件失败 */
#define META_COLOR_EXCEED       2 /* 颜色多于三种 */
#define META_INVALID            3 /* 格式非法 */

/* 颜色模式 */
#define COLOR_MODE_DEFAULT      0 /* 默认调色板模式，黑/白/指定颜色 */
#define COLOR_MODE_SPECIFIED    1 /* 指定模式，指定三种不同颜色 */

/* 输出格式 */
#define OUTPUT_FORMAT_C_CODE    0 /* 输出 8x8 Sprite集合，为 C 数组格式 */
#define OUTPUT_FORMAT_JSON      1 /* 输出 8x8 Sprite集合，为 JSON 数组格式 */
#define OUTPUT_FORMAT_C_EMBED   2 /* 输出 8bit 调色板位图不分割，为 C 数组格式 */
#define OUTPUT_FORMAT_BINARY    3 /* 输出到文件，二进制格式 */

/* BMP 文件头 */
typedef struct {
    unsigned short  bfType;     /* 标准文件头，应为两 char 'B' 'M' 开头 */
    unsigned int    bfSize;     /* BMP 文件总尺寸 */
    unsigned int    bfReserved; /* 保留字段永远是 0 */
    unsigned int    bfOffBits;  /* 指定从位图文件头到位图位的偏移量（以字节为单位 */
} BitmapFileHeader;

/* BMP 信息头 */
typedef struct {
    unsigned int    biSize;             /* 本结构的字节长度 */
    int             biWidth;            /* 图片宽度 */
    int             biHeight;           /* 图片高度 */
    unsigned short  biPlanes;           /* 指定颜色平面的数量，必须为 1 */
    unsigned short  biBitCount;         /* 指定每像素的位数，此程序只接受24 */
    unsigned int    biCompression;      /* 是否压缩，此程序只接受0 */
    unsigned int    biSizeImage;        /* 图片大小的字节数 */
    int             biXPelsPerMeter;    /* x 轴每米像素数 */
    int             biYPelsPerMeter;    /* y 轴每米像素数 */
    unsigned int    biClrUsed;          /* 位图使用的颜色数量 */
    unsigned int    biClrImportant;     /* 指定对显示位图重要的颜色索引数 */
} BitmapInfoHeader;

/* 读取的 24bit BMP 信息 */
typedef struct {
    int             width;  /* 图片宽度 */
    int             height; /* 图片高度 */
    unsigned char   raw[];  /* 存储RGB888的字节数组 */
} RawBitmap;

/* 输出 Sprite 的元数据 */
typedef struct {
    unsigned char color[3][3];  /* 目标 Sprite 的调色板，三种颜色的RGB值 */
    int colorIndex[3];          /* 目标 Sprite 的调色板，三种颜色在 VGA 13H 模式下的颜色索引 */
} Metadata;

/* 输出 Sprite */
typedef struct {
    int size;               /* 像素内容的字节长度 */
    int hrzCount;           /* 水平方向 Sprite 数量 */
    int vrtCount;           /* 垂直方向 Sprite 数量 */
    int hrzPadding;         /* 水平方向上填充了多少像素来补足 8 个pixel */
    int vrtPadding;         /* 垂直方向上填充了多少像素来补足 8 个pixel */
    unsigned char raw[];    /* 已经编码过的 Sprite */
} LunaBitmapResult;

/* CLI 参数 */
struct {
    const char* fileName;   /* 输入文件名 */
    const char* outName;    /* 输出文件名 */
    const char* metaName;   /* 元数据文件名 */
    int isOutputBinary;     /* 输出文件是否是二进制 */
    int debugMode;          /* 是否启动 DEBUG 输出 */
    int outputFormat;       /* 输出格式 */
} cliParams = { NULL, NULL, NULL, 0, 0, OUTPUT_FORMAT_C_CODE };

/* VGA 13H 模式的调色板对应的颜色值 */
const unsigned int VgaMode13hColorPalette[] = {
    0x000000, 0x0000AA, 0x00AA00, 0x00AAAA, 0xAA0000, 0xAA00AA, 0xAA5500, 0xAAAAAA,
    0x555555, 0x5555FF, 0x55FF55, 0x55FFFF, 0xFF5555, 0xFF55FF, 0xFFFF55, 0xFFFFFF,
    0x000000, 0x101010, 0x202020, 0x353535, 0x454545, 0x555555, 0x656565, 0x757575,
    0x8A8A8A, 0x9A9A9A, 0xAAAAAA, 0xBABABA, 0xCACACA, 0xDFDFDF, 0xEFEFEF, 0xFFFFFF,
    0x0000FF, 0x4100FF, 0x8200FF, 0xBE00FF, 0xFF00FF, 0xFF00BE, 0xFF0082, 0xFF0041,
    0xFF0000, 0xFF4100, 0xFF8200, 0xFFBE00, 0xFFFF00, 0xBEFF00, 0x82FF00, 0x41FF00,
    0x00FF00, 0x00FF41, 0x00FF82, 0x00FFBE, 0x00FFFF, 0x00BEFF, 0x0082FF, 0x0041FF,
    0x8282FF, 0x9E82FF, 0xBE82FF, 0xDF82FF, 0xFF82FF, 0xFF82DF, 0xFF82BE, 0xFF829E,
    0xFF8282, 0xFF9E82, 0xFFBE82, 0xFFDF82, 0xFFFF82, 0xDFFF82, 0xBEFF82, 0x9EFF82,
    0x82FF82, 0x82FF9E, 0x82FFBE, 0x82FFDF, 0x82FFFF, 0x82DFFF, 0x82BEFF, 0x829EFF,
    0xBABAFF, 0xCABAFF, 0xDFBAFF, 0xEFBAFF, 0xFFBAFF, 0xFFBAEF, 0xFFBADF, 0xFFBACA,
    0xFFBABA, 0xFFCABA, 0xFFDFBA, 0xFFEFBA, 0xFFFFBA, 0xEFFFBA, 0xDFFFBA, 0xCAFFBA,
    0xBAFFBA, 0xBAFFCA, 0xBAFFDF, 0xBAFFEF, 0xBAFFFF, 0xBAEFFF, 0xBADFFF, 0xBACAFF,
    0x000071, 0x1C0071, 0x390071, 0x550071, 0x710071, 0x710055, 0x710039, 0x71001C,
    0x710000, 0x711C00, 0x713900, 0x715500, 0x717100, 0x557100, 0x397100, 0x1C7100,
    0x007100, 0x00711C, 0x007139, 0x007155, 0x007171, 0x005571, 0x003971, 0x001C71,
    0x393971, 0x453971, 0x553971, 0x613971, 0x713971, 0x713961, 0x713955, 0x713945,
    0x713939, 0x714539, 0x715539, 0x716139, 0x717139, 0x617139, 0x557139, 0x457139,
    0x397139, 0x397145, 0x397155, 0x397161, 0x397171, 0x396171, 0x395571, 0x394571,
    0x515171, 0x595171, 0x615171, 0x695171, 0x715171, 0x715169, 0x715161, 0x715159,
    0x715151, 0x715951, 0x716151, 0x716951, 0x717151, 0x697151, 0x617151, 0x597151,
    0x517151, 0x517159, 0x517161, 0x517169, 0x517171, 0x516971, 0x516171, 0x515971,
    0x000041, 0x100041, 0x200041, 0x310041, 0x410041, 0x410031, 0x410020, 0x410010,
    0x410000, 0x411000, 0x412000, 0x413100, 0x414100, 0x314100, 0x204100, 0x104100,
    0x004100, 0x004110, 0x004120, 0x004131, 0x004141, 0x003141, 0x002041, 0x001041,
    0x202041, 0x282041, 0x312041, 0x392041, 0x412041, 0x412039, 0x412031, 0x412028,
    0x412020, 0x412820, 0x413120, 0x413920, 0x414120, 0x394120, 0x314120, 0x284120,
    0x204120, 0x204128, 0x204131, 0x204139, 0x204141, 0x203941, 0x203141, 0x202841,
    0x2D2D41, 0x312D41, 0x352D41, 0x3D2D41, 0x412D41, 0x412D3D, 0x412D35, 0x412D31,
    0x412D2D, 0x41312D, 0x41352D, 0x413D2D, 0x41412D, 0x3D412D, 0x35412D, 0x31412D,
    0x2D412D, 0x2D4131, 0x2D4135, 0x2D413D, 0x2D4141, 0x2D3D41, 0x2D3541, 0x2D3141,
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000
};

/* VGA 13H 模式调色板大小 */
const int SizeOfPalette = sizeof(VgaMode13hColorPalette) / sizeof(VgaMode13hColorPalette[0]);

/**
 * @brief 将 RGB 颜色值转换为 CIEXYZ 颜色空间。
 *
 * 该函数接收一个 RGB 颜色值（每个分量 0~255），进行 gamma 校正后，
 * 使用线性变换矩阵将其转换为 CIE 1931 XYZ 颜色空间下的三刺激值。
 *
 * @param r 红色分量（0~255）
 * @param g 绿色分量（0~255）
 * @param b 蓝色分量（0~255）
 * @param x 指向输出 X 分量的指针
 * @param y 指向输出 Y 分量的指针
 * @param z 指向输出 Z 分量的指针
 *
 * @note 使用的是基于 sRGB 和 D65 白点的标准转换矩阵。
 */
void rgb2xyz(unsigned char r, unsigned char g, unsigned char b, double* x, double* y, double* z) {
    double rn = r / 255.0;
    double gn = g / 255.0;
    double bn = b / 255.0;
    rn = (rn <= 0.04045) ? rn / 12.92 : pow((rn + 0.055) / 1.055, 2.4);
    gn = (gn <= 0.04045) ? gn / 12.92 : pow((gn + 0.055) / 1.055, 2.4);
    bn = (bn <= 0.04045) ? bn / 12.92 : pow((bn + 0.055) / 1.055, 2.4);
    *x = rn * 0.4124564 + gn * 0.3575761 + bn * 0.1804375;
    *y = rn * 0.2126729 + gn * 0.7151522 + bn * 0.0721750;
    *z = rn * 0.0193339 + gn * 0.1191920 + bn * 0.9503041;
}

/**
 * @brief 将 CIEXYZ 颜色空间的值转换为 CIELAB 颜色空间。
 *
 * 该函数将基于 D65 白点的 XYZ 值转换为 Lab 空间的 L、a、b 分量。
 * 转换过程中使用了非线性变换以模拟人眼感知特性。
 *
 * @param x 输入的 X 分量
 * @param y 输入的 Y 分量
 * @param z 输入的 Z 分量
 * @param l 指向输出 L（明度）分量的指针
 * @param a 指向输出 a（红-绿轴）分量的指针
 * @param b 指向输出 b（黄-蓝轴）分量的指针
 *
 * @note 使用参考白点为 D65（X=0.95047, Y=1.00000, Z=1.08883）。
 */
void xyz2lab(double x, double y, double z, double* l, double* a, double* b) {
    /* D65 */
    double refX = 0.95047;
    double refY = 1.0;
    double refZ = 1.08883;
    double xx = x / refX;
    double yy = y / refY;
    double zz = z / refZ;
    xx = (xx > 0.008856) ? pow(xx, 1.0/3.0) : (7.787 * xx) + (16.0/116.0);
    yy = (yy > 0.008856) ? pow(yy, 1.0/3.0) : (7.787 * yy) + (16.0/116.0);
    zz = (zz > 0.008856) ? pow(zz, 1.0/3.0) : (7.787 * zz) + (16.0/116.0);
    *l = (116.0 * yy) - 16.0;
    *a = 500.0 * (xx - yy);
    *b = 200.0 * (yy - zz);
}

/**
 * @brief 将 RGB 颜色值直接转换为 CIELAB 颜色空间值。
 *
 * 此函数通过先调用 rgb2xyz，再调用 xyz2lab，实现 RGB 到 Lab 的完整转换。
 * 适用于需要直接进行颜色比较、差异分析等应用场景。
 *
 * @param r 红色分量（0~255）
 * @param g 绿色分量（0~255）
 * @param b 蓝色分量（0~255）
 * @param l 指向输出 L（明度）分量的指针
 * @param a 指向输出 a（红-绿轴）分量的指针
 * @param lab_b 指向输出 b（黄-蓝轴）分量的指针
 */
void rgb2lab(unsigned char r, unsigned char g, unsigned char b, double* l, double* a, double* labB) {
    double x, y, z;
    rgb2xyz(r, g, b, &x, &y, &z);
    xyz2lab(x, y, z, l, a, labB);
}

/**
 * @brief 查找与指定 RGB 颜色在 Lab 空间中最接近的颜色索引。
 *
 * 此函数将输入的 RGB 颜色值转换为 Lab 颜色空间后，
 * 与一个预定义的颜色表（需在实现中定义）进行 ΔE 距离计算，
 * 返回距离最小的颜色索引。
 *
 * @param r 红色分量（0~255）
 * @param g 绿色分量（0~255）
 * @param b 蓝色分量（0~255）
 * @return int 最接近颜色在 VGA MODE 13H 颜色表中的索引
 */
int findClosestColorLab(unsigned char r, unsigned char g, unsigned char b) {
    double  inputL, inputA, inputB;
    int     bestIndex = 0;
    double  minDistance = 1e100;
    int     i;

    rgb2lab(r, g, b, &inputL, &inputA, &inputB);

    for (i = 0; i < 256; i++) {
        unsigned int    color = VgaMode13hColorPalette[i];
        unsigned char   palR = (color >> 16) & 0xFF;
        unsigned char   palG = (color >> 8) & 0xFF;
        unsigned char   palB = color & 0xFF;
        double          palL, palA, palB_val, dL, dA, dB, distance;

        rgb2lab(palR, palG, palB, &palL, &palA, &palB_val);
    
        dL = inputL - palL;
        dA = inputA - palA;
        dB = inputB - palB_val;
        distance = dL*dL + dA*dA + dB*dB;
    
        if (distance < minDistance) {
            minDistance = distance;
            bestIndex = i;
        }
    }
    return bestIndex;
}

/**
 * @brief 读取 BMP 文件
 *
 * 此函数会读取 BMP 文件，验证格式是否合法，丢弃无用信息，只返回 RawBitmap
 *
 * @param filename 文件路径
 * @param resultCode 读取结果
 * @return RawBitmap 读取的bmp信息，包括尺寸和像素
 */
RawBitmap* loadBitmapFile(const char *filename, int* resultCode) {
    FILE*               fp;
    RawBitmap*          pRawBitmap;
    BitmapFileHeader    bmHeader;
    BitmapInfoHeader    biHeader;

    if (resultCode) *resultCode = BMP_SUCCESS;

    fp = fopen(filename, "rb");
    if (fp == NULL) {
        if (resultCode) *resultCode = BMP_READ_FAILED;
        return NULL;
    }

    fread(&bmHeader.bfType, sizeof(bmHeader.bfType), 1, fp);
    fread(&bmHeader.bfSize, sizeof(bmHeader.bfSize), 1, fp);
    fread(&bmHeader.bfReserved, sizeof(bmHeader.bfReserved), 1, fp);
    fread(&bmHeader.bfOffBits, sizeof(bmHeader.bfOffBits), 1, fp);

    /* 验证文件头是否是BM */
    if (bmHeader.bfType != 0x4D42) {
        fclose(fp);
        if (resultCode) *resultCode = BMP_INVALID_HEADER;
        return NULL;
    }

    fread(&biHeader, sizeof(BitmapInfoHeader), 1, fp); 

    /* 验证宽度是否是8的倍数 */
    if (biHeader.biWidth % 8 != 0) {
        fclose(fp);
        if (resultCode) *resultCode = BMP_INVALID_WIDTH;
        return NULL;
    }

    /* 验证格式，非压缩与24位颜色 */
    if (biHeader.biCompression != 0 || biHeader.biBitCount != 24) {
        fclose(fp);
        if (resultCode) *resultCode = BMP_INVALID_FORMAT;
        return NULL;
    }

    fseek(fp, bmHeader.bfOffBits, SEEK_SET);
    pRawBitmap = (RawBitmap*)malloc(sizeof(RawBitmap) + biHeader.biSizeImage);

    pRawBitmap->width = biHeader.biWidth;
    pRawBitmap->height = biHeader.biHeight;

    fread(pRawBitmap->raw, biHeader.biSizeImage, 1, fp);

    fclose(fp);

    return pRawBitmap;
}

/**
 * @brief 从 RawBitmap 中读取某坐标对应的 RGB 颜色值
 *
 * 此函数会读取 rawBitmap 的像素并且把颜色RGB值写入传入的指针
 *
 * @param bmp bmp信息
 * @param x x坐标
 * @param y y坐标
 * @param r 对应像素的红色分量指针
 * @param g 对应像素的绿色分量指针
 * @param b 对应像素的蓝色分量指针
 * @return int 是否成功读取
 * @note 此处默认BMP使用从下到上的编码顺序，所以进行了y坐标的转换
 */
int getRawRgb(const RawBitmap* bmp, int x, int y, int *r, int *g, int *b) {
    if (bmp == NULL || x < 0 || x >= bmp->width || y < 0 || y >= bmp->height) {
        return 0;
    }
    int addr = (x + (bmp->height - 1 - y) * bmp->width) * 3;
    *b = bmp->raw[addr];
    *g = bmp->raw[addr + 1];
    *r = bmp->raw[addr + 2];
    return 1;
}

/**
 * @brief 把 RawBitmap 转换为 LunaBitmapResult 的格式
 *
 * 此函数会读取 rawBitmap 和 metadata 信息，根据指定的图片与元数据指定的颜色对图片进行拆分，拆为多个8x8的 Sprite。
 * 每个 Sprite 分三层，分别对应三种指定的 metadata 颜色。在不支持第三种颜色的设备上，将会显示为第二种颜色。
 *
 * @param bmp bmp信息
 * @param metadata 元数据
 * @return LunaBitmapResult* 转换结果
 */
LunaBitmapResult* convertRawBitmapToCode(const RawBitmap* bmp, const Metadata* metadata) {
    int hrzCount = bmp->width / 8;
    int vrtCount = bmp->height / 8 + (bmp->height % 8 > 0);
    int hrz, vrt;
    int i;
    int rawSize =  hrzCount * vrtCount * 3 * 8;
    LunaBitmapResult* lunaResult = (LunaBitmapResult *)malloc(sizeof(LunaBitmapResult) + rawSize);
    unsigned char* pBuf = lunaResult->raw;

    lunaResult->size        = rawSize;
    lunaResult->hrzCount    = hrzCount;
    lunaResult->vrtCount    = vrtCount;
    lunaResult->hrzPadding  = lunaResult->hrzCount * 8 - bmp->width;
    lunaResult->vrtPadding  = lunaResult->vrtCount * 8 - bmp->height;

    for (vrt = 0; vrt < vrtCount; ++vrt) {
        for (hrz = 0; hrz < hrzCount; ++hrz) {
            int layer, x, y;
            unsigned char sprite[3][8];
            memset(sprite, 0, 24);
            for (layer = 0; layer < 3; ++layer) {
                const unsigned char *checkColor = &metadata->color[layer][0];
                for (y = 0; y < 8; ++y) {
                    int realY = vrt * 8 + y;
                    if (realY >= bmp->height) {
                        break;
                    }
                    for (x = 0; x < 8; ++x) {
                        int realX = hrz * 8 + x;
                        int r, g, b;
                        int isColorMatch = 0;
                        getRawRgb(bmp, realX, realY, &r, &g, &b);
                        isColorMatch = r == checkColor[0] && g == checkColor[1] && b == checkColor[2];
                        sprite[layer][y] |= isColorMatch << (7 - x);
                    }
                }
            }
            /* 把第三种颜色的结果合并到第二种颜色的像素上 */
            /* 在不支持第三种颜色的设备上，显示为第二种颜色 */
            for (i = 0; i < 8; ++i) {
                sprite[1][i] |= sprite[2][i];
            }
            for (layer = 0; layer < 3; ++layer) {
                for (i = 0; i < 8; ++i) {
                    *pBuf++ = sprite[layer][i];
                }
            }
        }
    }

    return lunaResult;
}

/**
 * @brief 转换图片文件名，拼接出元数据文件的名称
 *
 * @param fileName 图片文件名
 * @param metaFileName 目标元数据文件名
 * @return char* 元数据的文件名
 * @note 已弃用
 */
char* getMetaFilePath(const char* fileName, char* metaFileName) {
    int fileNameLen = strlen(fileName);
    int dotPos = fileNameLen - 1;
    int i;
    char* pName = metaFileName;

    /* find . (dot) position */
    for (i = fileNameLen - 1; i >= 0; --i) {
        if (fileName[i] == '.') {
            dotPos = i;
            break;
        }
    }

    for (i = 0; i <= dotPos; ++i) {
        *pName++ = fileName[i];
    }
    strcpy(pName, "meta.txt");

    if (cliParams.debugMode) {
        fprintf(stderr, "[DEBUG] Meta File Name = '%s'\n", metaFileName);
    }

    return metaFileName;
}

/**
 * @brief 将文件名转换为C语言中可用的变量名
 *
 * @param fileName 图片文件名
 * @param metaFileName 目标变量名称
 * @return char* 变量名称
 */
char* convertFileNameToVariable(const char *fileName, char *varName) {
    int slashPos = -1;
    int fileNameLen = strlen(fileName);
    int dotPos = fileNameLen - 1;
    int i;
    char* pName = varName;

    /* 寻找 / 或者 \ */
    for (i = 0; i < fileNameLen; ++i) {
        if (fileName[i] == '/' || fileName[i] == '\\') {
            slashPos = i;
        }
    }
    /* 寻找 . */
    for (i = fileNameLen - 1; i >= 0; --i) {
        if (fileName[i] == '.') {
            dotPos = i;
            break;
        }
    }

    *pName = '_';
    for (i = slashPos + 1; i < dotPos; ++i) {
        char convertedChar = '_';
        const char readChar = fileName[i];
        if (readChar >= 'a' && readChar <= 'z') {
            convertedChar = 'A' + readChar - 'a';
        }
        else if ((readChar >= 'A' && readChar <= 'Z') || (readChar >= '0' && readChar <= '9')) {
            convertedChar = readChar;
        }
        if (convertedChar == '_' && *pName == '_') {
            continue;
        }
        else {
            *pName++ = convertedChar;
        }
    }
    *pName = '\0';

    if (cliParams.debugMode) {
        fprintf(stderr, "[DEBUG] Variable Name = '%s'\n", varName);
    }

    return varName;
}

/**
 * @brief 读取元数据文件
 *
 * 此函数将会读取元数据文件，获取 Sprite 的三种颜色
 * 
 * @param metaFilePath 元数据文件名
 * @param bmp rawBitmap数据
 * @param metadata metadata指针，读取结果将会写入此指针
 * @return int 读取结果状态码
 */
int loadMetadata(const char* metaFilePath, const RawBitmap* bmp, Metadata* metadata) {
    FILE *fp;
    int metaColor[3];
    int i;
    
    memset(metadata, 0, sizeof(Metadata));

    /*
     * 命令行参数未指定元数据文件，将使用默认模式
     * 颜色 1 为纯黑 0x000000，索引 0
     * 颜色 2 为纯白 0xffffff，索引 15
     * 颜色 3 将扫描整个 bmp 图片，检索到的非前两种颜色即为颜色3
     */
    if (!metaFilePath) {
        int rawSize = bmp->width * bmp->height * 3;
        int colorTbdFounded = 0;
        int thirdIndex = 0;
    
        /* 颜色1，纯黑 */
        metadata->color[0][0] = 0x00;
        metadata->color[0][1] = 0x00;
        metadata->color[0][2] = 0x00;
        metadata->colorIndex[0] = 0;

        /* 颜色2，纯白 */
        metadata->color[1][0] = 0xff;
        metadata->color[1][1] = 0xff;
        metadata->color[1][2] = 0xff;
        metadata->colorIndex[1] = 15;

        /* 扫描整个图片 */
        for (i = 0; i < rawSize; i += 3) {
            unsigned char b = bmp->raw[i + 0];
            unsigned char g = bmp->raw[i + 1];
            unsigned char r = bmp->raw[i + 2];
            /* 跳过纯黑 */
            if (r == 0x00 && g == 0x00 && b == 0x00) continue;
            /* 跳过纯白 */
            if (r == 0xff && g == 0xff && b == 0xff) continue;
            /* 跳过 0xff00ff，认为是透明色 */
            if (r == 0xff && g == 0x00 && b == 0xff) continue;
            /* 找到了第三种颜色 */
            if (!colorTbdFounded) {
                metadata->color[2][0] = r;
                metadata->color[2][1] = g;
                metadata->color[2][2] = b;
                colorTbdFounded = 1;
            }
            /* 错误情况，出现了第四种颜色，认定为非法格式 */
            else {
                if (!(r == metadata->color[2][0] && g == metadata->color[2][1] && b == metadata->color[2][2])) {
                    if (cliParams.debugMode) {
                        fprintf(stderr, "[DEBUG] Exceed (%d, %d) -> RGB(0x%x, 0x%x, 0x%x)\n", i % 8, bmp->height - 1 - i / 8, r, g, b);
                    }
                    return META_COLOR_EXCEED;
                }
            }
        }

        /* 获取三种颜色的索引值 */
        thirdIndex = findClosestColorLab(
            metadata->color[2][0],
            metadata->color[2][1],
            metadata->color[2][2]
        );
        metadata->colorIndex[2] = thirdIndex;

        if (cliParams.debugMode) {
            fprintf(stderr, "[DEBUG] 3rd Color Index(%d) -> RGB(0x%x)\n", thirdIndex, VgaMode13hColorPalette[thirdIndex]);
        }

        return META_SUCCESS;
    }

    fp = fopen(metaFilePath, "r");


    if (!fp) {
        return META_READ_FAILED;
    }

    /* 读取文件获取三种颜色 */
    fscanf(fp, "%x,%x,%x", &metaColor[0], &metaColor[1], &metaColor[2]);
    fclose(fp);

    /* 检查元数据给出的颜色是否合法，并获得索引值 */
    for (i = 0; i < 3; ++i) {
        if (!(metaColor[i] >= 0x000000 && metaColor[1] <= 0xffffff)) {
            return META_INVALID;
        }
        int r = (metaColor[i] >> 16) & 0xff;
        int g = (metaColor[i] >> 8) & 0xff;
        int b = metaColor[i] & 0xff;
        int index = findClosestColorLab(r, g, b);
        metadata->color[i][0] = r;
        metadata->color[i][1] = g;
        metadata->color[i][2] = b;
        metadata->colorIndex[i] = index;

        if (cliParams.debugMode) {
            fprintf(stderr, "[DEBUG] Palette[%d] Color Index(%d) -> RGB(0x%x)\n", i, index, VgaMode13hColorPalette[index]);
        }
    }

    return META_SUCCESS;
}

/**
 * @brief 将 LunaBitmapResult 的 Sprite 格式化为 C 数组风格
 * 
 * @param fp 输出文件指针
 * @param lunaResult 需要输出的 Sprite
 * @param metadata Sprite 元数据
 * @param destVarName 输出变量名
 */
void formatAsCCode(FILE* fp, const LunaBitmapResult* lunaResult, const Metadata* metadata, const char* destVarName) {
    const char* tabSpace = "    ";
    int hrzCount = lunaResult->hrzCount;
    int vrtCount = lunaResult->vrtCount;
    int hrz, vrt;
    int i, layer;
    const unsigned char* pRaw = lunaResult->raw;

    /* 输出变量名与定义 */
    fprintf(fp, "const unsigned char %s[] = {\n", destVarName);
    /* 输出水平和垂直的 sprite 数量 */
    fprintf(fp, "%s/* hrzCount */ %d, /* vrtCount */ %d,\n", tabSpace, hrzCount, vrtCount);
    /* 输出水平和垂直的 padding */
    fprintf(fp, "%s/* hrzPad */ %d, /* vrtPad */ %d,\n", tabSpace, lunaResult->hrzPadding, lunaResult->vrtPadding);

    /* 输出调色板信息 */
    for (i = 0; i < 3; ++i) {
        int index = metadata->colorIndex[i];
        fprintf(fp, "%s/* palette[%d][0x%06x] */ %d, \n", tabSpace, i, VgaMode13hColorPalette[index], index);
    }

    for (vrt = 0; vrt < vrtCount; ++vrt) {
        for (hrz = 0; hrz < hrzCount; ++hrz) {
            /* 最后一个 Sprite ? */
            int isLastSprite = vrt == vrtCount - 1 && hrz == hrzCount - 1;
            /* 输出 3 层不同颜色的 Sprite  */
            for (layer = 0; layer < 3; ++layer) {
                /* 最后一个 layer？ */
                int isLastlayer = layer == 2;
                fprintf(fp, "%s/* sprite[%d][%d] */ ", tabSpace, hrz + vrt *hrzCount, layer);
                for (i = 0; i < 8; ++i, ++pRaw) {
                    fprintf(fp, "0x%02x", *pRaw);
                    if (i == 7) { /* 最后一个字节？ */
                        if (!(isLastSprite && isLastlayer)) {
                            fprintf(fp, ",");
                        }
                    } else {
                        fprintf(fp, ", ");
                    }
                }
                fprintf(fp, "\n");
            }
        }
    }

    fprintf(fp, "};\n");
}

/**
 * @brief 将 LunaBitmapResult 的 Sprite 格式化为 JSON 数组风格
 * 
 * @param fp 输出文件指针
 * @param lunaResult 需要输出的 Sprite
 * @param metadata Sprite 元数据
 */
void formatAsJson(FILE* fp, const LunaBitmapResult* lunaResult, const Metadata* metadata) {
    int i, size = lunaResult->size;

    fprintf(fp, "[");
    /* 输出尺寸 */
    fprintf(fp, "%d,%d,", lunaResult->hrzCount, lunaResult->vrtCount);
    /* 输出 Padding */
    fprintf(fp, "%d,%d,", lunaResult->hrzPadding, lunaResult->vrtPadding);

    /* 输出调色板 */
    for (i = 0; i < 3; ++i) {
        int index = metadata->colorIndex[i];
        fprintf(fp, "%d,", index);
    }

    for (i = 0; i < size; ++i) {
        fprintf(fp, "%d", lunaResult->raw[i]);
        /* 检查是否是最后一个逗号 */
        if (i + 1 < size) {
            fprintf(fp, ",");
        }
    }

    fprintf(fp, "]\n");
}

/**
 * @brief 将 LunaBitmapResult 的 Sprite 编码为二进制文件
 * 
 * @param fp 输出文件指针
 * @param lunaResult 需要输出的 Sprite
 * @param metadata Sprite 元数据
 */
void encodeToBinary(FILE* fp, const LunaBitmapResult* lunaResult, const Metadata* metadata) {
    unsigned char hrzCount      = lunaResult->hrzCount,
                  vrtCount      = lunaResult->vrtCount,
                  hrzPadding    = lunaResult->hrzPadding,
                  vrtPadding    = lunaResult->vrtPadding;
    int rawSize = hrzCount * vrtCount * 8 * 3;
    const char* strMagicHeader = "IM";
    int i;

    fwrite(strMagicHeader, strlen(strMagicHeader), 1, fp);
    fwrite(&hrzCount, sizeof(hrzCount), 1, fp);
    fwrite(&vrtCount, sizeof(vrtCount), 1, fp);
    fwrite(&hrzPadding, sizeof(hrzPadding), 1, fp);
    fwrite(&vrtPadding, sizeof(vrtPadding), 1, fp);

    for (i = 0; i < 3; ++i) {
        unsigned char colorIndex = metadata->colorIndex[i];
        fwrite(&colorIndex, sizeof(colorIndex), 1, fp);
    }

    fwrite(lunaResult->raw, rawSize, 1, fp);
}

/**
 * @brief 将 RawBitmap 格式化为C数组风格
 * 
 * @param fp 输出文件指针
 * @param bmp 位图数据
 * @param destVarName 输出变量名
 */
void encodeToCEmbedCode(FILE *fp, const RawBitmap* bmp, const char* destVarName) {
    int x, y;
    int w = bmp->width;
    int h = bmp->height;
    const char* tabSpace = "    ";

    /* 输出注释 */
    fprintf(fp, "/* 8Bit palette image '%s' */\n", destVarName);
    /* 输出变量名与定义 */
    fprintf(fp, "const unsigned char %s[] = {\n", destVarName);
    /* 输出尺寸信息 */
    fprintf(fp, "%s/* width */ %d, /* height */ %d,\n", tabSpace, w, h);
    /* 输出每个像素对应的颜色索引 */
    for (y = 0; y < h; ++y) {
        fprintf(fp, "%s", tabSpace);
        for (x = 0; x < w; ++x) {
            int r, g, b, index;
            getRawRgb(bmp, x, y, &r, &g, &b);
            index = findClosestColorLab(r, g, b);
            if (x + 1 >= w) { /* 逗号判断 */
                fprintf(fp, "%d", index);
            } else {
                fprintf(fp, "%d, ", index);
            }
        }
        if (y + 1 >= h) { /* 最后一行？ */
            fprintf(fp, "\n");
        } else {
            fprintf(fp, ",\n");
        }
    }
    fprintf(fp, "};\n");
}

/**
 * @brief 解析命令行参数
 * 
 * @param argc 命令行参数数量
 * @param argv 命令行参数
 * @return int 解析是否成功
 */
int parseCommandLineParameters(int argc, char** argv) {
    int i = 1;
    while (i < argc) {
        const char* strFlag = argv[i];
        /* 开启调试模式 */
        if (strcmp(strFlag, "-d") == 0) {
            cliParams.debugMode = 1;
        }
        /* 输出文件名 */
        else if (strcmp(strFlag, "-o") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Invalid parameter: missing output file after -o flag.\n");
                return 0;
            }
            ++i;
            cliParams.outName = argv[i];
        }
        /* 元数据文件名 */
        else if (strcmp(strFlag, "-m") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Invalid parameter: missing output file after -m flag.\n");
                return 0;
            }
            ++i;
            cliParams.metaName = argv[i];
        }
        /* 选择输出格式 */
        else if (strcmp(strFlag, "-f") == 0) {
            const char* strParam;
            if (i + 1 >= argc) {
                fprintf(stderr, "Invalid parameter: missing format after -f flag.\n");
                return 0;
            }
            ++i;
            strParam = argv[i];
            if (strcmp(strParam, "json") == 0) {
                cliParams.outputFormat = OUTPUT_FORMAT_JSON;
            }
            else if (strcmp(strParam, "ccode") == 0) {
                cliParams.outputFormat = OUTPUT_FORMAT_C_CODE;
            }
            else if (strcmp(strParam, "cembed") == 0) {
                cliParams.outputFormat = OUTPUT_FORMAT_C_EMBED;
            }
            else if (strcmp(strParam, "binary") == 0) {
                cliParams.outputFormat = OUTPUT_FORMAT_BINARY;
                cliParams.isOutputBinary = 1;
            }
            else {
                fprintf(stderr, "Invalid parameter: unrecognized format '%s'.\n", strParam);
                return 0;
            }
        }
        /* 输入文件名 */
        else {
            if (cliParams.fileName != NULL) {
                fprintf(stderr, "Invalid flag: unrecognized flag '%s'.\n", strFlag);
                return 0;
            }
            cliParams.fileName = strFlag;
        }
        ++i;
    }

    if (cliParams.fileName == NULL) {
        fprintf(stderr, "Missing input file.\n");
        return 0;
    }

    return 1;
}

int main(int argc, char** argv) {
    RawBitmap*  bmp;
    int         cliResult, loadResult, metaResult;
    const char* fileName;
    char        varName[100];
    /* char        metaFileName[200]; */
    Metadata    metadata;
    FILE*       outFp = NULL;

    LunaBitmapResult* lunaResult;
    
    cliResult = parseCommandLineParameters(argc, argv);
    if (!cliResult) {
        fprintf(stderr, "Usage: %s filename.bmp [-d][-f json | ccode]\n", argv[0]);
        return 1;
    }

    fileName = cliParams.fileName;
    convertFileNameToVariable(fileName, varName);
    /* getMetaFilePath(fileName, metaFileName); */

    /* 读取 BMP 文件 */
    bmp = loadBitmapFile(fileName, &loadResult);
    switch (loadResult) {
    case BMP_READ_FAILED:
        fprintf(stderr, "Failed to read '%s'.\n", fileName);
        return 1;
    case BMP_INVALID_HEADER:
        fprintf(stderr, "'%s' is not a .bmp file.", fileName);
        return 1;
    case BMP_INVALID_FORMAT:
        fprintf(stderr, "BMP images should be 24-bit and use no compression.\n");
        return 1;
    case BMP_INVALID_WIDTH:
        fprintf(stderr, "The width of a BMP image should be a multiple of 8.\n");
        return 1;
    }
    
    /* 打开输出文件 */
    if (cliParams.outName) {
        outFp = fopen(cliParams.outName, cliParams.isOutputBinary ? "wb" : "w");
        if (!outFp) {
            fprintf(stderr, "Failed to write output file '%s'.\n", cliParams.outName);
            free(bmp);
            return 1;
        }
    } else {
        outFp = stdout;
    }

    if (cliParams.outputFormat == OUTPUT_FORMAT_C_EMBED) {
        encodeToCEmbedCode(outFp, bmp, varName);
    } else {
        /* 读取元数据 */
        metaResult = loadMetadata(cliParams.metaName, bmp, &metadata);
        switch (metaResult) {
        case META_READ_FAILED:
            fprintf(stderr, "Failed to load metadata file '%s'.\n", cliParams.metaName);
            free(bmp);
            return 1;
        case META_COLOR_EXCEED:
            fprintf(stderr, "There are too many colors, exceeding the limit of 3 colors.\n");
            free(bmp);
            return 1;
        case META_INVALID:
            fprintf(stderr, "Invalid metadata format.\n");
            free(bmp);
            return 1;
        }

        lunaResult = convertRawBitmapToCode(bmp, &metadata);

        /* 输出内容 */
        switch (cliParams.outputFormat) {
        case OUTPUT_FORMAT_C_CODE:
            formatAsCCode(outFp, lunaResult, &metadata, varName);
            break;
        case OUTPUT_FORMAT_JSON:
            formatAsJson(outFp, lunaResult, &metadata);
            break;
        case OUTPUT_FORMAT_BINARY:
            encodeToBinary(outFp, lunaResult, &metadata);
            break;
        }

        if (lunaResult) free(lunaResult);
    }

    if (outFp != stdout && outFp != NULL) {
        fclose(outFp);
    }
    free(bmp);

    return 0;
}