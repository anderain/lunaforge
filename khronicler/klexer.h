#ifndef _KLEXER_H_
#define _KLEXER_H_

#include "kommon.h"

#define KB_KEYWORD_DIM      "dim"
#define KB_KEYWORD_REDIM    "redim"
#define KB_KEYWORD_GOTO     "goto"
#define KB_KEYWORD_IF       "if"
#define KB_KEYWORD_ELSEIF   "elseif"
#define KB_KEYWORD_ELSE     "else"
#define KB_KEYWORD_WHILE    "while"
#define KB_KEYWORD_DO       "do"
#define KB_KEYWORD_FOR      "for"
#define KB_KEYWORD_TO       "to"
#define KB_KEYWORD_STEP     "step"
#define KB_KEYWORD_NEXT     "next"
#define KB_KEYWORD_CONTINUE "continue"
#define KB_KEYWORD_BREAK    "break"
#define KB_KEYWORD_END      "end"
#define KB_KEYWORD_RETURN   "return"
#define KB_KEYWORD_FUNC     "func"
#define KB_KEYWORD_EXIT     "exit"

typedef enum tagKbKeywordIdType {
    KBKID_NONE = 0,
    KBKID_DIM, KBKID_REDIM,
    KBKID_GOTO,
    KBKID_IF, KBKID_ELSEIF, KBKID_ELSE,
    KBKID_WHILE, KBKID_DO, KBKID_FOR, KBKID_TO, KBKID_STEP, KBKID_NEXT, KBKID_CONTINUE, KBKID_BREAK,
    KBKID_END,
    KBKID_RETURN, KBKID_FUNC,
    KBKID_EXIT
} KbKeywordIdType;

typedef enum tagKbTokenType {
    TOKEN_ERR,          TOKEN_LINE_END,     TOKEN_NUMERIC,
    TOKEN_IDENTIFIER,   TOKEN_OPERATOR,     TOKEN_PAREN_L,
    TOKEN_PAREN_R,      TOKEN_BRACKET_L,    TOKEN_BRACKET_R,
    TOKEN_COMMA,        TOKEN_STRING,       TOKEN_LABEL_SIGN,
    TOKEN_KEYWORD,      TOKEN_UNDEFINED
} KbTokenType;

typedef struct tagKbToken {
    KbTokenType iType;
    int         iSourceStart;
    int         iSourceLength;
    char        szContent[KB_PARSER_LINE_MAX];
} KbToken;

typedef struct tagKbLineAnalyzer {
    const char* szLine;
    const char* pCurrent;
    KbToken     token;
} KbLineAnalyzer;

void        KAnalyzer_NextToken         (KbLineAnalyzer *pAnalyzer);
void        KAnalyzer_RewindToken       (KbLineAnalyzer* pAnalyzer);
void        KAnalyzer_ResetToken        (KbLineAnalyzer* pAnalyzer);
void        KAnalyzer_Initialize        (KbLineAnalyzer* pAnalyzer, const char* szLineSource);
const char* KAnalyzer_GetCurrentPtr     (KbLineAnalyzer* pAnalyzer);
void        KAnalyzer_SetCurrentPtr     (KbLineAnalyzer* pAnalyzer, const char* pCurrent);
const char* KToken_GetTypeName          (int iTokenType);

#endif