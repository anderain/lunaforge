#include <stdlib.h>
#include <string.h>
#include "k_utils.h"
#include "kommon.h"

int StringCopy(char *dest, int max, const char *src) {
    int i;
    for (i = 0; i < max - 1 && src[i]; ++i) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
    return i;
}

char *StringDump(const char *str) {
    int length = strlen(str) + 1;
    char *buffer = (char *)malloc(length);
    StringCopy(buffer, length + 1, str);
    return buffer;
}

int StringEndWith(const char *str, const char *token) {
    if (str == NULL || token == NULL) {
        return 0;
    }
    
    int strLen = strlen(str);
    int tokenLen = strlen(token);
    
    if (tokenLen > strLen) {
        return 0;
    }

    return strcmp(str + strLen - tokenLen, token) == 0;
}

double kAtof(const char *str) {
    int sign;
    double number = 0.0, power = 1.0;
    /* 跳过空白 */
    while(isSpace(*str)) ++str;
    /* 处理正负号 */
    sign = (*str == '-') ? -1 : 1; 
    if (*str == '-' || *str == '+') {
        str++;
    }
    /* 处理小数点后的数字 */
    while(*str >= '0' && *str <= '9') {
        number = 10.0 * number + (*str - '0');
        str++;
    }
    /* 跳过小数点 */
    if(*str == '.') {
        str++;
    }
    /* 处理小数点后的数字 */
    while(*str >= '0' && *str <= '9') {
        number = 10.0 * number + (*str - '0');
        power *= 10.0;
        str++;
    }
    return sign * number / power;
}

#define MAX_PRECISION   (10)

static const double rounders[MAX_PRECISION + 1] = {
    0.5,                /* 0 */
    0.05,               /* 1 */
    0.005,              /* 2 */
    0.0005,             /* 3 */
    0.00005,            /* 4 */
    0.000005,           /* 5 */
    0.0000005,          /* 6 */
    0.00000005,         /* 7 */
    0.000000005,        /* 8 */
    0.0000000005,       /* 9 */
    0.00000000005       /* 10 */
};

char* kFtoa(double f, char * buf, int precision) {
    char* ptr = buf;
    char* p = ptr;
    char* p1;
    char c;
    long intPart;

    /* 检查精度边界 */
    if (precision > MAX_PRECISION)
        precision = MAX_PRECISION;

    /* 处理符号 */
    if (f < 0) {
        f = -f;
        *ptr++ = '-';
    }

    /* 负精度表示自动精度推测 */
    if (precision < 0) {
        if (f < 1.0) precision = 6;
        else if (f < 10.0) precision = 5;
        else if (f < 100.0) precision = 4;
        else if (f < 1000.0) precision = 3;
        else if (f < 10000.0) precision = 2;
        else if (f < 100000.0) precision = 1;
        else precision = 0;
    }

    /* 根据精度舍入值 */
    if (precision)
        f += rounders[precision];

    /* 整数部分... */
    intPart = (long)f;
    f -= intPart;

    if (!intPart)
        *ptr++ = '0';
    else {
        /* 保存起始指针 */
        p = ptr;

        /* 逆序转换 */
        while (intPart) {
            *p++ = '0' + intPart % 10;
            intPart /= 10;
        }

        /* 保存结束位置 */
        p1 = p;

        /* 反转结果 */
        while (p > ptr) {
            c = *--p;
            *p = *ptr;
            *ptr++ = c;
        }

        /* 恢复结束位置 */
        ptr = p1;
    }

    /* 小数部分 */
    if (precision) {
        /* 放置小数点 */
        *ptr++ = '.';

        /* 进行转换 */
        while (precision--) {
            f *= 10.0;
            c = (int)f % 10;
            *ptr++ = '0' + c;
            f -= c;
        }
    }
    *ptr = 0;

    /* 移除小数部分末尾多余的"0" */
    if (precision) {
        int j = 0;
        char *cBuf = buf;
        for (j = strlen(cBuf) - 1; j > 0 && cBuf[j] == '0'; --j) {
            cBuf[j] = 0;
        }
        if (j >= 1 && cBuf[j] == '.') cBuf[j] = 0;
    }

    return buf;
}

/* 重新实现 atoi()，某些平台缺少此函数 */
long kAtoi(const char* S) {
    long num = 0;
 
    int i = 0;
 
    /* 运行直到到达字符串末尾， */
    /* 或者当前字符为非数字 */
    while (S[i] && (S[i] >= '0' && S[i] <= '9')) {
        num = num * 10 + (S[i] - '0');
        i++;
    }
 
    return num;
}

/* itoa需要的翻转字符串功能  */
void kItoaReverse(char str[], int length) {
    int start = 0;
    int end = length -1;
    while (start < end)
    {
        char t = *(str+start);
        *(str+start) = *(str+end);
        *(str+end) = t;
        start++;
        end--;
    }
}
 
/* 重新实现 itoa()，某些平台缺少此函数 */
char* kItoa(int num, char* str, int base) {
    int i = 0;
    int isNegative = 0;
 
    /* 专门处理输入是0的情况 */
    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return str;
    }
 
    /* 在标准 itoa() 中，负数只处理十进制 */
    if (num < 0 && base == 10) {
        isNegative = 1;
        num = -num;
    }
 
    /* 处理单个数字 */
    while (num != 0) {
        int rem = num % base;
        str[i++] = (rem > 9)? (rem-10) + 'a' : rem + '0';
        num = num/base;
    }
 
    /* 如果数字为负数，则附加"-" */
    if (isNegative)
        str[i++] = '-';
 
    str[i] = '\0';
 
    /* 翻转内容 */
    kItoaReverse(str, i);
 
    return str;
}

int getLineTrimRemarks(const char* textPtr, char* line) {
    char *linePtr = line;
    const char *originalTextPtr = textPtr;

    /* 跳过空白 */
    while (isSpace(*textPtr)) ++textPtr;

    /* 从缓存中获取一行 */
    do {
        *linePtr++ = *textPtr++;
    } while(*textPtr != '\n' && *textPtr != '\0');

    *linePtr = '\0';

    /* 移除#开头的注释 */
    for (linePtr = line; *linePtr && *linePtr != '#'; ++linePtr) NULL;
    *linePtr = '\0';

    /* 移除尾部空白 */
    for (--linePtr; linePtr >= line && isSpace(*linePtr); --linePtr) {
        *linePtr = '\0';
    }

    /* 返回读取了多少字符 */
    return textPtr - originalTextPtr;
}

int getLineOnly(const char* textPtr, char* line) {
    char *linePtr = line;
    const char *originalTextPtr = textPtr;

    /* 从缓存中获取一行 */
    do {
        *linePtr++ = *textPtr++;
    } while(*textPtr != '\n' && *textPtr != '\0');

    /* 如果是换行，也读取进来 */
    if (*textPtr == '\n') *linePtr++ = *textPtr++;

    *linePtr-- = '\0';

    /* 移除末尾的空白 */
    while (isSpace(*linePtr)) *linePtr-- = '\0';

    /* 返回读取了多少字符 */
    return textPtr - originalTextPtr;
}

int checkIsLittleEndian() {
    /* 定义一个32位整数并赋值为1 */
    KDword num = 1;
    /* 将整数的地址转换为字符指针（访问单个字节） */
    KByte *bytePtr = (unsigned char *)&num;
    /* 检查第一个字节的值（最低内存地址） */
    return *bytePtr == 1;
}