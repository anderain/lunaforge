#include <stdlib.h>
#include <string.h>
#include "kutils.h"

VlistNode* vlnNewNode(void *data) {
    VlistNode* n = (VlistNode *)malloc(sizeof(VlistNode));
    n->prev = n->next = NULL;
    n->data = data;
    return n;
}

Vlist* vlNewList() {
    Vlist *l = (Vlist *)malloc(sizeof(Vlist));
    l->head = l->tail = NULL;
    l->size = 0;
    return l;
}

Vlist* vlPushBack(Vlist* _self, void *data) {
    VlistNode *newNode = vlnNewNode(data);

    if (_self->head == NULL) {
        _self->head = _self->tail = newNode;
    }
    else {
        VlistNode * tail = _self->tail;
        tail->next = newNode;
        newNode->prev = tail;
        _self->tail = newNode;
    }

    _self->size++;

    return _self;
}

void* vlPopFront(Vlist* _self) {
    VlistNode *head;
    void *data;

    head = _self->head;

    if (head == NULL) return NULL;

    data = head->data;

    _self->head = head->next;
    if (_self->head) {
        _self->head->prev = NULL;
    }
    else {
        _self->tail = NULL;
    }

    free(head);
    _self->size--;

    return data;
}

void* vlPopBack(Vlist* _self) {
    VlistNode *tail, *prevTail; 
    void * data;

    if (_self->size <= 0) {
        return NULL;
    }

    tail = _self->tail;
    data = tail->data;

    if (_self->size == 1) {
        _self->head = NULL;
        _self->tail = NULL;
        free(tail);
        _self->size = 0;
        return data;
    }

    prevTail = tail->prev;
    prevTail->next = NULL;
    _self->tail = prevTail;
    free(tail);
    _self->size--;

    return data;
}

void vlnDestroy(VlistNode* vln, void (* releaseData)(void *)) {
    if (vln->data != NULL && releaseData) {
        releaseData(vln->data);
        vln->data = NULL;
    }
    free(vln);
}

void vlDestroy(Vlist* _self, void (* releaseData)(void *)) {
    VlistNode *n1, *n2;

    n1 = _self->head;
    while (n1) {
        n2 = n1->next;
        vlnDestroy(n1, releaseData);
        n1 = n2;
    }

    free(_self);
}

int KUtils_StringCopy(char *dest, int max, const char *src) {
    int i;
    for (i = 0; i < max - 1 && src[i]; ++i) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
    return i;
}

char* KUtils_StringDump(const char *str) {
    int length = strlen(str) + 1;
    char *buffer = (char *)malloc(length);
    KUtils_StringCopy(buffer, length + 1, str);
    return buffer;
}

KBool KUtils_IsStringEqual(const char* szA, const char* szB) {
    return strcmp(szA, szB) == 0;
}

KBool KUtils_IsStringEndWith(const char *str, const char *token) {
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

char* KUtils_StringConcat(const char* szLeft, const char* szRight) {
    char* szBuf;
    size_t lenLeft, lenRight, lenTotal;

    /* 参数检查 */
    if (szLeft == NULL || szRight == NULL) {
        return NULL;
    }

    lenLeft = strlen(szLeft);
    lenRight = strlen(szRight);
    lenTotal = lenLeft + lenRight;

    szBuf = (char*)malloc(lenTotal + 1);
    if (szBuf == NULL) {
        return NULL; /* 内存分配失败 */
    }

    /* 复制字符串 */
    memcpy(szBuf, szLeft, lenLeft);
    memcpy(szBuf + lenLeft, szRight, lenRight);

    /* 添加结束符 */
    szBuf[lenTotal] = '\0';

    return szBuf;
}

double KUtils_Atof(const char *str) {
    int sign;
    double number = 0.0, power = 1.0;
    /* Skip whitespace */
    while(isSpace(*str)) ++str;
    /* Handle sign */
    sign = (*str == '-') ? -1 : 1; 
    if (*str == '-' || *str == '+') {
        str++;
    }
    /* Process digits before decimal point */
    while(*str >= '0' && *str <= '9') {
        number = 10.0 * number + (*str - '0');
        str++;
    }
    /* Skip decimal point */
    if(*str == '.') {
        str++;
    }
    /* Process digits after decimal point */
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

char* KUtils_Ftoa(double f, char * buf, int precision) {
    char* ptr = buf;
    char* p = ptr;
    char* p1;
    char c;
    long intPart;

    /* Check precision boundaries */
    if (precision > MAX_PRECISION)
        precision = MAX_PRECISION;

    /* Handle sign */
    if (f < 0) {
        f = -f;
        *ptr++ = '-';
    }

    /* Negative precision indicates automatic precision detection */
    if (precision < 0) {
        if (f < 1.0) precision = 6;
        else if (f < 10.0) precision = 5;
        else if (f < 100.0) precision = 4;
        else if (f < 1000.0) precision = 3;
        else if (f < 10000.0) precision = 2;
        else if (f < 100000.0) precision = 1;
        else precision = 0;
    }

    /* Round value according to precision */
    if (precision)
        f += rounders[precision];

    /* Integer part... */
    intPart = (long)f;
    f -= intPart;

    if (!intPart)
        *ptr++ = '0';
    else {
        /* Save start pointer */
        p = ptr;

        /* Convert in reverse order */
        while (intPart) {
            *p++ = '0' + intPart % 10;
            intPart /= 10;
        }

        /* Save end position */
        p1 = p;

        /* Reverse the result */
        while (p > ptr) {
            c = *--p;
            *p = *ptr;
            *ptr++ = c;
        }

        /* Restore end position */
        ptr = p1;
    }

    /* Decimal part */
    if (precision) {
        /* Place decimal point */
        *ptr++ = '.';

        /* Perform conversion */
        while (precision--) {
            f *= 10.0;
            c = (int)f % 10;
            *ptr++ = '0' + c;
            f -= c;
        }
    }
    *ptr = 0;

    /* Remove trailing zeros from decimal part */
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

/* Reimplement atoi(), some platforms lack this function */
long KUtils_Atoi(const char* S) {
    long num = 0;
 
    int i = 0;
 
    /* Run until reaching end of string, */
    /* or current character is not a digit */
    while (S[i] && (S[i] >= '0' && S[i] <= '9')) {
        num = num * 10 + (S[i] - '0');
        i++;
    }
 
    return num;
}

/* String reversal function needed for itoa */
static void KUtils_ItoaReverse(char str[], int length) {
    int start = 0;
    int end = length -1;
    while (start < end) {
        char t = *(str+start);
        *(str+start) = *(str+end);
        *(str+end) = t;
        start++;
        end--;
    }
}
 
/* Reimplement itoa(), some platforms lack this function */
char* KUtils_Itoa(int num, char* str, int base) {
    int i = 0;
    int isNegative = 0;
 
    /* Specifically handle input being 0 */
    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return str;
    }
 
    /* In standard itoa(), negative numbers are only handled for decimal base */
    if (num < 0 && base == 10) {
        isNegative = 1;
        num = -num;
    }
 
    /* Process individual digits */
    while (num != 0) {
        int rem = num % base;
        str[i++] = (rem > 9)? (rem-10) + 'a' : rem + '0';
        num = num/base;
    }
 
    /* If number is negative, append "-" */
    if (isNegative)
        str[i++] = '-';
 
    str[i] = '\0';
 
    /* Reverse the content */
    KUtils_ItoaReverse(str, i);
 
    return str;
}

#define KB_FLOAT_EPSILON (1e-6f)

int KUtils_FloatEqualRel(KFloat a, KFloat b) {
    KFloat diff = a >= b ? a - b : b - a;
    KFloat absA = a >= 0 ? a : -a;
    KFloat absB = b >= 0 ? b : -b;
    KFloat largest = absA > absB ? absA : absB;
    return diff <= largest * KB_FLOAT_EPSILON;
}

int KUtils_IsLittleEndian() {
    /* 定义一个32位整数并赋值为1 */
    KDword num = 1;
    /* 将整数的地址转换为字符指针（访问单个字节） */
    KByte *bytePtr = (unsigned char *)&num;
    /* 检查第一个字节的值（最低内存地址） */
    return *bytePtr == 1;
}