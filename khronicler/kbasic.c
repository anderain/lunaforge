#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "kbasic.h"
#include "k_utils.h"
#include "../artifacts/salvia89/salvia.h"

#define CTRL_LABEL_FORMAT "!_CS%03d_"
#define FUNC_LABEL_FORMAT "!_FC%03d_"

/* 关键字定义 */
typedef struct {
    const char *word;
} KeywordDef;

#define KEYWORD_DIM     "dim"
#define KEYWORD_GOTO    "goto"
#define KEYWORD_IF      "if"
#define KEYWORD_ELSEIF  "elseif"
#define KEYWORD_ELSE    "else"
#define KEYWORD_WHILE   "while"
#define KEYWORD_END     "end"
#define KEYWORD_BREAK   "break"
#define KEYWORD_RETURN  "return"
#define KEYWORD_EXIT    "exit"
#define KEYWORD_FUNC    "func"

static const KeywordDef KEYWORDS[] = {
    { KEYWORD_DIM       },
    { KEYWORD_GOTO      },
    { KEYWORD_IF        },
    { KEYWORD_ELSEIF    },
    { KEYWORD_ELSE      },
    { KEYWORD_WHILE     },
    { KEYWORD_END       },
    { KEYWORD_BREAK     },
    { KEYWORD_RETURN    },
    { KEYWORD_EXIT      },
    { KEYWORD_FUNC      },
    { NULL              }
};

/* 内建函数定义 */
typedef struct {
    const char* funcName;
    int         numArg;
    int         funcId;
} BuiltInFunction;

static const BuiltInFunction FUNCTION_LIST[] = {
    { "p",          1, KFID_P           },
    { "sin",        1, KFID_SIN         },
    { "cos",        1, KFID_COS         },
    { "tan",        1, KFID_TAN         }, 
    { "sqrt",       1, KFID_SQRT        },
    { "exp",        1, KFID_EXP         }, 
    { "abs",        1, KFID_ABS         },
    { "log",        1, KFID_LOG         },
    { "rand",       0, KFID_RAND        },
    { "len",        1, KFID_LEN         },
    { "val",        1, KFID_VAL         },
    { "asc",        1, KFID_ASC         },
    { "zeropad",    2, KFID_ZEROPAD     }
};

const char* DBG_TOKEN_NAME[] = {
    "TOKEN_ERR",    "TOKEN_END",    "TOKEN_NUM",
    "TOKEN_ID",     "TOKEN_OPR",    "TOKEN_FNC",
    "TOKEN_BKT",    "TOKEN_CMA",    "TOKEN_STR",
    "TOKEN_LABEL",
    "TOKEN_UDF"
};

typedef struct tagExprNode {
    int     type;
    int     param; /* 只在 type 是 TOKEN_OPR 的时候才有意义 */
    char    content[KB_TOKEN_LENGTH_MAX * 2];
    int     numChild;
    struct tagExprNode **children;
} ExprNode;

typedef enum {
    OPR_NEG = 0,
    /* 字符串操作链接 */
    OPR_CONCAT,
    /* 基本运算 加 减 乘 除 指数 */
    OPR_ADD,        OPR_SUB,        OPR_MUL,        OPR_DIV,        OPR_POW,
    /* 整数计算 整数除法 取模 */
    OPR_INTDIV,     OPR_MOD,
    /* 逻辑操作 非 与 或者*/
    OPR_NOT,        OPR_AND,        OPR_OR,
    /* 比较操作 */
    OPR_EQUAL,      OPR_EQUAL_REL,  OPR_NEQ,        OPR_GT,         OPR_LT,
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
    { "^",  OPR_POW     },
    { "\\", OPR_INTDIV  },
    { "%",  OPR_MOD     },
    { "!",  OPR_NOT     },
    { "&&", OPR_AND     },
    { "||", OPR_OR      },
    { "=",  OPR_EQUAL   },
    { "~=", OPR_EQUAL_REL},
    { "<>", OPR_NEQ     },
    { ">",  OPR_GT      },
    { "<",  OPR_LT      },
    { ">=", OPR_GTEQ    },
    { "<=", OPR_LTEQ    },
    /* 取负放在最下，不会被使用到 */
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
    { "OPR_POW",    200 },
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

Token* assignToken(Analyzer* pAnalyzer, TokenType type, const char *content, const char *eptrStart) {
    Token* token = &pAnalyzer->token;
    token->type = type;
    /* 此为转义过的 token 内容，可能丢失了转义字符、引号、左括号等内容 */
    StringCopy(token->content, KB_CONTEXT_STRING_BUFFER_MAX, content);
    /* 所以需要专门记录长度信息 */
    /* token 在此行中的起始下标 */
    token->sourceStartPos = eptrStart - pAnalyzer->expr;
    /* token 在此行中的原始长度 */
    token->sourceLength = pAnalyzer->eptr - eptrStart;
    return token;
}

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

    /* 运算符 */
    switch (firstChar) {
    case '+':   case '-':   case '*':   case '/':   case '^':
    case '%':   case '=':   case '!':   case '\\':
        buffer[0] = firstChar;
        buffer[1] = '\0';
        _self->eptr++;
        return assignToken(_self, TOKEN_OPR, buffer, eptrStart);
    case '(':   case ')':
        buffer[0] = firstChar;
        buffer[1] = '\0';
        _self->eptr++;
        return assignToken(_self, TOKEN_BKT, buffer, eptrStart);
    case ',':
        buffer[0] = firstChar;
        buffer[1] = '\0';
        _self->eptr++;
        return assignToken(_self, TOKEN_CMA, buffer, eptrStart);
    case ':':
        buffer[0] = firstChar;
        buffer[1] = '\0';
        _self->eptr++;
        return assignToken(_self, TOKEN_LABEL, buffer, eptrStart);
    case '>':
        secondChar = *(_self->eptr + 1);
        if (secondChar == '=') {
            buffer[0] = firstChar;
            buffer[1] = secondChar;
            buffer[2] = '\0';
            _self->eptr += 2;
            return assignToken(_self, TOKEN_OPR, buffer, eptrStart);
        } else {
            buffer[0] = firstChar;
            buffer[1] = '\0';
            _self->eptr++;
            return assignToken(_self, TOKEN_OPR, buffer, eptrStart);
        }
    case '<':
        secondChar = *(_self->eptr + 1);
        if (secondChar == '=') {
            buffer[0] = firstChar;
            buffer[1] = secondChar;
            buffer[2] = '\0';
            _self->eptr += 2;
            return assignToken(_self, TOKEN_OPR, buffer, eptrStart);
        } else if (secondChar == '>') {
            buffer[0] = firstChar;
            buffer[1] = secondChar;
            buffer[2] = '\0';
            _self->eptr += 2;
            return assignToken(_self, TOKEN_OPR, buffer, eptrStart);
        } else {
            buffer[0] = firstChar;
            buffer[1] = '\0';
            _self->eptr++;
            return assignToken(_self, TOKEN_OPR, buffer, eptrStart);
        }
    case '~':
        secondChar = *(_self->eptr + 1);
        if (secondChar == '=') {
            buffer[0] = firstChar;
            buffer[1] = secondChar;
            buffer[2] = '\0';
            _self->eptr += 2;
            return assignToken(_self, TOKEN_OPR, buffer, eptrStart);
        } else {
            buffer[0] = firstChar;
            buffer[1] = '\0';
            _self->eptr++;
            return assignToken(_self, TOKEN_UDF, buffer, eptrStart);
        }
    case '&':
        secondChar = *(_self->eptr + 1);
        if (secondChar == '&') {
            buffer[0] = firstChar;
            buffer[1] = secondChar;
            buffer[2] = '\0';
            _self->eptr += 2;
            return assignToken(_self, TOKEN_OPR, buffer, eptrStart);
        } else {
            buffer[0] = firstChar;
            buffer[1] = '\0';
            _self->eptr++;
            return assignToken(_self, TOKEN_OPR, buffer, eptrStart);
        }
    case '|':
        secondChar = *(_self->eptr + 1);
        if (secondChar == '|') {
            buffer[0] = firstChar;
            buffer[1] = secondChar;
            buffer[2] = '\0';
            _self->eptr += 2;
            return assignToken(_self, TOKEN_OPR, buffer, eptrStart);
        } else {
            buffer[0] = firstChar;
            buffer[1] = '\0';
            _self->eptr++;
            return assignToken(_self, TOKEN_UDF, buffer, eptrStart);
        }
    }

    /* 扫描到结尾 */
    if (firstChar == '\0') {
        return assignToken(_self, TOKEN_END, "", eptrStart);
    }
    /* 数字 */
    else if (isDigit(firstChar)) {
        while (isDigit(*_self->eptr)) {
            *pbuffer++ = *_self->eptr++;
        }
        /* 检查是不是带有小数 */
        if (*_self->eptr == '.') {
            /* 保存小数点 */
            *pbuffer++ = *_self->eptr++;
            /* 继续写入数字 */
            while (isDigit(*_self->eptr)) {
                *pbuffer++ = *_self->eptr++;
            }
        }

        *pbuffer = '\0';
        /* 过长 */
        if ((pbuffer - buffer) >= KB_CONTEXT_STRING_BUFFER_MAX) {
            assignToken(_self, TOKEN_ERR, "Number too long", eptrStart);
        }

        return assignToken(_self, TOKEN_NUM, buffer, eptrStart);
    }
    /* 标志符 identifier */
    else if (isAlpha(firstChar)) {
        int         i;
        int         isFunc = 0;
        const char* pIdEnd;

        while (isAlphaNum(*_self->eptr)) {
            *pbuffer++ = *_self->eptr++;
        }
        *pbuffer = '\0';

        /* 记录 identifier 停止位置 */
        pIdEnd = _self->eptr;

        /* 检查是否是关键字？*/
        for (i = 0; KEYWORDS[i].word; ++i) {
            if (StringEqual(KEYWORDS[i].word, buffer)) {
                return assignToken(_self, TOKEN_KEY, buffer, eptrStart);
            }
        }
        
        while (isSpace(*_self->eptr)) {
            _self->eptr++;
        }

        /* 后面是左括号，是函数定义或者函数调用 */
        if (*_self->eptr == '(') {
            _self->eptr++;
            isFunc = 1;
        }
        
        /* token 过长 */
        if ((pbuffer - buffer) >= KB_CONTEXT_STRING_BUFFER_MAX) {
            assignToken(_self, TOKEN_ERR, "Identifier too long", eptrStart);
        }

        if (isFunc) {
            /* 函数名：读取长度 = id长度 + 中间的空白 + 右括号 */
            return assignToken(_self, TOKEN_FNC, buffer, eptrStart);
        }
        else {
            /* 变量：读取长度 = 仅id长度 */
            _self->eptr = pIdEnd;
            return assignToken(_self, TOKEN_ID, buffer, eptrStart);
        }
    }
    /* 是字符串 */
    else if (firstChar == '"') {
        /* 跳过" */
        _self->eptr++;

        while (*_self->eptr != '"') {
            const char currentChar = *_self->eptr;
            /* 处理转义字符*/
            if (currentChar == '\\') {
                const char nextChar = *++_self->eptr;
                /* 换行 */
                if (nextChar == 'n') {
                    *pbuffer++ = '\n';
                }
                /* 回车 */
                else if (nextChar == 'r') {
                    *pbuffer++ = '\r';
                }
                /* 制表 */
                else if (nextChar == 't') {
                    *pbuffer++ = '\t';
                }
                /* 转义引号 */
                else if (nextChar == '"') {
                    *pbuffer++ = '\"';
                }
                /* 十六进制转义序列 */
                else if (nextChar == 'x') {
                    int hexDigitCount = 0;
                    int hexValue = 0;
                    while (hexDigitCount <= 2) {
                        char hexChar = *++_self->eptr;
                        if (isHexAlphaL(hexChar)) {
                            hexValue = (hexValue << 4) + (hexChar - 'a' + 0xA);
                        }
                        else if (isHexAlphaU(hexChar)) {
                            hexValue = (hexValue << 4) + (hexChar - 'A' + 0xA);
                        }
                        else if (isDigit(hexChar)) {
                            hexValue = (hexValue << 4) + (hexChar - '0' + 0);
                        }
                        else {
                            /* 不是十六进制数字，回退一位 */
                            --_self->eptr;
                            break;
                        }
                        ++hexDigitCount;
                    }
                    if (hexDigitCount <= 0) {
                        assignToken(_self, TOKEN_ERR, "Invalid hex escape char", eptrStart);
                    }
                    *pbuffer++ = hexValue;
                }
                /* 非法转义字符 */
                else {
                    assignToken(_self, TOKEN_ERR, "Invalid escape char", eptrStart);
                }
                _self->eptr++;
            }
            /* 不完整 */
            else if (currentChar == '\0') {
                assignToken(_self, TOKEN_ERR, "Incomplete string", eptrStart);
            }
            else {
                *pbuffer++ = *_self->eptr++;
            }
        }
        
        /* 跳过" */
        _self->eptr++;

        /* String过长 */
        if ((pbuffer - buffer) >= KB_CONTEXT_STRING_BUFFER_MAX) {
            assignToken(_self, TOKEN_ERR, "String too long", eptrStart);
        }

        *pbuffer = '\0';

        return assignToken(_self, TOKEN_STR, buffer, eptrStart);
    }
    /* 没定义的字符 */
    else {
        _self->eptr++;
        buffer[0] = firstChar;
        buffer[1] = '\0';
        return assignToken(_self, TOKEN_UDF, buffer, eptrStart);
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
    /* printf("[%s] %s\n", DBG_TOKEN_NAME[token->type], token->content); */
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
            /* 没有参数的函数调用 */
        }
        else {
            /* 至少有一个参数 */
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
        return 0;
    }
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
    /* 表达式里有语法错误 */
    if (!result) {
        if (errorRet) {
            errorRet->errorType = KBE_SYNTAX_ERROR;
            errorRet->errorPos = _self->eptr - _self->expr;
            if (_self->token.type == TOKEN_ERR) {
                StringCopy(errorRet->message, KB_CONTEXT_STRING_BUFFER_MAX, _self->token.content);
            }
        }
        return 0;
    }
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
    if (node->numChild > 0) {
        node->children = (ExprNode **)malloc(numChild * (sizeof(ExprNode *)));
        for (i = 0; i < node->numChild; ++i) {
            node->children[i] = NULL;
        }
    }
    else {
        node->children = NULL;
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

void enDestroy(ExprNode* node) {
    int i;

    if (node == NULL) return;

    if (node->children) {
        for (i = 0; i < node->numChild; ++i) {
            if (node->children[i]) free(node->children[i]);
        }
        free(node->children);
    }

    free(node);
}

void enDestroyVoidPtr(void* ptr) {
    enDestroy((ExprNode *)ptr);
}

void buildAstTryOperand(Analyzer *pAnalyzer, Vlist* pStackOperand, Vlist* pStackOperator);

void buildAstHandleBinaryOperator(Vlist* pStackOperand, Vlist* pStackOperator) {
    if (pStackOperator->size <= 0) {
        return;
    }
    ExprNode* enTop = (ExprNode *)vlPopBack(pStackOperator);
    /* 运算数出栈2个、作为运算符节点的子节点 */
    enTop->children[1] = (ExprNode *)vlPopBack(pStackOperand);
    enTop->children[0] = (ExprNode *)vlPopBack(pStackOperand);
    /* 运算符节（计算结果）点入运算数栈 */
    vlPushBack(pStackOperand, enTop);
}

void buildAstTryOperator(Analyzer* pAnalyzer, Vlist* pStackOperand, Vlist* pStackOperator) {
    Token *token = nextToken(pAnalyzer);
    if (token->type == TOKEN_OPR) {
        /* 创建运算符节点 */
        ExprNode* oprNode = enCreateByToken(token, 2);
        int currentPriority = OPERATOR_META[oprNode->param].priority;

        /* 检查现有的运算符堆栈，检查优先级 */
        while (pStackOperator->size > 0) {
            ExprNode *enTop = (ExprNode *)vlPeek(pStackOperator);
            /* 如果不是运算符，退出 */
            if (enTop->type != TOKEN_OPR) {
                break;
            }
            int topPriority = OPERATOR_META[enTop->param].priority;
            /* 如果栈顶的优先级较高 */
            if (topPriority >= currentPriority) {
                buildAstHandleBinaryOperator(pStackOperand, pStackOperator);
            }
            else {
                break;
            }
        }
        /* 本运算符节点入运算符栈 */
        vlPushBack(pStackOperator, oprNode);
        /* 右操作数入栈 */
        buildAstTryOperand(pAnalyzer, pStackOperand, pStackOperator);
    }
    else {
        rewindToken(pAnalyzer);
    }
}

void buildAstTryOperand(Analyzer *pAnalyzer, Vlist* pStackOperand, Vlist* pStackOperator) {
    Token *token;

    token = nextToken(pAnalyzer);

    /* 变量名 / 数字 / 字符串 */
    if (token->type == TOKEN_ID || token->type == TOKEN_NUM || token->type == TOKEN_STR) {
        /* 直接入运算数栈 */
        vlPushBack(pStackOperand, enCreateByToken(token, 0));
        /* 尝试下一个 token 是否是运算符 */
        buildAstTryOperator(pAnalyzer, pStackOperand, pStackOperator);
    }
    /* 一元运算符：取负 */
    else if (token->type == TOKEN_OPR && StringEqual("-", token->content)) {
        /* 创建负号节点但是不入栈 */
        ExprNode* enOprNeg = enCreateOperatorNegative(token);
        /* 负号后续的运算数入栈 */
        buildAstTryOperand(pAnalyzer, pStackOperand, pStackOperator);
        /* 后续运算数出栈，作为负号节点的子节点 */
        enOprNeg->children[0] = (ExprNode *)vlPopBack(pStackOperand);
        /* 负号节点入运算数栈 */
        vlPushBack(pStackOperand, enOprNeg);
        /* 尝试下一个 token 是否是运算符 */
        buildAstTryOperator(pAnalyzer, pStackOperand, pStackOperator);
    }
    /* 一元运算符：逻辑非 */
    else if (token->type == TOKEN_OPR && StringEqual("!", token->content)) {
        /* 创建逻辑非节点但是不入栈 */
        ExprNode* enOprNot = enCreateOperatorNot(token);
        /* 逻辑非后续的运算数入栈 */
        buildAstTryOperand(pAnalyzer, pStackOperand, pStackOperator);
        /* 后续运算数出栈，作为逻辑非节点的子节点 */
        enOprNot->children[0] = (ExprNode *)vlPopBack(pStackOperand);
        /* 逻辑非节点入运算数栈 */
        vlPushBack(pStackOperand, enOprNot);
        /* 尝试下一个 token 是否是运算符 */
        buildAstTryOperator(pAnalyzer, pStackOperand, pStackOperator);
    }
    /* 函数调用 */
    else if (token->type == TOKEN_FNC) {
        char*       funcName = StringDump(token->content);
        ExprNode*   funcNode = NULL;
        /* 入栈占位用的函数节点 */
        ExprNode*   funcNodePlaceholder;
        int         numArgs = 0;
        int         i;

        /* 生成一个占位用的函数节点并且入栈 */
        funcNodePlaceholder = enNew(TOKEN_FNC, "FUNC_PH", 0);
        vlPushBack(pStackOperator, funcNodePlaceholder);
    
        token = nextToken(pAnalyzer);
        if (token->type == TOKEN_BKT && *token->content == ')') {
            /* 没有参数 */
        } else {
            /* 至少一个参数 */
            rewindToken(pAnalyzer);
            while (1) {
                /* 运算数入栈 */
                buildAstTryOperand(pAnalyzer, pStackOperand, pStackOperator);
                /* 处理函数参数表达式 */
                while (pStackOperator->size > 0) {
                    ExprNode* enTop = (ExprNode *)vlPeek(pStackOperator);
                    /* 遇到占位符就退出 */
                    if (enTop == funcNodePlaceholder) {
                        break;
                    }
                    /* 处理运算符 */
                    buildAstHandleBinaryOperator(pStackOperand, pStackOperator);
                }
                numArgs++;
                /* 检查是否函数参数结束 */
                token = nextToken(pAnalyzer);
                if (token->type == TOKEN_CMA) {
                    continue;
                }
                else if (token->type == TOKEN_BKT && *token->content == ')') {
                    break;
                }
            }
        }
        /* 弹出并销毁占位符 */
        vlPopBack(pStackOperator);
        enDestroy(funcNodePlaceholder);

        /* 把子节点放入函数节点下 */
        funcNode = enNew(TOKEN_FNC, funcName, numArgs);
        free(funcName);
        for (i = 0; i < numArgs; ++i) {
            funcNode->children[numArgs - 1 - i] = (ExprNode *)vlPopBack(pStackOperand);
        }
        /* 函数节点入运算数栈 */
        vlPushBack(pStackOperand, funcNode);
        free(funcName);
        /* 尝试下一个 token 是否是运算符 */
        buildAstTryOperator(pAnalyzer, pStackOperand, pStackOperator);
    }
    /* 括号 */
    else if (token->type == TOKEN_BKT && *token->content == '(') {
        ExprNode* bktNode = enNew(TOKEN_BKT, "()", 1);
        /* 括号节点入操作符栈 */
        vlPushBack(pStackOperator, bktNode);
        /* 构建括号内的表达式 */
        buildAstTryOperand(pAnalyzer, pStackOperand, pStackOperator);
        /* 忽略最后的右括号 */
        nextToken(pAnalyzer);
        /* 处理合并括号中的表达式 */
        while (pStackOperator->size > 0) {
            ExprNode *enTop = (ExprNode *)vlPeek(pStackOperator);
            /* 遇到当前括号节点，弹出了所有的内容了，结束 */
            if (enTop == bktNode) {
                vlPopBack(pStackOperator);
                break;
            }
            /* 处理运算符 */
            buildAstHandleBinaryOperator(pStackOperand, pStackOperator);
        }
        /* 括号内表达式出栈，作为括号节点的子节点 */
        bktNode->children[0] = (ExprNode *)vlPopBack(pStackOperand);
        /* 括号节点入表达式栈 */
        vlPushBack(pStackOperand, bktNode);
        /* 尝试下一个 token 是否是运算符 */
        buildAstTryOperator(pAnalyzer, pStackOperand, pStackOperator);
    }
}

ExprNode* buildExpressionAbstractSyntaxTree(Analyzer *pAnalyzer) {
    Vlist*      pStackOperand;  /* ExprNode */
    Vlist*      pStackOperator; /* ExprNode */
    ExprNode*   enExprRoot;
    
    pStackOperand = vlNewList();
    pStackOperator = vlNewList();
    buildAstTryOperand(pAnalyzer, pStackOperand, pStackOperator);

    /* 检查运算符堆栈，如果不为空就弹出所有 */
    while (pStackOperator->size > 0) {
        buildAstHandleBinaryOperator(pStackOperand, pStackOperator);
    }

    enExprRoot = (ExprNode *)vlPopBack(pStackOperand);

    vlDestroy(pStackOperand, enDestroyVoidPtr);
    vlDestroy(pStackOperator, enDestroyVoidPtr);

    return enExprRoot;
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

int kbFormatBuildError(const KbBuildError *errorVal, char *strBuffer, int strLengthMax) {
    if (errorVal == NULL) {
        strBuffer[0] = 0;
        return 0;
    }
    
    switch (errorVal->errorType) {
    case KBE_NO_ERROR: 
        StringCopy(strBuffer, strLengthMax, "No error");
        break;
    case KBE_SYNTAX_ERROR: {
        Salvia_Format(strBuffer, "(%d) Syntax Error - %s", errorVal->errorPos, errorVal->message);
        break;
    }
    case KBE_UNDEFINED_IDENTIFIER: 
        Salvia_Format(strBuffer, "Undefined Identifier: \"%s\"", errorVal->message);
        break;
    case KBE_UNDEFINED_FUNC:
        Salvia_Format(strBuffer, "Undefined Function: \"%s\"", errorVal->message);
        break;
    case KBE_INVALID_NUM_ARGS:
        Salvia_Format(strBuffer, "Wrong number of arguments to function: \"%s\"", errorVal->message);
        break;
    case KBE_STRING_NO_SPACE:
        Salvia_Format(strBuffer, "No enough space for string");
        break;
    case KBE_TOO_MANY_VAR:
        Salvia_Format(strBuffer, "Too many variables. Failed to declare variable \"%s\"", errorVal->message);
        break;
    case KBE_ID_TOO_LONG:
        Salvia_Format(strBuffer, "Identifier too long - \"%s\"", errorVal->message);
        break;
    case KBE_DUPLICATED_LABEL:
        Salvia_Format(strBuffer, "Duplicated label: \"%s\"", errorVal->message);
        break;
    case KBE_DUPLICATED_VAR:
        Salvia_Format(strBuffer, "Duplicated variable: \"%s\"", errorVal->message);
        break;
    case KBE_UNDEFINED_LABEL:
        Salvia_Format(strBuffer, "Undefined variable: \"%s\"", errorVal->message);
        break;
    case KBE_GOTO_CROSS_SCOPE:
        Salvia_Format(strBuffer, "Goto statement cannot jump to label \"%s\" defined in a different scope", errorVal->message);
        break;
    case KBE_INCOMPLETE_CTRL_FLOW:
        Salvia_Format(strBuffer, "Incomplete control flow, missing 'end'.");
        break;
    case KBE_INCOMPLETE_FUNC:
        Salvia_Format(strBuffer, "Incomplete function.");
        break;
    case KBE_OTHER:
        StringCopy(strBuffer, strLengthMax, errorVal->message);
        break;
    }

    return 1;
}

KbCtrlFlowItem* csItemNew(int csType) {
    KbCtrlFlowItem* pCfItem = (KbCtrlFlowItem *)malloc(sizeof(KbCtrlFlowItem));
    pCfItem->csType = csType;
    pCfItem->iLabelEnd = -1;
    pCfItem->iLabelNext = -1;
    return pCfItem;
}

#define csItemRelease(pCfItem) free(pCfItem)

KbContext* contextCreate() {
    KbContext* context = (KbContext *)malloc(sizeof(KbContext));

    context->numVar = 0;
    context->stringBufferPtr = context->stringBuffer;
    memset(context->stringBuffer, 0, sizeof(context->stringBuffer));

    context->commandList = vlNewList();
    context->labelList = vlNewList();
    context->userFuncList = vlNewList();

    context->ctrlFlow.labelCounter = 0;
    context->ctrlFlow.stack = NULL;

    context->pCurrentFunc = NULL;
    context->funcLabelCounter = 0;

    return context;
}

void contextDestroy (KbContext *context) {
    if (!context) {
        return;
    }
    context->pCurrentFunc = NULL;
    vlDestroy(context->commandList, opCommandRelease);
    vlDestroy(context->labelList, free);
    vlDestroy(context->userFuncList, free);
    if (context->ctrlFlow.stack) {
        vlDestroy(context->ctrlFlow.stack, free);
    }

    free(context);
}

int contextSetVariable(KbContext *context, const char * varName) {
    if (context->numVar >= KB_CONTEXT_VAR_MAX) {
        return -1;
    }

    StringCopy(context->varList[context->numVar], sizeof(context->varList[0]), varName);

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

int contextSetLocalVariable(KbContext *context, const char * varName) {
    KbUserFunc* pUserFunc = context->pCurrentFunc;

    if (pUserFunc->numVar >= KB_CONTEXT_VAR_MAX) {
        return -1;
    }

    StringCopy(pUserFunc->varList[pUserFunc->numVar], sizeof(pUserFunc->varList[0]), varName);

    return pUserFunc->numVar++;
}

int contextGetLocalVariableIndex (const KbContext *context, const char * varToFind) {
    KbUserFunc* pUserFunc = context->pCurrentFunc;
    int i;
    for (i = 0; i < pUserFunc->numVar; ++i) {
        const char *varName = pUserFunc->varList[i];
        if (StringEqual(varName, varToFind)) {
            return i;
        }
    }
    return -1;
}

int contextAppendText(KbContext* context, const char* string) {
    char *oldBufferPtr = context->stringBufferPtr;

    if (context->stringBufferPtr - context->stringBuffer + strlen(string) > KB_CONTEXT_STRING_BUFFER_MAX) {
        return -1;
    }

    while ((*context->stringBufferPtr++ = *string++) != '\0');

    return oldBufferPtr - context->stringBuffer;
}

KbContextLabel* contextGetLabel(const KbContext *context, const char * labelNameToFind) {
    VlistNode *node;

    for (node = context->labelList->head; node != NULL; node = node->next) {
        KbContextLabel* label = (KbContextLabel *)node->data;
        if (StringEqual(label->name, labelNameToFind)) {
            return label;
        }
    }

    return NULL;
}

KbContextLabel* contextAddLabel(const KbContext *context, const char * labelName, KbLabelType labelType) {
    KbContextLabel* label = NULL;
    
    label = contextGetLabel(context, labelName);
    if (label) {
        return 0;
    }

    label = (KbContextLabel *)malloc(sizeof(KbContextLabel) + strlen(labelName) + 1);

    label->type = labelType;
    label->pos = 0;
    label->funcScope = NULL;
    StringCopy(label->name, KB_TOKEN_LENGTH_MAX, labelName);

    vlPushBack(context->labelList, label);

    return label;
}

KbCtrlFlowItem* contextCtrlPeek(const KbContext *context) {
    const VlistNode* tail = context->ctrlFlow.stack->tail;
    if (tail == NULL) {
        return NULL;
    }
    return (KbCtrlFlowItem *)(tail->data);
}

int contextCtrlPeekType(const KbContext *context) {
    KbCtrlFlowItem* pCfItem;
    pCfItem = contextCtrlPeek(context);
    if (!pCfItem) {
        return KBCS_NONE;
    }
    return pCfItem->csType;
}

KbContextLabel* contextCtrlAddLabel(KbContext* context, int labelIndex) {
    char szLabelBuf[100];

    if (labelIndex < 0) {
        return NULL;
    }
    Salvia_Format(szLabelBuf, CTRL_LABEL_FORMAT, labelIndex);
    
    return contextAddLabel(context, szLabelBuf, KBL_CTRL);
}

KbContextLabel* contextFuncAddLabel(KbContext* context, int labelIndex) {
    char szLabelBuf[100];

    if (labelIndex < 0) {
        return NULL;
    }
    Salvia_Format(szLabelBuf, FUNC_LABEL_FORMAT, labelIndex);
    
    return contextAddLabel(context, szLabelBuf, KBL_CTRL);
}

KbContextLabel* contextCtrlGetLabel(const KbContext* context, int labelIndex) {
    char szLabelBuf[100];
    if (labelIndex < 0) {
        return NULL;
    }
    Salvia_Format(szLabelBuf, CTRL_LABEL_FORMAT, labelIndex);
    return contextGetLabel(context, szLabelBuf);
}

KbContextLabel* contextFuncGetLabel(const KbContext* context, int labelIndex) {
    char szLabelBuf[100];
    if (labelIndex < 0) {
        return NULL;
    }
    Salvia_Format(szLabelBuf, FUNC_LABEL_FORMAT, labelIndex);
    return contextGetLabel(context, szLabelBuf);
}

void contextCtrlResetCounter(KbContext* context) {
    context->ctrlFlow.labelCounter = 0;
    if (context->ctrlFlow.stack) {
        vlDestroy(context->ctrlFlow.stack, free);
    }
    context->ctrlFlow.stack = vlNewList();
}

KbCtrlFlowItem* contextCtrlPush(KbContext* context, int csType) {
    KbCtrlFlowItem* pCfItem = csItemNew(csType);
    switch (csType) {
    case KBCS_IF_THEN:
        pCfItem->iLabelNext = context->ctrlFlow.labelCounter++;
        pCfItem->iLabelEnd = context->ctrlFlow.labelCounter++;
        break;
    case KBCS_ELSE_IF:
        pCfItem->iLabelNext = context->ctrlFlow.labelCounter++;
        pCfItem->iLabelEnd = -1;
        break;
    case KBCS_ELSE:
        pCfItem->iLabelNext = -1;
        pCfItem->iLabelEnd = -1;
        break;
    case KBCS_WHILE:
        pCfItem->iLabelNext = context->ctrlFlow.labelCounter++;
        pCfItem->iLabelEnd = context->ctrlFlow.labelCounter++;
        break;
    default:
        pCfItem->iLabelNext = -1;
        pCfItem->iLabelEnd = -1;
    }
    vlPushBack(context->ctrlFlow.stack, pCfItem);
    contextCtrlAddLabel(context, pCfItem->iLabelNext);
    contextCtrlAddLabel(context, pCfItem->iLabelEnd);
    
    return pCfItem;
}

KbUserFunc* contextFuncAdd(KbContext* context, const char* funcName, int numArg) {
    KbUserFunc* pUserFunc = (KbUserFunc *)malloc(sizeof(KbUserFunc));

    StringCopy(pUserFunc->funcName, KB_IDENTIFIER_LEN_MAX + 1, funcName);
    pUserFunc->numArg = numArg;
    pUserFunc->numVar = 0;
    pUserFunc->iLblFuncBegin = context->funcLabelCounter++;
    pUserFunc->iLblFuncEnd = context->funcLabelCounter++;
    vlPushBack(context->userFuncList, pUserFunc);

    contextFuncAddLabel(context, pUserFunc->iLblFuncBegin);
    contextFuncAddLabel(context, pUserFunc->iLblFuncEnd);

    return pUserFunc;
}

KbUserFunc* contextFuncFind(KbContext* context, const char* funcName, int* pIndex) {
    VlistNode* node;
    int i = 0;
    for (
        node = context->userFuncList->head;
        node != NULL;
        node = node->next, ++i
    ) {
        KbUserFunc* pUserFunc = (KbUserFunc *)node->data;
        if (StringEqual(funcName, pUserFunc->funcName)) {
            if (pIndex != NULL) *pIndex = i;
            return pUserFunc;
        }
    }
    return NULL;
}

KbOpCommand* opCommandCreateBuiltInFunction(int funcId) {
    KbOpCommand* cmd = (KbOpCommand*)malloc(sizeof(KbOpCommand));

    cmd->op = KBO_CALL_BUILT_IN;
    cmd->param.index = funcId;

    return cmd;
}

KbOpCommand* opCommandCreateUserFunction(int funcIndex) {
    KbOpCommand* cmd = (KbOpCommand*)malloc(sizeof(KbOpCommand));

    cmd->op = KBO_CALL_USER;
    cmd->param.index = funcIndex;

    return cmd;
}

KbOpCommand* opCommandCreatePushVar(int varIndex) {
    KbOpCommand* cmd = (KbOpCommand*)malloc(sizeof(KbOpCommand));

    cmd->op = KBO_PUSH_VAR;
    cmd->param.index = varIndex;

    return cmd;
}

KbOpCommand* opCommandCreatePushLocal(int varIndex) {
    KbOpCommand* cmd = (KbOpCommand*)malloc(sizeof(KbOpCommand));

    cmd->op = KBO_PUSH_LOCAL;
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

KbOpCommand* opCommandCreateAssignLocal(int varIndex) {
    KbOpCommand* cmd = (KbOpCommand*)malloc(sizeof(KbOpCommand));

    cmd->op = KBO_ASSIGN_LOCAL;
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

KbOpCommand* opCommandCreateReturn() {
    KbOpCommand* cmd = (KbOpCommand*)malloc(sizeof(KbOpCommand));

    cmd->op = KBO_RETURN;
    cmd->param.index = 0;

    return cmd;
}

KbOpCommand* opCommandCreateGoto(KbContextLabel *label) {
    KbOpCommand* cmd = (KbOpCommand*)malloc(sizeof(KbOpCommand));

    cmd->op = KBO_GOTO;
    cmd->param.ptr = label;

    return cmd;
}

KbOpCommand* opCommandCreateIfGoto(KbContextLabel *label) {
    KbOpCommand* cmd = (KbOpCommand*)malloc(sizeof(KbOpCommand));

    cmd->op = KBO_IF_GOTO;
    cmd->param.ptr = label;

    return cmd;
}

KbOpCommand* opCommandCreateUnlessGoto(KbContextLabel *label) {
    KbOpCommand* cmd = (KbOpCommand*)malloc(sizeof(KbOpCommand));

    cmd->op = KBO_UNLESS_GOTO;
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

    /* 函数调用 */
    if (node->type == TOKEN_FNC) {
        const BuiltInFunction*  targetFunc = NULL;
        int                 funcDefLength = sizeof(FUNCTION_LIST) / sizeof(FUNCTION_LIST[0]);
        char*               funcName = node->content;
        int                 iUserFuncIndex = -1;
        KbUserFunc*         pFunc = NULL;
    
        /* 检查是不是用户定义的函数 */
        pFunc = contextFuncFind(context, funcName, &iUserFuncIndex);
        /* 是用户定义的函数 */
        if (pFunc) {
            if (pFunc->numArg != node->numChild) {
                errorRet->errorPos = 0;
                errorRet->errorType = KBE_INVALID_NUM_ARGS;
                StringCopy(errorRet->message, KB_ERROR_MESSAGE_MAX, funcName);
                return 0;
            }
            vlPushBack(context->commandList, opCommandCreateUserFunction(iUserFuncIndex));
        }
        /* 尝试寻找内建函数 */
        else {
            for (i = 0; i < funcDefLength; ++i) {
                const BuiltInFunction *func = FUNCTION_LIST + i;
                if (StringEqual(func->funcName, funcName)) {
                    targetFunc = func;
                    break;
                }
            }
            /* 找不到 */
            if (targetFunc == NULL) {
                errorRet->errorPos = 0;
                errorRet->errorType = KBE_UNDEFINED_FUNC;
                StringCopy(errorRet->message, KB_ERROR_MESSAGE_MAX, funcName);
                return 0;
            }
            /* 参数数量不正确 */
            if (targetFunc->numArg != node->numChild) {
                errorRet->errorPos = 0;
                errorRet->errorType = KBE_INVALID_NUM_ARGS;
                StringCopy(errorRet->message, KB_ERROR_MESSAGE_MAX, funcName);
                return 0;
            }
            vlPushBack(context->commandList, opCommandCreateBuiltInFunction(targetFunc->funcId));
        }
    }
    /* 运算符 */
    else if (node->type == TOKEN_OPR) {
        vlPushBack(context->commandList, opCommandCreateOperator(node->param));
    }
    /* 变量 */
    else if (node->type == TOKEN_ID) {
        int varIndex = -1;
        int isFuncScope = context->pCurrentFunc != NULL;
        int isLocalVar = 0;
        char* varName = node->content;
        /* 当前在函数内，先尝试在函数内寻找 */
        if (isFuncScope) {
            varIndex = contextGetLocalVariableIndex(context, varName);
            if (varIndex >= 0) {
                isLocalVar = 1;
            }
        }
        /* 如果没找到，或者在全局，再在全局寻找 */
        if (varIndex < 0) {
            varIndex = contextGetVariableIndex(context, varName);
        }
        /* 变量未定义 */
        if (varIndex < 0) {
            errorRet->errorPos = 0;
            errorRet->errorType = KBE_UNDEFINED_IDENTIFIER;
            StringCopy(errorRet->message, KB_ERROR_MESSAGE_MAX, varName);
            return 0;
        }
        if (isLocalVar) {
            vlPushBack(context->commandList, opCommandCreatePushLocal(varIndex));
        }
        else {
            vlPushBack(context->commandList, opCommandCreatePushVar(varIndex));
        }
    }
    /* 字符串 */
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
    /* 数字 */
    else if (node->type == TOKEN_NUM) {
        KB_FLOAT num = (KB_FLOAT)kAtof(node->content);
        vlPushBack(context->commandList, opCommandCreatePushNum(num));
    }
    /* 括号 */
    else if (node->type == TOKEN_BKT) {
        /* 什么也不用做 */
    }
    /* 未定义的 node */
    else {
        errorRet->errorPos = 0;
        errorRet->errorType = KBE_OTHER;
        StringCopy(errorRet->message, KB_ERROR_MESSAGE_MAX, "unknown express");
        return 0;
    }

    return 1;
}

void dbgPrintContextCommand(const KbOpCommand *cmd) {
    printf("%-15s ", _KOCODE_NAME[cmd->op]);

    /* 带整数参数的cmd */
    if (cmd->op == KBO_PUSH_VAR
     || cmd->op == KBO_PUSH_LOCAL
     || cmd->op == KBO_PUSH_STR
     || cmd->op == KBO_CALL_BUILT_IN
     || cmd->op == KBO_CALL_USER
     || cmd->op == KBO_ASSIGN_VAR
     || cmd->op == KBO_ASSIGN_LOCAL
     || cmd->op == KBO_GOTO
     || cmd->op == KBO_IF_GOTO
     || cmd->op == KBO_UNLESS_GOTO) {
        printf("%-8d", cmd->param.index);
    }
    /* 带浮点数参数的cmd */
    else if (cmd->op == KBO_PUSH_NUM) {
        char szNum[KB_TOKEN_LENGTH_MAX];
        kFtoa((float)cmd->param.number, szNum, DEFAUL_FTOA_PRECISION);
        printf("%-8s", szNum);
    }
    /* 不带参数的cmd */
    else if ((cmd->op >= KBO_OPR_NEG && cmd->op <= KBO_OPR_LTEQ)
     || cmd->op == KBO_POP
     || cmd->op == KBO_STOP
     || cmd->op == KBO_RETURN
    ) {
        /* 打印空白 */
        printf("%-8s", " ");
    }
    /* 无法识别的cmd */
    else {
        printf("INVALID CMD [%x]", cmd->op);
    }
}

void dbgPrintContextCommandList (const KbContext *context) {
    VlistNode*  node    = context->commandList->head;
    int         lineNum = 0;

    while (node != NULL) {
        printf("%03d ", lineNum);
        dbgPrintContextCommand((KbOpCommand *)node->data);
        printf("\n");
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
    static const char * LABEL_TYPE_NAME[] = { "USER", "CTRL" };

    VlistNode *node = context->labelList->head;
    while (node != NULL) {
        KbContextLabel* label = (KbContextLabel *)node->data;
        printf("%-15s: [%4s] %03d\n", label->name, LABEL_TYPE_NAME[label->type], label->pos);
        node = node->next;
    }
}

void dbgPrintContextListFunction(const KbContext *context) {
    const VlistNode *node = context->userFuncList->head;
    while (node != NULL) {
        const KbUserFunc* pFunc = (KbUserFunc *)node->data;
        printf("%15s: args=%03d, vars=%03d\n", pFunc->funcName, pFunc->numArg, pFunc->numVar);
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
    
    /* 设置内置全局变量 */
    /* contextSetVariable(context, "hh"); */
    /* contextSetVariable(context, "mm"); */
    /* contextSetVariable(context, "ss"); */
    /* contextSetVariable(context, "ms"); */

    return context;
}

int kbScanLineSyntax(const char* line, KbContext *context, KbBuildError *errorRet) {
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
    /* 空行 */
    if (token->type == TOKEN_END) {
        /* 什么也不做 */
    }
    /* func 函数定义 */
    else if (token->type == TOKEN_KEY && StringEqual(KEYWORD_FUNC, token->content)) {
        KbUserFunc* pUserFunc = NULL;
    
        /* 当前函数定义还没结束 */
        if (context->pCurrentFunc != NULL) {
            compileLineReturnWithError(KBE_SYNTAX_ERROR, "Nested function not allowed.");
        }
        /* 当前控制结构还没结束 */
        if (context->ctrlFlow.stack->size > 0) {
            compileLineReturnWithError(KBE_SYNTAX_ERROR, "Cannot define function in control flow.");
        }
        /* 匹配函数名称 */
        token = nextToken(&analyzer);
        if (token->type != TOKEN_FNC) {
            compileLineReturnWithError(KBE_SYNTAX_ERROR, "Missing function name");
        }
        /* 检查函数名称长度 */
        if (strlen(token->content) > KB_IDENTIFIER_LEN_MAX) {
            compileLineReturnWithError(KBE_ID_TOO_LONG, token->content);
        }
        /* 在上下文中创建函数 */
        pUserFunc = contextFuncAdd(context, token->content, 0);
        context->pCurrentFunc = pUserFunc;
        /* 尝试下一个token */
        token = nextToken(&analyzer);
        /* 右括号结束 */
        if (token->type == TOKEN_BKT && token->content[0] == ')') {
            /* 无参数 */
        }
        /* 匹配参数列表*/
        else {
            int     varIndex = -1;
            char*   varName = NULL;
            /* token回退 */
            rewindToken(&analyzer);
            while (1) {
                /* 参数名字 */
                token = nextToken(&analyzer);
                if (token->type != TOKEN_ID) {
                    compileLineReturnWithError(KBE_SYNTAX_ERROR, "Error in function parameter list.");
                }
                pUserFunc->numArg++;
                /* 把参数名字创建为变量 */
                varName = token->content;
                varIndex = contextGetLocalVariableIndex(context, varName);
                if (varIndex < 0) {
                    varIndex = contextSetLocalVariable(context, varName);
                    if (varIndex < 0) {
                        compileLineReturnWithError(KBE_TOO_MANY_VAR, varName);
                    }
                }
                /* 变量已经存在了，报错 */
                else {
                    compileLineReturnWithError(KBE_DUPLICATED_VAR, varName);
                }
                /* 尝试下一个token */
                token = nextToken(&analyzer);
                /* 右括号，结束 */
                if (token->type == TOKEN_BKT && token->content[0] == ')') {
                    break;
                }
                /* 逗号，继续匹配 */
                else if (token->type == TOKEN_CMA) {
                    continue;
                }
                /* 错误 */
                compileLineReturnWithError(KBE_SYNTAX_ERROR, "Error in function parameter list.");
            }
        }
        /* 匹配函数定义结束 */
        token = nextToken(&analyzer);
        if (token->type != TOKEN_END) {
            compileLineReturnWithError(KBE_SYNTAX_ERROR, "Error in function declaring");
        }
    }
    /* if 语句 */
    else if (token->type == TOKEN_KEY && StringEqual(KEYWORD_IF, token->content)) {
        /* 匹配表达式 */
        ret = matchExpr(&analyzer);
        if (!ret) {
            compileLineReturnWithError(KBE_SYNTAX_ERROR, "Invalid express in if statement");
        }
        /* 下一个 token，结束或者 goto */
        token = nextToken(&analyzer);
        /* token 是 goto，if ... goto 格式 */
        if (token->type == TOKEN_KEY && StringEqual(KEYWORD_GOTO, token->content)) {
            /* 匹配标签 */
            token = nextToken(&analyzer);
            if (token->type != TOKEN_ID) {
                compileLineReturnWithError(KBE_SYNTAX_ERROR, "Missing label in if ... goto statement");
            }
            /* 匹配行结束 */
            token = nextToken(&analyzer);
            if (token->type != TOKEN_END) {
                compileLineReturnWithError(KBE_SYNTAX_ERROR, "Error in if statement");
            }
            return 1;
        }
        /* token 是本行结束，if / else 模式 */
        else if (token->type == TOKEN_END) {
            contextCtrlPush(context, KBCS_IF_THEN);
        }
        else {
            compileLineReturnWithError(KBE_SYNTAX_ERROR, "Error in if statement");
        }
    }
    /* elseif 语句 */
    else if (token->type == TOKEN_KEY && StringEqual(KEYWORD_ELSEIF, token->content)) {
        /* 匹配表达式 */
        ret = matchExpr(&analyzer);
        if (!ret) {
            compileLineReturnWithError(KBE_SYNTAX_ERROR, "Invalid express in elseif statement");
        }
        /* 匹配语句结束 */
        token = nextToken(&analyzer);
        if (token->type != TOKEN_END) {
            compileLineReturnWithError(KBE_SYNTAX_ERROR, "Invalid elseif statement");
        }
        peakCsType = contextCtrlPeekType(context);
        /* elseif 的前一个控制结构应该是 elseif 或者 if then */
        if (peakCsType != KBCS_IF_THEN && peakCsType != KBCS_ELSE_IF) {
            compileLineReturnWithError(KBE_SYNTAX_ERROR, "Invalid 'elseif'");
        }
        contextCtrlPush(context, KBCS_ELSE_IF);
    }
    /* else 语句 */
    else if (token->type == TOKEN_KEY && StringEqual(KEYWORD_ELSE, token->content)) {
        /* 匹配语句结束 */
        token = nextToken(&analyzer);
        if (token->type != TOKEN_END) {
            compileLineReturnWithError(KBE_SYNTAX_ERROR, "Invalid else statement");
        }
        peakCsType = contextCtrlPeekType(context); 
        /* else 的前一个控制结构应该是 else if 或者 if then */
        if (peakCsType != KBCS_IF_THEN && peakCsType != KBCS_ELSE_IF) {
            compileLineReturnWithError(KBE_SYNTAX_ERROR, "Invalid 'else'");
        }
        contextCtrlPush(context, KBCS_ELSE);
    }
    /* while 语句 */
    else if (token->type == TOKEN_KEY && StringEqual(KEYWORD_WHILE, token->content)) {
        /* 匹配表达式 */
        ret = matchExpr(&analyzer);
        if (!ret) {
            compileLineReturnWithError(KBE_SYNTAX_ERROR, "Invalid express in while statement");
        }
        /* 匹配语句结束 */
        token = nextToken(&analyzer);
        if (token->type != TOKEN_END) {
            compileLineReturnWithError(KBE_SYNTAX_ERROR, "Invalid while statement");
        }
        contextCtrlPush(context, KBCS_WHILE);
    }
    /* break 语句 */
    else if (token->type == TOKEN_KEY && StringEqual(KEYWORD_BREAK, token->content)) {
        /* 匹配语句结束 */
        token = nextToken(&analyzer);
        if (token->type != TOKEN_END) {
            compileLineReturnWithError(KBE_SYNTAX_ERROR, "Invalid token after 'break'");
        }
        /* 从栈顶向下寻找循环语句 */
        else {
            VlistNode* node = context->ctrlFlow.stack->tail;
            int found = 0;
            while (node) {
                const KbCtrlFlowItem* pCfItem;
                pCfItem = (KbCtrlFlowItem *)(node->data);
                if (pCfItem->csType == KBCS_WHILE) {
                    found = 1;
                    break;
                }
                node = node->prev;
            }
            if (!found) {
                compileLineReturnWithError(KBE_SYNTAX_ERROR, "Invalid 'break', no matching loop found.");
            }
        }
    }
    /* end 语句 */
    else if (token->type == TOKEN_KEY && StringEqual(KEYWORD_END, token->content)) {
        int popTarget = KBCS_NONE;
        int popFunc = 0;
        /* 获取 end 之后的 token，可以是 if / while */
        token = nextToken(&analyzer);
        if (token->type == TOKEN_KEY && StringEqual(KEYWORD_IF, token->content)) {
            popTarget = KBCS_IF_THEN;
        }
        else if (token->type == TOKEN_KEY && StringEqual(KEYWORD_WHILE, token->content)) {
            popTarget = KBCS_WHILE;
        }
        else if (token->type == TOKEN_KEY && StringEqual(KEYWORD_FUNC, token->content)) {
            popFunc = 1;
        }
        else {
            compileLineReturnWithError(KBE_SYNTAX_ERROR, "Invalid token after `end`");
        }
        /* 匹配语句结束 */
        token = nextToken(&analyzer);
        if (token->type != TOKEN_END) {
            compileLineReturnWithError(KBE_SYNTAX_ERROR, "");
        }
        /* 检查最近的函数定义 */
        if (popFunc) {
            if (context->pCurrentFunc == NULL) {
                compileLineReturnWithError(KBE_SYNTAX_ERROR, "The 'end func' not in a function");
            }
            context->pCurrentFunc = NULL;
        }
        /* 弹出最近 if 语句块 */
        else if (popTarget == KBCS_IF_THEN) {
            while (1) {
                int peakCsType = contextCtrlPeekType(context);
                /* 先弹出 else / else if */
                if (peakCsType == KBCS_ELSE || peakCsType == KBCS_ELSE_IF) {
                    csItemRelease(vlPopBack(context->ctrlFlow.stack));
                }
                /* 弹出 if，结束 */
                else if (peakCsType == KBCS_IF_THEN) {
                    csItemRelease(vlPopBack(context->ctrlFlow.stack));
                    break;
                }
                /* 其他的均为错误 */
                else {
                    compileLineReturnWithError(KBE_SYNTAX_ERROR, "Invalid 'end if', no matching if statement found.");
                }
            }
        }
        /* 弹出最近 while 语句块 */
        else if (popTarget == KBCS_WHILE) {
            int peakCsType = contextCtrlPeekType(context);
            if (peakCsType == KBCS_WHILE) {
                csItemRelease(vlPopBack(context->ctrlFlow.stack));
            }
            /* 其他的均为错误 */
            else {
                compileLineReturnWithError(KBE_SYNTAX_ERROR, "Invalid 'end while', no matching while statement found.");
            }
        }
    }
    /* exit 语句 */
    else if (token->type == TOKEN_KEY && StringEqual(KEYWORD_EXIT, token->content)) {
        /* 匹配行结束 */
        token = nextToken(&analyzer);
        if (token->type != TOKEN_END) {
            compileLineReturnWithError(KBE_SYNTAX_ERROR, "Error in exit statement");
        }
    }
    /* return 语句 */
    else if (token->type == TOKEN_KEY && StringEqual(KEYWORD_RETURN, token->content)) {
        /* 匹配表达式 */
        ret = matchExpr(&analyzer);
        if (!ret) {
            compileLineReturnWithError(KBE_SYNTAX_ERROR, "Invalid express in return statement");
        }
        /* 匹配行结束 */
        token = nextToken(&analyzer);
        if (token->type != TOKEN_END) {
            compileLineReturnWithError(KBE_SYNTAX_ERROR, "Error in return statement");
        }
    }
    /* goto 语句 */
    else if (token->type == TOKEN_KEY && StringEqual(KEYWORD_GOTO, token->content)) {
        /* 匹配标签 */
        token = nextToken(&analyzer);
        if (token->type != TOKEN_ID) {
            compileLineReturnWithError(KBE_SYNTAX_ERROR, "Missing label in goto statement");
        }
        /* 匹配行结束 */
        token = nextToken(&analyzer);
        if (token->type != TOKEN_END) {
            compileLineReturnWithError(KBE_SYNTAX_ERROR, "Error in goto statement");
        }
    }
    /* 用户自己写的 label */
    else if (token->type == TOKEN_LABEL) {
        char            labelName[KB_TOKEN_LENGTH_MAX];
        KbContextLabel* label;

        token = nextToken(&analyzer);
        if (token->type != TOKEN_ID) {
            compileLineReturnWithError(KBE_SYNTAX_ERROR, "Invalid label name.");
        }

        StringCopy(labelName, sizeof(labelName), token->content);

        token = nextToken(&analyzer);
        /* 匹配结束*/
        if (token->type != TOKEN_END) {
            compileLineReturnWithError(KBE_SYNTAX_ERROR, "Error in label command.");
        }
        /* 标签长度太长 */
        if (strlen(labelName) > KB_IDENTIFIER_LEN_MAX) {
            compileLineReturnWithError(KBE_ID_TOO_LONG, labelName);
        }
        /* 添加新标签 */
        label = contextAddLabel(context, labelName, KBL_USER);
        if (!label) {
            compileLineReturnWithError(KBE_DUPLICATED_LABEL, labelName);
        }
        /* 用户自己创建的 label，需要添加所在的函数域 */
        label->funcScope = context->pCurrentFunc;
    }
    /* dim 变量声明 */
    else if (token->type == TOKEN_KEY && StringEqual(KEYWORD_DIM, token->content)) {
        /* 匹配是变量名 */
        token = nextToken(&analyzer);
        if (token->type != TOKEN_ID) {
            compileLineReturnWithError(KBE_SYNTAX_ERROR, "Missing variable in dim statement");
        }
        /* 变量长度太长 */
        if (strlen(token->content) > KB_IDENTIFIER_LEN_MAX) {
            compileLineReturnWithError(KBE_ID_TOO_LONG, token->content);
        }
        /* 匹配行结束或者= */
        token = nextToken(&analyzer);
        /* 等号，带赋值的dim */
        if (token->type == TOKEN_OPR && StringEqual("=", token->content)) {
            /* 表达式 */
            if (!checkExpr(&analyzer, errorRet)) {
                compileLineReturnWithError(KBE_SYNTAX_ERROR, "Invalid expression.");
            }
            /* 匹配行结束 */
            token = nextToken(&analyzer);
            if (token->type != TOKEN_END) {
                compileLineReturnWithError(KBE_SYNTAX_ERROR, "Error in dim statement.");
            }
        }
        /* 直接结束 */
        else if (token->type == TOKEN_END) {
            /* 什么也不做 */
        }
        /* 其他情况是错误*/
        else {
            compileLineReturnWithError(KBE_SYNTAX_ERROR, "Error in dim statement");
        }
    }
    /* 可能是赋值 */
    else if (token->type == TOKEN_ID) {
        /* 匹配 '=' */ 
        token = nextToken(&analyzer);
        if (token->type == TOKEN_OPR && StringEqual("=", token->content)) {
            /* 检查表达式合法性 */
            if (!checkExpr(&analyzer, errorRet)) {
                compileLineReturnWithError(KBE_SYNTAX_ERROR, "Invalid expression.");
            }
        }
        /* 其他情况，此行是表达式 */
        else {
            /* 解析器回到开始位置 */
            resetToken(&analyzer);
            /* 检查表达式合法性 */
            if (!checkExpr(&analyzer, errorRet)) {
                compileLineReturnWithError(KBE_SYNTAX_ERROR, "Invalid expression.");
            }
        }
        /* 匹配行结束 */
        token = nextToken(&analyzer);
        if (token->type != TOKEN_END) {
            compileLineReturnWithError(KBE_SYNTAX_ERROR, "Error in assignment statement.");
        }
    }
    /* 其他情况，此行是表达式 */
    else {
        /* 解析器回到开始位置 */
        resetToken(&analyzer);
        /* 检查表达式合法性 */
        if (!checkExpr(&analyzer, errorRet)) {
            compileLineReturnWithError(KBE_SYNTAX_ERROR, "Invalid expression.");
        }
        /* 匹配行结束 */
        token = nextToken(&analyzer);
        if (token->type != TOKEN_END) {
            compileLineReturnWithError(KBE_SYNTAX_ERROR, "Error in expression.");
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

    /* 尝试读取第一个token */
    token = nextToken(&analyzer);

    /* 空行 */
    if (token->type == TOKEN_END) {
        /* 什么也不做 */
    }
    /* func 函数定义 */
    else if (token->type == TOKEN_KEY && StringEqual(KEYWORD_FUNC, token->content)) {
        KbContextLabel* pLabelFuncStart;
        KbContextLabel* pLabelFuncEnd;
        KbUserFunc*     pFunc;
        /* 找到函数并且设置为当前上下文的函数 */
        token = nextToken(&analyzer);
        pFunc = contextFuncFind(context, token->content, NULL);
        pLabelFuncStart = contextFuncGetLabel(context, pFunc->iLblFuncBegin);
        pLabelFuncEnd = contextFuncGetLabel(context, pFunc->iLblFuncEnd);
        /* 添加跳转指令，全局执行跳过函数定义 */
        vlPushBack(context->commandList, opCommandCreateGoto(pLabelFuncEnd));
        /* 更新函数开始标签的位置 */
        pLabelFuncStart->pos = context->commandList->size;
        context->pCurrentFunc = pFunc;
    }
    /* if 语句 */
    else if (token->type == TOKEN_KEY && StringEqual(KEYWORD_IF, token->content)) {        
        /* 编译表达式 */
        exprRoot = buildExpressionAbstractSyntaxTree(&analyzer);
        ret = compileExprTree(context, exprRoot, errorRet);
        if (!ret) {
            compileLineReturn(0);
        }
        enDestroy(exprRoot);
        /* 下一个 token，结束或者 goto */
        token = nextToken(&analyzer);
        /* if ... goto 模式*/
        if (token->type == TOKEN_KEY && StringEqual(KEYWORD_GOTO, token->content)) {
            KbContextLabel *label;
            /* 跳转标签 */
            token = nextToken(&analyzer);
            /* 找到标签定义 */
            label = contextGetLabel(context, token->content);
            if (!label) {
                /* 错误：goto用到了没定义过的标签 */
                compileLineReturnWithError(KBE_UNDEFINED_LABEL, token->content);
            }
            /* 检查跳转的 label 是不是不在同一个域 */
            if (label->funcScope != context->pCurrentFunc) {
                compileLineReturnWithError(KBE_GOTO_CROSS_SCOPE, token->content);
            }
            /* 生成 cmd */
            vlPushBack(context->commandList, opCommandCreateIfGoto(label));
        }
        /* if / else 模式 */
        else if (token->type == TOKEN_END) {
            KbCtrlFlowItem* pCfItem;
            KbContextLabel* label;
            /* iLabelNext: 条件不满足的时候，跳去下一条 else if / else */
            pCfItem = contextCtrlPush(context, KBCS_IF_THEN);
            label = contextCtrlGetLabel(context, pCfItem->iLabelNext);
            /* 生成 cmd */
            vlPushBack(context->commandList, opCommandCreateUnlessGoto(label));
        }
    }
    /* elseif 语句 */
    else if (token->type == TOKEN_KEY && StringEqual(KEYWORD_ELSEIF, token->content)) {
        KbContextLabel* label;
        KbContextLabel* prevLabel;
        KbCtrlFlowItem* pCfItem;
        KbCtrlFlowItem* pPrevCsItem;
    
        /* 寻找最近的 if */
        VlistNode *stackNode = context->ctrlFlow.stack->tail;
        while (stackNode) {
            pPrevCsItem = (KbCtrlFlowItem *)stackNode->data;
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
        exprRoot = buildExpressionAbstractSyntaxTree(&analyzer);
        ret = compileExprTree(context, exprRoot, errorRet);
        if (!ret) {
            compileLineReturn(0);
        }
        enDestroy(exprRoot);

        /* 找到条件不满足时跳转的标签 */
        pCfItem = contextCtrlPush(context, KBCS_ELSE_IF);
        label = contextCtrlGetLabel(context, pCfItem->iLabelNext);

        /* 生成cmd */
        vlPushBack(context->commandList, opCommandCreateUnlessGoto(label));
    }
    /* else 语句 */
    else if (token->type == TOKEN_KEY && StringEqual(KEYWORD_ELSE, token->content)) {
        KbContextLabel* prevLabel;
        KbCtrlFlowItem* pPrevCsItem;
    
        /* 寻找最近的 if */
        VlistNode *stackNode = context->ctrlFlow.stack->tail;
        while (stackNode) {
            pPrevCsItem = (KbCtrlFlowItem *)stackNode->data;
            if (pPrevCsItem->csType == KBCS_IF_THEN) break;
            stackNode = stackNode->prev;
        }
        /* 条件满足的时候执行完毕跳转到 end if */
        prevLabel = contextCtrlGetLabel(context, pPrevCsItem->iLabelEnd);
        /* 生成cmd */
        vlPushBack(context->commandList, opCommandCreateGoto(prevLabel));

        /* 更新上一条 */
        pPrevCsItem = contextCtrlPeek(context);
        prevLabel = contextCtrlGetLabel(context, pPrevCsItem->iLabelNext);

        /* 更新上一个项目的跳转地址 */
        prevLabel->pos = context->commandList->size;
    }
    /* while 语句 */
    else if (token->type == TOKEN_KEY && StringEqual(KEYWORD_WHILE, token->content)) {
        KbContextLabel* labelEnd;
        KbContextLabel* labelStart;
        KbCtrlFlowItem* pCfItem;
        pCfItem = contextCtrlPush(context, KBCS_WHILE);
        /* 循环开头的标签，更新位置 */
        labelStart = contextCtrlGetLabel(context, pCfItem->iLabelNext);
        labelStart->pos = context->commandList->size;
        /* 编译表达式 */
        exprRoot = buildExpressionAbstractSyntaxTree(&analyzer);
        ret = compileExprTree(context, exprRoot, errorRet);
        if (!ret) {
            compileLineReturn(0);
        }
        enDestroy(exprRoot);
        /* 不满足的时候跳转标签 */
        labelEnd = contextCtrlGetLabel(context, pCfItem->iLabelEnd);
        /* 生成cmd */
        vlPushBack(context->commandList, opCommandCreateUnlessGoto(labelEnd));
    }
    /* break 语句 */
    else if (token->type == TOKEN_KEY && StringEqual(KEYWORD_BREAK, token->content)) {
        /* 从栈顶向下寻找循环语句 */
        VlistNode*          node = context->ctrlFlow.stack->tail;
        KbCtrlFlowItem*   pCfItem;
        KbContextLabel*     label;
        while (node) {
            pCfItem = (KbCtrlFlowItem *)(node->data);
            if (pCfItem->csType == KBCS_WHILE) {
                break;
            }
            node = node->prev;
        }
        /* 生成cmd，跳转回循环结束 */
        label = contextCtrlGetLabel(context, pCfItem->iLabelEnd);
        vlPushBack(context->commandList, opCommandCreateGoto(label));
    }
    /* end 语句 */
    else if (token->type == TOKEN_KEY && StringEqual(KEYWORD_END, token->content)) {
        KbContextLabel* label;
        KbCtrlFlowItem* pPrevCsItem;
        int popTarget = KBCS_NONE;
        int popFunc = 0;
        /* 获取 end 之后的 token，可以是 if / while */
        token = nextToken(&analyzer);
        if (token->type == TOKEN_KEY && StringEqual(KEYWORD_IF, token->content)) {
            popTarget = KBCS_IF_THEN;
        }
        else if (token->type == TOKEN_KEY && StringEqual(KEYWORD_WHILE, token->content)) {
            popTarget = KBCS_WHILE;
        }
        else if (token->type == TOKEN_KEY && StringEqual(KEYWORD_FUNC, token->content)) {
            popFunc = 1;
        }
        /* 函数结束 */
        if (popFunc) {
            KbOpCommand* pOpCmd = NULL;
            label = contextFuncGetLabel(context, context->pCurrentFunc->iLblFuncEnd);
            context->pCurrentFunc = NULL;
            /* 如果用户写了return 就不自动添加return了 */
            if (context->commandList->size > 0) {
                pOpCmd = (KbOpCommand *)context->commandList->tail->data;
            }
            if (pOpCmd == NULL || pOpCmd->op != KBO_RETURN) {
                /* 生成cmd，默认情况返回0 */
                vlPushBack(context->commandList, opCommandCreatePushNum(0));
                vlPushBack(context->commandList, opCommandCreateReturn());
            }
            /* 更新函数结束标签的位置 */
            label->pos = context->commandList->size;
        }
        /* 弹出最近 if / elseif / else 语句块 */
        else if (popTarget == KBCS_IF_THEN) {
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
                csItemRelease(vlPopBack(context->ctrlFlow.stack));
                if (csType == KBCS_IF_THEN) {
                    break;
                }
            }
        }
        /* 弹出最近 while 语句块 */
        else if (popTarget == KBCS_WHILE) {
            /* 从堆栈中拿出来上一个项目 */
            pPrevCsItem = contextCtrlPeek(context);
            /* 生成cmd，跳转回循环开头 */
            label = contextCtrlGetLabel(context, pPrevCsItem->iLabelNext);
            vlPushBack(context->commandList, opCommandCreateGoto(label));
            /* 更新上一个项目的跳转地址 */
            label = contextCtrlGetLabel(context, pPrevCsItem->iLabelEnd);
            label->pos = context->commandList->size;
            /* 弹出 while */
            csItemRelease(vlPopBack(context->ctrlFlow.stack));
        }
    }
    /* exit 语句 */
    else if (token->type == TOKEN_KEY && StringEqual(KEYWORD_EXIT, token->content)) {
        vlPushBack(context->commandList, opCommandCreateStop());
    }
    /* return 语句 */
    else if (token->type == TOKEN_KEY && StringEqual(KEYWORD_RETURN, token->content)) {
        /* 编译表达式 */
        exprRoot = buildExpressionAbstractSyntaxTree(&analyzer);
        ret = compileExprTree(context, exprRoot, errorRet);
        if (!ret) {
            compileLineReturn(0);
        }
        enDestroy(exprRoot);
        /* 生成cmd */
        vlPushBack(context->commandList, opCommandCreateReturn());
    }
    /* goto 语句 */
    else if (token->type == TOKEN_KEY && StringEqual(KEYWORD_GOTO, token->content)) {
        KbContextLabel* label;
        /* 获取标签 */
        token = nextToken(&analyzer);
        label = contextGetLabel(context, token->content);
        if (!label) {
            /* 错误，引用了未定义的标签 */
            compileLineReturnWithError(KBE_UNDEFINED_LABEL, token->content);
        }
        /* 检查跳转的 label 是不是不在同一个域 */
        if (label->funcScope != context->pCurrentFunc) {
            compileLineReturnWithError(KBE_GOTO_CROSS_SCOPE, token->content);
        }
        vlPushBack(context->commandList, opCommandCreateGoto(label));
    }
    /* 用户自己写的 label */
    else if (token->type == TOKEN_LABEL) {
        KbContextLabel* label;
        /* 获取标签名 */
        token = nextToken(&analyzer);
        label = contextGetLabel(context, token->content);
        /* 更新标签在字cmd的位置（当前长度） */
        label->pos = context->commandList->size;
    }
    /* dim 变量声明 */
    else if (token->type == TOKEN_KEY && StringEqual(KEYWORD_DIM, token->content)) {
        char*   varName;;
        int     varIndex;
        int     isFuncScope = context->pCurrentFunc != NULL;
        /* 获取变量名 */
        token = nextToken(&analyzer);
        varName = token->content;
        /* 当前在函数内 */
        if (isFuncScope) {
            /* 获取变量 index */
            varIndex = contextGetLocalVariableIndex(context, varName);
            /* 创建新变量 */
            if (varIndex < 0) {
                varIndex = contextSetLocalVariable(context, varName);
                if (varIndex < 0) {
                    compileLineReturnWithError(KBE_TOO_MANY_VAR, varName);
                }
            }
            /* 变量已经定义过了 */
            else {
                compileLineReturnWithError(KBE_DUPLICATED_VAR, varName);
            }
        }
        /* 当前是全局*/
        else {
            /* 获取变量 index */
            varIndex = contextGetVariableIndex(context, varName);
            /* 创建新变量 */
            if (varIndex < 0) {
                varIndex = contextSetVariable(context, varName);
                if (varIndex < 0) {
                    compileLineReturnWithError(KBE_TOO_MANY_VAR, varName);
                }
            }
            /* 变量已经定义过了 */
            else {
                compileLineReturnWithError(KBE_DUPLICATED_VAR, varName);
            }
        }
        /* 匹配行结束或者= */
        token = nextToken(&analyzer);
        /* 赋值 = */
        if (token->type == TOKEN_OPR && StringEqual("=", token->content)) {
            exprRoot = buildExpressionAbstractSyntaxTree(&analyzer);
            ret = compileExprTree(context, exprRoot, errorRet);
            if (!ret) {
                compileLineReturn(0);
            }
            enDestroy(exprRoot);
            /* 生成cmd，赋值 */
            if (isFuncScope) {
                vlPushBack(context->commandList, opCommandCreateAssignLocal(varIndex));
            } else {
                vlPushBack(context->commandList, opCommandCreateAssignVar(varIndex));
            }
        }
        /* 直接结束 */
        else if (token->type == TOKEN_END) {
            /* 什么也不做 */
        }
    }
    /* 可能是赋值 */
    else if (token->type == TOKEN_ID) {
        char    varName[KB_TOKEN_LENGTH_MAX];
        int     varIndex = -1;
        int     isFuncScope = context->pCurrentFunc != NULL;
        int     isLocalVar = 0;
    
        StringCopy(varName, sizeof(varName), token->content);

        /* 尝试下一个 token */
        token = nextToken(&analyzer);
        /* 匹配 '=' */ 
        if (token->type == TOKEN_OPR && StringEqual("=", token->content)) {
            /* 当前在函数内，先尝试在函数内寻找 */
            if (isFuncScope) {
                varIndex = contextGetLocalVariableIndex(context, varName);
                if (varIndex >= 0) {
                    isLocalVar = 1;
                }
            }
            /* 如果没找到，或者在全局，再在全局寻找 */
            if (varIndex < 0) {
                varIndex = contextGetVariableIndex(context, varName);
            }
            if (varIndex < 0) {
                compileLineReturnWithError(KBE_UNDEFINED_IDENTIFIER, varName);
                return 0;
            }
            /* 编译表达式 */
            exprRoot = buildExpressionAbstractSyntaxTree(&analyzer);
            ret = compileExprTree(context, exprRoot, errorRet);
            if (!ret) {
                compileLineReturn(0);
            }
            enDestroy(exprRoot);
            /* 生成cmd，赋值 */
            if (isLocalVar) {
                vlPushBack(context->commandList, opCommandCreateAssignLocal(varIndex));
            }
            else {
                vlPushBack(context->commandList, opCommandCreateAssignVar(varIndex));
            }
        }
        /* 其他情况，此行是表达式 */
        else {
            /* 解析器回到开始位置 */
            resetToken(&analyzer);
            /* 编译表达式 */
            exprRoot = buildExpressionAbstractSyntaxTree(&analyzer);
            ret = compileExprTree(context, exprRoot, errorRet);
            if (!ret) {
                compileLineReturn(0);
            }
            enDestroy(exprRoot);
            /* pop 未使用的值 */
            vlPushBack(context->commandList, opCommandCreatePop());
        }
    }
    /* 其他情况，此行是表达式 */
    else {
        /* 解析器回到开始位置 */
        resetToken(&analyzer);
        /* 编译表达式 */
        exprRoot = buildExpressionAbstractSyntaxTree(&analyzer);
        ret = compileExprTree(context, exprRoot, errorRet);
        if (!ret) {
            compileLineReturn(0);
        }
        enDestroy(exprRoot);
        /* pop 未使用的值 */
        vlPushBack(context->commandList, opCommandCreatePop());
    }
    
    compileLineReturn(1);
}

int kbCompileEnd(KbContext *context) {
    VlistNode *node;

    /* 更新所有GOTO / IFGOTO cmd的标签，把ptr改为具体的cmd位置 */
    for (node = context->commandList->head; node != NULL; node = node->next) { 
        KbOpCommand *cmd = (KbOpCommand *)node->data;
        
        if (cmd->op == KBO_IF_GOTO || cmd->op == KBO_GOTO || cmd->op == KBO_UNLESS_GOTO) {
            KbContextLabel *label = (KbContextLabel *)cmd->param.ptr;
            cmd->param.index = label->pos;
        }
    }
    /* 手动在末尾添加一个停止命令 */
    vlPushBack(context->commandList, opCommandCreateStop());
    return 1;
}

int kbSerialize(
    const KbContext*    context,
    KByte**             ppRaw,
    int *               pByteLength
) {
    KByte*  pEntireRaw;
    int     byteLengthHeader;
    int     numFunc;
    int     byteLengthFunc;
    int     byteLengthCmd;
    int     numCmd;
    int     byteLengthString;
    int     byteLengthEntire;
    int     i;

    KbBinaryHeader*     pHeader;
    KbExportedFunction* pExpFunc;
    KbOpCommand*        pCmd;
    char *              pString;
    VlistNode*          node;

    /* 序列化后的内存布局
    ---------- layout ----------   <--- 0
    |                          |
    |        * header *        |      + sizeof(KbBinaryHeader)
    |                          |
    ----------------------------   <--- funcBlockStart
    |                          |
    |         * func *         |      + byteLengthFunc
    |                          |
    ----------------------------   <--- cmdBlockStart
    |                          |
    |         * cmd *          |      + byteLengthCmd
    |                          |
    ----------------------------   <--- stringBlockStart
    |                          |
    |        * string *        |      + byteLengthString
    |                          |
    ----------------------------
    */

    numFunc = context->userFuncList->size;

    /* 计算分段长度和总长度 */
    byteLengthHeader    = sizeof(KbBinaryHeader);
    byteLengthFunc      = sizeof(KbExportedFunction) * numFunc;
    numCmd              = context->commandList->size;
    byteLengthCmd       = sizeof(KbOpCommand) * numCmd;
    byteLengthString    = context->stringBufferPtr - context->stringBuffer;
    byteLengthEntire    = byteLengthHeader
                        + byteLengthFunc
                        + byteLengthCmd
                        + byteLengthString;

    /* 申请空间*/
    pEntireRaw  = (KByte *)malloc(byteLengthEntire);
    pHeader     = (KbBinaryHeader *)pEntireRaw;
    memset(pHeader, 0, sizeof(KbBinaryHeader));

    /* 写入文件头 */
    pHeader->headerMagic.bVal[0]    = HEADER_MAGIC_BYTE_0;
    pHeader->headerMagic.bVal[1]    = HEADER_MAGIC_BYTE_1;
    pHeader->headerMagic.bVal[2]    = HEADER_MAGIC_BYTE_2;
    pHeader->headerMagic.bVal[3]    = HEADER_MAGIC_BYTE_3;
    pHeader->isLittleEndian         = checkIsLittleEndian();
    pHeader->numVariables           = context->numVar;
    pHeader->numFunc                = numFunc;
    pHeader->funcBlockStart         = 0 + byteLengthHeader;
    pHeader->cmdBlockStart          = pHeader->funcBlockStart + byteLengthFunc;
    pHeader->numCmd                 = numCmd;
    pHeader->stringBlockStart       = pHeader->cmdBlockStart + byteLengthCmd;
    pHeader->stringBlockLength      = byteLengthString;
    /* 设置写入用的指针头 */
    pExpFunc    = (KbExportedFunction *)(pEntireRaw + pHeader->funcBlockStart);
    pCmd        = (KbOpCommand *)(pEntireRaw + pHeader->cmdBlockStart);
    pString     = (char *)(pEntireRaw + pHeader->stringBlockStart);
    /* 写入用户函数定义 */
    for (
        node = context->userFuncList->head;
        node != NULL;
        node = node->next
    ) {
        KbUserFunc*     pUserFunc = (KbUserFunc *)node->data;
        KbContextLabel* pLabel = contextFuncGetLabel(context, pUserFunc->iLblFuncBegin);
        memset(pExpFunc, 0, sizeof(*pExpFunc));
        StringCopy(pExpFunc->funcName, KB_IDENTIFIER_LEN_MAX + 1, pUserFunc->funcName);
        pExpFunc->numArg = pUserFunc->numArg;
        pExpFunc->numVar = pUserFunc->numVar;
        pExpFunc->pos = pLabel->pos;
        pExpFunc++;
    }
    /* 写入cmd */
    node = context->commandList->head;
    for (i = 0; node != NULL; ++i, node = node->next) {
        memcpy(pCmd + i, node->data, sizeof(KbOpCommand));
    }
    /* 写入字符串 */
    memset(pString, 0, byteLengthString);
    memcpy(pString, context->stringBuffer, byteLengthString);

    *ppRaw = pEntireRaw;
    *pByteLength = byteLengthEntire;

    return 1;
}

void dbgPrintHeader(const KbBinaryHeader* h) {
    printf("header              = 0x%x\n",  h->headerMagic.dVal);
    printf("is little endian    = %d\n",    h->isLittleEndian);
    printf("extension id        = %s\n",    h->szExtensionId);
    printf("variable num        = %d\n",    h->numVariables);
    printf("func block start    = %d\n",    h->funcBlockStart);
    printf("func num            = %d\n",    h->numFunc);
    printf("cmd block start     = %d\n",    h->cmdBlockStart);
    printf("cmd num             = %d\n",    h->numCmd);
    printf("string block start  = %d\n",    h->stringBlockStart);
    printf("string block length = %d\n",    h->stringBlockLength);
}