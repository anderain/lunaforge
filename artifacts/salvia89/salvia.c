#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "salvia.h"

#define NUM_STR_MAX 100

enum {
    STATE_PLAIN = 0,
    STATE_FLAGS,
    STATE_WIDTH,
    STATE_PRECISION,
    STATE_LENGTH,
    STATE_SPECIFIER
} ReaderState;

enum {
    FLAG_NONE           = 0,
    FLAG_LEFT_JUSTIFY   = 1,
    FLAG_FORCE_SIGN     = 2,
    FLAG_SPACE_PADDING  = 4,
    FLAG_ZERO_PADDING   = 8
} Flags;

/*
 * 与 C 语言的 printf 的格式化标志保持一致
 * %[uFlags][width][.iPrecision][cLength]specifier
 */
typedef struct {
    unsigned int    uFlags;
    int             iMinWidth;
    int             iPrecision;
    char            cLength;
} FormatSpec;

#define         Salvia_StrLen               strlen
#define         Salvia_StrCpy               strcpy
#define         Salvia_IsDigit(c)           ((c) >= '0' && (c) <= '9')
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
                    (formatSpec.uFlags & FLAG_ZERO_PADDING) ? '0' : ' ',
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

static void Salvia_ClearFormatSpec(FormatSpec* pFormatSpec) {
    pFormatSpec->uFlags         = FLAG_NONE | FLAG_SPACE_PADDING;
    pFormatSpec->iMinWidth      = 0;
    pFormatSpec->iPrecision     = -1;
    pFormatSpec->cLength        = 0;
}

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