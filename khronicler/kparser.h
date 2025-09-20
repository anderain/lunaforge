#ifndef _KPARSER_H_
#define _KPARSER_H_

#include "kutils.h"

typedef enum tagStatementId {
    STATEMENT_NONE = 0,
    STATEMENT_FUNC,
    STATEMENT_IF,
    STATEMENT_IF_GOTO,
    STATEMENT_ELSEIF,
    STATEMENT_ELSE,
    STATEMENT_WHILE,
    STATEMENT_DO_WHILE,
    STATEMENT_FOR,
    STATEMENT_NEXT,
    STATEMENT_BREAK,
    STATEMENT_CONTINUE,
    STATEMENT_END,
    STATEMENT_EXIT,
    STATEMENT_RETURN,
    STATEMENT_GOTO,
    STATEMENT_DIM,
    STATEMENT_DIM_ARRAY,
    STATEMENT_REDIM,
    STATEMENT_LABEL,
    STATEMENT_ASSIGN,
    STATEMENT_ASSIGN_ARRAY,
    STATEMENT_EXPR
} StatementId;

typedef enum tagAstNodeType {
    AST_EMPTY = 0,
    AST_PROGRAM,
    AST_FUNCTION_DECLARE,
    AST_IF_GOTO,
    AST_IF,
    AST_THEN,
    AST_ELSEIF,
    AST_ELSE,
    AST_WHILE,
    AST_DO_WHILE,
    AST_FOR,
    AST_BREAK,
    AST_CONTINUE,
    AST_EXIT,
    AST_RETURN,
    AST_GOTO,
    AST_DIM,
    AST_DIM_ARRAY,
    AST_REDIM,
    AST_ASSIGN,
    AST_ASSIGN_ARRAY,
    AST_LABEL_DECLARE,
    AST_UNARY_OPERATOR,
    AST_BINARY_OPERATOR,
    AST_PAREN,
    AST_LITERAL_NUMERIC,
    AST_LITERAL_STRING,
    AST_VARIABLE,
    AST_ARRAY_ACCESS,
    AST_FUNCTION_CALL
} AstNodeType;

typedef struct tagKbAstFuncParam {
    VarDeclTypeId   iType;
    char            szName[1];
} KbAstFuncParam;

typedef struct tagAstNode {
    int                 iType;
    int                 iControlId;
    int                 iLineNumber;
    struct tagAstNode*  pAstParent;
    union {
        struct {
            Vlist* pListStatements; /* <AstNode> */
            int iNumControl;
        } sProgram;
        struct {
            char* szFunction;
            Vlist* pListParameters; /* <AstFuncParam> */
            Vlist* pListStatements; /* <AstNode> */
        } sFunctionDeclare;
        struct {
            char* szLabelName;
            struct tagAstNode* pAstCondition;
        } sIfGoto;
        struct {
            struct tagAstNode* pAstCondition;
            struct tagAstNode* pAstThen;
            Vlist* pListElseIf; /* <AstNode> */
            struct tagAstNode* pAstElse;
        } sIf;
        struct {
            Vlist* pListStatements; /* <AstNode> */
        } sThen;
        struct {
            struct tagAstNode* pAstCondition;
            Vlist* pListStatements; /* <AstNode> */
        } sElseIf;
        struct {
            Vlist* pListStatements; /* <AstNode> */
        } sElse;
        struct {
            struct tagAstNode* pAstCondition;
            Vlist* pListStatements; /* <AstNode> */
        } sWhile;
        struct {
            struct tagAstNode* pAstCondition;
            Vlist* pListStatements; /* <AstNode> */
        } sDoWhile;
        struct {
            char* szVariable;
            struct tagAstNode* pAstRangeFrom;
            struct tagAstNode* pAstRangeTo;
            struct tagAstNode* pAstStep;
            Vlist* pListStatements; /* <AstNode> */
        } sFor;
        struct {
            struct tagAstNode* pAstExpression;
        } sExit;
        struct {
            struct tagAstNode* pAstExpression;
        } sReturn;
        struct {
            char* szLabelName;
        } sGoto;
        struct {
            char* szVariable;
            struct tagAstNode* pAstInitializer;
        } sDim;
        struct {
            char* szArrayName;
            struct tagAstNode* pAstDimension;
        } sDimArray;
        struct {
            char* szArrayName;
            struct tagAstNode* pAstDimension;
        } sRedim;
        struct {
            char* szVariable;
            struct tagAstNode* pAstValue;
        } sAssign;
        struct {
            char* szArrayName;
            struct tagAstNode* pAstSubscript;
            struct tagAstNode* pAstValue;
        } sAssignArray;
        struct {
            char* szLabelName;
        } sLabel;
        struct {
            int iOperatorId;
            struct tagAstNode* pAstOperand;
        } sUnaryOperator;
        struct {
            struct tagAstNode* pAstExpr;
        } sParen;
        struct {
            int iOperatorId;
            struct tagAstNode* pAstLeftOperand;
            struct tagAstNode* pAstRightOperand;
        } sBinaryOperator;
        struct {
            KFloat fValue;
        } sLiteralNumeric;
        struct {
            char* szValue;
        } sLiteralString;
        struct {
            char *szName;
        } sVariable;
        struct {
            char* szArrayName;
            struct tagAstNode* pAstSubscript;
        } sArrayAccess;
        struct {
            char* szFunction;
            Vlist* pListArguments; /* <AstNode> */
        } sFunctionCall;
    } uData;
} KbAstNode;

typedef enum TagSyntaxErrorId {
    SYN_NO_ERROR = 0,
    SYN_EXPECT_LINE_END,
    SYN_FUNC_MISSING_NAME,
    SYN_FUNC_MISSING_LEFT_PAREN,
    SYN_FUNC_INVALID_PARAMETERS,
    SYN_FUNC_NESTED,
    SYN_IF_GOTO_MISSING_LABEL,
    SYN_ELSEIF_NOT_MATCH,
    SYN_ELSE_NOT_MATCH,
    SYN_FOR_MISSING_VARIABLE,
    SYN_FOR_MISSING_EQUAL,
    SYN_FOR_MISSING_TO,
    SYN_FOR_VAR_MISMATCH,
    SYN_NEXT_NOT_MATCH,
    SYN_BREAK_OUTSIDE_LOOP,
    SYN_CONTINUE_OUTSIDE_LOOP,
    SYN_END_KEYWORD_NOT_MATCH,
    SYN_END_KEYWORD_INVALID,
    SYN_RETURN_OUTSIDE_FUNC,
    SYN_GOTO_MISSING_LABEL,
    SYN_DIM_MISSING_VARIABLE,
    SYN_DIM_INVALID,
    SYN_DIM_ARRAY_MISSING_BRACKET_R,
    SYN_REDIM_MISSING_VARIABLE,
    SYN_REDIM_MISSING_BRACKET_L,
    SYN_REDIM_MISSING_BRACKET_R,
    SYN_EXPR_INVALID,
    SYN_UNTERMINATED_FUNC_OR_CTRL,
    SYN_UNRECOGNIZED
} SyntaxErrorId;

typedef struct tagKbSourceParser {
    int         iLineNumber;
    int         iControlCounter;
    const char* szSource;
    const char* pSourceCurrent;
    int         iErrorId;
    KbAstNode*  pAstCurrent;
} KbSourceParser;

const char* KSyntaxError_GetMessageById (SyntaxErrorId iSyntaxErrorId);
const char* KSyntaxError_GetNameById    (SyntaxErrorId iSyntaxErrorId);
const char* KStatement_GetNameById      (StatementId iStatement);
const char* KAst_GetNameById            (AstNodeType iType);
void        KAstNode_Destroy            (KbAstNode* pAstNode);
KbAstNode*  KAstNode_Create             (AstNodeType iAstType, KbAstNode* pAstParent);
KbAstNode*  KSourceParser_Parse         (const char* szSource, SyntaxErrorId* pIntSyntaxErrorId, StatementId* pIntStopStatement, int* pIntStopLineNumber);

#endif