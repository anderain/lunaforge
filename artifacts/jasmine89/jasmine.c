#include <stdlib.h>
#include <string.h>
#include "jasmine.h"

/* Token analyzer */

typedef enum {
    ITT_END,                ITT_UNKNOWN,            ITT_ERROR,              ITT_NUM_INT,
    ITT_NUM_FRACTION,       ITT_NUM_EXPONENT,       ITT_STRING,             ITT_TRUE,
    ITT_FALSE,              ITT_NULL,               ITT_CURLY_BRACE_LEFT,   ITT_CURLY_BRACE_RIGHT,
    ITT_SQUARE_BKT_LEFT,    ITT_SQUARE_BKT_RIGHT,   ITT_COLON,              ITT_COMMA
} JasmineTokenType;

JasmineToken* JasmineParser_AssignToken(JasmineToken* token, const char* strValue, int tokenType) {
    if (!token) return NULL;

    JasmineString_Copy(token->content, sizeof(token->content), strValue);
    token->type = tokenType;

    return token;
}

void JasmineParser_WindBack(JasmineParser* parser) {
    parser->eptr -= JasmineString_Length(parser->token.content);
}

#define JasmineTokenStream_Tail          (*ptrBuf++)
#define JasmineParser_ReadChar(p)        (*(((p)->eptr)++))
#define JasmineParser_PeakChar(p)        (*((p)->eptr))
#define JasmineParser_RewindChar(p)      (--(p)->eptr)
#define JasmineParser_CharAsToken(c, type) {             \
    JasmineTokenStream_Tail = c;                         \
    JasmineTokenStream_Tail = '\0';                      \
    JasmineParser_AssignToken(token, tokenBuf, type);    \
} NULL


JasmineToken* JasmineParser_ReadToken(JasmineParser* parser) {
    char            tokenBuf[TOKEN_LENGTH_MAX];
    char*           ptrBuf = tokenBuf;
    JasmineToken*   token = &parser->token;
    char            nextChar;
    int             tokenOperatorType = -1;

    /* Skip white space */
    while(JasmineChar_IsSpace(JasmineParser_PeakChar(parser))) {
        nextChar = JasmineParser_ReadChar(parser);
        if (nextChar == '\n') {
            parser->lineNumber++;
        }
    }

    nextChar = JasmineParser_PeakChar(parser);

    switch(nextChar) {
        case '{': tokenOperatorType = ITT_CURLY_BRACE_LEFT;     break;
        case '}': tokenOperatorType = ITT_CURLY_BRACE_RIGHT;    break;
        case '[': tokenOperatorType = ITT_SQUARE_BKT_LEFT;      break;
        case ']': tokenOperatorType = ITT_SQUARE_BKT_RIGHT;     break;
        case ':': tokenOperatorType = ITT_COLON;                break;
        case ',': tokenOperatorType = ITT_COMMA;                break;
    }

    if (tokenOperatorType != -1) {
        JasmineParser_CharAsToken(nextChar, tokenOperatorType);
        nextChar = JasmineParser_ReadChar(parser);
        return token;
    }

    /* Parser's content ends here */
    if (nextChar == 0) {
        JasmineParser_AssignToken(token, "", ITT_END);
        return token;
    }
    /* Numberic */
    else if (nextChar == '+' || nextChar == '-' || JasmineChar_IsDigit(nextChar)) {
        int tokenType = ITT_NUM_INT;

        if (nextChar == '+' || nextChar == '-') {
            JasmineTokenStream_Tail = JasmineParser_ReadChar(parser);
        }

        while (JasmineChar_IsDigit(JasmineParser_PeakChar(parser))) {
            JasmineTokenStream_Tail = JasmineParser_ReadChar(parser);
        }
        
        nextChar = JasmineParser_PeakChar(parser);

        /* Fraction */
        if (nextChar == '.') {
            /* Save '.' */
            JasmineTokenStream_Tail = JasmineParser_ReadChar(parser);

            nextChar = JasmineParser_PeakChar(parser);
            /* Error: next char is not digit */
            if (!JasmineChar_IsDigit(nextChar)) {
                JasmineTokenStream_Tail = nextChar;
                JasmineTokenStream_Tail = '\0';
                JasmineParser_AssignToken(token, tokenBuf, ITT_ERROR);
                return token;
            }

            while (JasmineChar_IsDigit(JasmineParser_PeakChar(parser))) {
                JasmineTokenStream_Tail = JasmineParser_ReadChar(parser);
            }

            nextChar = JasmineParser_PeakChar(parser);

            tokenType = ITT_NUM_FRACTION;
        }

        /* Exponent */
        if (nextChar == 'e' || nextChar == 'E') {
            /* Save 'e' or 'E' */
            JasmineTokenStream_Tail = JasmineParser_ReadChar(parser);
            
            nextChar = JasmineParser_PeakChar(parser);
            /* Error: not a legal expoent */
            if (nextChar == '+' || nextChar == '-') {
                JasmineTokenStream_Tail = JasmineParser_ReadChar(parser);
                nextChar = JasmineParser_PeakChar(parser);
            }

            /* Error: next char is not digit */
            if (!JasmineChar_IsDigit(nextChar)) {
                JasmineTokenStream_Tail = nextChar;
                JasmineTokenStream_Tail = '\0';
                JasmineParser_AssignToken(token, tokenBuf, ITT_ERROR);
                return token;
            }

            while (JasmineChar_IsDigit(JasmineParser_PeakChar(parser))) {
                JasmineTokenStream_Tail = JasmineParser_ReadChar(parser);
            }

            tokenType = ITT_NUM_EXPONENT;
        }

        /* Numberic end */
        JasmineTokenStream_Tail = '\0';

        JasmineParser_AssignToken(token, tokenBuf, tokenType);
        return token;
    }
    /* String */
    else if (nextChar == '"') {
        /* Save '"' */
        JasmineTokenStream_Tail = JasmineParser_ReadChar(parser);

        for (;;) {
            nextChar = JasmineParser_PeakChar(parser);
            /* Error: string cannot contain control characters */
            if (JasmineChar_IsCtrl(nextChar)) {
                JasmineTokenStream_Tail = nextChar;
                JasmineTokenStream_Tail = '\0';
                JasmineParser_AssignToken(token, tokenBuf, ITT_ERROR);
                return token;
            }
            else if (nextChar == '\\') {
                /* Save '\' */
                JasmineTokenStream_Tail = JasmineParser_ReadChar(parser);
                
                nextChar = JasmineParser_PeakChar(parser);
                if (nextChar == '"' || /* quotation mark */
                    nextChar == '\\'|| /* reverse solidus */
                    nextChar == '/' || /* solidus */
                    nextChar == 'b' || /* backspace */
                    nextChar == 'f' || /* formfeed */
                    nextChar == 'n' || /* linefeed */
                    nextChar == 'r' || /* carriage return */
                    nextChar == 't') { /* horizontal tab */
                    JasmineTokenStream_Tail = JasmineParser_ReadChar(parser);
                    continue;
                }
                /* Hex */
                else if (nextChar == 'u') {
                    int i;
                    /* Save 'u' */
                    JasmineTokenStream_Tail = JasmineParser_ReadChar(parser);
                    /* Read 4 hex digits */
                    for (i = 0; i < 4; ++i) {
                        nextChar = JasmineParser_PeakChar(parser);
                        /* Error: not a hex digit */
                        if (!JasmineChar_IsHexDigit(nextChar)) {
                            JasmineTokenStream_Tail = nextChar;
                            JasmineTokenStream_Tail = '\0';
                            JasmineParser_AssignToken(token, tokenBuf, ITT_ERROR);
                            return token;
                        }
                        JasmineTokenStream_Tail = JasmineParser_ReadChar(parser);
                    }
                    continue;
                }

                /* Error */
                JasmineTokenStream_Tail = nextChar;
                JasmineTokenStream_Tail = '\0';
                JasmineParser_AssignToken(token, tokenBuf, ITT_ERROR);
            }
            /* ends */
            else if (nextChar == '"') {
                JasmineTokenStream_Tail = JasmineParser_ReadChar(parser);
                break;
            }
            /* other char */
            else {
                JasmineTokenStream_Tail = JasmineParser_ReadChar(parser);
            }
        }
        JasmineTokenStream_Tail = '\0';
        JasmineParser_AssignToken(token, tokenBuf, ITT_STRING);
        return token;
    }
    /* Symbol */
    else if (JasmineChar_IsAlpha(nextChar)) {
        for (;;) {
            nextChar = JasmineParser_PeakChar(parser);
            if (JasmineChar_IsAlpha(nextChar) || JasmineChar_IsAlpha(nextChar)) {
                JasmineTokenStream_Tail = JasmineParser_ReadChar(parser);
            }
            else {
                break;
            }
        }
        JasmineTokenStream_Tail = '\0';

        if (JasmineString_Equal(tokenBuf, "true")) {
            JasmineParser_AssignToken(token, tokenBuf, ITT_TRUE);
            return token;
        }
        else if (JasmineString_Equal(tokenBuf, "false")) {
            JasmineParser_AssignToken(token, tokenBuf, ITT_FALSE);
            return token;
        }
        else if (JasmineString_Equal(tokenBuf, "null")) {
            JasmineParser_AssignToken(token, tokenBuf, ITT_NULL);
            return token;
        }

        JasmineParser_AssignToken(token, tokenBuf, ITT_UNKNOWN);
        return token;
    }

    JasmineParser_CharAsToken(nextChar, ITT_UNKNOWN);
    nextChar = JasmineParser_ReadChar(parser);

    return token;
}

JasmineParser* JasmineParser_Create(const char* contents) {
    JasmineParser* parser;

    parser              = (JasmineParser *)malloc(sizeof(JasmineParser));
    parser->expr        = contents;
    parser->eptr        = parser->expr;
    parser->lineNumber  = 1;

    return parser;
}

void JasmineParser_Dispose(JasmineParser* parser) {
    if (parser == NULL) return;
    free(parser);
}

/* List */

JasmineLinkedListNode* JasmineLListNode_Create(void *data) {
    JasmineLinkedListNode* n = (JasmineLinkedListNode *)malloc(sizeof(JasmineLinkedListNode));
    n->prev = n->next = NULL;
    n->data = data;
    return n;
}

JasmineLinkedList* JasmineLList_Create() {
    JasmineLinkedList *l = (JasmineLinkedList *)malloc(sizeof(JasmineLinkedList));
    l->head = l->tail = NULL;
    l->size = 0;
    return l;
}

JasmineLinkedList* JasmineLList_PushBack(JasmineLinkedList* _self, void *data) {
    JasmineLinkedListNode *new_node = JasmineLListNode_Create(data);

    if (_self->head == NULL) {
        _self->head = _self->tail = new_node;
    }
    else {
        JasmineLinkedListNode * tail = _self->tail;
        tail->next = new_node;
        new_node->prev = tail;
        _self->tail = new_node;
    }

    _self->size++;

    return _self;
}

void* JasmineLList_PopFront(JasmineLinkedList* _self) {
    JasmineLinkedListNode *head;
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

void JasmineLListNode_Dispose(JasmineLinkedListNode* vln, void (* releaseData)(void *)) {
    if (vln->data != NULL) {
        releaseData(vln->data);
        vln->data = NULL;
    }
    free(vln);
}

void JasmineLList_Dispose(JasmineLinkedList* _self, void (* releaseData)(void *)) {
    JasmineLinkedListNode *n1, *n2;

    n1 = _self->head;
    while (n1) {
        n2 = n1->next;
        JasmineLListNode_Dispose(n1, releaseData);
        n1 = n2;
    }

    free(_self);
}

/* Parser */

JasmineNode* JasmineNode_CreateNull() {
    JasmineNode* node   = NULL;
    node                = (JasmineNode *)malloc(sizeof(JasmineNode));
    node->type          = JASMINE_NODE_NULL;
    return node;
}

JasmineNode* JasmineNode_CreateString(char* strVal, int useDump) {
    JasmineNode* node   = JasmineNode_CreateNull();
    node->type          = JASMINE_NODE_STRING;
    node->data.pString  = useDump ? JasmineString_Dump(strVal) : strVal;
    return node;
}

JasmineNode* JasmineNode_CreateNum(JasmineFloat numVal) {
    JasmineNode* node   = JasmineNode_CreateNull();
    node->type          = JASMINE_NODE_NUMBER;
    node->data.fNumber  = numVal;
    return node;
}

JasmineNode* JasmineNode_CreateBool(int boolVal) {
    JasmineNode* node   = JasmineNode_CreateNull();
    node->type          = JASMINE_NODE_BOOLEAN;
    node->data.iBoolean = boolVal;
    return node;
}

JasmineNode* JasmineNode_CreateObject() {
    JasmineNode*    node    = JasmineNode_CreateNull();
    node->type              = JASMINE_NODE_OBJECT;
    node->data.listObj      = JasmineLList_Create();
    return node;
}

JasmineNode* JasmineNode_CreateArray() {
    JasmineNode* node       = JasmineNode_CreateNull();
    node->type              = JASMINE_NODE_ARRAY;
    node->data.listArray    = JasmineLList_Create();
    return node;
}

void JasmineNode_Dispose(JasmineNode* node) {
    if (node == NULL) {
        return;
    }
    if (node->type == JASMINE_NODE_NUMBER || node->type == JASMINE_NODE_BOOLEAN || node->type == JASMINE_NODE_NULL) {
        free(node);
    }
    else if (node->type == JASMINE_NODE_STRING) {
        free(node->data.pString);
        free(node);
    }
    else if (node->type == JASMINE_NODE_OBJECT) {
        JasmineLList_Dispose(node->data.listObj, JasmineKeyValue_DisposeVoidPtr);
        node->data.listObj = NULL;
        free(node);
    }
    else if (node->type == JASMINE_NODE_ARRAY) {
        JasmineLList_Dispose(node->data.listArray, JasmineNode_DisposeVoidPtr);
        free(node);
    }
}

void JasmineNode_DisposeVoidPtr (void* node) {
    JasmineNode_Dispose((JasmineNode*)node);
}

/* Key Value */

JasmineKeyValue* JasmineKeyValue_Create(char* keyName, JasmineNode* jasmineNode, int useDumpKeyName) {
    JasmineKeyValue* kv;

    kv = (JasmineKeyValue *)malloc(sizeof(JasmineKeyValue));

    kv->keyName     = useDumpKeyName ? JasmineString_Dump(keyName) : keyName;
    kv->jasmineNode = jasmineNode;

    return kv;
}

void JasmineKeyValue_Dispose(JasmineKeyValue* kv) {
    if (kv == NULL) return;

    free(kv->keyName);
    JasmineNode_Dispose(kv->jasmineNode);
}

void JasmineKeyValue_DisposeVoidPtr(void* kv) {
    JasmineKeyValue_Dispose((JasmineKeyValue*) kv);
}

/* Parser program */
int JasmineToken_IsNonObjValue(JasmineToken* token) {
    int type = token->type;
    if (type == ITT_NUM_INT         || type == ITT_NUM_FRACTION ||
        type == ITT_NUM_EXPONENT    || type == ITT_STRING       ||
        type == ITT_TRUE            || type == ITT_FALSE        || type == ITT_NULL) {
        return 1;
    }
    return 0;
}

JasmineNode* JasmineToken_CreateFromNonObjValue(JasmineToken *token) {
    int type = token->type;

    if (type == ITT_NUM_INT || type == ITT_NUM_FRACTION || type == ITT_NUM_EXPONENT) {
        return JasmineNode_CreateNum(Jasmine_Atof(token->content));
    }
    else if (type == ITT_STRING) {
        return JasmineNode_CreateString(Jasmine_ExtractString(token->content), 0);
    }
    else if (type == ITT_TRUE) {
        return JasmineNode_CreateBool(1);
    }
    else if (type == ITT_FALSE) {
        return JasmineNode_CreateBool(0);
    }
    else if (type == ITT_NULL) {
        return JasmineNode_CreateNull();
    }

    return NULL;
}

#define Jasmine_ReturnSyntaxError() {           \
    parser->errorCode = JASMINE_ERROR_SYNTAX;   \
    JasmineString_Append(                       \
        parser->errorString,                    \
        sizeof(parser->errorString),            \
        "Syntax error: invalid token : "        \
    );                                          \
    JasmineString_Append(                       \
        parser->errorString,                    \
        sizeof(parser->errorString),            \
        token->content                          \
    );                                          \
    JasmineNode_Dispose(node);                  \
    return NULL;                                \
} NULL

/* * Parse Object */
/* Left square bucket has been read by caller */

JasmineNode* Jasmine_ParseObject(JasmineParser* parser) {
    JasmineNode*    node;
    JasmineToken*   token;

    node = JasmineNode_CreateObject();

    for (;;) {
        token = JasmineParser_ReadToken(parser);
        /* Ends */
        if (token->type == ITT_CURLY_BRACE_RIGHT) {
            break;
        }
        /* Key-Value */
        else if (token->type == ITT_STRING) {
            char *keyName = NULL;

            keyName = Jasmine_ExtractString(token->content);

            token = JasmineParser_ReadToken(parser);
            if (token->type != ITT_COLON) {
                Jasmine_ReturnSyntaxError();
            }

            token = JasmineParser_ReadToken(parser);
            /* Array */
            if (token->type == ITT_SQUARE_BKT_LEFT) {
                JasmineNode* childNode = Jasmine_ParseArray(parser);

                if (!childNode) {
                    JasmineNode_Dispose(node);
                    return NULL;
                }

                JasmineLList_PushBack(node->data.listArray, JasmineKeyValue_Create(keyName, childNode, 0));
            }
            /* Object */
            else if (token->type == ITT_CURLY_BRACE_LEFT) {
                JasmineNode* childNode = Jasmine_ParseObject(parser);

                if (!childNode) {
                    JasmineNode_Dispose(node);
                    return NULL;
                }

                JasmineLList_PushBack(node->data.listArray, JasmineKeyValue_Create(keyName, childNode, 0));
            }
            /* Value */
            else if (JasmineToken_IsNonObjValue(token)) {
                JasmineLList_PushBack(node->data.listArray, JasmineKeyValue_Create(keyName, JasmineToken_CreateFromNonObjValue(token), 0));
            }
            /* Error */
            else {
                if (!keyName) free(keyName);
                Jasmine_ReturnSyntaxError();
            }
        }
        /* Error */
        else {
            Jasmine_ReturnSyntaxError();
        }

        token = JasmineParser_ReadToken(parser);

        /* End! */
        if (token->type == ITT_CURLY_BRACE_RIGHT) {
            break;
        }
        else if (token->type == ITT_COMMA) {
            continue;
        }
        else {
            Jasmine_ReturnSyntaxError();
        }

    }

    return node;
}

/* * Parse Array */
/* Left curly brace has been read by caller */

JasmineNode* Jasmine_ParseArray(JasmineParser* parser) {
    JasmineNode*    node;
    JasmineToken*   token;

    node = JasmineNode_CreateArray();

    for (;;) {
        token = JasmineParser_ReadToken(parser);
        /* End */
        if (token->type == ITT_CURLY_BRACE_RIGHT) {
            break;
        }
        /* Array */
        else if (token->type == ITT_SQUARE_BKT_LEFT) {
            JasmineNode* childNode = Jasmine_ParseArray(parser);

            if (!childNode) {
                JasmineNode_Dispose(node);
                return NULL;
            }

            JasmineLList_PushBack(node->data.listArray, childNode);
        }
        /* Object */
        else if (token->type == ITT_CURLY_BRACE_LEFT) {
            JasmineNode* childNode = Jasmine_ParseObject(parser);

            if (!childNode) {
                JasmineNode_Dispose(node);
                return NULL;
            }

            JasmineLList_PushBack(node->data.listArray, childNode);
        }
        /* Value */
        else if (JasmineToken_IsNonObjValue(token)) {
            JasmineLList_PushBack(node->data.listArray, JasmineToken_CreateFromNonObjValue(token));
        }
        /* Error */
        else {
            Jasmine_ReturnSyntaxError();
        }

        token = JasmineParser_ReadToken(parser);

        /* End! */
        if (token->type == ITT_SQUARE_BKT_RIGHT) {
            break;
        }
        else if (token->type == ITT_COMMA) {
            continue;
        }
        else {
            Jasmine_ReturnSyntaxError();
        }

    }

    return node;
}

JasmineNode* Jasmine_Parse(const char * szPlainText, JasmineParser** parserArg) {
    JasmineParser*  parser  = NULL;
    JasmineToken*   token   = NULL;
    JasmineNode*    node    = NULL;

    parser = JasmineParser_Create(szPlainText);
    token = JasmineParser_ReadToken(parser);

    if (parserArg) *parserArg = parser;

    JasmineString_Copy(parser->errorString, sizeof(parser->errorString),"[JASMINE] ");
    parser->errorCode = JASMINE_ERROR_NO_ERROR;

    /* File = Object */
    if (token->type == ITT_CURLY_BRACE_LEFT) {
        node = Jasmine_ParseObject(parser);

        if (!node) {
            /* JasmineNode_Dispose(node); */
            return NULL;
        }
        
        token = JasmineParser_ReadToken(parser);
        
        if (token->type != ITT_END) {
            Jasmine_ReturnSyntaxError();
        }
        
        return node;
    }
    /*  File = Array */
    else if (token->type == ITT_SQUARE_BKT_LEFT) {
        node = Jasmine_ParseArray(parser);
        
        if (!node) {
            /* JasmineNode_Dispose(node); */
            return NULL;
        }
        
        token = JasmineParser_ReadToken(parser);
        
        if (token->type != ITT_END) {
            Jasmine_ReturnSyntaxError();
        }
        
        return node;
    }
    /* Error */
    else {
        parser->errorCode = JASMINE_ERROR_START_BRACE;
        JasmineString_Append(parser->errorString, sizeof(parser->errorString), "JSON should start with '[' or '{'");
        return NULL;
    }

    return NULL;
}
