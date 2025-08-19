#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "salvia.h"

#define NUM_STR_MAX 100

/**
 * @brief 格式解析状态枚举
 */
enum {
    STATE_PLAIN = 0,      /** < 普通文本状态 */
    STATE_FLAGS,          /** < 标志符解析状态   */
    STATE_WIDTH,          /** < 宽度解析状态 */
    STATE_PRECISION,      /** < 精度解析状态 */
    STATE_LENGTH,         /** < 长度修饰符解析状态 */
    STATE_SPECIFIER       /** < 类型说明符解析状态 */
} ReaderState;

/**
 * @brief 格式化标志位枚举
 */
enum {
    FLAG_NONE           = 0,   /** < 无标志 */
    FLAG_LEFT_JUSTIFY   = 1,   /** < 左对齐标志(-) */
    FLAG_FORCE_SIGN     = 2,   /** < 强制显示正号标志(+) */
    FLAG_SPACE_PADDING  = 4,   /** < 正数前加空格标志( ) */
    FLAG_ZERO_PADDING   = 8    /** < 使用零填充标志(0) */
} Flags;

/** 
 * @brief 格式说明符结构体
 * 
 * @details 与C语言的printf格式化标志保持一致:
 * %[flags][width][.precision][length]specifier
 */
typedef struct {
    unsigned int    uFlags;     /** < 标志位组合 */
    int             iMinWidth;  /** < 最小字段宽度 */
    int             iPrecision; /** < 精度(对于数字是小数位数，字符串是最大字符数) */
    char            cLength;    /** < 长度修饰符(h/l/L等) */
} FormatSpec;

#define Salvia_StrLen               strlen                          /** < 字符串长度计算宏 */
#define Salvia_StrCpy               strcpy                          /** < 字符串复制宏 */
#define Salvia_IsDigit(c)           ((c) >= '0' && (c) <= '9')      /** < 判断是否为数字字符 */

static void     Salvia_ClearFormatSpec      (FormatSpec* pFormatSpec);
static char*    Salvia_WriteString          (char* szBuf, const char* szSource, char cPadding, int iMaxWrite, int iWidth, int bIsLeftAlign);
static int      Salvia_Atoi                 (const char* pSource);
static char*    Salvia_ItoaWithPadding      (int iNum, char* pStrBuf, int iBase, int bUppercase, int bForceSign, int iMinDigit);

/* 重置临时缓存指针 */
#define ResetTempBuffer() (pCurTempBuf = szTempBuf)

/* 重设状态并且回到 Plain 状态 */
#define ResetFormatState() {                        \
            Salvia_ClearFormatSpec(&formatSpec);    \
            ResetTempBuffer();                      \
            iState = STATE_PLAIN;                   \
        } NULL

/**
 * @brief 格式化输出字符串(类似printf)
 * 
 * @param szBuf 目标缓冲区，用于存储格式化后的字符串
 * @param szFormat 格式化字符串
 * @param ... 可变参数，根据格式字符串需要传入相应参数
 * @return int 返回写入的字符数(不包括结尾的'\0')
 * 
 * @details
 * 支持以下格式说明符:
 * - %d: 十进制整数
 * - %s: 字符串
 * - %c: 字符
 */
 int Salvia_Format(char* szBuf, const char* szFormat, ...) {
    char*       pCurBuf = szBuf;        /* 当前写入位置 */
    const char* pCurFormat = szFormat;  /* 当前读取位置 */
    const char* pLastFormatStart;       /* 最后一个格式化符号的开始处 */
    char        szTempBuf[NUM_STR_MAX]; /* 临时缓存，用于读入 width / precision */
    char*       pCurTempBuf;            /* 临时缓存写入指针*/
    char        c;                      /* 从 szFormat 中读取的单个字符 */
    int         iState;                 /* 读取状态机的状态 */
    FormatSpec  formatSpec;             /* 格式化参数 */
    va_list     args;                   /* 可变参数 */

    va_start(args, szFormat);

    ResetFormatState();

    while ((c = *pCurFormat)) {
        switch (iState) {
        default:
        /* 普通字符 */
        case STATE_PLAIN:
            /* 遇到个格式化开始 */
            if (c == '%') {
                pLastFormatStart = pCurFormat;
                pCurFormat++;
                iState = STATE_FLAGS;
            }
            /* 其他普通字符，写入目标缓存 */
            else {
                *pCurBuf++ = c;
                pCurFormat++;
            }
            break;
        case STATE_FLAGS:
            switch (c) {
            case '-': /* 左对齐 */
                formatSpec.uFlags |= FLAG_LEFT_JUSTIFY;
                pCurFormat++;
                break;
            case '+': /* 强制正负号 */
                formatSpec.uFlags |= FLAG_FORCE_SIGN;
                pCurFormat++;
                break;
            case ' ': /* 空格填充 */
                formatSpec.uFlags |= FLAG_SPACE_PADDING;
                pCurFormat++;
                break;
            case '0': /* 零填充 */
                formatSpec.uFlags |= FLAG_ZERO_PADDING;
                pCurFormat++;
                break;
            default: /* 其他字符，进入下一状态 */
                iState = STATE_WIDTH;
                break;
            }
            break;
        case STATE_WIDTH:
            /* 是数字，把数字读取到缓存里 */
            if (Salvia_IsDigit(c)) {
                *pCurTempBuf++ = c;
                pCurFormat++;
            }
            /* 不是数字，结束读取，进入下状态 */
            else {
                *pCurTempBuf = '\0';
                formatSpec.iMinWidth = Salvia_Atoi(szTempBuf);
                ResetTempBuffer();
                /* 进入精度读取 */
                if (c == '.') {
                    iState = STATE_PRECISION;
                    pCurFormat++;
                }
                /* 进入长度修饰符读取 */
                else {
                    iState = STATE_LENGTH;
                }
            }
            break;
        case STATE_PRECISION:
            /* 是数字，把数字读取到缓存里 */
            if (Salvia_IsDigit(c)) {
                *pCurTempBuf++ = c;
                pCurFormat++;
            }
            /* 不是数字，结束读取，进入下状态 */
            else {
                *pCurTempBuf = '\0';
                formatSpec.iPrecision = Salvia_Atoi(szTempBuf);
                ResetTempBuffer();
                iState = STATE_LENGTH;
            }
            break;
        case STATE_LENGTH:
            /* TODO: 实现长度修饰符读取*/
            iState = STATE_SPECIFIER;
            break;
        case STATE_SPECIFIER:
            switch (c) {
            case 'd': { /* 十进制整数 */
                int iPrecision = formatSpec.iPrecision;
                int bForceSign = (formatSpec.uFlags & FLAG_FORCE_SIGN) ? 1 : 0;
                int intValue = va_arg(args, int);
                /* 最极端情况 */
                if (iPrecision == 0 && intValue == 0) {
                    if (intValue < 0) {
                        szTempBuf[0] = '-';
                        szTempBuf[1] = '\0';
                    }
                    else if (bForceSign) {
                        szTempBuf[0] = '+';
                        szTempBuf[1] = '\0';
                    }
                    else {
                        szTempBuf[0] = '\0';
                    }
                }
                /* 普通情况 */
                else {
                    if (iPrecision < 0 &&
                        (formatSpec.uFlags & FLAG_ZERO_PADDING) &&
                        !(formatSpec.uFlags & FLAG_LEFT_JUSTIFY) &&
                        formatSpec.iMinWidth > 0
                    ) {
                        iPrecision = formatSpec.iMinWidth;
                        if (intValue < 0 || bForceSign) {
                            iPrecision--;
                        }
                    }
                    Salvia_ItoaWithPadding(intValue, szTempBuf, 10, 0, bForceSign, iPrecision);
                }
                pCurBuf = Salvia_WriteString(
                    pCurBuf,
                    szTempBuf,
                    ' ',
                    -1,
                    formatSpec.iMinWidth,
                    (formatSpec.uFlags & FLAG_LEFT_JUSTIFY) ? 1 : 0
                );
                break;
            }
            case 's':   /* 字符串 */
                pCurBuf = Salvia_WriteString(
                    pCurBuf,
                    va_arg(args, const char *),
                    /* (formatSpec.uFlags & FLAG_ZERO_PADDING) ? '0' : ' ', */
                    ' ', /* 与 GCC ANSI 标准保持一致，不使用 0 填充 */
                    formatSpec.iPrecision,
                    formatSpec.iMinWidth,
                    (formatSpec.uFlags & FLAG_LEFT_JUSTIFY) ? 1 : 0
                );
                break;
            case 'c':   /* 字符 */
                break;
            case '%':   /* 百分号本身 */
                *pCurBuf++ = '%';
                break;
            default: {  /* 非法的输入，不格式化，直接输出内容 */
                    const char* pFmt;
                    for (
                        pFmt = pLastFormatStart;
                        pFmt <= pCurFormat;
                        ++pFmt, ++pCurBuf
                    ) {
                        *pCurBuf = *pFmt;
                    }
                    break;
                }
            }
            ++pCurFormat;
            /* 重置格式标志, 回到普通字符状态 */
            ResetFormatState();
        }
    }

    /* 格式读入未完成 */
    if (iState != STATE_PLAIN) {
        const char* pFmt;
        for (
            pFmt = pLastFormatStart;
            pFmt < pCurFormat;
            ++pFmt, ++pCurBuf
        ) {
            *pCurBuf = *pFmt;
        }
    }

    va_end(args);

    /* 最后置入 '\0' 结尾*/
    *pCurBuf = '\0';
    /* 返回最后写入位置与 szBuf 的差值，即写入的字节数 */
    return pCurBuf - szBuf;
}

/**
 * @brief 将字符串写入缓冲区，支持对齐和填充
 * 
 * @param szBuf 目标缓冲区
 * @param szSource 源字符串 
 * @param cPadding 填充字符
 * @param iMaxWrite 最大写入字符数(-1表示无限制)
 * @param iWidth 字段总宽度
 * @param bIsLeftAlign 是否左对齐(1=左对齐，0=右对齐)
 * @return char* 返回写入后的缓冲区位置
 * 
 * @details
 * 该函数会根据指定的宽度和对齐方式，在写入字符串前后添加填充字符。
 * 如果源字符串长度超过iMaxWrite，则会被截断。
 */
static char* Salvia_WriteString(char* szBuf, const char* szSource, char cPadding, int iMaxWrite, int iWidth, int bIsLeftAlign) {
    int iStrLength = Salvia_StrLen(szSource);
    int iLengthToWrite;
    int iPadding;
    int i;

    /* 计算显示的字符数量、补位宽度 */
    if (iMaxWrite < 0) {
        iLengthToWrite = iStrLength;
    }
    else if (iMaxWrite < iStrLength) {
        iLengthToWrite = iMaxWrite;
    }
    else {
        iLengthToWrite = iStrLength;
    }
    iPadding = iWidth - iLengthToWrite;

    /* 右对齐，写入字符串之前补位 */
    if (!bIsLeftAlign) {
        for (i = 0; i < iPadding; ++i) {
            *szBuf++ = cPadding;
        }
    }
    /* 写入字符串 */
    for (i = 0; i < iLengthToWrite; ++i) {
        *szBuf++ = szSource[i];
    }
    /* 左对齐，写入字符串之后补位 */
    if (bIsLeftAlign) {
        for (i = 0; i < iPadding; ++i) {
            *szBuf++ = cPadding;
        }
    }
    return szBuf;
}

/**
 * @brief 初始化/清除格式说明符结构体
 * @param pFormatSpec 要初始化的格式说明符指针
 * @details 将所有字段设置为默认值:
 * - uFlags: FLAG_NONE | FLAG_SPACE_PADDING
 * - iMinWidth: 0
 * - iPrecision: -1
 * - cLength: 0
 */
static void Salvia_ClearFormatSpec(FormatSpec* pFormatSpec) {
    pFormatSpec->uFlags         = FLAG_NONE | FLAG_SPACE_PADDING;
    pFormatSpec->iMinWidth      = 0;
    pFormatSpec->iPrecision     = -1;
    pFormatSpec->cLength        = 0;
}

/**
 * @brief 字符串转整数的实现
 * @param pSource 源字符串
 * @return 转换后的整数值
 */
static int Salvia_Atoi(const char* pSource) {
    int result = 0;
    while (*pSource != '\0' && Salvia_IsDigit(*pSource)) {
        result = (result << 3) + (result << 1) + (*pSource - '0');
        pSource++;
    }
    return result;
}

static void Salvia_ItoaReverse(char* pStrBuf, int iLength) {
    int iStart = 0;
    int iEnd = iLength -1;
    while (iStart < iEnd) {
        char t = *(pStrBuf + iStart);
        *(pStrBuf + iStart) = *(pStrBuf + iEnd);
        *(pStrBuf + iEnd) = t;
        iStart++;
        iEnd--;
    }
}

/**
 * @brief 带填充和符号的整数转字符串
 * @param iNum 要转换的整数
 * @param pStrBuf 目标缓冲区
 * @param iBase 进制(2-36)
 * @param bUppercase 是否大写字母(16进制时有效)
 * @param bForceSign 是否强制显示正号
 * @param iMinDigit 最小数字位数(不够时补零)
 * @return 返回目标缓冲区指针
 */
static int Salvia_ItoaNoSign(int iNum, char* pStrBuf, int iBase, int bUppercase) {
    int i = 0;
    int bIsNegative = 0;
    char hexDigitStart = bUppercase ? 'A' : 'a';

    if (iNum == 0) {
        pStrBuf[i++] = '0';
        pStrBuf[i] = '\0';
        return bIsNegative;
    }

    if (iNum < 0 && iBase == 10) {
        bIsNegative = 1;
        iNum = -iNum;
    }

    while (iNum != 0) {
        int rem = iNum % iBase;
        pStrBuf[i++] = (rem > 9)? (rem-10) + hexDigitStart : rem + '0';
        iNum = iNum / iBase;
    }

    pStrBuf[i] = '\0';

    Salvia_ItoaReverse(pStrBuf, i);
 
    return bIsNegative;
}

/**
 * @brief 带填充和符号的整数到字符串转换
 * 
 * @param iNum 要转换的整数
 * @param pStrBuf 存放结果的缓冲区
 * @param iBase 进制(2-36)
 * @param bUppercase 是否使用大写字母(16进制时有效)
 * @param bForceSign 是否强制显示正号
 * @param iMinDigit 最小数字位数(不足时补零)
 * @return char* 返回结果缓冲区指针
 * 
 * @details
 * 该函数会:
 * 1. 处理数字的符号(负号或强制正号)
 * 2. 根据需要添加前导零以达到最小位数
 * 3. 支持2到36进制的转换
 * 4. 对于16进制可指定大小写格式
 */
static char* Salvia_ItoaWithPadding(int iNum, char* pStrBuf, int iBase, int bUppercase, int bForceSign, int iMinDigit) {
    char    szBuf[NUM_STR_MAX];
    char*   pCurBuf = pStrBuf;
    int     bIsNegative;
    int     iStrLength;
    int     iZeroPadding;
    int     i;

    bIsNegative = Salvia_ItoaNoSign(iNum, szBuf, iBase, bUppercase);
    iStrLength = Salvia_StrLen(szBuf);
    iZeroPadding = iMinDigit - iStrLength;
    /* 写入符号 */
    if (bIsNegative) {
        *pCurBuf++ = '-';
    }
    else if (bForceSign) {
        *pCurBuf++= '+';
    }
    /* 写入补零 */
    for (i = 0; i < iZeroPadding; ++i) {
        *pCurBuf++ = '0';
    }
    /* 复制数字部分 */
    Salvia_StrCpy(pCurBuf, szBuf);

    return pStrBuf;
}