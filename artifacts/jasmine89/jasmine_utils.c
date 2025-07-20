#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "jasmine.h"

const char* JASMINE_NODE_TYPE_NAME[] = {
    "NULL", "OBJECT", "ARRAY", "STRING", "NUMBER", "BOOLEAN"
};

int JasmineSchema_Validate(
    const char* pNodeName,
    JasmineNode* pNode, 
    const JasmineSchema* pSchema,
    char *errMessage
) {
    if (pNodeName == NULL || pNode == NULL || pSchema == NULL) {
        return 0;
    }
    if (pNode->type != pSchema->type) {
        if (errMessage) {
            sprintf(
                errMessage,
                "The type of property '%s' should be %s, but it gets %s",
                pNodeName,
                JASMINE_NODE_TYPE_NAME[pSchema->type],
                JASMINE_NODE_TYPE_NAME[pNode->type]
            );
        }
        return 0;
    }
    /* Validate children in array */
    if (pNode->type == JASMINE_NODE_ARRAY) {
        JasmineLinkedListNode* listNode = pNode->data.listArray->head;
        while (listNode != NULL) {
            JasmineNode* pChild = (JasmineNode *)listNode->data;
            /* Children type mismatch */
            if (pChild->type != pSchema->childrenType) {
                if (errMessage) {
                    sprintf(
                        errMessage,
                        "The elements in the array '%s' should be %s, but it gets %s",
                        pNodeName,
                        JASMINE_NODE_TYPE_NAME[pSchema->childrenType],
                        JASMINE_NODE_TYPE_NAME[pChild->type]
                    );
                }
                return 0;
            }
            listNode = listNode->next;
        }
        return 1;
    }
    /* Validate properties */
    else if (pNode->type == JASMINE_NODE_OBJECT && pSchema->propertySchema) {
        const JasmineObjectPropertySchema* pProperty;
        /* Check properties in schema */
        for (pProperty = pSchema->propertySchema; pProperty && pProperty->name != NULL; ++pProperty) {
            JasmineNode* pChild = JasmineObject_Child(pNode, pProperty->name);
            int childResult = 0;
            if (pChild == NULL) {
                if (pProperty->schema.isOptional) {
                    continue;
                }
                if (errMessage) {
                    sprintf(
                        errMessage,
                        "There is no property '%s' in '%s'.",
                        pProperty->name,
                        pNodeName
                    );
                }
                return 0;
            }
            childResult = JasmineSchema_Validate(pProperty->name, pChild, &pProperty->schema, errMessage);
            if (!childResult) {
                return 0;
            }
        }
    }
    return 1;
}

/* Note! */
/* Need to free return value manually */
char* JasmineConfig_LoadStringDumped(JasmineNode* _node, const char *path, const char* fallbackString) {
    JasmineNode* nodeFound = JasmineConfig_FindNode(_node, path);

    /* Not found */
    if (nodeFound == NULL) {
        return JasmineString_Dump(fallbackString);
    }
    /* Array: fallback */
    else if (JasmineNode_IsArray(nodeFound)) {
        return JasmineString_Dump(fallbackString);
    }
    /* Object: fallback */
    else if (JasmineNode_IsObject(nodeFound)) {
        return JasmineString_Dump(fallbackString);
    }
    /* String: dump */
    else if (JasmineNode_IsString(nodeFound)) {
        return JasmineString_Dump(nodeFound->data.pString);
    }
    /* Number */
    else if (JasmineNode_IsNumber(nodeFound)) {
        char buf[TOKEN_LENGTH_MAX];
        Jasmine_Ftoa(nodeFound->data.fNumber, buf);
        return JasmineString_Dump(buf);
    }
    /* Boolean: "true" / "false" */
    else if (JasmineNode_IsBoolean(nodeFound)) {
        return JasmineString_Dump(nodeFound->data.iBoolean ? "true" : "false");
    }
    /* Null: "null" */
    else if (JasmineNode_IsNull(nodeFound)) {
        /* return JasmineString_Dump("null"); */
        return JasmineString_Dump(fallbackString);
    }

    return JasmineString_Dump(fallbackString);
}

JasmineFloat JasmineConfig_LoadFloat(JasmineNode* _node, const char *path, JasmineFloat fallbackValue) {
    JasmineNode* nodeFound = JasmineConfig_FindNode(_node, path);

    /* printf("found type %s\n", JasmineNode_TypeString(nodeFound)); */

    /* Not found */
    if (nodeFound == NULL) {
        return fallbackValue;
    }
    /* Array: fallback */
    else if (JasmineNode_IsArray(nodeFound)) {
        return fallbackValue;
    }
    /* Object: fallback */
    else if (JasmineNode_IsObject(nodeFound)) {
        return fallbackValue;
    }
    /* String: convert to float */
    else if (JasmineNode_IsString(nodeFound)) {
        return Jasmine_Atof(nodeFound->data.pString);
    }
    /* Number */
    else if (JasmineNode_IsNumber(nodeFound)) {
        return (JasmineFloat)(nodeFound->data.fNumber);
    }
    /* Boolean: convert to 0.0 or 1.0 */
    else if (JasmineNode_IsBoolean(nodeFound)) {
        return (JasmineFloat)(nodeFound->data.iBoolean);
    }
    /* Null: fallback */
    else if (JasmineNode_IsNull(nodeFound)) {
        return fallbackValue;
    }

    return fallbackValue;
}

int JasmineConfig_LoadInteger(JasmineNode* _node, const char *path, int fallbackValue) {
    JasmineNode* nodeFound = JasmineConfig_FindNode(_node, path);

    /* Not found */
    if (nodeFound == NULL) {
        return fallbackValue;
    }
    /* Array: fallback */
    else if (JasmineNode_IsArray(nodeFound)) {
        return fallbackValue;
    }
    /* Object: fallback */
    else if (JasmineNode_IsObject(nodeFound)) {
        return fallbackValue;
    }
    /* String: convert to int */
    else if (JasmineNode_IsString(nodeFound)) {
        return Jasmine_Atoi(nodeFound->data.pString);
    }
    /* Number: cast to int */
    else if (JasmineNode_IsNumber(nodeFound)) {
        return (int)(nodeFound->data.fNumber);
    }
    /* Boolean: convert to 0 or 1 */
    else if (JasmineNode_IsBoolean(nodeFound)) {
        return nodeFound->data.iBoolean;
    }
    /* Null: use 0 */
    else if (JasmineNode_IsNull(nodeFound)) {
        return 0;
    }

    return fallbackValue;
}

/* Config loading */
/* path example = "person.12.age" */
/* obj.person[12].age */

#define JASMINE_CONFIG_PATH_SPLITTER '.'

JasmineNode * JasmineConfig_FindNode(JasmineNode *node, const char *path) {
    int             numSplitted             = 1;
    int             i                       = 0;
    int             prev                    = 0;
    char            buf[TOKEN_LENGTH_MAX]   = "";
    JasmineNode*    currentNode             = node;
    char**          explodedPath            = NULL;

    for (i = 0; path[i] != '\0'; ++i) {
        if (JASMINE_CONFIG_PATH_SPLITTER == path[i]) {
            numSplitted++;
        }
    }

    explodedPath = (char **)malloc(sizeof(char *) * numSplitted);

    for (i = 0; i < numSplitted; ++i) {
        int j;
        int k = 0;
        for (j = prev; path[j] && path[j] != JASMINE_CONFIG_PATH_SPLITTER; ++j, ++k) {
            buf[k] = path[j];
        }
        buf[k] = '\0';
        prev = j + 1;
        explodedPath[i] = JasmineString_Dump(buf);
    }

    for (i = 0; i < numSplitted; ++i) {
        const char *path = explodedPath[i];

        /* Debug */
        /* printf("[%d] path = %s\n", i, path); */
        /* TestPrintNode(currentNode, 0, 0); */

        /* Length of splitted path = 0 ? */
        if (JasmineString_Length(path) <= 0) {
            currentNode = NULL;
            break;
        }
        /* Index in array */
        else if (JasmineChar_IsDigit(path[0])) {
            int index;

            index       = Jasmine_Atoi(path);
            currentNode = JasmineArray_At(currentNode, index);

            if (currentNode == NULL) {
                break;
            }
        }
        /* Property in object */
        else {
            currentNode = JasmineObject_Child(currentNode, path);

            if (currentNode == NULL) {
                break;
            }
        }
    }

    for (i = 0; i < numSplitted; ++i) {
        free(explodedPath[i]);
    }
    free(explodedPath);

    /* printf("found!"); */
    /* TestPrintNode(currentNode, 0, 0); */

    return currentNode;
}

/* Get child utilites */

JasmineNode * JasmineObject_Child(JasmineNode* _node, const char *key) {
    const char*             property = key;
    JasmineLinkedListNode*  ln;
    JasmineNode*            nodeFound = NULL;

    /* Not a object */
    if (!JasmineNode_IsObject(_node)) {
        return NULL;
    }

    for (ln = _node->data.listObj->head; ln != NULL; ln = ln->next) {
        JasmineKeyValue* kv = (JasmineKeyValue *)ln->data;
        
        if (JasmineString_Equal(kv->keyName, property)) {
            nodeFound = kv->jasmineNode;
            break;
        }
    }

    return nodeFound;
}

JasmineNode * JasmineArray_At(JasmineNode* _node, int index) {
    int                     j;
    JasmineLinkedListNode*  ln;

    /* Not a array */
    if (!JasmineNode_IsArray(_node)) {
        return NULL;
    }

    ln = _node->data.listArray->head;

    for (j = 0; j < index && ln != NULL; ++j) {
        ln = ln->next;
    }

    if (ln == NULL) {
        return NULL;
    }

    return (JasmineNode *)ln->data;
}


/* String utilites */

int JasmineString_Copy(char *dest, int max, const char *src) {
    int i;
    for (i = 0; i < max - 1 && src[i]; ++i) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
    return i;
}

int JasmineString_Append(char *dest, int max, const char *src) {
    int len = JasmineString_Length(dest);
    return JasmineString_Copy(dest + len, max - len, src);
}

char* JasmineString_Dump(const char *str) {
    int length = JasmineString_Length(str) + 1;
    char *buffer = (char *)malloc(length);
    JasmineString_Copy(buffer, length + 1, str);
    return buffer;
}

static int Jasmine_CharHexDigitToValue(int c) {
    if (c >= '0' && c <= '9') {
        return c - '0' + 0;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 0xa;
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'f' + 0xa;
    }
    return 0;
}

/* Number <-> Array Utilites */

char* Jasmine_ExtractString(const char* stringInJson) {
    unsigned char   strbuf[TOKEN_LENGTH_MAX];
    unsigned char*  ptrbuf = strbuf;
    const char*     ptrsrc = stringInJson;

    /* skip first '"' */
    ++ptrsrc;   

    while (*ptrsrc != '\0' && *ptrsrc != '"') {
        char ic = *ptrsrc;
        if (ic == '\\') {
            /* skip '\'  */
            ptrsrc++;
            ic = *ptrsrc;
            switch (ic) {
                case '"'  : /* quotation mark */
                case '\\' : /* reverse solidus */
                case '/'  : /* solidus */
                    *ptrbuf++ = *ptrsrc++;
                    break;
                case 'b'  : /* backspace */
                    *ptrbuf++ = '\b'; ptrsrc++;
                    break;
                case 'f'  : /* formfeed */
                    *ptrbuf++ = '\f'; ptrsrc++;
                    break;
                case 'n'  : /* linefeed */
                    *ptrbuf++ = '\n'; ptrsrc++;
                    break;
                case 'r'  : /* carriage return */
                    *ptrbuf++ = '\r'; ptrsrc++;
                    break;
                case 't'  : /* horizontal tab */
                    *ptrbuf++ = '\t'; ptrsrc++;
                    break;
                case 'u'  : { /* hex */
                    int hh, hl, lh, ll;
                    
                    ptrsrc++;

                    hh = Jasmine_CharHexDigitToValue(*ptrsrc++);
                    hl = Jasmine_CharHexDigitToValue(*ptrsrc++);
                    lh = Jasmine_CharHexDigitToValue(*ptrsrc++);
                    ll = Jasmine_CharHexDigitToValue(*ptrsrc++);

                    *ptrbuf++ = (lh << 4) + ll; /* low byte */
                    *ptrbuf++ = (hh << 4) + hl; /* high byte */

                    break;
                }
            }
        } else {
            *ptrbuf++ = ic;
            ptrsrc++;
        }
    }

    *ptrbuf++ = '\0';

    return JasmineString_Dump((const char *)strbuf);
}

JasmineFloat Jasmine_Atof(const char* str) {
    int             isNeg       = 0;
    int             isExpNeg    = 0;
    const char*     ptr         = str;
    int             integer     = 0;
    JasmineFloat    fraction    = 0;
    int             exponent    = 0;
    int             digit       = 0;
    JasmineFloat    scale       = 0.1f;

    if (*ptr == '+') {
        ptr++;
    }
    else if (*ptr == '-') {
        isNeg = 1;
        ptr++;
    }

    while (JasmineChar_IsDigit(*ptr)) {
        digit = *ptr - '0';
        integer = (integer << 3) + (integer << 1) + digit;
        ptr++;
    }

    if (*ptr == '.') {
        ptr++;
        while (JasmineChar_IsDigit(*ptr)) {
            int digit = *ptr - '0';
            fraction += scale * digit;
            scale = scale * 0.1f;
            ptr++;
        }
    }

    if (*ptr == 'E' || *ptr == 'e') {
        ptr++;
        if (*ptr == '+') {
            ptr++;
        }
        else if (*ptr == '-') {
            isExpNeg = 1;
            ptr++;
        }
        while (JasmineChar_IsDigit(*ptr)) {
            digit = *ptr - '0';
            exponent = (exponent << 3) + (exponent << 1) + digit;
            ptr++;
        }
    }

    if (exponent == 0) {
        fraction = integer + fraction;
    }
    else {
        fraction = (JasmineFloat)((integer + fraction) * pow(10, isExpNeg ? -exponent : exponent));
    }

    return isNeg ? -fraction : fraction;
}

/* A utility function to reverse a string */
static void Jasmine_ItoaReverse(char str[], int length) {
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
 
char* Jasmine_Itoa(int num, char* str, int base) {
    int i = 0;
    int isNegative = 0;
 
    /* Handle 0 explicitly, otherwise empty string is printed for 0 */
    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return str;
    }
 
    /* In standard itoa(), negative numbers are handled only with */
    /* base 10. Otherwise numbers are considered unsigned. */
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
 
    /* If number is negative, append '-' */
    if (isNegative)
        str[i++] = '-';
 
    str[i] = '\0'; /* Append string terminator */
 
    /* Reverse the string */
    Jasmine_ItoaReverse(str, i);
 
    return str;
}

int Jasmine_Atoi(const char* S) {
    int num = 0;
 
    int i = 0;
 
    /* run till the end of the string is reached, or the */
    /* current character is non-numeric */
    while (S[i] && (S[i] >= '0' && S[i] <= '9')) {
        num = num * 10 + (S[i] - '0');
        i++;
    }
 
    return num;
}

#define FTOA_MAX_PRECISION  (10)

static const double FTOA_ROUNDERS[FTOA_MAX_PRECISION + 1] = {
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

static char* localFtoa(double f, char * buf, int precision) {
    char * ptr = buf;
    char * p = ptr;
    char * p1;
    char c;
    long intPart;

    /* check precision bounds */
    if (precision > FTOA_MAX_PRECISION)
        precision = FTOA_MAX_PRECISION;

    /* sign stuff */
    if (f < 0) {
        f = -f;
        *ptr++ = '-';
    }

    if (precision < 0) { /* negative precision == automatic precision guess */
        if (f < 1.0) precision = 6;
        else if (f < 10.0) precision = 5;
        else if (f < 100.0) precision = 4;
        else if (f < 1000.0) precision = 3;
        else if (f < 10000.0) precision = 2;
        else if (f < 100000.0) precision = 1;
        else precision = 0;
    }

    /* round value according the precision */
    if (precision)
        f += FTOA_ROUNDERS[precision];

    /* integer part... */
    intPart = (long)f;
    f -= intPart;

    if (!intPart)
        *ptr++ = '0';
    else {
        /* save start pointer */
        p = ptr;

        /* convert (reverse order) */
        while (intPart)
        {
            *p++ = '0' + intPart % 10;
            intPart /= 10;
        }

        /* save end pos */
        p1 = p;

        /* reverse result */
        while (p > ptr)
        {
            c = *--p;
            *p = *ptr;
            *ptr++ = c;
        }

        /* restore end pos */
        ptr = p1;
    }

    /* decimal part */
    if (precision) {
        /* place decimal point */
        *ptr++ = '.';

        /* convert */
        while (precision--)
        {
            f *= 10.0;
            c = (int)f;
            *ptr++ = '0' + c;
            f -= c;
        }
    }

    /* terminating zero */
    *ptr = 0;

    return buf;
}

char* Jasmine_Ftoa(double value, char* buffer) {
    char* cbuf;
    int j = 0;
    
    cbuf = localFtoa(value, buffer, FTOA_MAX_PRECISION - 2);

    for (j = strlen(cbuf) - 1; j > 0 && cbuf[j] == '0'; --j) {
        cbuf[j] = 0;
    }

    if (j >= 1 && cbuf[j] == '.') cbuf[j] = 0;

    return buffer;
}