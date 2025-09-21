#include <stdlib.h>
#include <string.h>
#include "klexer.h"
#include "kparser.h"
#include "kalias.h"

const struct {
    const char* szOperator;
    OperatorId  iOperatorId;
} BinaryOperatorSzIdMap[] = {
    { "&",  OPR_CONCAT      },
    { "+",  OPR_ADD         },
    { "-",  OPR_SUB         },
    { "*",  OPR_MUL         },
    { "/",  OPR_DIV         },
    { "^",  OPR_POW         },
    { "\\", OPR_INTDIV      },
    { "%",  OPR_MOD         },
    { "!",  OPR_NOT         },
    { "&&", OPR_AND         },
    { "||", OPR_OR          },
    { "=",  OPR_EQUAL       },
    { "~=", OPR_APPROX_EQ   },
    { "<>", OPR_NEQ         },
    { ">",  OPR_GT          },
    { "<",  OPR_LT          },
    { ">=", OPR_GTEQ        },
    { "<=", OPR_LTEQ        }
};

static OperatorId getOperatorId(const char* szOperator) {
    static const int size = sizeof(BinaryOperatorSzIdMap) / sizeof(BinaryOperatorSzIdMap[0]);
    int i;
    for (i = 0; i < size; ++i) {
        if (IsStringEqual(BinaryOperatorSzIdMap[i].szOperator, szOperator)) {
            return BinaryOperatorSzIdMap[i].iOperatorId;
        }
    }
    return OPR_NONE;
}

const char* KStatement_GetNameById(StatementId iStatement) {
    static const char* SZ_STATEMENT_NAME[] = {
        "none",
        "function declare",
        "if",
        "if...goto",
        "elseif",
        "else",
        "while",
        "do...while",
        "for",
        "next",
        "break",
        "continue",
        "end",
        "exit",
        "return",
        "goto",
        "dim",
        "dim array",
        "redim",
        "label declare",
        "assign",
        "assign array",
        "expression"
    };
    return SZ_STATEMENT_NAME[iStatement];
}

const char* KAst_GetNameById(AstNodeType iType) {
    static const char* SZ_AST_NAME[] = {
        "Empty",
        "Program",
        "FunctionDeclare",
        "IfGoto",
        "If",
        "Then",
        "ElseIf",
        "Else",
        "While",
        "DoWhile",
        "For",
        "Break",
        "Continue",
        "Exit",
        "Return",
        "Goto",
        "Dim",
        "DimArray",
        "Redim",
        "Assign",
        "AssignArray",
        "LabelDeclare",
        "UnaryOperator",
        "BinaryOperator",
        "Paren",
        "LiteralNumeric",
        "LiteralString",
        "Variable",
        "ArrayAccess",
        "FunctionCall"
    };
    return SZ_AST_NAME[iType];
}

static const struct {
    char* szName;
    char* szMessage;
} SYNTAX_ERROR_DETAIL[] = {
    { "SYN_NO_ERROR",                       "No syntax error." },
    { "SYN_EXPECT_LINE_END",                "Expected end of line but found extra tokens." },
    { "SYN_FUNC_MISSING_NAME",              "Function definition missing function name." },
    { "SYN_FUNC_MISSING_LEFT_PAREN",        "Function definition missing left parenthesis." },
    { "SYN_FUNC_INVALID_PARAMETERS",        "Invalid function parameter list." },
    { "SYN_FUNC_NESTED",                    "Nested function definitions are not allowed." },
    { "SYN_IF_GOTO_MISSING_LABEL",          "'if ... goto' statement missing target label." },
    { "SYN_ELSEIF_NOT_MATCH",               "'elseif' without matching 'if'." },
    { "SYN_ELSE_NOT_MATCH",                 "'else' without matching 'if'." },
    { "SYN_FOR_MISSING_VARIABLE",           "'for' statement missing loop variable." },
    { "SYN_FOR_MISSING_EQUAL",              "'for' statement missing '=' after variable." },
    { "SYN_FOR_MISSING_TO",                 "'for' statement missing 'to' keyword." },
    { "SYN_FOR_VAR_MISMATCH",               "'for' loop variable mismatch" },
    { "SYN_NEXT_NOT_MATCH",                 "'next' without matching 'for'." },
    { "SYN_BREAK_OUTSIDE_LOOP",             "'break' used outside of loop." },
    { "SYN_CONTINUE_OUTSIDE_LOOP",          "'continue' used outside of loop." },
    { "SYN_END_KEYWORD_NOT_MATCH",          "'end' does not match the nearest block." },
    { "SYN_END_KEYWORD_INVALID",            "'end' must be followed by 'if', WHILE, or FUNC." },
    { "SYN_RETURN_OUTSIDE_FUNC",            "'return' used outside of function." },
    { "SYN_GOTO_MISSING_LABEL",             "'goto' statement missing target label." },
    { "SYN_DIM_MISSING_VARIABLE",           "'dim' statement missing variable name." },
    { "SYN_DIM_INVALID",                    "Invalid 'dim' syntax." },
    { "SYN_DIM_ARRAY_MISSING_BRACKET_R",    "'dim' array missing right bracket." },
    { "SYN_REDIM_MISSING_VARIABLE",         "'redim' statement missing variable name." },
    { "SYN_REDIM_MISSING_BRACKET_L",        "'redim' statement missing left bracket." },
    { "SYN_REDIM_MISSING_BRACKET_R",        "'redim' statement missing right bracket." },
    { "SYN_EXPR_INVALID",                   "Invalid expression syntax." },
    { "SYN_UNTERMINATED_FUNC_OR_CTRL",      "Program ended but a function or control structure was not closed." },
    { "SYN_UNRECOGNIZED",                   "Unrecognized statement." }
};

const char* KSyntaxError_GetMessageById(SyntaxErrorId iSyntaxErrorId) {
    if (iSyntaxErrorId < 0 || iSyntaxErrorId > SYN_UNRECOGNIZED) return "N/A";
    return SYNTAX_ERROR_DETAIL[iSyntaxErrorId].szMessage;
}

const char* KSyntaxError_GetNameById(SyntaxErrorId iSyntaxErrorId) {
    if (iSyntaxErrorId < 0 || iSyntaxErrorId > SYN_UNRECOGNIZED) return "N/A";
    return SYNTAX_ERROR_DETAIL[iSyntaxErrorId].szName;
}

static AstNode* createAst(AstNodeType iAstType, KbAstNode* pAstParent) {
    AstNode* pAstNode = (AstNode *)malloc(sizeof(AstNode));
    
    pAstNode->pAstParent    = pAstParent;
    pAstNode->iType         = iAstType;
    pAstNode->iLineNumber   = -1;
    pAstNode->iControlId    = 0;

    switch (iAstType) {
        default:
        case AST_EMPTY:
            break;
        case AST_PROGRAM:
            pAstNode->uData.sProgram.pListStatements = vlNewList();
            break;
        case AST_FUNCTION_DECLARE:
            pAstNode->uData.sFunctionDeclare.szFunction = NULL;
            pAstNode->uData.sFunctionDeclare.pListParameters = vlNewList(); 
            pAstNode->uData.sFunctionDeclare.pListStatements = vlNewList();
            break;
        case AST_IF_GOTO:
            pAstNode->uData.sIfGoto.szLabelName = NULL;
            pAstNode->uData.sIfGoto.pAstCondition = NULL;
            break;
        case AST_IF:
            pAstNode->uData.sIf.pAstCondition = NULL;
            pAstNode->uData.sIf.pAstThen = createAst(AST_THEN, pAstNode);
            pAstNode->uData.sIf.pListElseIf = vlNewList();
            pAstNode->uData.sIf.pAstElse = NULL;
            break;
        case AST_THEN:
            pAstNode->uData.sThen.pListStatements = vlNewList();
            break;
        case AST_ELSEIF:
            pAstNode->uData.sElseIf.pAstCondition = NULL;
            pAstNode->uData.sElseIf.pListStatements = vlNewList();
            break;
        case AST_ELSE:
            pAstNode->uData.sElse.pListStatements = vlNewList();
            break;
        case AST_WHILE:
            pAstNode->uData.sWhile.pAstCondition = NULL;
            pAstNode->uData.sWhile.pListStatements = vlNewList();
            break;
        case AST_DO_WHILE:
            pAstNode->uData.sDoWhile.pAstCondition = NULL;
            pAstNode->uData.sDoWhile.pListStatements = vlNewList();
            break;
        case AST_FOR:
            pAstNode->uData.sFor.szVariable = NULL;
            pAstNode->uData.sFor.pAstRangeFrom = NULL;
            pAstNode->uData.sFor.pAstRangeTo = NULL;
            pAstNode->uData.sFor.pAstStep = NULL;
            pAstNode->uData.sFor.pListStatements = vlNewList();
            break;
        case AST_BREAK:
            break;
        case AST_CONTINUE:
            break;
        case AST_EXIT:
            pAstNode->uData.sExit.pAstExpression = NULL;
            break;
        case AST_RETURN:
            pAstNode->uData.sReturn.pAstExpression = NULL;
            break;
        case AST_GOTO:
            pAstNode->uData.sGoto.szLabelName = NULL;
            break;
        case AST_DIM:
            pAstNode->uData.sDim.szVariable = NULL;
            pAstNode->uData.sDim.pAstInitializer = NULL;
            break;
        case AST_DIM_ARRAY:
            pAstNode->uData.sDimArray.szArrayName = NULL;
            pAstNode->uData.sDimArray.pAstDimension = NULL;
            break;
        case AST_REDIM:
            pAstNode->uData.sRedim.szArrayName = NULL;
            pAstNode->uData.sRedim.pAstDimension = NULL;
            break;
        case AST_ASSIGN:
            pAstNode->uData.sAssign.szVariable = NULL;
            pAstNode->uData.sAssign.pAstValue = NULL;
            break;
        case AST_ASSIGN_ARRAY:
            pAstNode->uData.sAssignArray.szArrayName = NULL;
            pAstNode->uData.sAssignArray.pAstSubscript = NULL;
            pAstNode->uData.sAssignArray.pAstValue = NULL;
            break;
        case AST_LABEL_DECLARE:
            pAstNode->uData.sLabel.szLabelName = NULL;
            break;
        case AST_UNARY_OPERATOR:
            pAstNode->uData.sUnaryOperator.iOperatorId = OPR_NONE;
            pAstNode->uData.sUnaryOperator.pAstOperand = NULL;
            break;
        case AST_BINARY_OPERATOR:
            pAstNode->uData.sBinaryOperator.iOperatorId = OPR_NONE;
            pAstNode->uData.sBinaryOperator.pAstLeftOperand = NULL;
            pAstNode->uData.sBinaryOperator.pAstRightOperand = NULL;
            break;
        case AST_PAREN:
            pAstNode->uData.sParen.pAstExpr = NULL;
            break;
        case AST_LITERAL_NUMERIC:
            pAstNode->uData.sLiteralNumeric.fValue = 0;
            break;
        case AST_LITERAL_STRING:
            pAstNode->uData.sLiteralString.szValue = NULL;
            break;
        case AST_VARIABLE:
            pAstNode->uData.sVariable.szName = NULL;
            break;
        case AST_ARRAY_ACCESS:
            pAstNode->uData.sArrayAccess.szArrayName = NULL;
            pAstNode->uData.sArrayAccess.pAstSubscript = NULL;
            break;
        case AST_FUNCTION_CALL:
            pAstNode->uData.sFunctionCall.szFunction = NULL;
            pAstNode->uData.sFunctionCall.pListArguments = vlNewList();
            break;
    }

    return pAstNode;
}

static AstNode* createAstWithLineNumber(AstNodeType iAstType, KbAstNode* pAstParent, int iLineNumber) {
    AstNode* pAstNode = createAst(iAstType, pAstParent);
    pAstNode->iLineNumber = iLineNumber;
    return pAstNode;
}

static void destroyAstVoidPtr(void* pAstNodeVoidPtr) {
    destroyAst((AstNode *)pAstNodeVoidPtr);
}

static void checkNullAndFreeString(char* pString) {
    if (!pString) return;
    free(pString);
}

void KAstNode_Destroy(KbAstNode* pAstNode) {
    if (pAstNode == NULL) {
        return;
    }

    switch (pAstNode->iType) {
        default:
        case AST_EMPTY:
            break;
        case AST_PROGRAM:
            vlDestroy(pAstNode->uData.sProgram.pListStatements, destroyAstVoidPtr);
            break;
        case AST_FUNCTION_DECLARE:
            checkNullAndFreeString(pAstNode->uData.sFunctionDeclare.szFunction);
            vlDestroy(pAstNode->uData.sFunctionDeclare.pListParameters, free); 
            vlDestroy(pAstNode->uData.sFunctionDeclare.pListStatements, destroyAstVoidPtr);
            break;
        case AST_IF_GOTO:
            checkNullAndFreeString(pAstNode->uData.sIfGoto.szLabelName);
            destroyAst(pAstNode->uData.sIfGoto.pAstCondition);
            break;
        case AST_IF:
            destroyAst(pAstNode->uData.sIf.pAstCondition);
            destroyAst(pAstNode->uData.sIf.pAstThen);
            vlDestroy(pAstNode->uData.sIf.pListElseIf, destroyAstVoidPtr);
            destroyAst(pAstNode->uData.sIf.pAstElse);
            break;
        case AST_THEN:
            vlDestroy(pAstNode->uData.sThen.pListStatements, destroyAstVoidPtr);
            break;
        case AST_ELSEIF:
            destroyAst(pAstNode->uData.sElseIf.pAstCondition);
            vlDestroy(pAstNode->uData.sElseIf.pListStatements, destroyAstVoidPtr);
            break;
        case AST_ELSE:
            vlDestroy(pAstNode->uData.sElse.pListStatements, destroyAstVoidPtr);
            break;
        case AST_WHILE:
            destroyAst(pAstNode->uData.sWhile.pAstCondition);
            vlDestroy(pAstNode->uData.sWhile.pListStatements, destroyAstVoidPtr);
            break;
        case AST_DO_WHILE:
            destroyAst(pAstNode->uData.sDoWhile.pAstCondition);
            vlDestroy(pAstNode->uData.sDoWhile.pListStatements, destroyAstVoidPtr);
            break;
        case AST_FOR:
            checkNullAndFreeString(pAstNode->uData.sFor.szVariable);
            destroyAst(pAstNode->uData.sFor.pAstRangeFrom);
            destroyAst(pAstNode->uData.sFor.pAstRangeTo);
            destroyAst(pAstNode->uData.sFor.pAstStep);
            vlDestroy(pAstNode->uData.sFor.pListStatements, destroyAstVoidPtr);
            break;
        case AST_BREAK:
            break;
        case AST_CONTINUE:
            break;
        case AST_EXIT:
            destroyAst(pAstNode->uData.sExit.pAstExpression);
            break;
        case AST_RETURN:
            destroyAst(pAstNode->uData.sReturn.pAstExpression);
            break;
        case AST_GOTO:
            checkNullAndFreeString(pAstNode->uData.sGoto.szLabelName);
            break;
        case AST_DIM:
            checkNullAndFreeString(pAstNode->uData.sDim.szVariable);
            destroyAst(pAstNode->uData.sDim.pAstInitializer);
            break;
        case AST_DIM_ARRAY:
            checkNullAndFreeString(pAstNode->uData.sDimArray.szArrayName);
            destroyAst(pAstNode->uData.sDimArray.pAstDimension);
            break;
        case AST_REDIM:
            checkNullAndFreeString(pAstNode->uData.sRedim.szArrayName);
            destroyAst(pAstNode->uData.sRedim.pAstDimension);
            break;
        case AST_ASSIGN:
            checkNullAndFreeString(pAstNode->uData.sAssign.szVariable);
            destroyAst(pAstNode->uData.sAssign.pAstValue);
            break;
        case AST_ASSIGN_ARRAY:
            checkNullAndFreeString(pAstNode->uData.sAssignArray.szArrayName);
            destroyAst(pAstNode->uData.sAssignArray.pAstSubscript);
            destroyAst(pAstNode->uData.sAssignArray.pAstValue);
            break;
        case AST_LABEL_DECLARE:
            checkNullAndFreeString(pAstNode->uData.sLabel.szLabelName);
            break;
        case AST_UNARY_OPERATOR:
            destroyAst(pAstNode->uData.sUnaryOperator.pAstOperand);
            break;
        case AST_BINARY_OPERATOR:
            destroyAst(pAstNode->uData.sBinaryOperator.pAstLeftOperand);
            destroyAst(pAstNode->uData.sBinaryOperator.pAstRightOperand);
            break;
        case AST_PAREN:
            destroyAst(pAstNode->uData.sParen.pAstExpr);
            break;
        case AST_LITERAL_NUMERIC:
            break;
        case AST_LITERAL_STRING:
            checkNullAndFreeString(pAstNode->uData.sLiteralString.szValue);
            break;
        case AST_VARIABLE:
            checkNullAndFreeString(pAstNode->uData.sVariable.szName);
            break;
        case AST_ARRAY_ACCESS:
            checkNullAndFreeString(pAstNode->uData.sArrayAccess.szArrayName);
            destroyAst(pAstNode->uData.sArrayAccess.pAstSubscript);
            break;
        case AST_FUNCTION_CALL:
            checkNullAndFreeString(pAstNode->uData.sFunctionCall.szFunction);
            vlDestroy(pAstNode->uData.sFunctionCall.pListArguments, destroyAstVoidPtr);
            break;
    }

    free(pAstNode);
}

static void initParser(Parser* pParser, const char* szSource) {
    pParser->iErrorId           = SYN_NO_ERROR;
    pParser->iControlCounter    = 0;
    pParser->iLineNumber        = 0;
    pParser->szSource           = szSource;
    pParser->pSourceCurrent     = szSource;
    pParser->pAstCurrent        = NULL;
}

static int fetchLine(const char* pSource, char* szLineBuf) {
    char *pWriter = szLineBuf;
    const char* pReader = pSource;

    /* 从缓存中获取一行 */
    do {
        *pWriter++ = *pReader++;
    } while(*pReader != '\n' && *pReader != '\0');

    /* 如果是换行，也读取进来 */
    if (*pReader == '\n') *pWriter++ = *pReader++;

    *pWriter-- = '\0';

    /* 移除末尾的空白 */
    while (isSpace(*pWriter)) *pWriter-- = '\0';

    /* 返回读取了多少字符 */
    return pReader - pSource;
}

#define tokenIs(szString)           (IsStringEqual((szString), pToken->szContent))
#define tokenTypeIs(iTokenType)     (pToken->iType == (iTokenType))
#define tokenIsKeyword(szKeyword)   (pToken->iType == TOKEN_KEYWORD && IsStringEqual((szKeyword), pToken->szContent))

static KBool matchExpr(Analyzer* pAnalyzer);

static KBool matchTryBinaryOperator(Analyzer *pAnalyzer) {
    Token* pToken = &pAnalyzer->token;
    nextToken(pAnalyzer);
    if (tokenTypeIs(TOKEN_OPERATOR)) {
        /* 非双目操作符，错误 */
        if (tokenIs("!")) {
            return KB_FALSE;
        }
        return matchExpr(pAnalyzer);
    }
    rewindToken(pAnalyzer);
    return KB_TRUE;
}

static KBool matchExpr(Analyzer* pAnalyzer) {
    Token* pToken = &pAnalyzer->token;
    nextToken(pAnalyzer);
    /* 字面值 */
    if (tokenTypeIs(TOKEN_NUMERIC) || tokenTypeIs(TOKEN_STRING)) {
        return matchTryBinaryOperator(pAnalyzer);
    }
    /* 单目操作符 */
    else if (tokenTypeIs(TOKEN_OPERATOR) && (tokenIs("!") || tokenIs("-"))) {
        return matchExpr(pAnalyzer) && matchTryBinaryOperator(pAnalyzer);
    }
    /* 标志符 */
    else if (tokenTypeIs(TOKEN_IDENTIFIER)) {
        nextToken(pAnalyzer);
        /* 函数调用 */
        if (tokenTypeIs(TOKEN_PAREN_L)) {
            nextToken(pAnalyzer);
            if (tokenTypeIs(TOKEN_PAREN_R)) {
                /* 无参数 */
            }
            else {
                rewindToken(pAnalyzer);
                /* 有参数 */
                for(;;) {
                    KBool bIsValid = matchExpr(pAnalyzer);
                    if (!bIsValid) {
                        return KB_FALSE;
                    }
                    nextToken(pAnalyzer);
                    if (tokenTypeIs(TOKEN_COMMA)) {
                        continue;
                    }
                    else if (tokenTypeIs(TOKEN_PAREN_R)) {
                        break;
                    }
                    else {
                        return KB_FALSE;
                    }
                }
            }
        }
        /* 数组 */
        else if (tokenTypeIs(TOKEN_BRACKET_L)) {
            KBool bIsValid = matchExpr(pAnalyzer);
            if (!bIsValid) return KB_FALSE;
            nextToken(pAnalyzer);
            if (!tokenTypeIs(TOKEN_BRACKET_R)) return KB_FALSE;
        }
        /* 不是数组或者函数调用，回退一个token*/
        else {
            rewindToken(pAnalyzer);
        }
        return matchTryBinaryOperator(pAnalyzer);
    }
    /* 左括号 */
    else if (tokenTypeIs(TOKEN_PAREN_L)) {
        KBool bIsValid = matchExpr(pAnalyzer);
        if (!bIsValid) return KB_FALSE;
        nextToken(pAnalyzer);
        if (!tokenTypeIs(TOKEN_PAREN_R)) return KB_FALSE;
        return matchTryBinaryOperator(pAnalyzer);
    }
    return KB_FALSE;
}

void buildExprAstTryOperand(Analyzer *pAnalyzer, Vlist* pStackOperand, Vlist* pStackOperator);

void buildExprAstHandleBinaryOperator(Vlist* pStackOperand, Vlist* pStackOperator) {
    AstNode* pAstTop;
    if (pStackOperator->size <= 0) {
        return;
    }
    pAstTop = (AstNode *)vlPopBack(pStackOperator);
    /* 运算数出栈2个、作为运算符节点的子节点 */
    pAstTop->uData.sBinaryOperator.pAstRightOperand = (AstNode *)vlPopBack(pStackOperand);
    pAstTop->uData.sBinaryOperator.pAstLeftOperand = (AstNode *)vlPopBack(pStackOperand);
    /* 运算符节（计算结果）点入运算数栈 */
    vlPushBack(pStackOperand, pAstTop);
}

void buildExprAstTryOperator(Analyzer* pAnalyzer, Vlist* pStackOperand, Vlist* pStackOperator) {
    Token* pToken = &pAnalyzer->token;
    nextToken(pAnalyzer);
    if (tokenTypeIs(TOKEN_OPERATOR)) {
        /* 创建运算符节点 */
        AstNode*    pAstOpr = createAst(AST_BINARY_OPERATOR, NULL);
        OperatorId  iOprId  = getOperatorId(pToken->szContent);
        int         iCurrentPriority = getOperatorPriorityById(iOprId);
        pAstOpr->uData.sBinaryOperator.iOperatorId = iOprId;
        

        /* 检查现有的运算符堆栈，检查优先级 */
        while (pStackOperator->size > 0) {
            AstNode*    pAstTop = (AstNode *)vlPeek(pStackOperator);
            OperatorId  iTopOprId;
            int         iTopPriority;
            /* 如果不是运算符，退出 */
            if (pAstTop->iType != AST_BINARY_OPERATOR) {
                break;
            }
            iTopOprId = pAstTop->uData.sBinaryOperator.iOperatorId;
            iTopPriority = getOperatorPriorityById(iTopOprId);
            /* 如果栈顶的优先级较高 */
            if (iTopPriority >= iCurrentPriority) {
                buildExprAstHandleBinaryOperator(pStackOperand, pStackOperator);
            }
            else {
                break;
            }
        }
        /* 本运算符节点入运算符栈 */
        vlPushBack(pStackOperator, pAstOpr);
        /* 右操作数入栈 */
        buildExprAstTryOperand(pAnalyzer, pStackOperand, pStackOperator);
    }
    else {
        rewindToken(pAnalyzer);
    }
}

void buildExprAstTryOperand(Analyzer *pAnalyzer, Vlist* pStackOperand, Vlist* pStackOperator) {
    Token* pToken = &pAnalyzer->token;

    nextToken(pAnalyzer);

    /* 字面值：数字 */
    if (tokenTypeIs(TOKEN_NUMERIC)) {
        AstNode *pAstLiteral = createAst(AST_LITERAL_NUMERIC, NULL);
        pAstLiteral->uData.sLiteralNumeric.fValue = Atof(pToken->szContent);
        /* 直接入运算数栈 */
        vlPushBack(pStackOperand, pAstLiteral);
        /* 尝试下一个 token 是否是运算符 */
        buildExprAstTryOperator(pAnalyzer, pStackOperand, pStackOperator);
    }
    /* 字面值：字符串*/
    else if (tokenTypeIs(TOKEN_STRING)) {
        AstNode *pAstLiteral = createAst(AST_LITERAL_STRING, NULL);
        pAstLiteral->uData.sLiteralString.szValue = StringDump(pToken->szContent);
        /* 直接入运算数栈 */
        vlPushBack(pStackOperand, pAstLiteral);
        /* 尝试下一个 token 是否是运算符 */
        buildExprAstTryOperator(pAnalyzer, pStackOperand, pStackOperator);
    }
    /* 一元运算符：取负 */
    else if (tokenTypeIs(TOKEN_OPERATOR) && tokenIs("-")) {
        /* 创建负号节点但是不入栈 */
        AstNode* pAstOprNeg = createAst(AST_UNARY_OPERATOR, NULL);
        pAstOprNeg->uData.sUnaryOperator.iOperatorId = OPR_NEG;
        /* 负号后续的运算数入栈 */
        buildExprAstTryOperand(pAnalyzer, pStackOperand, pStackOperator);
        /* 后续运算数出栈，作为负号节点的子节点 */
        pAstOprNeg->uData.sUnaryOperator.pAstOperand = (AstNode *)vlPopBack(pStackOperand);
        /* 负号节点入运算数栈 */
        vlPushBack(pStackOperand, pAstOprNeg);
        /* 尝试下一个 token 是否是运算符 */
        buildExprAstTryOperator(pAnalyzer, pStackOperand, pStackOperator);
    }
    /* 一元运算符：逻辑非 */
    else if (tokenTypeIs(TOKEN_OPERATOR) && tokenIs("!")) {
        /* 创建逻辑非节点但是不入栈 */
        AstNode* pAstOprNot = createAst(AST_UNARY_OPERATOR, NULL);
        pAstOprNot->uData.sUnaryOperator.iOperatorId = OPR_NOT;
        /* 逻辑非后续的运算数入栈 */
        buildExprAstTryOperand(pAnalyzer, pStackOperand, pStackOperator);
        /* 后续运算数出栈，作为逻辑非节点的子节点 */
        pAstOprNot->uData.sUnaryOperator.pAstOperand = (AstNode *)vlPopBack(pStackOperand);
        /* 逻辑非节点入运算数栈 */
        vlPushBack(pStackOperand, pAstOprNot);
        /* 尝试下一个 token 是否是运算符 */
        buildExprAstTryOperator(pAnalyzer, pStackOperand, pStackOperator);
    }
    /* 标志符 */
    else if (tokenTypeIs(TOKEN_IDENTIFIER)) {
        char* szIdentifier = StringDump(pToken->szContent);
        nextToken(pAnalyzer);
        /* 函数调用 */
        if (tokenTypeIs(TOKEN_PAREN_L)) {
            /* 生成函数调用节点并且入运算符栈 */
            AstNode* pAstFuncCall = createAst(AST_FUNCTION_CALL, NULL);
            pAstFuncCall->uData.sFunctionCall.szFunction = szIdentifier;
            vlPushBack(pStackOperator, pAstFuncCall);
            
            nextToken(pAnalyzer);
            if (tokenTypeIs(TOKEN_PAREN_R)) {
                /* 没有参数 */
            }
            else {
                /* 至少一个参数 */
                rewindToken(pAnalyzer);
                for(;;) {
                    /* 运算数入栈 */
                    buildExprAstTryOperand(pAnalyzer, pStackOperand, pStackOperator);
                    /* 处理函数参数表达式 */
                    while (pStackOperator->size > 0) {
                        AstNode* pAstTop = (AstNode *)vlPeek(pStackOperator);
                        /* 栈上遇到函数调用就退出 */
                        if (pAstTop == pAstFuncCall) {
                            break;
                        }
                        /* 处理运算符 */
                        buildExprAstHandleBinaryOperator(pStackOperand, pStackOperator);
                    }
                    /* 弹出运算数放到函数调用节点里 */
                    vlPushBack(pAstFuncCall->uData.sFunctionCall.pListArguments, vlPopBack(pStackOperand));
                    /* 检查是否函数参数结束 */
                    nextToken(pAnalyzer);
                    if (tokenTypeIs(TOKEN_COMMA)) {
                        continue;
                    }
                    else if (tokenTypeIs(TOKEN_PAREN_R)) {
                        break;
                    }
                }
            }
            /* 函数节点从运算符栈弹出，入运算数栈 */
            vlPushBack(pStackOperand, vlPopBack(pStackOperator));
        }
        /* 数组 */
        else if (tokenTypeIs(TOKEN_BRACKET_L)) {
            AstNode* pAstArrEl = createAst(AST_ARRAY_ACCESS, NULL);
            pAstArrEl->uData.sArrayAccess.szArrayName = szIdentifier;
            /* 数组节点入操作符栈 */
            vlPushBack(pStackOperator, pAstArrEl);
            /* 构建方括号内的表达式 */
            buildExprAstTryOperand(pAnalyzer, pStackOperand, pStackOperator);
            /* 忽略最后的右中括号 */
            nextToken(pAnalyzer);
            /* 处理合并方括号中的表达式 */
            while (pStackOperator->size > 0) {
                AstNode *pAstTop = (AstNode *)vlPeek(pStackOperator);
                /* 遇到当前数组元素节点，弹出了所有的内容了，结束 */
                if (pAstTop == pAstArrEl) {
                    vlPopBack(pStackOperator);
                    break;
                }
                /* 处理运算符 */
                buildExprAstHandleBinaryOperator(pStackOperand, pStackOperator);
            }
            /* 方括号内表达式出栈，作为数组元素节点的子节点 */
            pAstArrEl->uData.sArrayAccess.pAstSubscript = (AstNode *)vlPopBack(pStackOperand);
            /* 数组元素节点入表达式栈 */
            vlPushBack(pStackOperand, pAstArrEl);
        }
        /* 变量 */
        else {
            AstNode *pAstVar = createAst(AST_VARIABLE, NULL);
            pAstVar->uData.sVariable.szName = szIdentifier;
            /* 直接入运算数栈 */
            vlPushBack(pStackOperand, pAstVar);
            /* 退回上一个token */
            rewindToken(pAnalyzer);
        }
        /* 尝试下一个 token 是否是运算符 */
        buildExprAstTryOperator(pAnalyzer, pStackOperand, pStackOperator);
    }
    /* 括号 */
    else if (tokenTypeIs(TOKEN_PAREN_L)) {
        AstNode* pAstParen = createAst(AST_PAREN, NULL);
        /* 括号节点入操作符栈 */
        vlPushBack(pStackOperator, pAstParen);
        /* 构建括号内的表达式 */
        buildExprAstTryOperand(pAnalyzer, pStackOperand, pStackOperator);
        /* 忽略最后的右括号 */
        nextToken(pAnalyzer);
        /* 处理合并括号中的表达式 */
        while (pStackOperator->size > 0) {
            AstNode *pAstTop = (AstNode *)vlPeek(pStackOperator);
            /* 遇到当前括号节点，弹出了所有的内容了，结束 */
            if (pAstTop == pAstParen) {
                vlPopBack(pStackOperator);
                break;
            }
            /* 处理运算符 */
            buildExprAstHandleBinaryOperator(pStackOperand, pStackOperator);
        }
        /* 括号内表达式出栈，作为括号节点的子节点 */
        pAstParen->uData.sParen.pAstExpr = (AstNode *)vlPopBack(pStackOperand);
        /* 括号节点入表达式栈 */
        vlPushBack(pStackOperand, pAstParen);
        /* 尝试下一个 token 是否是运算符 */
        buildExprAstTryOperator(pAnalyzer, pStackOperand, pStackOperator);
    }
}

AstNode* buildExprAst(Analyzer *pAnalyzer) {
    Vlist*      pStackOperand;  /* AstNode */
    Vlist*      pStackOperator; /* AstNode */
    AstNode*    pAstExprRoot;
    
    pStackOperand = vlNewList();
    pStackOperator = vlNewList();
    buildExprAstTryOperand(pAnalyzer, pStackOperand, pStackOperator);

    /* 检查运算符堆栈，如果不为空就弹出所有 */
    while (pStackOperator->size > 0) {
        buildExprAstHandleBinaryOperator(pStackOperand, pStackOperator);
    }

    pAstExprRoot = (AstNode *)vlPopBack(pStackOperand);

    vlDestroy(pStackOperand, destroyAstVoidPtr);
    vlDestroy(pStackOperator, destroyAstVoidPtr);

    return pAstExprRoot;
}

static void pushAstNodeUnderCurrent(Parser* pParser, AstNode* pAstNew) {
    AstNode* pAstNode = pParser->pAstCurrent;
    Vlist* pListStatements = NULL;
    switch (pAstNode->iType) {
        case AST_PROGRAM:
            pListStatements = pAstNode->uData.sProgram.pListStatements;
            break;
        case AST_FUNCTION_DECLARE:
            pListStatements = pAstNode->uData.sFunctionDeclare.pListStatements;
            break;
        case AST_THEN:
            pListStatements = pAstNode->uData.sThen.pListStatements;
            break;
        case AST_ELSEIF:
            pListStatements = pAstNode->uData.sElseIf.pListStatements;
            break;
        case AST_ELSE:
            pListStatements = pAstNode->uData.sElse.pListStatements;
            break;
        case AST_WHILE:
            pListStatements = pAstNode->uData.sWhile.pListStatements;
            break;
        case AST_DO_WHILE:
            pListStatements = pAstNode->uData.sDoWhile.pListStatements;
            break;
        case AST_FOR:
            pListStatements = pAstNode->uData.sFor.pListStatements;
            break;
        default:
            pListStatements = NULL;
    }
    if (!pListStatements) return;
    vlPushBack(pListStatements, pAstNew);
}

#define currentAstIs(iAstType)      (pParser->pAstCurrent->iType == (iAstType))

#define stopWithError(iErrId, iStType) {                    \
    destroyAst(pAstCurrentLine);                            \
    *pIntStopStatement = (iStType);                         \
    return (iErrId);                                        \
} NULL

#define matchTokenType(iTokenType, iErrId, iStType) {       \
    nextToken(pAnalyzer);                                   \
    if (pToken->iType != (iTokenType)) {                    \
        stopWithError(iErrId, iStType);                     \
    }                                                       \
} NULL

#define matchTokenTypeAndContent(iTyp, sz, iErr, iStType) { \
    nextToken(pAnalyzer);                                   \
    if (pToken->iType != (iTyp)                             \
        || !IsStringEqual((sz), pToken->szContent)) {       \
        stopWithError(iErr, iStType);                       \
    }                                                       \
} NULL

#define matchValidExpr(iStType) {                           \
    KBool bIsExprValid = matchExpr(pAnalyzer);              \
    if (!bIsExprValid) {                                    \
        stopWithError(SYN_EXPR_INVALID, iStType);           \
    }                                                       \
} NULL

static SyntaxErrorId parseLineAsAst(
    Analyzer*       pAnalyzer,
    Parser*         pParser,
    StatementId*    pIntStopStatement
) {
    Token*      pToken = &pAnalyzer->token;
    AstNode*    pAstCurrentLine = NULL;

    nextToken(pAnalyzer);
    /* 函数定义 */
    if (tokenIsKeyword(KB_KEYWORD_FUNC)) {
        const StatementId iStatement = STATEMENT_FUNC;
        /* 检查是否能在此处声明函数 */
        if (!currentAstIs(AST_PROGRAM)) {
            stopWithError(SYN_FUNC_NESTED, iStatement);
        }
        /* 为本行创建函数定义AST节点 */
        pAstCurrentLine = createAstWithLineNumber(AST_FUNCTION_DECLARE, pParser->pAstCurrent, pParser->iLineNumber);
        /* 控制结构添加Id */
        pAstCurrentLine->iControlId = ++pParser->iControlCounter;
        /* 匹配函数名称 */
        matchTokenType(TOKEN_IDENTIFIER, SYN_FUNC_MISSING_NAME, iStatement);
        pAstCurrentLine->uData.sFunctionDeclare.szFunction = StringDump(pToken->szContent);
        /* 匹配左括号 */
        matchTokenType(TOKEN_PAREN_L, SYN_FUNC_MISSING_LEFT_PAREN, iStatement);
        /* 检查参数列表 */
        nextToken(pAnalyzer);
        /* 没有参数 */
        if (tokenTypeIs(TOKEN_PAREN_R)) {
            /* 什么都不做 */
        }
        /* 有参数列表 */
        else {
            AstFuncParam*   pFuncParam;
            int             iIdLength;
        
            rewindToken(pAnalyzer);
            for(;;) {
                /* 匹配一个参数名称 */
                matchTokenType(TOKEN_IDENTIFIER, SYN_FUNC_INVALID_PARAMETERS, iStatement);
                /* 创建新的 FuncParam 并加入到 AST */
                iIdLength = StringLength(pToken->szContent);
                pFuncParam = (AstFuncParam *)malloc(sizeof(AstFuncParam) + iIdLength);
                pFuncParam->iType = VARDECL_PRIMITIVE;
                StringCopy(pFuncParam->szName, iIdLength + 1, pToken->szContent);
                vlPushBack(pAstCurrentLine->uData.sFunctionDeclare.pListParameters, pFuncParam);
                /* 检查下一个token  */
                nextToken(pAnalyzer);
                /* 下一个 token 是 '[' */
                if (tokenTypeIs(TOKEN_BRACKET_L)) {
                    /* 变量设置为数组类型 */
                    pFuncParam->iType = VARDECL_ARRAY;
                    /* 匹配 ']' */
                    matchTokenType(TOKEN_BRACKET_R, SYN_FUNC_INVALID_PARAMETERS, iStatement);
                    /* 检查下一个token  */
                    nextToken(pAnalyzer);
                }
                /* 下一个 token 是右括号，结束匹配 */
                if (tokenTypeIs(TOKEN_PAREN_R)) {
                    break;
                }
                /* 下一个 token 是逗号，继续匹配新的参数 */
                else if (tokenTypeIs(TOKEN_COMMA)) {
                    continue;
                }
                else {
                    stopWithError(SYN_FUNC_INVALID_PARAMETERS, iStatement);
                }
            }
        }
        /* 匹配行结束 */
        matchTokenType(TOKEN_LINE_END, SYN_EXPECT_LINE_END, iStatement);
        /* 本行节点插入到上级节点的 statement 列表中 */
        pushAstNodeUnderCurrent(pParser, pAstCurrentLine);
        /* 本行的函数节点作为 parser 的当前节点 */
        pParser->pAstCurrent = pAstCurrentLine;
    
        return SYN_NO_ERROR;
    }
    /* if 语句 */
    else if (tokenIsKeyword(KB_KEYWORD_IF)) {
        const char* pExprCondStart;
        pExprCondStart = getCurrentPtr(pAnalyzer);
        matchValidExpr(STATEMENT_IF);
        nextToken(pAnalyzer);
        /* if goto 语句*/
        if (tokenIs(KB_KEYWORD_GOTO)) {
            const StatementId iStatement = STATEMENT_IF_GOTO;
            pAstCurrentLine = createAstWithLineNumber(AST_IF_GOTO, NULL, pParser->iLineNumber);
            /* 匹配标签 */
            matchTokenType(TOKEN_IDENTIFIER, SYN_IF_GOTO_MISSING_LABEL, iStatement);
            pAstCurrentLine->uData.sIfGoto.szLabelName = StringDump(pToken->szContent);
            /* 匹配行结束 */
            matchTokenType(TOKEN_LINE_END, SYN_EXPECT_LINE_END, iStatement);
            /* 构建条件表达式的AST */
            setCurrentPtr(pAnalyzer, pExprCondStart);
            pAstCurrentLine->uData.sIfGoto.pAstCondition = buildExprAst(pAnalyzer);
            /* 本行节点插入到上级节点的 statement 列表中 */
            pushAstNodeUnderCurrent(pParser, pAstCurrentLine);
        }
        /* if else 语句 */
        else if (tokenTypeIs(TOKEN_LINE_END)) {
            pAstCurrentLine = createAstWithLineNumber(AST_IF, pParser->pAstCurrent, pParser->iLineNumber);
            /* 控制结构添加Id */
            pAstCurrentLine->iControlId = ++pParser->iControlCounter;
            /* 构建条件表达式的AST */
            setCurrentPtr(pAnalyzer, pExprCondStart);
            pAstCurrentLine->uData.sIf.pAstCondition = buildExprAst(pAnalyzer);
            /* 本行节点插入到上级节点的 statement 列表中 */
            pushAstNodeUnderCurrent(pParser, pAstCurrentLine);
            /* if 节点的 then 节点作为 parser 的当前节点 */
            pParser->pAstCurrent = pAstCurrentLine->uData.sIf.pAstThen;
        }
        /* 错误 */
        else {
            stopWithError(SYN_EXPECT_LINE_END, STATEMENT_IF);
        }
    
        return SYN_NO_ERROR;
    }
    /* elseif 语句 */
    else if (tokenIsKeyword(KB_KEYWORD_ELSEIF)) {
        StatementId     iStatement      = STATEMENT_ELSEIF;
        AstNode*        pAstIf          = NULL;
        const char*     pExprCondStart  = NULL;

        pExprCondStart = getCurrentPtr(pAnalyzer);
        /* 检查当前控制结构是不是 if then / elseif */
        if (!currentAstIs(AST_THEN) && !currentAstIs(AST_ELSEIF)) {
            stopWithError(SYN_ELSEIF_NOT_MATCH, iStatement);
        }
        pAstIf = pParser->pAstCurrent->pAstParent;
        /* 匹配表达式 */
        matchValidExpr(iStatement);
        /* 匹配行结束 */
        matchTokenType(TOKEN_LINE_END, SYN_EXPECT_LINE_END, iStatement);
        
        pAstCurrentLine = createAstWithLineNumber(AST_ELSEIF, pAstIf, pParser->iLineNumber);
        /* 构建条件表达式的AST */
        setCurrentPtr(pAnalyzer, pExprCondStart);
        pAstCurrentLine->uData.sElseIf.pAstCondition = buildExprAst(pAnalyzer);
        /* elseif 节点添加到 if 的 elseif 列表中 */
        vlPushBack(pAstIf->uData.sIf.pListElseIf, pAstCurrentLine);
        /* 本行 elseif 节点作为 parser 的当前节点 */
        pParser->pAstCurrent = pAstCurrentLine;

        return SYN_NO_ERROR;
    }
    /* else 语句 */
    else if (tokenIsKeyword(KB_KEYWORD_ELSE)) {
        StatementId     iStatement  = STATEMENT_ELSE;
        AstNode*        pAstIf      = NULL;
        /* 检查当前控制结构是不是 if then / elseif */
        if (!currentAstIs(AST_THEN) && !currentAstIs(AST_ELSEIF)) {
            stopWithError(SYN_ELSE_NOT_MATCH, iStatement);
        }
        pAstIf = pParser->pAstCurrent->pAstParent;

        /* 匹配行结束 */
        matchTokenType(TOKEN_LINE_END, SYN_EXPECT_LINE_END, iStatement);
        
        pAstCurrentLine = createAstWithLineNumber(AST_ELSE, pAstIf, pParser->iLineNumber);
        /* else 节点设置为 if 的 else */
        pAstIf->uData.sIf.pAstElse = pAstCurrentLine;
        /* 本行 else 节点作为 parser 的当前节点 */
        pParser->pAstCurrent = pAstCurrentLine;

        return SYN_NO_ERROR;
    }
    /* while 语句 */
    else if (tokenIsKeyword(KB_KEYWORD_WHILE)) {
        /* 记录表达式位置 */
        const char* pExprCondStart = getCurrentPtr(pAnalyzer);
        /* 当前控制结构为 do...while */
        if (currentAstIs(AST_DO_WHILE)) {
            StatementId iStatement = STATEMENT_DO_WHILE;
            /* 匹配表达式 */
            matchValidExpr(iStatement);
            /* 匹配行结束 */
            matchTokenType(TOKEN_LINE_END, SYN_EXPECT_LINE_END, iStatement);
            /* 获取当前的 do...while节点 */
            AstNode* pAstDoWhile = pParser->pAstCurrent;
            /* 构建条件表达式的AST */
            setCurrentPtr(pAnalyzer, pExprCondStart);
            pAstDoWhile->uData.sDoWhile.pAstCondition = buildExprAst(pAnalyzer);
            /* parser 当前 ast 节点退回 do...while 的父节点 */
            pParser->pAstCurrent = pAstDoWhile->pAstParent;
            return SYN_NO_ERROR;
        }
        /* 不是 do...while 结束，创建 while 节点*/
        else {
            StatementId iStatement = STATEMENT_WHILE;
            /* 匹配表达式 */
            matchValidExpr(iStatement);
            /* 匹配行结束 */
            matchTokenType(TOKEN_LINE_END, SYN_EXPECT_LINE_END, iStatement);
            /* 创建 while AST节点 */
            pAstCurrentLine = createAstWithLineNumber(AST_WHILE, pParser->pAstCurrent, pParser->iLineNumber);
            /* 控制结构添加Id */
            pAstCurrentLine->iControlId = ++pParser->iControlCounter;
            /* 构建条件表达式的AST */
            setCurrentPtr(pAnalyzer, pExprCondStart);
            pAstCurrentLine->uData.sWhile.pAstCondition = buildExprAst(pAnalyzer);
            /* 本行 while 节点添加到当前节点的语句中 */
            pushAstNodeUnderCurrent(pParser, pAstCurrentLine);
            /* 本行 while 节点作为 parser 的当前节点 */
            pParser->pAstCurrent = pAstCurrentLine;
            return SYN_NO_ERROR;
        }
    }
    /* do..while 语句 */
    else if (tokenIsKeyword(KB_KEYWORD_DO)) {
        StatementId iStatement = STATEMENT_DO_WHILE;
        /* 匹配行结束 */
        matchTokenType(TOKEN_LINE_END, SYN_EXPECT_LINE_END, iStatement);
        /* 创建 do...while 节点 */
        pAstCurrentLine = createAstWithLineNumber(AST_DO_WHILE, pParser->pAstCurrent, pParser->iLineNumber);
        /* 控制结构添加Id */
        pAstCurrentLine->iControlId = ++pParser->iControlCounter;
        /* 本行 do...while 节点添加到当前节点的语句中 */
        pushAstNodeUnderCurrent(pParser, pAstCurrentLine);
        /* 本行 do...while 节点作为 parser 的当前节点 */
        pParser->pAstCurrent = pAstCurrentLine;
        return SYN_NO_ERROR;
    }
    /* for 语句 */
    else if (tokenIsKeyword(KB_KEYWORD_FOR)) {
        StatementId     iStatement      = STATEMENT_FOR;
        const char*     pExprRangeFrom  = NULL;
        const char*     pExprRangeTo    = NULL;
        const char*     pExprStep       = NULL;
    
        pAstCurrentLine = createAstWithLineNumber(AST_FOR, pParser->pAstCurrent, pParser->iLineNumber);
        /* 控制结构添加Id */
        pAstCurrentLine->iControlId = ++pParser->iControlCounter;
        /* 匹配变量名 */
        matchTokenType(TOKEN_IDENTIFIER, SYN_FOR_MISSING_VARIABLE, iStatement);
        pAstCurrentLine->uData.sFor.szVariable = StringDump(pToken->szContent);
        /* 匹配等号 */
        matchTokenTypeAndContent(TOKEN_OPERATOR, "=", SYN_FOR_MISSING_EQUAL, iStatement);
        /* 匹配 from 值*/
        pExprRangeFrom = getCurrentPtr(pAnalyzer);
        matchValidExpr(iStatement);
        /* 匹配关键字 to */
        matchTokenTypeAndContent(TOKEN_KEYWORD, KB_KEYWORD_TO, SYN_FOR_MISSING_TO, iStatement);
        /* 匹配 to 值*/
        pExprRangeTo = getCurrentPtr(pAnalyzer);
        matchValidExpr(iStatement);
        /* 检查是否有 Step 部分*/
        nextToken(pAnalyzer);
        if (tokenIsKeyword(KB_KEYWORD_STEP)) {
            /* 匹配 step 值*/
            pExprStep = getCurrentPtr(pAnalyzer);
            matchValidExpr(iStatement);
        } else {
            rewindToken(pAnalyzer);
        }
        /* 匹配行结束 */
        matchTokenType(TOKEN_LINE_END, SYN_EXPECT_LINE_END, iStatement);
        /* 构建各个值部分的Ast */
        setCurrentPtr(pAnalyzer, pExprRangeFrom);
        pAstCurrentLine->uData.sFor.pAstRangeFrom = buildExprAst(pAnalyzer);
        setCurrentPtr(pAnalyzer, pExprRangeTo);
        pAstCurrentLine->uData.sFor.pAstRangeTo = buildExprAst(pAnalyzer);
        if (pExprStep) {
            setCurrentPtr(pAnalyzer, pExprStep);
            pAstCurrentLine->uData.sFor.pAstStep = buildExprAst(pAnalyzer);
        }
        /* 本行 for 节点添加到当前节点的语句中 */
        pushAstNodeUnderCurrent(pParser, pAstCurrentLine);
        /* 本行 for 节点作为 parser 的当前节点 */
        pParser->pAstCurrent = pAstCurrentLine;
        return SYN_NO_ERROR;
    }
    /* next 语句，结束 for 控制结构 */
    else if (tokenIsKeyword(KB_KEYWORD_NEXT)) {
        StatementId     iStatement  = STATEMENT_NEXT;
        AstNode*        pAstFor     = NULL;
        /* 检查当前语句块是不是 for */
        if (!currentAstIs(AST_FOR)) {
            stopWithError(SYN_NEXT_NOT_MATCH, iStatement);
        }
        /* 获取当前的 for 节点 */
        pAstFor = pParser->pAstCurrent;
        /* 检查是否是 next [varName] 的语法 */
        nextToken(pAnalyzer);
        if (tokenTypeIs(TOKEN_IDENTIFIER)) {
            /* 检查 next 后的变量名是否与 for 匹配 */
            if (!IsStringEqual(pAstFor->uData.sFor.szVariable, pToken->szContent)) {
                stopWithError(SYN_FOR_VAR_MISMATCH, iStatement);
            }
            /* 匹配行结束 */
            matchTokenType(TOKEN_LINE_END, SYN_EXPECT_LINE_END, iStatement);
        }
        else if (tokenTypeIs(TOKEN_LINE_END)) {
            /* 行结束，什么也不做 */
        }
        else {
            stopWithError(SYN_EXPECT_LINE_END, iStatement);
        }
        /* parser 当前 ast 节点退回 for 的父节点 */
        pParser->pAstCurrent = pAstFor->pAstParent;
        return SYN_NO_ERROR;
    }
    /* break 语句 */
    else if (tokenIsKeyword(KB_KEYWORD_BREAK)) {
        StatementId     iStatement  = STATEMENT_BREAK;
        AstNode*        pAstSearch  = NULL;
        KBool           bIsInLoop   = KB_FALSE;
        /* 向上寻找是否在函数声明内 */
        for (pAstSearch = pParser->pAstCurrent; pAstSearch; pAstSearch = pAstSearch->pAstParent) {
            if (pAstSearch->iType == AST_FOR || pAstSearch->iType == AST_WHILE || pAstSearch->iType == AST_DO_WHILE) {
                bIsInLoop = KB_TRUE;
                break;
            }
        }
        if (!bIsInLoop) {
            stopWithError(SYN_BREAK_OUTSIDE_LOOP, iStatement);
        }
        /* 匹配行结束 */
        matchTokenType(TOKEN_LINE_END, SYN_EXPECT_LINE_END, iStatement);
        /* 本行 for 节点添加到当前节点的语句中 */
        pAstCurrentLine = createAstWithLineNumber(AST_BREAK, pParser->pAstCurrent, pParser->iLineNumber);
        pushAstNodeUnderCurrent(pParser, pAstCurrentLine);
        return SYN_NO_ERROR;
    }
    /* continue 语句 */
    else if (tokenIsKeyword(KB_KEYWORD_CONTINUE)) {
        StatementId     iStatement  = STATEMENT_CONTINUE;
        AstNode*        pAstSearch  = NULL;
        KBool           bIsInLoop   = KB_FALSE;
    
        /* 向上寻找是否在循环声明内 */
        for (pAstSearch = pParser->pAstCurrent; pAstSearch; pAstSearch = pAstSearch->pAstParent) {
            if (pAstSearch->iType == AST_FOR || pAstSearch->iType == AST_WHILE || pAstSearch->iType == AST_DO_WHILE) {
                bIsInLoop = KB_TRUE;
                break;
            }
        }
        if (!bIsInLoop) {
            stopWithError(SYN_CONTINUE_OUTSIDE_LOOP, iStatement);
        }
        /* 匹配行结束 */
        matchTokenType(TOKEN_LINE_END, SYN_EXPECT_LINE_END, iStatement);
        /* 本行 for 节点添加到当前节点的语句中 */
        pAstCurrentLine = createAstWithLineNumber(AST_CONTINUE, pParser->pAstCurrent, pParser->iLineNumber);
        pushAstNodeUnderCurrent(pParser, pAstCurrentLine);
        return SYN_NO_ERROR;
    }
    /* end 语句，结束 if / while 等控制结构或者函数 */
    else if (tokenIsKeyword(KB_KEYWORD_END)) {
        StatementId iStatement = STATEMENT_END;
    
        nextToken(pAnalyzer);
        /* if / elseif / else 语句块结束 */
        if (tokenIsKeyword(KB_KEYWORD_IF)) {
            AstNode* pAstIf;
            if (!(currentAstIs(AST_THEN) || currentAstIs(AST_ELSEIF) || currentAstIs(AST_ELSE))) {
                stopWithError(SYN_END_KEYWORD_NOT_MATCH, iStatement);
            }
            /* 匹配行结束 */
            matchTokenType(TOKEN_LINE_END, SYN_EXPECT_LINE_END, iStatement);
            /* parser 当前 ast 节点退回 if 的父节点 */
            pAstIf = pParser->pAstCurrent->pAstParent;
            pParser->pAstCurrent = pAstIf->pAstParent;
            return SYN_NO_ERROR;
        }
        /* while 语句块结束 */
        else if (tokenIsKeyword(KB_KEYWORD_WHILE)) {
            AstNode* pAstWhile;
            if (!currentAstIs(AST_WHILE)) {
                stopWithError(SYN_END_KEYWORD_NOT_MATCH, iStatement);
            }
            /* 匹配行结束 */
            matchTokenType(TOKEN_LINE_END, SYN_EXPECT_LINE_END, iStatement);
            /* parser 当前 ast 节点退回 while 的父节点 */
            pAstWhile = pParser->pAstCurrent;
            pParser->pAstCurrent = pAstWhile->pAstParent;
            return SYN_NO_ERROR;
        }
        /* 函数声明结束 */
        else if (tokenIsKeyword(KB_KEYWORD_FUNC)) {
            if (!currentAstIs(AST_FUNCTION_DECLARE)) {
                stopWithError(SYN_END_KEYWORD_NOT_MATCH, iStatement);
            }
            /* 匹配行结束 */
            matchTokenType(TOKEN_LINE_END, SYN_EXPECT_LINE_END, iStatement);
            /* parser 当前 ast 节点退回父节点 */
            pParser->pAstCurrent = pParser->pAstCurrent->pAstParent;
            return SYN_NO_ERROR;
        }
        stopWithError(SYN_END_KEYWORD_INVALID, iStatement);
    }
    /* exit 语句 */
    else if (tokenIsKeyword(KB_KEYWORD_EXIT)) {
        StatementId iStatement = STATEMENT_EXIT;

        pAstCurrentLine = createAstWithLineNumber(AST_EXIT, NULL, pParser->iLineNumber);
        nextToken(pAnalyzer);
        /* 无值 */
        if (tokenTypeIs(TOKEN_LINE_END)) {
            pAstCurrentLine->uData.sExit.pAstExpression = NULL;
        }
        else {
            const char* pExprExit;
            rewindToken(pAnalyzer);
            pExprExit = getCurrentPtr(pAnalyzer);
            /* 匹配表达式 */
            matchValidExpr(iStatement);
            /* 匹配行结束 */
            matchTokenType(TOKEN_LINE_END, SYN_EXPECT_LINE_END, iStatement);
            /* 构建表达式 ast */
            setCurrentPtr(pAnalyzer, pExprExit);
            pAstCurrentLine->uData.sExit.pAstExpression = buildExprAst(pAnalyzer);
        }
        /* 本行节点插入到上级节点的 statement 列表中 */
        pushAstNodeUnderCurrent(pParser, pAstCurrentLine);
        return SYN_NO_ERROR;
    }
    /* return 语句 */
    else if (tokenIsKeyword(KB_KEYWORD_RETURN)) {
        StatementId     iStatement  = STATEMENT_RETURN;
        AstNode*        pAstSearch  = NULL;
        KBool           bIsInFunc   = KB_FALSE;
    
        /* 向上寻找是否在函数声明内 */
        for (pAstSearch = pParser->pAstCurrent; pAstSearch; pAstSearch = pAstSearch->pAstParent) {
            if (pAstSearch->iType == AST_FUNCTION_DECLARE) {
                bIsInFunc = KB_TRUE;
                break;
            }
        }
        /* 不在函数内，使用 return 不正确 */
        if (!bIsInFunc) {
            stopWithError(SYN_RETURN_OUTSIDE_FUNC, iStatement);   
        }
        /* 创建 return ast 节点 */
        pAstCurrentLine = createAstWithLineNumber(AST_RETURN, NULL, pParser->iLineNumber);
        nextToken(pAnalyzer);
        /* 无值 */
        if (tokenTypeIs(TOKEN_LINE_END)) {
            pAstCurrentLine->uData.sReturn.pAstExpression = NULL;
        }
        else {
            const char* pExprReturn;
            rewindToken(pAnalyzer);
            pExprReturn = getCurrentPtr(pAnalyzer);
            /* 匹配表达式 */
            matchValidExpr(iStatement);
            /* 匹配行结束 */
            matchTokenType(TOKEN_LINE_END, SYN_EXPECT_LINE_END, iStatement);
            /* 构建表达式 ast */
            setCurrentPtr(pAnalyzer, pExprReturn);
            pAstCurrentLine->uData.sReturn.pAstExpression = buildExprAst(pAnalyzer);
        }
        /* 本行节点插入到上级节点的 statement 列表中 */
        pushAstNodeUnderCurrent(pParser, pAstCurrentLine);
        return SYN_NO_ERROR;
    }
    /* goto 语句 */
    else if (tokenIsKeyword(KB_KEYWORD_GOTO)) {
        StatementId iStatement = STATEMENT_GOTO;
        pAstCurrentLine = createAstWithLineNumber(AST_GOTO, NULL, pParser->iLineNumber);
        /* 匹配标签名称 */
        matchTokenType(TOKEN_IDENTIFIER, SYN_GOTO_MISSING_LABEL, iStatement);
        pAstCurrentLine->uData.sGoto.szLabelName = StringDump(pToken->szContent);
        /* 匹配行结束 */
        matchTokenType(TOKEN_LINE_END, SYN_EXPECT_LINE_END, iStatement);
        /* 本行节点插入到上级节点的 statement 列表中 */
        pushAstNodeUnderCurrent(pParser, pAstCurrentLine);
        return SYN_NO_ERROR;
    }
    /* dim 语句 */
    else if (tokenIsKeyword(KB_KEYWORD_DIM)) {
        char* szIdentifier;
        /* 匹配变量名 */
        matchTokenType(TOKEN_IDENTIFIER, SYN_DIM_MISSING_VARIABLE, STATEMENT_DIM);
        szIdentifier = StringDump(pToken->szContent);
        
        nextToken(pAnalyzer);
        /* 定义变量，无初始化 */
        if (tokenTypeIs(TOKEN_LINE_END)) {
            pAstCurrentLine = createAstWithLineNumber(AST_DIM, NULL, pParser->iLineNumber);
            pAstCurrentLine->uData.sDim.szVariable = szIdentifier;
        }
        /* 定义变量带初始化 */
        else if (tokenTypeIs(TOKEN_OPERATOR) && tokenIs("=")) {
            StatementId   iStatement = STATEMENT_DIM;
            const char*     pExprInit;
            pExprInit = getCurrentPtr(pAnalyzer);
            pAstCurrentLine = createAstWithLineNumber(AST_DIM, NULL, pParser->iLineNumber);
            pAstCurrentLine->uData.sDim.szVariable = szIdentifier;
            /* 匹配表达式 */
            matchValidExpr(iStatement);
            /* 匹配行结束 */
            matchTokenType(TOKEN_LINE_END, SYN_EXPECT_LINE_END, iStatement);
            /* 构建表达式 ast */
            setCurrentPtr(pAnalyzer, pExprInit);
            pAstCurrentLine->uData.sDim.pAstInitializer = buildExprAst(pAnalyzer);
        }
        /* 定义数组 */
        else if (tokenTypeIs(TOKEN_BRACKET_L)) {
            StatementId   iStatement = STATEMENT_DIM_ARRAY;
            const char*     pExprDimension;
            pAstCurrentLine = createAstWithLineNumber(AST_DIM_ARRAY, NULL, pParser->iLineNumber);
            pAstCurrentLine->uData.sDimArray.szArrayName = szIdentifier;
            /* 匹配表达式 */
            pExprDimension = getCurrentPtr(pAnalyzer);
            matchValidExpr(iStatement);
            /* 匹配右中括号 */
            matchTokenType(TOKEN_BRACKET_R, SYN_DIM_ARRAY_MISSING_BRACKET_R, iStatement);
            /* 匹配行结束 */
            matchTokenType(TOKEN_LINE_END, SYN_EXPECT_LINE_END, iStatement);
            /* 构建表达式 ast */
            setCurrentPtr(pAnalyzer, pExprDimension);
            pAstCurrentLine->uData.sDimArray.pAstDimension = buildExprAst(pAnalyzer);
        }
        else {
            stopWithError(SYN_DIM_INVALID, STATEMENT_DIM);
        }
        /* 本行节点插入到上级节点的 statement 列表中 */
        pushAstNodeUnderCurrent(pParser, pAstCurrentLine);
        return SYN_NO_ERROR;
    }
    /* redim 语句 */
    else if (tokenIsKeyword(KB_KEYWORD_REDIM)) {
        StatementId     iStatement      = STATEMENT_REDIM;
        const char*     pExprDimension  = NULL;

        pAstCurrentLine = createAstWithLineNumber(AST_REDIM, NULL, pParser->iLineNumber);
        /* 匹配变量名 */
        matchTokenType(TOKEN_IDENTIFIER, SYN_REDIM_MISSING_VARIABLE, iStatement);
        pAstCurrentLine->uData.sRedim.szArrayName = StringDump(pToken->szContent);
        /* 匹配左中括号 */
        matchTokenType(TOKEN_BRACKET_L, SYN_REDIM_MISSING_BRACKET_L, iStatement);
        /* 匹配表达式 */
        pExprDimension = getCurrentPtr(pAnalyzer);
        matchValidExpr(iStatement);
        /* 匹配右中括号 */
        matchTokenType(TOKEN_BRACKET_R, SYN_REDIM_MISSING_BRACKET_R, iStatement);
        /* 匹配行结束 */
        matchTokenType(TOKEN_LINE_END, SYN_EXPECT_LINE_END, iStatement);
        /* 构建表达式 ast */
        setCurrentPtr(pAnalyzer, pExprDimension);
        pAstCurrentLine->uData.sRedim.pAstDimension = buildExprAst(pAnalyzer);
        /* 本行节点插入到上级节点的 statement 列表中 */
        pushAstNodeUnderCurrent(pParser, pAstCurrentLine);
        return SYN_NO_ERROR;
    }
    /* 标志符 */
    else if (tokenTypeIs(TOKEN_IDENTIFIER)) {
        char* szIdentifier = StringDump(pToken->szContent);
        nextToken(pAnalyzer);
        /* 标签定义 */
        if (tokenTypeIs(TOKEN_LABEL_SIGN)) {
            pAstCurrentLine = createAstWithLineNumber(AST_LABEL_DECLARE, NULL, pParser->iLineNumber);
            pAstCurrentLine->uData.sLabel.szLabelName = szIdentifier;
            /* 匹配行结束 */
            matchTokenType(TOKEN_LINE_END, SYN_EXPECT_LINE_END, STATEMENT_LABEL);
            /* 本行节点插入到上级节点的 statement 列表中 */
            pushAstNodeUnderCurrent(pParser, pAstCurrentLine);
            return SYN_NO_ERROR;
        }
        /* 赋值给变量 */
        else if (tokenTypeIs(TOKEN_OPERATOR) && tokenIs("=")) {
            StatementId   iStatement = STATEMENT_ASSIGN;
            const char*     pExprValue;
            pAstCurrentLine = createAstWithLineNumber(AST_ASSIGN, NULL, pParser->iLineNumber);
            pAstCurrentLine->uData.sLabel.szLabelName = szIdentifier;
            /* 匹配表达式 */
            pExprValue = getCurrentPtr(pAnalyzer);
            matchValidExpr(iStatement);
            /* 匹配行结束 */
            matchTokenType(TOKEN_LINE_END, SYN_EXPECT_LINE_END, iStatement);
            /* 构建表达式 ast */
            setCurrentPtr(pAnalyzer, pExprValue);
            pAstCurrentLine->uData.sAssign.pAstValue = buildExprAst(pAnalyzer);
            /* 本行节点插入到上级节点的 statement 列表中 */
            pushAstNodeUnderCurrent(pParser, pAstCurrentLine);
            return SYN_NO_ERROR;
        }
        /* 可能是赋值给数组 */
        else if (tokenTypeIs(TOKEN_BRACKET_L)) {
            StatementId     iStatement = STATEMENT_ASSIGN_ARRAY;
            const char*     pExprSubscript;
            const char*     pExprValue;
            /* 尝试表达式 */
            pExprSubscript = getCurrentPtr(pAnalyzer);
            if (!matchExpr(pAnalyzer)) {
                free(szIdentifier);
                goto TreatLineAsAnExpression;
            }
            /* 尝试右中括号 */
            nextToken(pAnalyzer);
            if (!tokenTypeIs(TOKEN_BRACKET_R)) {
                free(szIdentifier);
                goto TreatLineAsAnExpression;
            }
            /* 尝试等号 */
            nextToken(pAnalyzer);
            if (!(tokenTypeIs(TOKEN_OPERATOR) && tokenIs("="))) {
                free(szIdentifier);
                goto TreatLineAsAnExpression;
            }
            pAstCurrentLine = createAstWithLineNumber(AST_ASSIGN_ARRAY, NULL, pParser->iLineNumber);
            pAstCurrentLine->uData.sAssignArray.szArrayName = szIdentifier;
            /* 匹配表达式 */
            pExprValue = getCurrentPtr(pAnalyzer);
            matchValidExpr(iStatement);
            /* 匹配行结束 */
            matchTokenType(TOKEN_LINE_END, SYN_EXPECT_LINE_END, iStatement);
            /* 构建表达式 ast */
            setCurrentPtr(pAnalyzer, pExprSubscript);
            pAstCurrentLine->uData.sAssignArray.pAstSubscript = buildExprAst(pAnalyzer);
            setCurrentPtr(pAnalyzer, pExprValue);
            pAstCurrentLine->uData.sAssignArray.pAstValue = buildExprAst(pAnalyzer);
            /* 本行节点插入到上级节点的 statement 列表中 */
            pushAstNodeUnderCurrent(pParser, pAstCurrentLine);
            return SYN_NO_ERROR;
        }
        goto TreatLineAsAnExpression;
    }
    /* 空行 */
    else if (tokenTypeIs(TOKEN_LINE_END)) {
        return SYN_NO_ERROR;
    }
    /* 尝试匹配表达式 */
    else {
TreatLineAsAnExpression:
        resetToken(pAnalyzer);
        matchValidExpr(STATEMENT_EXPR);
        matchTokenType(TOKEN_LINE_END, SYN_EXPECT_LINE_END, STATEMENT_EXPR);
        resetToken(pAnalyzer);
        pAstCurrentLine = buildExprAst(pAnalyzer);
        /* 手动给表达式添加行号 */
        pAstCurrentLine->iLineNumber = pParser->iLineNumber;
        pushAstNodeUnderCurrent(pParser, pAstCurrentLine);
        return SYN_NO_ERROR;
    }
    return SYN_UNRECOGNIZED;
}

KbAstNode* KSourceParser_Parse(
    const char*     szSource,
    SyntaxErrorId*  pIntSyntaxErrorId,
    StatementId*    pIntStopStatement,
    int*            pIntStopLineNumber
) {
    Parser      parser;
    char        szLineBuf[KB_PARSER_LINE_MAX];
    int         iCharRead;
    AstNode*    pAstProgram;

    *pIntSyntaxErrorId  = SYN_NO_ERROR;
    *pIntStopStatement  = STATEMENT_NONE;
    *pIntStopLineNumber = 0;

    pAstProgram = createAst(AST_PROGRAM, NULL);

    initParser(&parser, szSource);
    parser.pAstCurrent = pAstProgram;

    while(parser.pSourceCurrent[0]) {
        int         iLineErrorId;
        Analyzer    analyzer;
        KBool       bContinueParseLine = KB_FALSE;
        const char* szLineContinuePtr;
    
        /* 从源代码中读取一行 */
        *pIntStopLineNumber = ++parser.iLineNumber;
        iCharRead = fetchLine(parser.pSourceCurrent, szLineBuf);
        parser.pSourceCurrent += iCharRead;
    
        /* 解析本行内容 */
        szLineContinuePtr = szLineBuf;
        do {
            initAnalyzer(&analyzer, szLineContinuePtr);
            iLineErrorId = parseLineAsAst(&analyzer, &parser, pIntStopStatement);
            /* 解析行发生错误 */
            if (iLineErrorId != SYN_NO_ERROR) {
                *pIntSyntaxErrorId = iLineErrorId;
                destroyAst(pAstProgram);
                return NULL;
            }
            /* 检查一下是读到 ';' 结束，还是一行真的结束 */
            if (*analyzer.pCurrent == ';') {
                /* 跳过 ';' 继续解析本行 */
                szLineContinuePtr = analyzer.pCurrent + 1;
                bContinueParseLine = KB_TRUE;
                continue;
            }
            /* 其他情况，遇到了 '\0' 或者 '#' 注释，结束本行解析 */
            else {
                bContinueParseLine = KB_FALSE;
            }
        } while(bContinueParseLine);
    }

    /* 有未完成的控制结构 */
    if (parser.pAstCurrent != pAstProgram) {
        *pIntSyntaxErrorId = SYN_UNTERMINATED_FUNC_OR_CTRL;
        destroyAst(pAstProgram);
        return NULL;
    }

    /* 控制结构总数量 */
    pAstProgram->uData.sProgram.iNumControl = parser.iControlCounter;

    return pAstProgram;
}

#define extReturnError(iExtErr) {       \
    *pIntExtErrorId = (iExtErr);        \
    return KB_FALSE;                    \
} NULL

#define extTokenTypeIs(t)    (pToken->iType == (t))

#define extTokenIs(t, str)    (pToken->iType == (t) && IsStringEqual(pToken->szContent, (str)))

#define extMatchType(iType, iExtErr) {  \
    nextToken(pAnalyzer);               \
    if (!extTokenTypeIs(iType)) {        \
        extReturnError(iExtErr);        \
    }                                   \
} NULL

#define extMatch(iType, str, iExtErr) { \
    nextToken(pAnalyzer);               \
    if (!extTokenIs(iType, str)) {      \
        extReturnError(iExtErr);        \
    }                                   \
} NULL

KBool KExtension_Parse(
    char*               szExtensionId,
    Vlist*              pListExtFuncs,
    const char*         szSource,
    ExtensionErrorId*   pIntExtErrorId,
    int*                pIntStopLineNumber
) {
    Parser      parser;
    char        szLineBuf[KB_PARSER_LINE_MAX];
    int         iCharRead;

    *pIntExtErrorId  = EXT_NO_ERROR;
    *pIntStopLineNumber = 0;

    initParser(&parser, szSource);

    while(parser.pSourceCurrent[0]) {
        Analyzer    analyzer;
        Analyzer*   pAnalyzer = &analyzer;
        Token*      pToken = &analyzer.token;
        KBool       bContinueParseLine = KB_FALSE;
        const char* szLineContinuePtr;
    
        /* 从源代码中读取一行 */
        *pIntStopLineNumber = ++parser.iLineNumber;
        iCharRead = fetchLine(parser.pSourceCurrent, szLineBuf);
        parser.pSourceCurrent += iCharRead;
    
        /* 解析本行内容 */
        szLineContinuePtr = szLineBuf;
        do {
            initAnalyzer(&analyzer, szLineContinuePtr);
            /* 获取第一个 token，解析行内容 */
            nextToken(pAnalyzer);
            /* 定义 extension 名字 */
            if (extTokenIs(TOKEN_IDENTIFIER, "extension")) {
                /* 重复定义 extension id */
                if (StringLength(szExtensionId) > 0) {
                    extReturnError(EXT_ID_DUPLICATED);
                }
                /* 匹配 '=' */
                extMatch(TOKEN_OPERATOR, "=", EXT_ID_SYNTAX_ERROR);
                /* 匹配字符串 */
                extMatchType(TOKEN_STRING, EXT_ID_SYNTAX_ERROR);
                /* ExtensionId 字符串过长 */
                if (StringLength(pToken->szContent) > KB_IDENTIFIER_LEN_MAX) {
                    extReturnError(EXT_ID_TOO_LONG);
                }
                StringCopy(szExtensionId, sizeof(szExtensionId), pToken->szContent);
                /* 匹配行结束 */
                extMatchType(TOKEN_LINE_END, EXT_EXPECT_LINE_END);
            }
            /* 定义函数 */
            else if (extTokenIs(TOKEN_KEYWORD, KB_KEYWORD_FUNC)) {
                /* 创建拓展函数并且添加到上下文 */
                ExtFunc* pExtFunc = (ExtFunc *)malloc(sizeof(ExtFunc));
                memset(pExtFunc, 0, sizeof(ExtFunc));
                vlPushBack(pListExtFuncs, pExtFunc);
                /* 匹配 callId */
                extMatchType(TOKEN_NUMERIC, EXT_FUNC_MISSING_ID);
                pExtFunc->iCallId = (int)Atof(pToken->szContent);
                /* 匹配箭头 */
                extMatch(TOKEN_OPERATOR, "-", EXT_FUNC_MISSING_ARROW);
                extMatch(TOKEN_OPERATOR, ">", EXT_FUNC_MISSING_ARROW);
                /* 匹配函数名称 */
                extMatchType(TOKEN_IDENTIFIER, EXT_FUNC_MISSING_NAME);
                /* 函数名过长 */
                if (StringLength(pToken->szContent) > KB_IDENTIFIER_LEN_MAX) {
                    extReturnError(EXT_ID_TOO_LONG);
                }
                StringCopy(pExtFunc->szFuncName, sizeof(pExtFunc->szFuncName), pToken->szContent);
                /* 匹配括号 '(' */
                extMatchType(TOKEN_PAREN_L, EXT_FUNC_INVALID_PARAMS);
                /* 匹配参数列表 */
                nextToken(pAnalyzer);
                if (!extTokenTypeIs(TOKEN_PAREN_R)) {
                    rewindToken(pAnalyzer);
                    for (;;) {
                        /* 匹配函数参数 */
                        extMatchType(TOKEN_IDENTIFIER, EXT_FUNC_INVALID_PARAMS);
                        pExtFunc->iNumParams++;
                        /* 检查下一个token */
                        nextToken(pAnalyzer);
                        /* 逗号，继续匹配下一个参数 */
                        if (extTokenTypeIs(TOKEN_COMMA)) {
                            continue;
                        }
                        /* 右括号 ')'，匹配结束 */
                        else if (extTokenTypeIs(TOKEN_PAREN_R)) {
                            break;
                        }
                        /* 其他，错误 */
                        else {
                            extReturnError(EXT_FUNC_INVALID_PARAMS);
                        }
                    }
                }
                /* 匹配 return  */
                extMatch(TOKEN_KEYWORD, KB_KEYWORD_RETURN, EXT_FUNC_MISSING_RETURN);
                /* 匹配类型字符串，暂时只是读入，不处理*/
                extMatchType(TOKEN_STRING, EXT_FUNC_MISSING_TYPE);
                /* 匹配行结束 */
                extMatchType(TOKEN_LINE_END, EXT_EXPECT_LINE_END);
            }
            /* 空行 */
            else if (extTokenTypeIs(TOKEN_LINE_END)) {
                /* 什么也不做 */
            }
            /* 其他，无法识别 */
            else {
                extReturnError(EXT_UNRECOGNIZED);
            }
            /* 检查一下是读到 ';' 结束，还是一行真的结束 */
            if (*(pAnalyzer->pCurrent) == ';') {
                /* 跳过 ';' 继续解析本行 */
                szLineContinuePtr = analyzer.pCurrent + 1;
                bContinueParseLine = KB_TRUE;
                continue;
            }
            /* 其他情况，遇到了 '\0' 或者 '#' 注释，结束本行解析 */
            else {
                bContinueParseLine = KB_FALSE;
            }
        } while(bContinueParseLine);
    }

    if (StringLength(szExtensionId) <= 0) {
        extReturnError(EXT_ID_NOT_FOUND);
    }

    return KB_TRUE;
}
