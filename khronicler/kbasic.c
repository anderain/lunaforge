#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "kbasic.h"
#include "k_utils.h"

// keywords
typedef struct {
    const char *word;
} KeywordDef;

#define KEYWORD_LET     "let"
#define KEYWORD_GOTO    "goto"
#define KEYWORD_IF      "if"
#define KEYWORD_THEN    "then"
#define KEYWORD_ELSEIF  "elseif"
#define KEYWORD_ELSE    "else"
#define KEYWORD_WHILE   "while"
#define KEYWORD_END     "end"
#define KEYWORD_BREAK   "break"
#define KEYWORD_GOSUB   "gosub"

static const KeywordDef KEYWORDS[] = {
    { KEYWORD_LET       },
    { KEYWORD_GOTO      },
    { KEYWORD_IF        },
    { KEYWORD_THEN      },
    { KEYWORD_ELSEIF    },
    { KEYWORD_ELSE      },
    { KEYWORD_WHILE     },
    { KEYWORD_END       },
    { KEYWORD_BREAK     },
    { KEYWORD_GOSUB     },
    { NULL              }
};

// function: defined num of parameters
typedef struct {
    const char* funcName;
    int         numArgs;
    int         funcId;
} FunctionDef;

static const FunctionDef FUNCTION_LIST[] = {
    { "p",          1, KFID_P           },
    { "sin",        1, KFID_SIN         },
    { "cos",        1, KFID_COS         },
    { "tan",        1, KFID_TAN         }, 
    { "sqrt",       1, KFID_SQRT        },
    { "exp",        1, KFID_EXP         }, 
    { "abs",        1, KFID_ABS         },
    { "log",        1, KFID_LOG         },
    { "rand",       0, KFID_RAND        },
    { "zeropad",    2, KFID_ZEROPAD     },
};

typedef enum {
    TOKEN_ERR,      TOKEN_END,      TOKEN_NUM,
    TOKEN_ID,       TOKEN_OPR,      TOKEN_FNC,
    TOKEN_BKT,      TOKEN_CMA,      TOKEN_STR,
    TOKEN_LABEL,    TOKEN_KEY,
    TOKEN_UDF
} TokenType;

const char* DBG_TOKEN_NAME[] = {
    "TOKEN_ERR",    "TOKEN_END",    "TOKEN_NUM",
    "TOKEN_ID",     "TOKEN_OPR",    "TOKEN_FNC",
    "TOKEN_BKT",    "TOKEN_CMA",    "TOKEN_STR",
    "TOKEN_LABEL",
    "TOKEN_UDF"
};

typedef struct {
    int type;
    int sourceLength;
    char content[KB_CONTEXT_STRING_BUFFER_MAX];
} Token;

typedef struct tagExprNode {
    int type;
    // 'param' is only for operator
    // to store OPERATOR 
    int param;
    char content[KB_CONTEXT_STRING_BUFFER_MAX];
    struct tagExprNode **children;
    int numChild;
} ExprNode;

typedef enum {
    OPR_NEG = 0,
    OPR_CONCAT,
    OPR_ADD,        OPR_SUB,        OPR_MUL,        OPR_DIV,
    OPR_INTDIV,     OPR_MOD,
    OPR_NOT,        OPR_AND,        OPR_OR,
    OPR_EQUAL,      OPR_NEQ,        OPR_GT,         OPR_LT,
    OPR_GTEQ,       OPR_LTEQ 
} OperatorType;

const struct {
    char * oprStr;
    OperatorType oprTypeVal;
} OPERATOR_DEF[] = {
    { "&",  OPR_CONCAT  },
    { "+",  OPR_ADD     },
    { "-",  OPR_SUB     },
    { "*",  OPR_MUL     },
    { "/",  OPR_DIV     },
    { "\\", OPR_INTDIV  },
    { "%",  OPR_MOD     },
    { "!",  OPR_NOT     },
    { "&&", OPR_AND     },
    { "||", OPR_OR      },
    { "=",  OPR_EQUAL   },
    { "<>",  OPR_NEQ    },
    { ">",  OPR_GT      },
    { "<",  OPR_LT      },
    { ">=", OPR_GTEQ    },
    { "<=", OPR_LTEQ    },
    // place at bottom, not in use
    { "-",  OPR_NEG     }
};

const int OPERATOR_DEF_LENGTH = sizeof(OPERATOR_DEF) / sizeof(OPERATOR_DEF[0]);

const struct {
    const char *oprName;
    int priority;
} OPERATOR_META[] = {
    { "OPR_NEG",    500 },
    { "OPR_CONCAT", 90  },
    { "OPR_ADD",    100 },
    { "OPR_SUB",    100 },
    { "OPR_MUL",    200 },
    { "OPR_DIV",    200 },
    { "OPR_INTDIV", 150 },
    { "OPR_MOD",    200 },
    { "OPR_NOT",    50  },
    { "OPR_AND",    40  },
    { "OPR_OR",     30  },
    { "OPR_EQUAL",  50  },
    { "OPR_NEQ",    50  },
    { "OPR_GT",     60  },
    { "OPR_LT",     60  },
    { "OPR_GTEQ",   60  },
    { "OPR_LTEQ",   60  }
};

#define exprNodeLeftChild(en)   ((en)->children[0])
#define exprNodeRightChild(en)  ((en)->children[1])

Token* setToken(Token *token, TokenType type, const char *content, int sourceLength) {
    token->type = type;
    StringCopy(token->content, KB_CONTEXT_STRING_BUFFER_MAX, content);
    token->sourceLength = sourceLength;
    // printf("token: %s %s\n", token->content, DBG_TOKEN_NAME[token->type]);
    return token;
}

typedef struct tagAnalyzer {
    const char *expr;
    const char *eptr;
    Token token;
} Analyzer;

Token* nextToken(Analyzer *_self) {
    char firstChar, secondChar;
    char buffer[KB_TOKEN_LENGTH_MAX * 2];
    char *pbuffer = buffer;
    const char *eptrStart;

    while (isSpace(*_self->eptr)) {
        _self->eptr++;
    }

    eptrStart = _self->eptr;

    firstChar = *_self->eptr;

    // operator 
    switch (firstChar) {
    case '+':   case '-':   case '*':   case '/':   case '^':
    case '%':   case '=':   case '!':   case '\\':
        buffer[0] = firstChar;
        buffer[1] = '\0';
        _self->eptr++;
        return setToken(&_self->token, TOKEN_OPR, buffer, _self->eptr - eptrStart);
    case '(':   case ')':
        buffer[0] = firstChar;
        buffer[1] = '\0';
        _self->eptr++;
        return setToken(&_self->token, TOKEN_BKT, buffer, _self->eptr - eptrStart);
    case ',':
        buffer[0] = firstChar;
        buffer[1] = '\0';
        _self->eptr++;
        return setToken(&_self->token, TOKEN_CMA, buffer, _self->eptr - eptrStart);
    case ':':
        buffer[0] = firstChar;
        buffer[1] = '\0';
        _self->eptr++;
        return setToken(&_self->token, TOKEN_LABEL, buffer, _self->eptr - eptrStart);
    case '>':
        secondChar = *(_self->eptr + 1);
        if (secondChar == '=') {
            buffer[0] = firstChar;
            buffer[1] = secondChar;
            buffer[2] = '\0';
            _self->eptr += 2;
            return setToken(&_self->token, TOKEN_OPR, buffer, _self->eptr - eptrStart);
        } else {
            buffer[0] = firstChar;
            buffer[1] = '\0';
            _self->eptr++;
            return setToken(&_self->token, TOKEN_OPR, buffer, _self->eptr - eptrStart);
        }
    case '<':
        secondChar = *(_self->eptr + 1);
        if (secondChar == '=') {
            buffer[0] = firstChar;
            buffer[1] = secondChar;
            buffer[2] = '\0';
            _self->eptr += 2;
            return setToken(&_self->token, TOKEN_OPR, buffer, _self->eptr - eptrStart);
        } else if (secondChar == '>') {
            buffer[0] = firstChar;
            buffer[1] = secondChar;
            buffer[2] = '\0';
            _self->eptr += 2;
            return setToken(&_self->token, TOKEN_OPR, buffer, _self->eptr - eptrStart);
        } else {
            buffer[0] = firstChar;
            buffer[1] = '\0';
            _self->eptr++;
            return setToken(&_self->token, TOKEN_OPR, buffer, _self->eptr - eptrStart);
        }
    case '&':
        secondChar = *(_self->eptr + 1);
        if (secondChar == '&') {
            buffer[0] = firstChar;
            buffer[1] = secondChar;
            buffer[2] = '\0';
            _self->eptr += 2;
            return setToken(&_self->token, TOKEN_OPR, buffer, _self->eptr - eptrStart);
        } else {
            buffer[0] = firstChar;
            buffer[1] = '\0';
            _self->eptr++;
            return setToken(&_self->token, TOKEN_OPR, buffer, _self->eptr - eptrStart);
        }
    case '|':
        secondChar = *(_self->eptr + 1);
        if (secondChar == '|') {
            buffer[0] = firstChar;
            buffer[1] = secondChar;
            buffer[2] = '\0';
            _self->eptr += 2;
            return setToken(&_self->token, TOKEN_OPR, buffer, _self->eptr - eptrStart);
        } else {
            buffer[0] = firstChar;
            buffer[1] = '\0';
            _self->eptr++;
            return setToken(&_self->token, TOKEN_UDF, buffer, _self->eptr - eptrStart);
        }
    }

    // expr end here 
    if (firstChar == '\0') {
        return setToken(&_self->token, TOKEN_END, "", 0);
    }
    // numberic
    else if (isDigit(firstChar)) {
        while (isDigit(*_self->eptr)) {
            *pbuffer++ = *_self->eptr++;
        }
        // check if is float
        if (*_self->eptr == '.') {
            // write '.' to buffer
            *pbuffer++ = *_self->eptr++;
            // continue read digits
            while (isDigit(*_self->eptr)) {
                *pbuffer++ = *_self->eptr++;
            }
        }

        *pbuffer = '\0';

        // Token error: to long
        if ((pbuffer - buffer) >= KB_CONTEXT_STRING_BUFFER_MAX) {
            return setToken(&_self->token, TOKEN_ERR, "Number too long", _self->eptr - eptrStart);
        }

        return setToken(&_self->token, TOKEN_NUM, buffer, _self->eptr - eptrStart);
    }
    // identifier
    else if (isAlpha(firstChar)) {
        int i;
        int isFunc = 0;

        while (isAlphaNum(*_self->eptr)) {
            *pbuffer++ = *_self->eptr++;
        }
        *pbuffer = '\0';

        // check is keyword?
        for (i = 0; KEYWORDS[i].word; ++i) {
            if (StringEqual(KEYWORDS[i].word, buffer)) {
                return setToken(&_self->token, TOKEN_KEY, buffer, _self->eptr - eptrStart);
            }
        }
        
        while (isSpace(*_self->eptr)) {
            _self->eptr++;
        }

        // func
        if (*_self->eptr == '(') {
            _self->eptr++;
            isFunc = 1;
        }
        
        // Token error: to long
        if ((pbuffer - buffer) >= KB_CONTEXT_STRING_BUFFER_MAX) {
            return setToken(&_self->token, TOKEN_ERR, "Identifier too long", _self->eptr - eptrStart);
        }

        if (isFunc) {
            return setToken(&_self->token, TOKEN_FNC, buffer, _self->eptr - eptrStart);
        }
        else {
            return setToken(&_self->token, TOKEN_ID, buffer, _self->eptr - eptrStart);
        }
    }
    // string
    else if (firstChar == '"') {
        // skip first '"'
        _self->eptr++;

        while (*_self->eptr != '"') {
            const char currentChar = *_self->eptr;

            // escape sequence
            if (currentChar == '\\') {
                const char nextChar = *++_self->eptr;
                // newline
                if (nextChar == 'n') {
                    *pbuffer++ = '\n';
                }
                // carriage return
                else if (nextChar == 'r') {
                    *pbuffer++ = '\r';
                }
                // tab
                else if (nextChar == 't') {
                    *pbuffer++ = '\t';
                }
                // "
                else if (nextChar == '"') {
                    *pbuffer++ = '\"';
                }
                else {
                    return setToken(&_self->token, TOKEN_ERR, "Invalid escape", _self->eptr - eptrStart);
                }
                _self->eptr++;
            }
            // incomplete
            else if (currentChar == '\0') {
                return setToken(&_self->token, TOKEN_ERR, "Incomplete string", _self->eptr - eptrStart);
            }
            else {
                *pbuffer++ = *_self->eptr++;
            }
        }
        
        // skip last '"'
        _self->eptr++;

        // Token error: to long
        if ((pbuffer - buffer) >= KB_CONTEXT_STRING_BUFFER_MAX) {
            return setToken(&_self->token, TOKEN_ERR, "String too long", _self->eptr - eptrStart);
        }

        *pbuffer = '\0';

        return setToken(&_self->token, TOKEN_STR, buffer, _self->eptr - eptrStart);
    }
    // undefined
    else {
        _self->eptr++;
        buffer[0] = firstChar;
        buffer[1] = '\0';
        return setToken(&_self->token, TOKEN_UDF, buffer, _self->eptr - eptrStart);
    }
    return NULL;
}

void rewindToken(Analyzer *_self) {
    _self->eptr -= _self->token.sourceLength;
}

void resetToken(Analyzer *_self) {
    _self->eptr = _self->expr;
}

int matchExpr(Analyzer *_self);

int matchTryNext(Analyzer *_self) {
    Token *next = nextToken(_self);
    if (next->type == TOKEN_OPR) {
        return matchExpr(_self);
    }
    else {
        rewindToken(_self);
    }
    return 1;
}

int matchExpr(Analyzer *_self) {
    Token *token = nextToken(_self);
    // printf("[%s] %s\n", DBG_TOKEN_NAME[token->type], token->content);
    if (token->type == TOKEN_ID || token->type == TOKEN_NUM || token->type == TOKEN_STR) {
        return matchTryNext(_self);
    }
    else if ((token->type == TOKEN_OPR && StringEqual(token->content, "-"))
            || (token->type == TOKEN_OPR && StringEqual(token->content, "!"))) {
        return matchExpr(_self) && matchTryNext(_self);
    }
    else if (token->type == TOKEN_FNC) {
        token = nextToken(_self);
        if (token->type == TOKEN_BKT && *token->content == ')') {
            // no arg?
        }
        else {
            // at least 1 arg
            rewindToken(_self);
            while (1) {
                int result = matchExpr(_self);
                if (!result)
                    return 0;
                token = nextToken(_self);
                if (token->type == TOKEN_CMA)
                    continue;
                else if (token->type == TOKEN_BKT && *token->content == ')')
                    break;
                else
                    return 0;
            }
        }
        return matchTryNext(_self);
    }
    else if (token->type == TOKEN_BKT && StringEqual(token->content, "(")) {
        int success = matchExpr(_self);
        if (!success) return 0;
        token = nextToken(_self);
        if (token->type == TOKEN_BKT && StringEqual(token->content, ")")) {
            Token *next = nextToken(_self);
            if (next->type == TOKEN_OPR) {
                return matchExpr(_self);
            }
            else {
                rewindToken(_self);
            }
            return 1;
        }
        return 0; //return matchTryNext(_self);
    }
    // else {
    //  // Debug
    //  printf("Token = %s, %s\n", token->content, DBG_TOKEN_NAME[token->type]);
    // }
    return 0;
}

void displaySyntaxError(Analyzer *_self) {
    int i;
    rewindToken(_self);
    puts("[Syntax error]");
    puts(_self->expr);
    for (i = 0; i < _self->eptr - _self->expr; ++i) {
        printf(" ");
    }
    printf("^\n");
}

int checkExpr(Analyzer *_self, KbBuildError *errorRet) {
    int result = matchExpr(_self);
    // syntax error
    if (!result) {
        // return error
        // displaySyntaxError(_self);
        if (errorRet) {
            errorRet->errorType = KBE_SYNTAX_ERROR;
            errorRet->errorPos = _self->eptr - _self->expr;
            if (_self->token.type == TOKEN_ERR) {
                StringCopy(errorRet->message, KB_CONTEXT_STRING_BUFFER_MAX, _self->token.content);
            }
        }
        return 0;
    }
    else {
        // checkout not end
        Token *token = nextToken(_self);
        if (token->type != TOKEN_END) {
            // return error
            // displaySyntaxError(_self);
            if (errorRet) {
                errorRet->errorType = KBE_SYNTAX_ERROR;
                errorRet->errorPos = _self->eptr - _self->expr;
            }
            return 0;
        }
    }
    resetToken(_self);
    return 1;
}

ExprNode * enNew(const int type, const char *content, int numChild) {
    int i;

    ExprNode *node = (ExprNode *)malloc(sizeof(ExprNode));

    node->type = type;
    node->param = 0;

    if (type == TOKEN_OPR) {
        for (i = 0; i < OPERATOR_DEF_LENGTH; ++i) {
            if (StringEqual(content, OPERATOR_DEF[i].oprStr)) {
                node->param = OPERATOR_DEF[i].oprTypeVal;
                break;
            }
        }
    }

    StringCopy(node->content, KB_CONTEXT_STRING_BUFFER_MAX, content);

    node->numChild = numChild;
    node->children = (ExprNode **)malloc(numChild * (sizeof(ExprNode *)));
    for (i = 0; i < node->numChild; ++i) {
        node->children[i] = NULL;
    }

    return node;
}

ExprNode * enCreateByToken(const Token * token, int numChild) {
    return enNew(token->type, token->content, numChild);
}

ExprNode * enCreateOperatorNegative(const Token * token) {
    ExprNode *newNode = enNew(token->type, token->content, 1);

    newNode->param = OPR_NEG;

    return newNode;
}

ExprNode * enCreateOperatorNot(const Token * token) {
    ExprNode *newNode = enNew(token->type, token->content, 1);

    newNode->param = OPR_NOT;

    return newNode;
}

void enDestroy(ExprNode *node) {
    int i;

    if (node == NULL) return;

    for (i = 0; i < node->numChild; ++i) {
        if (node->children[i]) free(node->children[i]);
    }
    free(node->children);

    free(node);
}

ExprNode* buildExprTree(Analyzer *_self);

ExprNode* buildTryNext(Analyzer *_self, ExprNode *leftNode) {
    Token *token = nextToken(_self);

    if (token->type == TOKEN_OPR) {
        ExprNode *oprNode = enCreateByToken(token, 2);
        ExprNode *rightNode = buildExprTree(_self);

        exprNodeLeftChild(oprNode) = leftNode;
        exprNodeRightChild(oprNode) = rightNode;

        return oprNode;
    }
    else {
        rewindToken(_self);
        return leftNode;
    }

    return NULL;
}

ExprNode* buildExprTree(Analyzer *_self) {
    Token *token;

    token = nextToken(_self);
    // printf("token = %s,%s\n", DBG_TOKEN_NAME[token->type], token->content);

    if (token->type == TOKEN_ID || token->type == TOKEN_NUM || token->type == TOKEN_STR) {
        ExprNode *leftNode = enCreateByToken(token, 0);
        return buildTryNext(_self, leftNode);
    }
    else if (token->type == TOKEN_OPR && StringEqual("-", token->content)) { // negative '-'
        ExprNode* oprNode = enCreateOperatorNegative(token); // enCreateByToken(token, 1);
        exprNodeLeftChild(oprNode) = buildExprTree(_self);
        return buildTryNext(_self, oprNode);
    }
    else if (token->type == TOKEN_OPR && StringEqual("!", token->content)) { // not '!'
        ExprNode* oprNode = enCreateOperatorNot(token); 
        exprNodeLeftChild(oprNode) = buildExprTree(_self);
        return buildTryNext(_self, oprNode);
    }
    else if (token->type == TOKEN_FNC) {
        char *funcName = StringDump(token->content);
        ExprNode *funcNode = NULL;
        ExprNode *childBuf[100];
        int numChild = 0, i;

        token = nextToken(_self);
        if (token->type == TOKEN_BKT && *token->content == ')') {
            // no arg
        } else {
            // at least 1 arg
            rewindToken(_self);
            while (1) {
                ExprNode *child = buildExprTree(_self);

                childBuf[numChild++] = child;

                token = nextToken(_self);
                if (token->type == TOKEN_CMA) {
                    continue;
                }
                else if (token->type == TOKEN_BKT && *token->content == ')') {
                    break;
                }
            }
        }


        funcNode = enNew(TOKEN_FNC, funcName, numChild);
        for (i = 0; i < numChild; ++i) {
            funcNode->children[i] = childBuf[i];
        }
        free(funcName);

        return buildTryNext(_self, funcNode);
    }
    else if (token->type == TOKEN_BKT && *token->content == '(') {
        ExprNode *bktNode = enNew(TOKEN_BKT, "()", 1);
        bktNode->children[0] = buildExprTree(_self);
        nextToken(_self); // ignore ')'

        return buildTryNext(_self, bktNode);
    }

    return NULL;
}

void travelExpr(ExprNode *en, int tab) {
    int i;
    for (i = 0; i < tab; ++i) printf(" ");
    if (en->numChild > 0) {
        if (en->type == TOKEN_OPR) {
            printf("<en text=\"%s\" type=\"%s\" opr=\"%s\">\n", en->content, DBG_TOKEN_NAME[en->type], OPERATOR_META[en->param].oprName);
        }
        else {
            printf("<en text=\"%s\" type=\"%s\">\n", en->content, DBG_TOKEN_NAME[en->type]);
        }

        for (i = 0; i < en->numChild; ++i) {
            travelExpr(en->children[i], tab + 2);
        }

        for (i = 0; i < tab; ++i) {
            printf(" ");
        }
        printf("</en>\n");
    }
    else {
        if (en->type == TOKEN_OPR) {
            printf("<en type=\"%s\" opr=\"%s\"> %s </en>\n", DBG_TOKEN_NAME[en->type], en->content, OPERATOR_META[en->param].oprName);
        }
        else {
            printf("<en type=\"%s\"> %s </en>\n", DBG_TOKEN_NAME[en->type], en->content);
        }
    }
}

#define kbFormatErrorAppend(newPart) (p += StringCopy(p, strLengthMax - (p - strBuffer), newPart))

int kbFormatBuildError(const KbBuildError *errorVal, char *strBuffer, int strLengthMax) {
    char *p = strBuffer;

    if (errorVal == NULL) {
        strBuffer[0] = 0;
        return 0;
    }
    
    if (errorVal->errorType == KBE_NO_ERROR) {
        StringCopy(strBuffer, strLengthMax, "No error");
    }
    else if (errorVal->errorType == KBE_SYNTAX_ERROR) {
        char buf[100];
        sprintf(buf, "(%d) ", errorVal->errorPos);
        kbFormatErrorAppend(buf);
        kbFormatErrorAppend("Syntax Error");
        if (!StringEqual("", errorVal->message)) {
            kbFormatErrorAppend(errorVal->message);
        }
    }
    else if (errorVal->errorType == KBE_UNDEFINED_IDENTIFIER) {
        kbFormatErrorAppend("Undefined Identifier: ");
        kbFormatErrorAppend(errorVal->message);
    }
    else if (errorVal->errorType == KBE_UNDEFINED_FUNC) {
        kbFormatErrorAppend("Undefined Function: ");
        kbFormatErrorAppend(errorVal->message);
    }
    else if (errorVal->errorType == KBE_INVALID_NUM_ARGS) {
        kbFormatErrorAppend("Wrong number of arguments of function \"");
        kbFormatErrorAppend(errorVal->message);
        kbFormatErrorAppend("\"");
    }
    else if (errorVal->errorType == KBE_STRING_NO_SPACE) {
        kbFormatErrorAppend("No enough space for string");
    }
    else if (errorVal->errorType == KBE_TOO_MANY_VAR) {
        kbFormatErrorAppend("Too many variables. Failed to declare \"");
        kbFormatErrorAppend(errorVal->message);
        kbFormatErrorAppend("\"");
    }
    else if (errorVal->errorType == KBE_DUPLICATED_LABEL) {
        kbFormatErrorAppend("Duplicated label: \"");
        kbFormatErrorAppend(errorVal->message);
        kbFormatErrorAppend("\"");
    }
    else if (errorVal->errorType == KBE_UNDEFINED_LABEL) {
        kbFormatErrorAppend("Undefined label: \"");
        kbFormatErrorAppend(errorVal->message);
        kbFormatErrorAppend("\"");
    }
    else if (errorVal->errorType == KBE_OTHER) {
        kbFormatErrorAppend(errorVal->message);
    }

    return 1;
}

ExprNode* sortExprTree(ExprNode *en, int *ptrChangeFlag) {
    // printf("en=%s -> '%s' childs = %d\n", DBG_TOKEN_NAME[en->type], en->content, en->numChild);
    if (en == NULL) {
        return NULL;
    }
    else if (en->type == TOKEN_FNC || en->type == TOKEN_BKT) {
        int i;
        for (i = 0; i < en->numChild; ++i) {
            en->children[i] = sortExprTree(en->children[i], ptrChangeFlag);
        }
        return en;
    }
    else if (en->type == TOKEN_OPR) {
        ExprNode *enr, *enrl;
        int i;

        // exprNodeLeftChild(en) = sortExprTree(exprNodeLeftChild(en), ptrChangeFlag);
        // exprNodeRightChild(en) = sortExprTree(exprNodeRightChild(en), ptrChangeFlag);
        
        for (i = 0; i < en->numChild; ++i) {
            en->children[i] = sortExprTree(en->children[i], ptrChangeFlag);
        }
        enr = en->children[en->numChild - 1];

        if (enr->type == TOKEN_OPR) {
            const int enrPriority = OPERATOR_META[enr->param].priority;
            const int enPriority = OPERATOR_META[en->param].priority;

            if (enPriority >= enrPriority) {
                *ptrChangeFlag = 1;
            
                enrl = exprNodeLeftChild(enr);
                en->children[en->numChild - 1] = enrl;
                exprNodeLeftChild(enr) = en;

                // exprNodeRightChild(enr) = sortExprTree(exprNodeRightChild(enr), ptrChangeFlag);
                return enr;
            }
        }
    }
    return en;
}

void sortExpr(ExprNode ** enPtr) {
    int changeFlag;

    do {
        changeFlag = 0;
        *enPtr = sortExprTree(*enPtr, &changeFlag);
    } while (changeFlag);
}

KbCtrlStructItem* csItemNew(int csType) {
    KbCtrlStructItem* pCsItem = (KbCtrlStructItem *)malloc(sizeof(KbCtrlStructItem));
    pCsItem->csType = csType;
    pCsItem->iLabelEnd = -1;
    pCsItem->iLabelNext = -1;
    return pCsItem;
}

#define csItemRelease(pCsItem) free(pCsItem)

KbContext* contextCreate() {
    KbContext* context = (KbContext *)malloc(sizeof(KbContext));

    context->numVar = 0;
    context->stringBufferPtr = context->stringBuffer;
    memset(context->stringBuffer, 0, sizeof(context->stringBuffer));

    context->commandList = vlNewList();
    context->labelList = vlNewList();

    context->ctrlStruct.labelCounter = 0;
    context->ctrlStruct.stack = NULL;

    return context;
}

void contextDestroy (KbContext *context) {
    if (!context) {
        return;
    }
    vlDestroy(context->commandList, opCommandRelease);
    vlDestroy(context->labelList, free);
    if (context->ctrlStruct.stack) {
        vlDestroy(context->ctrlStruct.stack, free);
    }

    free(context);
}

int contextSetVariable(KbContext *context, const char * varName) {
    if (context->numVar >= KB_CONTEXT_VAR_MAX) {
        return -1;
    }

    StringCopy(context->varList[context->numVar], KB_CONTEXT_STRING_BUFFER_MAX, varName);

    return context->numVar++;
}

int contextGetVariableIndex (const KbContext *context, const char * varToFind) {
    int i;
    for (i = 0; i < context->numVar; ++i) {
        const char *varName = context->varList[i];
        if (StringEqual(varName, varToFind)) {
            return i;
        }
    }
    return -1;
}

int contextAppendText(KbContext *context, const char * string) {
    char *oldBufferPtr = context->stringBufferPtr;

    if (context->stringBufferPtr - context->stringBuffer + strlen(string) > KB_CONTEXT_STRING_BUFFER_MAX) {
        return -1;
    }

    while ((*context->stringBufferPtr++ = *string++) != '\0');

    return oldBufferPtr - context->stringBuffer;
}

KbLabel* contextGetLabel(const KbContext *context, const char * labelNameToFind) {
    VlistNode *node;

    for (node = context->labelList->head; node != NULL; node = node->next) {
        KbLabel* label = (KbLabel *)node->data;
        if (StringEqual(label->name, labelNameToFind)) {
            return label;
        }
    }

    return NULL;
}

int contextAddLabel(const KbContext *context, const char * labelNameToFind) {
    KbLabel* label = NULL;
    
    label = contextGetLabel(context, labelNameToFind);

    // already exist
    if (label) {
        return 0;
    }

    label = (KbLabel *)malloc(sizeof(KbLabel) + strlen(labelNameToFind) + 1);

    label->pos = 0;
    StringCopy(label->name, KB_TOKEN_LENGTH_MAX, labelNameToFind);

    vlPushBack(context->labelList, label);

    return 1;
}

KbCtrlStructItem* contextCtrlPeek(const KbContext *context) {
    const VlistNode* tail = context->ctrlStruct.stack->tail;
    if (tail == NULL) {
        return NULL;
    }
    return (KbCtrlStructItem *)(tail->data);
}

int contextCtrlPeekType(const KbContext *context) {
    KbCtrlStructItem* pCsItem;
    pCsItem = contextCtrlPeek(context);
    if (!pCsItem) {
        return KBCS_NONE;
    }
    return pCsItem->csType;
}

#define CTRL_LABEL_FORMAT "!_CS%03d_"

void contextCtrlAddLabel(KbContext* context, int labelIndex) {
    char szLabelBuf[100];

    if (labelIndex < 0) {
        return;
    }
    sprintf(szLabelBuf, CTRL_LABEL_FORMAT, labelIndex);
    
    contextAddLabel(context, szLabelBuf);
}

KbLabel* contextCtrlGetLabel(KbContext* context, int labelIndex) {
    char szLabelBuf[100];

    if (labelIndex < 0) {
        return NULL;
    }
    sprintf(szLabelBuf, CTRL_LABEL_FORMAT, labelIndex);
    
    return contextGetLabel(context, szLabelBuf);
}

void contextCtrlResetCounter(KbContext* context) {
    context->ctrlStruct.labelCounter = 0;
    if (context->ctrlStruct.stack) {
        vlDestroy(context->ctrlStruct.stack, free);
    }
    context->ctrlStruct.stack = vlNewList();
}

KbCtrlStructItem* contextCtrlPush(KbContext* context, int csType) {
    KbCtrlStructItem* pCsItem = csItemNew(csType);
    switch (csType) {
    case KBCS_IF_THEN:
        pCsItem->iLabelNext = context->ctrlStruct.labelCounter++;
        pCsItem->iLabelEnd = context->ctrlStruct.labelCounter++;
        break;
    case KBCS_ELSE_IF:
        pCsItem->iLabelNext = context->ctrlStruct.labelCounter++;
        pCsItem->iLabelEnd = -1;
        break;
    case KBCS_ELSE:
        pCsItem->iLabelNext = -1;
        pCsItem->iLabelEnd = -1;
        break;
    case KBCS_WHILE:
        pCsItem->iLabelNext = context->ctrlStruct.labelCounter++;
        pCsItem->iLabelEnd = context->ctrlStruct.labelCounter++;
        break;
    default:
        pCsItem->iLabelNext = -1;
        pCsItem->iLabelEnd = -1;
    }
    vlPushBack(context->ctrlStruct.stack, pCsItem);
    contextCtrlAddLabel(context, pCsItem->iLabelNext);
    contextCtrlAddLabel(context, pCsItem->iLabelEnd);
    
    return pCsItem;
}

KbOpCommand* opCommandCreateBuiltInFunction(int funcId) {
    KbOpCommand* cmd = (KbOpCommand*)malloc(sizeof(KbOpCommand));

    cmd->op = KBO_CALL_BUILT_IN;
    cmd->param.index = funcId;

    return cmd;
}

KbOpCommand* opCommandCreatePushVar(int varIndex) {
    KbOpCommand* cmd = (KbOpCommand*)malloc(sizeof(KbOpCommand));

    cmd->op = KBO_PUSH_VAR;
    cmd->param.index = varIndex;

    return cmd;
}

KbOpCommand* opCommandCreatePushNum(KB_FLOAT num) {
    KbOpCommand* cmd = (KbOpCommand*)malloc(sizeof(KbOpCommand));

    cmd->op = KBO_PUSH_NUM;
    cmd->param.number = num;

    return cmd;
}

KbOpCommand* opCommandCreatePushStr(int strIndex) {
    KbOpCommand* cmd = (KbOpCommand*)malloc(sizeof(KbOpCommand));

    cmd->op = KBO_PUSH_STR;
    cmd->param.index = strIndex;

    return cmd;
}

KbOpCommand* opCommandCreatePop() {
    KbOpCommand* cmd = (KbOpCommand*)malloc(sizeof(KbOpCommand));

    cmd->op = KBO_POP;
    cmd->param.index = 0;

    return cmd;
}

KbOpCommand* opCommandCreateAssignVar(int varIndex) {
    KbOpCommand* cmd = (KbOpCommand*)malloc(sizeof(KbOpCommand));

    cmd->op = KBO_ASSIGN_VAR;
    cmd->param.index = varIndex;

    return cmd;
}

KbOpCommand* opCommandCreateOperator(OperatorType oprType) {
    KbOpCommand* cmd = (KbOpCommand*)malloc(sizeof(KbOpCommand));

    cmd->op = oprType + KBO_OPR_NEG;
    cmd->param.index = 0;

    return cmd;
}

KbOpCommand* opCommandCreateStop() {
    KbOpCommand* cmd = (KbOpCommand*)malloc(sizeof(KbOpCommand));

    cmd->op = KBO_STOP;
    cmd->param.index = 0;

    return cmd;
}

KbOpCommand* opCommandCreateGoto(KbLabel *label) {
    KbOpCommand* cmd = (KbOpCommand*)malloc(sizeof(KbOpCommand));

    cmd->op = KBO_GOTO;
    cmd->param.ptr = label;

    return cmd;
}

KbOpCommand* opCommandCreateIfGoto(KbLabel *label) {
    KbOpCommand* cmd = (KbOpCommand*)malloc(sizeof(KbOpCommand));

    cmd->op = KBO_IFGOTO;
    cmd->param.ptr = label;

    return cmd;
}

int compileExprTree(KbContext *context, ExprNode *node, KbBuildError *errorRet) {
    int i;

    for (i = 0; i < node->numChild; ++i) {
        int ret;
        ret = compileExprTree(context, node->children[i], errorRet);
        if (!ret) return 0;
    }

    // function
    if (node->type == TOKEN_FNC) {
        const FunctionDef*  targetFunc = NULL;
        int                 funcDefLength = sizeof(FUNCTION_LIST) / sizeof(FUNCTION_LIST[0]);
        
        for (i = 0; i < funcDefLength; ++i) {
            const FunctionDef *func = FUNCTION_LIST + i;
            if (StringEqual(func->funcName, node->content)) {
                targetFunc = func;
                break;
            }
        }

        // undefined function
        if (targetFunc == NULL) {
            errorRet->errorPos = 0;
            errorRet->errorType = KBE_UNDEFINED_FUNC;
            StringCopy(errorRet->message, KB_ERROR_MESSAGE_MAX, node->content);
            return 0;
        }

        // invalid number of parameters
        if (targetFunc->numArgs != node->numChild) {
            errorRet->errorPos = 0;
            errorRet->errorType = KBE_INVALID_NUM_ARGS;
            StringCopy(errorRet->message, KB_ERROR_MESSAGE_MAX, node->content);
            return 0;
        }

        vlPushBack(context->commandList, opCommandCreateBuiltInFunction(targetFunc->funcId));
    }
    // operator
    else if (node->type == TOKEN_OPR) {
        vlPushBack(context->commandList, opCommandCreateOperator(node->param));
    }
    // variable
    else if (node->type == TOKEN_ID) {
        int varIndex = contextGetVariableIndex(context, node->content);

        // undefined variable
        if (varIndex < 0) {
            errorRet->errorPos = 0;
            errorRet->errorType = KBE_UNDEFINED_IDENTIFIER;
            StringCopy(errorRet->message, KB_ERROR_MESSAGE_MAX, node->content);
            return 0;
        }

        vlPushBack(context->commandList, opCommandCreatePushVar(varIndex));
    }
    // string
    else if (node->type == TOKEN_STR) {
        int strIndex = contextAppendText(context, node->content);
        if (strIndex < 0) {
            errorRet->errorPos = 0;
            errorRet->errorType = KBE_STRING_NO_SPACE;
            errorRet->message[0] = '\0';
            return 0;
        }
        vlPushBack(context->commandList, opCommandCreatePushStr(strIndex));
    }
    // number
    else if (node->type == TOKEN_NUM) {
        KB_FLOAT num = (KB_FLOAT)kAtof(node->content);
        vlPushBack(context->commandList, opCommandCreatePushNum(num));
    }
    // bkt
    else if (node->type == TOKEN_BKT) {
        // do nothing here
    }
    else {
        // undefined variable
        errorRet->errorPos = 0;
        errorRet->errorType = KBE_OTHER;
        StringCopy(errorRet->message, KB_ERROR_MESSAGE_MAX, "unknown express");
        return 0;
    }

    return 1;
}

void dbgPrintContextCommand(const KbOpCommand *cmd) {
    printf("%-15s ", _KOCODE_NAME[cmd->op]);

    if (cmd->op == KBO_PUSH_VAR
     || cmd->op == KBO_PUSH_STR
     || cmd->op == KBO_CALL_BUILT_IN
     || cmd->op == KBO_ASSIGN_VAR
     || cmd->op == KBO_GOTO
     || cmd->op == KBO_IFGOTO) {
        printf("%d", cmd->param.index);
    }
    else if (cmd->op == KBO_PUSH_NUM) {
        char szNum[KB_TOKEN_LENGTH_MAX];
        kFtoa((float)cmd->param.number, szNum, DEFAUL_FTOA_PRECISION);
        printf("%s", szNum);
    }
    else if ((cmd->op >= KBO_OPR_NEG && cmd->op <= KBO_OPR_LTEQ) || cmd->op == KBO_POP || cmd->op == KBO_STOP) {
        // nothing here
    }
    else {
        printf("ERROR");
    }

    printf("\n");
}

void dbgPrintContextCommandList (const KbContext *context) {
    VlistNode*  node    = context->commandList->head;
    int         lineNum = 0;

    while (node != NULL) {
        printf("%03d ", lineNum);
        dbgPrintContextCommand((KbOpCommand *)node->data);
        ++lineNum;
        node = node->next;
    }
}

void dbgPrintContextListText(const KbContext *context) {
    const char *ptr = context->stringBuffer;
    int i = 0;

    while (ptr < context->stringBufferPtr) {
        printf("String %2i : \"%s\"\n", i, ptr);
        ptr += strlen(ptr) + 1;
        ++i;
    }
}

void dbgPrintContextListLabel(const KbContext *context) {
    VlistNode *node = context->labelList->head;

    while (node != NULL) {
        KbLabel* label = (KbLabel *)node->data;
        printf("%-15s: %03d\n", label->name, label->pos);
        node = node->next;
    }
}

void dbgPrintContextVariables(const KbContext *context) {
    int i;
    for (i = 0; i < context->numVar; ++i) {
        printf("Variable %2d : %s\n", i, context->varList[i]);
    }
}

#define compileLineReturn(ret) return(ret)

#define compileLineReturnWithError(errCode, msg) NULL;          \
    errorRet->errorType = errCode;                              \
    errorRet->errorPos = analyzer.eptr - analyzer.expr;         \
    StringCopy(errorRet->message, KB_ERROR_MESSAGE_MAX, msg);   \
    compileLineReturn(0)

KbContext* kbCompileStart(const char * line) {
    KbContext * context;

    context = contextCreate();
    
    /* 内置变量 */
    // contextSetVariable(context, "hh");
    // contextSetVariable(context, "mm");
    // contextSetVariable(context, "ss");
    // contextSetVariable(context, "ms");

    return context;
}

int kbScanControlStructureAndLabel(const char* line, KbContext *context, KbBuildError *errorRet) {
    Analyzer    analyzer;
    Token*      token;
    int         ret;
    int         peakCsType;

    analyzer.expr = line;
    resetToken(&analyzer);

    errorRet->errorType = KBE_NO_ERROR;
    errorRet->message[0] = '\0';

    /* 尝试读取第一个 token */
    token = nextToken(&analyzer);
    
    /* if 语句 */
    if (token->type == TOKEN_KEY && StringEqual(KEYWORD_IF, token->content)) {
        /* 检查是不是 if ... then 格式 */
        if (!StringEndWith(line, KEYWORD_THEN)) {
            return 1;
        }
        contextCtrlPush(context, KBCS_IF_THEN);
    }
    /* elseif 语句 */
    else if (token->type == TOKEN_KEY && StringEqual(KEYWORD_ELSEIF, token->content)) {
        /* 匹配表达式 */
        ret = matchExpr(&analyzer);
        if (!ret) {
            compileLineReturnWithError(KBE_SYNTAX_ERROR, ", invalid express in elseif statement");
        }
        /* 匹配语句结束 */
        token = nextToken(&analyzer);
        if (token->type != TOKEN_END) {
            compileLineReturnWithError(KBE_SYNTAX_ERROR, ", invalid elseif statement");
        }
        peakCsType = contextCtrlPeekType(context);
        /* elseif 的前一个控制结构应该是 elseif 或者 if then */
        if (peakCsType != KBCS_IF_THEN && peakCsType != KBCS_ELSE_IF) {
            compileLineReturnWithError(KBE_SYNTAX_ERROR, ", invalid `elseif`");
        }
        contextCtrlPush(context, KBCS_ELSE_IF);
    }
    /* else 语句 */
    else if (token->type == TOKEN_KEY && StringEqual(KEYWORD_ELSE, token->content)) {
        /* 匹配语句结束 */
        token = nextToken(&analyzer);
        if (token->type != TOKEN_END) {
            compileLineReturnWithError(KBE_SYNTAX_ERROR, ", invalid else statement");
        }
        peakCsType = contextCtrlPeekType(context); 
        /* else 的前一个控制结构应该是 else if 或者 if then */
        if (peakCsType != KBCS_IF_THEN && peakCsType != KBCS_ELSE_IF) {
            compileLineReturnWithError(KBE_SYNTAX_ERROR, ", invalid `else`");
        }
        contextCtrlPush(context, KBCS_ELSE);
    }
    /* while 语句 */
    else if (token->type == TOKEN_KEY && StringEqual(KEYWORD_WHILE, token->content)) {
        /* 匹配表达式 */
        ret = matchExpr(&analyzer);
        if (!ret) {
            compileLineReturnWithError(KBE_SYNTAX_ERROR, ", invalid express in while statement");
        }
        /* 匹配语句结束 */
        token = nextToken(&analyzer);
        if (token->type != TOKEN_END) {
            compileLineReturnWithError(KBE_SYNTAX_ERROR, ", invalid while statement");
        }
        contextCtrlPush(context, KBCS_WHILE);
    }
    /* break 语句 */
    else if (token->type == TOKEN_KEY && StringEqual(KEYWORD_BREAK, token->content)) {
        /* 匹配语句结束 */
        token = nextToken(&analyzer);
        if (token->type != TOKEN_END) {
            compileLineReturnWithError(KBE_SYNTAX_ERROR, ", invalid token after `break`");
        }
        /* 从栈顶向下寻找循环语句 */
        else {
            VlistNode* node = context->ctrlStruct.stack->tail;
            int found = 0;
            while (node) {
                const KbCtrlStructItem* pCsItem;
                pCsItem = (KbCtrlStructItem *)(node->data);
                if (pCsItem->csType == KBCS_WHILE) {
                    found = 1;
                    break;
                }
                node = node->prev;
            }
            if (!found) {
                compileLineReturnWithError(KBE_SYNTAX_ERROR, ", invalid `break`, no matching loop found.");
            }
        }
    }
    /* end 语句 */
    else if (token->type == TOKEN_KEY && StringEqual(KEYWORD_END, token->content)) {
        int popTarget = KBCS_NONE;
        /* 获取 end 之后的 token，可以是 if / while */
        token = nextToken(&analyzer);
        if (token->type == TOKEN_KEY && StringEqual(KEYWORD_IF, token->content)) {
            popTarget = KBCS_IF_THEN;
        }
        else if (token->type == TOKEN_KEY && StringEqual(KEYWORD_WHILE, token->content)) {
            popTarget = KBCS_WHILE;
        }
        else {
            compileLineReturnWithError(KBE_SYNTAX_ERROR, ", invalid token after `end`");
        }
        /* 匹配语句结束 */
        token = nextToken(&analyzer);
        if (token->type != TOKEN_END) {
            compileLineReturnWithError(KBE_SYNTAX_ERROR, "");
        }
        /* 弹出最近 if 语句块 */
        if (popTarget == KBCS_IF_THEN) {
            while (1) {
                int peakCsType = contextCtrlPeekType(context);
                /* 先弹出 else / else if */
                if (peakCsType == KBCS_ELSE || peakCsType == KBCS_ELSE_IF) {
                    csItemRelease(vlPopBack(context->ctrlStruct.stack));
                }
                /* 弹出 if，结束 */
                else if (peakCsType == KBCS_IF_THEN) {
                    csItemRelease(vlPopBack(context->ctrlStruct.stack));
                    break;
                }
                /* 其他的均为错误 */
                else {
                    compileLineReturnWithError(KBE_SYNTAX_ERROR, ", invalid `end if`, no matching if statement found.");
                }
            }
        }
        /* 弹出最近 while 语句块 */
        else if (popTarget == KBCS_WHILE) {
            int peakCsType = contextCtrlPeekType(context);
            if (peakCsType == KBCS_WHILE) {
                csItemRelease(vlPopBack(context->ctrlStruct.stack));
            }
            /* 其他的均为错误 */
            else {
                compileLineReturnWithError(KBE_SYNTAX_ERROR, ", invalid `end while`, no matching while statement found.");
            }
        }
    } 
    /* 用户手动写的 label */
    else if (token->type == TOKEN_LABEL) {
        char labelName[KB_TOKEN_LENGTH_MAX];

        token = nextToken(&analyzer);
        if (token->type != TOKEN_ID) {
            compileLineReturnWithError(KBE_SYNTAX_ERROR, ", invalid label");
        }

        StringCopy(labelName, sizeof(labelName), token->content);

        token = nextToken(&analyzer);
        if (token->type != TOKEN_END) {
            compileLineReturnWithError(KBE_SYNTAX_ERROR, " in label command");
        }

        ret = contextAddLabel(context, labelName);
        if (!ret) {
            compileLineReturnWithError(KBE_DUPLICATED_LABEL, labelName);
        }
    }

    return 1;
}

int kbCompileLine(const char * line, KbContext *context, KbBuildError *errorRet) {
    Analyzer    analyzer;
    ExprNode*   exprRoot;
    Token*      token;
    int         ret;

    analyzer.expr = line;
    resetToken(&analyzer);

    errorRet->errorType = KBE_NO_ERROR;
    errorRet->message[0] = '\0';

    // try first token
    token = nextToken(&analyzer);

    // empty
    if (token->type == TOKEN_END) {
        // nothing to do
    }
    /* if 语句 */
    else if (token->type == TOKEN_KEY && StringEqual(KEYWORD_IF, token->content)) {
        char            labelName[KB_TOKEN_LENGTH_MAX];
        const char *    exprStartEptr;

        /* 保存表达式的位置 */
        exprStartEptr = analyzer.eptr;

        ret = matchExpr(&analyzer);
        if (!ret) {
            compileLineReturnWithError(KBE_SYNTAX_ERROR, ", invalid express in if statement");
        }
        
        token = nextToken(&analyzer);
        /* if goto 模式*/
        if (token->type == TOKEN_KEY && StringEqual(KEYWORD_GOTO, token->content)) {
            KbLabel *label;
        
            /* 跳转标签 */
            token = nextToken(&analyzer);
            if (token->type != TOKEN_ID) {
                compileLineReturnWithError(KBE_SYNTAX_ERROR, " missing label in if statement");
            }

            StringCopy(labelName, sizeof(labelName), token->content);

            /* 确认本行结束 */
            token = nextToken(&analyzer);
            if (token->type != TOKEN_END) {
                compileLineReturnWithError(KBE_SYNTAX_ERROR, " in if statement");
            }

            /* 重设位置，构建表达式 */
            analyzer.eptr = exprStartEptr;
            exprRoot = buildExprTree(&analyzer);
            sortExpr(&exprRoot);
            
            /* 编译表达式 */
            ret = compileExprTree(context, exprRoot, errorRet);
            if (!ret) {
                compileLineReturn(0);
            }

            enDestroy(exprRoot);

            /* 找到条件不满足时跳转的标签 */
            label = contextGetLabel(context, labelName);
            if (!label) {
                compileLineReturnWithError(KBE_UNDEFINED_LABEL, labelName);
            }

            vlPushBack(context->commandList, opCommandCreateIfGoto(label));
        }
        /* if then 模式 */
        else if (token->type == TOKEN_KEY && StringEqual(KEYWORD_THEN, token->content)) {
            KbCtrlStructItem* pCsItem;
            KbLabel* label;
        
            /* 确认本行结束 */
            token = nextToken(&analyzer);
            if (token->type != TOKEN_END) {
                compileLineReturnWithError(KBE_SYNTAX_ERROR, " in if statement");
            }

            /* 重设位置，构建表达式 */
            analyzer.eptr = exprStartEptr;
            exprRoot = buildExprTree(&analyzer);
            sortExpr(&exprRoot);
            
            /* 编译表达式 */
            ret = compileExprTree(context, exprRoot, errorRet);
            if (!ret) {
                compileLineReturn(0);
            }
            enDestroy(exprRoot);

            /* iLabelNext: 条件不满足的时候，跳去下一条 else if / else */
            pCsItem = contextCtrlPush(context, KBCS_IF_THEN);
            label = contextCtrlGetLabel(context, pCsItem->iLabelNext);

            vlPushBack(context->commandList, opCommandCreateOperator(OPR_NOT));
            vlPushBack(context->commandList, opCommandCreateIfGoto(label));
        }
        /* 错误 */
        else {
            compileLineReturnWithError(KBE_SYNTAX_ERROR, " missing 'goto' or 'then' in if statement");
        }
    }
    /* elseif 语句 */
    else if (token->type == TOKEN_KEY && StringEqual(KEYWORD_ELSEIF, token->content)) {
        KbLabel* label;
        KbLabel* prevLabel;
        KbCtrlStructItem* pCsItem;
        KbCtrlStructItem* pPrevCsItem;
    
        /* 寻找最近的 if */
        VlistNode *stackNode = context->ctrlStruct.stack->tail;
        while (stackNode) {
            pPrevCsItem = (KbCtrlStructItem *)stackNode->data;
            if (pPrevCsItem->csType == KBCS_IF_THEN) break;
            stackNode = stackNode->prev;
        }
        /* 条件满足的时候执行完毕跳转到 end if */
        prevLabel = contextCtrlGetLabel(context, pPrevCsItem->iLabelEnd);
        vlPushBack(context->commandList, opCommandCreateGoto(prevLabel));

        /* 更新上一条 */
        pPrevCsItem = contextCtrlPeek(context);
        prevLabel = contextCtrlGetLabel(context, pPrevCsItem->iLabelNext);

        /* 更新上一个项目的跳转地址 */
        prevLabel->pos = context->commandList->size;

        /* 编译表达式 */
        exprRoot = buildExprTree(&analyzer);
        sortExpr(&exprRoot);
        ret = compileExprTree(context, exprRoot, errorRet);
        if (!ret) {
            compileLineReturn(0);
        }
        enDestroy(exprRoot);

        /* 找到条件不满足时跳转的标签 */
        pCsItem = contextCtrlPush(context, KBCS_ELSE_IF);
        label = contextCtrlGetLabel(context, pCsItem->iLabelNext);

        /* 指令写入 */
        vlPushBack(context->commandList, opCommandCreateOperator(OPR_NOT));
        vlPushBack(context->commandList, opCommandCreateIfGoto(label));
    }
    /* else 语句 */
    else if (token->type == TOKEN_KEY && StringEqual(KEYWORD_ELSE, token->content)) {
        KbLabel* prevLabel;
        KbCtrlStructItem* pPrevCsItem;
    
        /* 寻找最近的 if */
        VlistNode *stackNode = context->ctrlStruct.stack->tail;
        while (stackNode) {
            pPrevCsItem = (KbCtrlStructItem *)stackNode->data;
            if (pPrevCsItem->csType == KBCS_IF_THEN) break;
            stackNode = stackNode->prev;
        }
        /* 条件满足的时候执行完毕跳转到 end if */
        prevLabel = contextCtrlGetLabel(context, pPrevCsItem->iLabelEnd);
        vlPushBack(context->commandList, opCommandCreateGoto(prevLabel));

        /* 更新上一条 */
        pPrevCsItem = contextCtrlPeek(context);
        prevLabel = contextCtrlGetLabel(context, pPrevCsItem->iLabelNext);

        /* 更新上一个项目的跳转地址 */
        prevLabel->pos = context->commandList->size;
    }
    /* while 语句 */
    else if (token->type == TOKEN_KEY && StringEqual(KEYWORD_WHILE, token->content)) {
        KbLabel* labelEnd;
        KbLabel* labelStart;
        KbCtrlStructItem* pCsItem;
    
        pCsItem = contextCtrlPush(context, KBCS_WHILE);
        /* 循环开头的标签，更新位置 */
        labelStart = contextCtrlGetLabel(context, pCsItem->iLabelNext);
        labelStart->pos = context->commandList->size;

        exprRoot = buildExprTree(&analyzer);
        sortExpr(&exprRoot);
        
        /* 编译表达式 */
        ret = compileExprTree(context, exprRoot, errorRet);
        if (!ret) {
            compileLineReturn(0);
        }

        enDestroy(exprRoot);
    
        /* 不满足的时候跳转标签 */
        labelEnd = contextCtrlGetLabel(context, pCsItem->iLabelEnd);
    
        /* 指令写入 */
        vlPushBack(context->commandList, opCommandCreateOperator(OPR_NOT));
        vlPushBack(context->commandList, opCommandCreateIfGoto(labelEnd));
    }
    /* end 语句 */
    else if (token->type == TOKEN_KEY && StringEqual(KEYWORD_END, token->content)) {
        KbLabel* label;
        KbCtrlStructItem* pPrevCsItem;
        int popTarget = KBCS_NONE;

        /* 获取 end 之后的 token，可以是 if / while */
        token = nextToken(&analyzer);
        if (token->type == TOKEN_KEY && StringEqual(KEYWORD_IF, token->content)) {
            popTarget = KBCS_IF_THEN;
        }
        else if (token->type == TOKEN_KEY && StringEqual(KEYWORD_WHILE, token->content)) {
            popTarget = KBCS_WHILE;
        }
        
        /* 弹出最近 if / elseif / else 语句块 */
        if (popTarget == KBCS_IF_THEN) {
            /* 不断弹出直到弹出if */
            while (1) {
                int csType;
                pPrevCsItem = contextCtrlPeek(context);
                csType = pPrevCsItem->csType;
                /* 更新前面的 if 的退出语句块标签 */
                if (csType == KBCS_IF_THEN) {
                    label = contextCtrlGetLabel(context, pPrevCsItem->iLabelEnd);
                    label->pos = context->commandList->size;
                    label = contextCtrlGetLabel(context, pPrevCsItem->iLabelNext);
                    if (label->pos <= 0) {
                        label->pos = context->commandList->size;
                    }
                }
                csItemRelease(vlPopBack(context->ctrlStruct.stack));
                if (csType == KBCS_IF_THEN) {
                    break;
                }
            }
        }
        /* 弹出最近 while 语句块 */
        else if (popTarget == KBCS_WHILE) {
            /* 从堆栈中拿出来上一个项目 */
            pPrevCsItem = contextCtrlPeek(context);
            /* 指令写入，跳转回循环开头 */
            label = contextCtrlGetLabel(context, pPrevCsItem->iLabelNext);
            vlPushBack(context->commandList, opCommandCreateGoto(label));
            /* 更新上一个项目的跳转地址 */
            label = contextCtrlGetLabel(context, pPrevCsItem->iLabelEnd);
            label->pos = context->commandList->size;
            /* 弹出 while */
            csItemRelease(vlPopBack(context->ctrlStruct.stack));
        }
    }
    /* break 语句 */
    else if (token->type == TOKEN_KEY && StringEqual(KEYWORD_BREAK, token->content)) {
        /* 从栈顶向下寻找循环语句 */
        VlistNode*          node = context->ctrlStruct.stack->tail;
        KbCtrlStructItem*   pCsItem;
        KbLabel*            label;
        while (node) {
            pCsItem = (KbCtrlStructItem *)(node->data);
            if (pCsItem->csType == KBCS_WHILE) {
                break;
            }
            node = node->prev;
        }
        /* 指令写入，跳转回循环结束 */
        label = contextCtrlGetLabel(context, pCsItem->iLabelEnd);
        vlPushBack(context->commandList, opCommandCreateGoto(label));
    }
    // let statement
    else if (token->type == TOKEN_KEY && StringEqual(KEYWORD_LET, token->content)) {
        char        varName[KB_TOKEN_LENGTH_MAX];
        const char* exprStartEptr;
        int         varIndex;

        // get name of variable
        token = nextToken(&analyzer);
        if (token->type != TOKEN_ID) {
            compileLineReturnWithError(KBE_SYNTAX_ERROR, " in let statement, missing variable");
        }
        StringCopy(varName, sizeof(varName), token->content);

        // match '='
        token = nextToken(&analyzer);
        if (!(token->type == TOKEN_OPR && StringEqual("=", token->content))) {
            compileLineReturnWithError(KBE_SYNTAX_ERROR, " in let statement, require '='");
        }

        varIndex = contextGetVariableIndex(context, varName);
        // create var
        if (varIndex < 0) {
            varIndex = contextSetVariable(context, varName);
            if (varIndex < 0) {
                compileLineReturnWithError(KBE_TOO_MANY_VAR, varName);
            }
        }

        // remain part is expr
        exprStartEptr = analyzer.eptr;

        if (!checkExpr(&analyzer, errorRet)) {
            compileLineReturn(0);
        }

        analyzer.eptr = exprStartEptr;
        exprRoot = buildExprTree(&analyzer);
        sortExpr(&exprRoot);
        // printf("let: '%s' = ", varName); travelExpr(exprRoot, 0);
        
        // compile expr
        ret = compileExprTree(context, exprRoot, errorRet);
        if (!ret) {
            compileLineReturn(0);
        }

        enDestroy(exprRoot);

        // add assign command
        vlPushBack(context->commandList, opCommandCreateAssignVar(varIndex));
    }
    // goto command
    else if (token->type == TOKEN_KEY && StringEqual(KEYWORD_GOTO, token->content)) {
        char        labelName[KB_TOKEN_LENGTH_MAX];
        KbLabel*    label;

        // match label
        token = nextToken(&analyzer);
        if (token->type != TOKEN_ID) {
            compileLineReturnWithError(KBE_SYNTAX_ERROR, " missing label in goto statement");
        }

        StringCopy(labelName, sizeof(labelName), token->content);

        // match end
        token = nextToken(&analyzer);
        if (token->type != TOKEN_END) {
            compileLineReturnWithError(KBE_SYNTAX_ERROR, " in goto statement");
        }

        // find label
        label = contextGetLabel(context, labelName);
        if (!label) {
            compileLineReturnWithError(KBE_UNDEFINED_LABEL, labelName);
        }

        vlPushBack(context->commandList, opCommandCreateGoto(label));
    }
    // label mark, update position
    else if (token->type == TOKEN_LABEL) {
        const char *labelName;
        KbLabel *label;

        token = nextToken(&analyzer);
        if (token->type != TOKEN_ID) {
            compileLineReturnWithError(KBE_SYNTAX_ERROR, ", invalid label");
        }

        labelName = token->content;

        // update position of label
        label = contextGetLabel(context, labelName);

        if (!label) {
            compileLineReturnWithError(KBE_UNDEFINED_LABEL, labelName);
        }

        label->pos = context->commandList->size;
    }
    else {
        rewindToken(&analyzer);

        if (!checkExpr(&analyzer, errorRet)) {
            compileLineReturn(0);
        }

        exprRoot = buildExprTree(&analyzer);
        sortExpr(&exprRoot);
        // printf("expr:"); travelExpr(exprRoot, 0);
        
        // compile expr
        ret = compileExprTree(context, exprRoot, errorRet);
        if (!ret) {
            compileLineReturn(0);
        }

        enDestroy(exprRoot);

        // pop no use value
        vlPushBack(context->commandList, opCommandCreatePop());
    }

    compileLineReturn(1);
}

int kbCompileEnd(KbContext *context) {
    VlistNode *node;

    // update position of goto / ifgoto
    for (node = context->commandList->head; node != NULL; node = node->next) { 
        KbOpCommand *cmd = (KbOpCommand *)node->data;
        
        if (cmd->op == KBO_IFGOTO || cmd->op == KBO_GOTO) {
            KbLabel *label = (KbLabel *)cmd->param.ptr;
            cmd->param.index = label->pos;
        }
    }

    // add assign command
    vlPushBack(context->commandList, opCommandCreateStop());
    return 1;
}

int kbSerialize(
    const           KbContext* context,
    unsigned char** pRaw,
    int *           pByteLength
) {
    unsigned char * entireRaw;
    int             byteLengthHeader;
    int             byteLengthCmd;
    int             numCmd;
    int             byteLengthString;
    int             byteLengthEntire;
    int             i;
    //
    KBASIC_BINARY_HEADER*   pHeader;
    KbOpCommand *           pCmd;
    char *                  pString;
    //
    VlistNode*       node;

    // ---------- layout ----------
    //
    //          * header *
    //
    // ----------------------------
    //
    //           * cmd *
    //
    // ----------------------------
    //
    //          * string *
    //
    // ----------------------------

    // calculate length

    byteLengthHeader    = sizeof(KBASIC_BINARY_HEADER);
    numCmd              = context->commandList->size;
    byteLengthCmd       = sizeof(KbOpCommand) * numCmd;
    byteLengthString    = context->stringBufferPtr - context->stringBuffer;
    byteLengthEntire    = byteLengthHeader
                        + byteLengthCmd
                        + byteLengthString;

    // allocate space and write header
    entireRaw = (unsigned char *)malloc(byteLengthEntire);

    pHeader                     = (KBASIC_BINARY_HEADER *)entireRaw;
    pHeader->headerMagic        = HEADER_MAGIC_FLAG;
    pHeader->numVariables       = context->numVar;
    pHeader->cmdBlockStart      = 0 + byteLengthHeader;
    pHeader->numCmd             = numCmd;
    pHeader->stringBlockStart   = pHeader->cmdBlockStart + byteLengthCmd;
    pHeader->stringBlockLength  = byteLengthString;

    // set pointers for writting
    pCmd       = (KbOpCommand *)(entireRaw + pHeader->cmdBlockStart);
    pString    = (char *)(entireRaw + pHeader->stringBlockStart);

    // dump command
    node = context->commandList->head;
    for (i = 0; node != NULL; ++i, node = node->next) {
        memcpy(pCmd + i, node->data, sizeof(KbOpCommand));
    }

    // dump string block
    memset(pString, 0, byteLengthString);
    memcpy(pString, context->stringBuffer, byteLengthString);

    *pRaw = entireRaw;
    *pByteLength = byteLengthEntire;

    return 1;
}

void dbgPrintHeader(KBASIC_BINARY_HEADER* h) {
    printf("header              = 0x%x\n", h->headerMagic);
    printf("variable num        = %d\n", h->numVariables);
    printf("cmd block start     = %d\n", h->cmdBlockStart);
    printf("cmd num             = %d\n", h->numCmd);
    printf("string block start  = %d\n", h->stringBlockStart);
    printf("string block length = %d\n", h->stringBlockLength);
}