#include "klexer.h"
#include "kutils.h"
#include "kalias.h"

typedef struct {
    char*   szKeyword;
    int     iKeywordId;
} KeywordIdMapItem;

static const KeywordIdMapItem KeywordsIdMap[] = {
    { KB_KEYWORD_DIM,       KBKID_DIM       },
    { KB_KEYWORD_REDIM,     KBKID_REDIM     },
    { KB_KEYWORD_GOTO,      KBKID_GOTO      },
    { KB_KEYWORD_IF,        KBKID_IF        },
    { KB_KEYWORD_ELSEIF,    KBKID_ELSEIF    },
    { KB_KEYWORD_ELSE,      KBKID_ELSE      },
    { KB_KEYWORD_WHILE,     KBKID_WHILE     },
    { KB_KEYWORD_DO,        KBKID_DO        },
    { KB_KEYWORD_FOR,       KBKID_FOR       },
    { KB_KEYWORD_TO,        KBKID_TO        },
    { KB_KEYWORD_STEP,      KBKID_STEP      },
    { KB_KEYWORD_NEXT,      KBKID_NEXT      },
    { KB_KEYWORD_CONTINUE,  KBKID_CONTINUE  },
    { KB_KEYWORD_BREAK,     KBKID_BREAK     },
    { KB_KEYWORD_END,       KBKID_END       },
    { KB_KEYWORD_RETURN,    KBKID_RETURN    },
    { KB_KEYWORD_FUNC,      KBKID_FUNC      },
    { KB_KEYWORD_EXIT,      KBKID_EXIT      }
};

static int getKeywordIdFromString(const char *szText) {
    static int  iNumKeywords = sizeof(KeywordsIdMap) / sizeof(KeywordsIdMap[0]);
    int         i;
    
    for (i = 0; i < iNumKeywords; ++i) {
        if (IsStringEqual(KeywordsIdMap[i].szKeyword, szText)) {
            return KeywordsIdMap[i].iKeywordId;
        }
    }

    return KBKID_NONE;
}

static void assignToken(KbLineAnalyzer* pAnalyzer, KbTokenType iType, const char *szContent, const char *pSourceStart) {
    KbToken* pToken = &pAnalyzer->token;
    pToken->iType = iType;
    /* 此为转义过的 token 内容，可能丢失了转义字符、引号、左括号等内容，所以需要专门记录长度信息  */
    StringCopy(pToken->szContent, sizeof(pToken->szContent), szContent);
    /* token 在此行中的起始下标 */
    pToken->iSourceStart = pSourceStart - pAnalyzer->szLine;
    /* token 在此行中的原始长度 */
    pToken->iSourceLength = pAnalyzer->pCurrent - pSourceStart;
}

void KAnalyzer_NextToken(KbLineAnalyzer *pAnalyzer) {
    char        firstChar, secondChar;
    char        szBuffer[KB_TOKEN_LENGTH_MAX * 2];
    char*       pBuffer = szBuffer;
    const char* pSourceStart;

    while (isSpace(*pAnalyzer->pCurrent)) {
        pAnalyzer->pCurrent++;
    }

    pSourceStart = pAnalyzer->pCurrent;

    firstChar = *pAnalyzer->pCurrent;

    /* 运算符 / 注释 / 标签符号 */
    switch (firstChar) {
    case ';': /* 遇到分号即认为行结束，parser会判断后续逻辑，跳过分号继续本行 */
    case '#': /* 遇到注释即认为行结束 */
        return assignToken(pAnalyzer, TOKEN_LINE_END, "", pSourceStart);
    case '+':   case '-':   case '*':   case '/':   case '^':
    case '%':   case '=':   case '!':   case '\\':
        szBuffer[0] = firstChar;
        szBuffer[1] = '\0';
        pAnalyzer->pCurrent++;
        return assignToken(pAnalyzer, TOKEN_OPERATOR, szBuffer, pSourceStart);
    case '(':
        szBuffer[0] = firstChar;
        szBuffer[1] = '\0';
        pAnalyzer->pCurrent++;
        return assignToken(pAnalyzer, TOKEN_PAREN_L, szBuffer, pSourceStart);
    case ')':
        szBuffer[0] = firstChar;
        szBuffer[1] = '\0';
        pAnalyzer->pCurrent++;
        return assignToken(pAnalyzer, TOKEN_PAREN_R, szBuffer, pSourceStart);
    case '[':
        szBuffer[0] = firstChar;
        szBuffer[1] = '\0';
        pAnalyzer->pCurrent++;
        return assignToken(pAnalyzer, TOKEN_BRACKET_L, szBuffer, pSourceStart);
    case ']':
        szBuffer[0] = firstChar;
        szBuffer[1] = '\0';
        pAnalyzer->pCurrent++;
        return assignToken(pAnalyzer, TOKEN_BRACKET_R, szBuffer, pSourceStart);
    case ',':
        szBuffer[0] = firstChar;
        szBuffer[1] = '\0';
        pAnalyzer->pCurrent++;
        return assignToken(pAnalyzer, TOKEN_COMMA, szBuffer, pSourceStart);
    case ':':
        szBuffer[0] = firstChar;
        szBuffer[1] = '\0';
        pAnalyzer->pCurrent++;
        return assignToken(pAnalyzer, TOKEN_LABEL_SIGN, szBuffer, pSourceStart);
    case '>':
        secondChar = *(pAnalyzer->pCurrent + 1);
        if (secondChar == '=') {
            szBuffer[0] = firstChar;
            szBuffer[1] = secondChar;
            szBuffer[2] = '\0';
            pAnalyzer->pCurrent += 2;
            return assignToken(pAnalyzer, TOKEN_OPERATOR, szBuffer, pSourceStart);
        } else {
            szBuffer[0] = firstChar;
            szBuffer[1] = '\0';
            pAnalyzer->pCurrent++;
            return assignToken(pAnalyzer, TOKEN_OPERATOR, szBuffer, pSourceStart);
        }
    case '<':
        secondChar = *(pAnalyzer->pCurrent + 1);
        if (secondChar == '=') {
            szBuffer[0] = firstChar;
            szBuffer[1] = secondChar;
            szBuffer[2] = '\0';
            pAnalyzer->pCurrent += 2;
            return assignToken(pAnalyzer, TOKEN_OPERATOR, szBuffer, pSourceStart);
        } else if (secondChar == '>') {
            szBuffer[0] = firstChar;
            szBuffer[1] = secondChar;
            szBuffer[2] = '\0';
            pAnalyzer->pCurrent += 2;
            return assignToken(pAnalyzer, TOKEN_OPERATOR, szBuffer, pSourceStart);
        } else {
            szBuffer[0] = firstChar;
            szBuffer[1] = '\0';
            pAnalyzer->pCurrent++;
            return assignToken(pAnalyzer, TOKEN_OPERATOR, szBuffer, pSourceStart);
        }
    case '~':
        secondChar = *(pAnalyzer->pCurrent + 1);
        if (secondChar == '=') {
            szBuffer[0] = firstChar;
            szBuffer[1] = secondChar;
            szBuffer[2] = '\0';
            pAnalyzer->pCurrent += 2;
            return assignToken(pAnalyzer, TOKEN_OPERATOR, szBuffer, pSourceStart);
        } else {
            szBuffer[0] = firstChar;
            szBuffer[1] = '\0';
            pAnalyzer->pCurrent++;
            return assignToken(pAnalyzer, TOKEN_UNDEFINED, szBuffer, pSourceStart);
        }
    case '&':
        secondChar = *(pAnalyzer->pCurrent + 1);
        if (secondChar == '&') {
            szBuffer[0] = firstChar;
            szBuffer[1] = secondChar;
            szBuffer[2] = '\0';
            pAnalyzer->pCurrent += 2;
            return assignToken(pAnalyzer, TOKEN_OPERATOR, szBuffer, pSourceStart);
        } else {
            szBuffer[0] = firstChar;
            szBuffer[1] = '\0';
            pAnalyzer->pCurrent++;
            return assignToken(pAnalyzer, TOKEN_OPERATOR, szBuffer, pSourceStart);
        }
    case '|':
        secondChar = *(pAnalyzer->pCurrent + 1);
        if (secondChar == '|') {
            szBuffer[0] = firstChar;
            szBuffer[1] = secondChar;
            szBuffer[2] = '\0';
            pAnalyzer->pCurrent += 2;
            return assignToken(pAnalyzer, TOKEN_OPERATOR, szBuffer, pSourceStart);
        } else {
            szBuffer[0] = firstChar;
            szBuffer[1] = '\0';
            pAnalyzer->pCurrent++;
            return assignToken(pAnalyzer, TOKEN_UNDEFINED, szBuffer, pSourceStart);
        }
    }

    /* 行结束 */
    if (firstChar == '\0') {
        return assignToken(pAnalyzer, TOKEN_LINE_END, "", pSourceStart);
    }
    /* 数字 */
    else if (isDigit(firstChar)) {
        while (isDigit(*pAnalyzer->pCurrent)) {
            *pBuffer++ = *pAnalyzer->pCurrent++;
        }
        /* 检查是不是带有小数 */
        if (*pAnalyzer->pCurrent == '.') {
            /* 保存小数点 */
            *pBuffer++ = *pAnalyzer->pCurrent++;
            /* 继续写入数字 */
            while (isDigit(*pAnalyzer->pCurrent)) {
                *pBuffer++ = *pAnalyzer->pCurrent++;
            }
        }

        *pBuffer = '\0';
        /* 过长 */
        /* if ((pBuffer - szBuffer) >= KB_CONTEXT_STRING_BUFFER_MAX) {
            assignToken(pAnalyzer, TOKEN_ERR, "Number too long", pSourceStart);
        } */

        return assignToken(pAnalyzer, TOKEN_NUMERIC, szBuffer, pSourceStart);
    }
    /* 标志符 */
    else if (isAlpha(firstChar)) {
        int iKeywordId;

        while (isAlphaNum(*pAnalyzer->pCurrent)) {
            *pBuffer++ = *pAnalyzer->pCurrent++;
        }
        *pBuffer = '\0';

        /* 检查是否是关键字 */
        iKeywordId = getKeywordIdFromString(szBuffer);
        if (iKeywordId != KBKID_NONE) {
            return assignToken(pAnalyzer, TOKEN_KEYWORD, szBuffer, pSourceStart);
        }

        /* token 过长 */
        /* if ((pBuffer - szBuffer) >= KB_CONTEXT_STRING_BUFFER_MAX) {
            assignToken(pAnalyzer, TOKEN_ERR, "Identifier too long", pSourceStart);
        } */

        return assignToken(pAnalyzer, TOKEN_IDENTIFIER, szBuffer, pSourceStart);
    }
    /* 字符串 */
    else if (firstChar == '"') {
        /* 跳过双引号 */
        pAnalyzer->pCurrent++;

        while (*pAnalyzer->pCurrent != '"') {
            const char currentChar = *pAnalyzer->pCurrent;
            /* 处理转义字符 */
            if (currentChar == '\\') {
                const char nextChar = *++pAnalyzer->pCurrent;
                /* Line feed */
                if (nextChar == 'n') {
                    *pBuffer++ = '\n';
                }
                /* Carriage return */
                else if (nextChar == 'r') {
                    *pBuffer++ = '\r';
                }
                /* Tab */
                else if (nextChar == 't') {
                    *pBuffer++ = '\t';
                }
                /* Escaped quote */
                else if (nextChar == '"') {
                    *pBuffer++ = '\"';
                }
                /* 十六进制数 */
                else if (nextChar == 'x') {
                    int hexDigitCount = 0;
                    int hexValue = 0;
                    while (hexDigitCount <= 2) {
                        char hexChar = *++pAnalyzer->pCurrent;
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
                            /* Not a hexadecimal digit, backtrack one position */
                            --pAnalyzer->pCurrent;
                            break;
                        }
                        ++hexDigitCount;
                    }
                    if (hexDigitCount <= 0) {
                        assignToken(pAnalyzer, TOKEN_ERR, "Invalid hex escape char", pSourceStart);
                    }
                    *pBuffer++ = hexValue;
                }
                /* 非法转义字符 */
                else {
                    assignToken(pAnalyzer, TOKEN_ERR, "Invalid escape char", pSourceStart);
                }
                pAnalyzer->pCurrent++;
            }
            /* 不完整的字符串 */
            else if (currentChar == '\0') {
                assignToken(pAnalyzer, TOKEN_ERR, "Incomplete string", pSourceStart);
            }
            else {
                *pBuffer++ = *pAnalyzer->pCurrent++;
            }
        }
        
        /* 跳过双引号 */
        pAnalyzer->pCurrent++;

        /* String too long */
        /* if ((pBuffer - szBuffer) >= KB_CONTEXT_STRING_BUFFER_MAX) {
            assignToken(pAnalyzer, TOKEN_ERR, "String too long", pSourceStart);
        } */

        *pBuffer = '\0';

        return assignToken(pAnalyzer, TOKEN_STRING, szBuffer, pSourceStart);
    }
    /* 未知字符 */
    else {
        pAnalyzer->pCurrent++;
        szBuffer[0] = firstChar;
        szBuffer[1] = '\0';
        return assignToken(pAnalyzer, TOKEN_UNDEFINED, szBuffer, pSourceStart);
    }
}

void KAnalyzer_RewindToken(KbLineAnalyzer* pAnalyzer) {
    pAnalyzer->pCurrent -= pAnalyzer->token.iSourceLength;
}

void KAnalyzer_ResetToken(KbLineAnalyzer* pAnalyzer) {
    pAnalyzer->pCurrent = pAnalyzer->szLine;
}

const char* KAnalyzer_GetCurrentPtr(KbLineAnalyzer* pAnalyzer) {
    return pAnalyzer->pCurrent;
}

void KAnalyzer_SetCurrentPtr(KbLineAnalyzer* pAnalyzer, const char* pCurrent) {
    pAnalyzer->pCurrent = pCurrent;
}

void KAnalyzer_Initialize(KbLineAnalyzer* pAnalyzer, const char* szLineSource) {
    pAnalyzer->szLine = szLineSource;
    resetToken(pAnalyzer);
}

const char* KToken_GetTypeName(int iTokenType) {
    static const char* TokenName[] = {
        "Error",        "LineEnd",      "Numeric",
        "Identifier",   "Operator",     "ParenLeft",
        "ParenRight",   "BracketLeft",  "BracketRight",
        "Comma",        "String",       "LabelSign",
        "Keyword",      "Undefined"
    };

    if (iTokenType < TOKEN_ERR || iTokenType > TOKEN_UNDEFINED) {
        return "N/A";
    }

    return TokenName[iTokenType - TOKEN_ERR];
}