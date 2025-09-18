#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kompiler.h"
#include "kalias.h"

typedef struct {
    const char* szName;
    int         iNumParams;
    BuiltFuncId iFuncId;
} BuiltInFunc;

static const BuiltInFunc BuiltInFunctions[] = {
    { "p",          1, KBUILT_IN_FUNC_P         },
    { "sin",        1, KBUILT_IN_FUNC_SIN       },
    { "cos",        1, KBUILT_IN_FUNC_COS       },
    { "tan",        1, KBUILT_IN_FUNC_TAN       },
    { "sqrt",       1, KBUILT_IN_FUNC_SQRT      },
    { "exp",        1, KBUILT_IN_FUNC_EXP       },
    { "abs",        1, KBUILT_IN_FUNC_ABS       },
    { "log",        1, KBUILT_IN_FUNC_LOG       },
    { "floor",      1, KBUILT_IN_FUNC_FLOOR     },
    { "ceil",       1, KBUILT_IN_FUNC_CEIL      },
    { "rand",       0, KBUILT_IN_FUNC_RAND      },
    { "len",        1, KBUILT_IN_FUNC_LEN       },
    { "val",        1, KBUILT_IN_FUNC_VAL       },
    { "chr",        1, KBUILT_IN_FUNC_CHR       },
    { "asc",        1, KBUILT_IN_FUNC_ASC       }
};

static const BuiltInFunc* findBuiltInFunc(const char* szName) {
    static const int size = sizeof(BuiltInFunctions) / sizeof(BuiltInFunctions[0]);
    int i;
    for (i = 0; i < size; ++i) {
        const BuiltInFunc* pBuiltInFunc = BuiltInFunctions + i;
        if (IsStringEqual(pBuiltInFunc->szName, szName)) {
            return pBuiltInFunc;
        }
    }
    return NULL;
}

static const struct {
    char* szName;
    char* szMessage;
} SEMANTIC_ERROR_DETAIL[] = {
    { "SEM_NO_ERROR",                   "No semantic errors detected" },
    { "SEM_UNRECOGNIZED_AST",           "Unrecognized abstract syntax tree node" },
    { "SEM_NOT_A_PROGRAM",              "Input is not a valid KBasic program" },
    { "SEM_VAR_NAME_TOO_LONG",          "Variable name is too long" },
    { "SEM_VAR_DUPLICATED",             "Duplicate variable declaration" },
    { "SEM_VAR_NOT_FOUND",              "Undefined variable" },
    { "SEM_VAR_IS_NOT_ARRAY",           "Variable is not an array" },
    { "SEM_VAR_IS_NOT_PRIMITIVE",       "Variable is not a primitive type" },
    { "SEM_FUNC_NAME_TOO_LONG",         "Function name is too long" },
    { "SEM_FUNC_DUPLICATED",            "Duplicate function definition" },
    { "SEM_FUNC_NOT_FOUND",             "Undefined function" },
    { "SEM_FUNC_ARG_LIST_MISMATCH",     "Argument count mismatch in call to function" },
    { "SEM_LABEL_NAME_TOO_LONG",        "Label name is too long" },
    { "SEM_LABEL_DUPLICATED",           "Duplicate label definition" },
    { "SEM_GOTO_LABEL_NOT_FOUND",       "Undefined label" },
    { "SEM_GOTO_LABEL_SCOPE_MISMATCH",  "Cannot jump to label across function boundaries" },
    { "SEM_STR_POOL_EXCEED",            "String constant pool capacity exceeded" }
};

const char* KSemanticError_GetNameById(SemanticErrorId iSemanticErrorId) {
    if (iSemanticErrorId < 0 || iSemanticErrorId > SEM_STR_POOL_EXCEED) return "N/A";
    return SEMANTIC_ERROR_DETAIL[iSemanticErrorId].szName;
}

const char* KSemanticError_GetMessageById(SemanticErrorId iSemanticErrorId) {
    if (iSemanticErrorId < 0 || iSemanticErrorId > SEM_STR_POOL_EXCEED) return "N/A";
    return SEMANTIC_ERROR_DETAIL[iSemanticErrorId].szMessage;
}

static VarDecl* createVarDecl(const char* szName, VarDeclTypeId iVarType) {
    VarDecl* pVarDecl = (VarDecl *)malloc(sizeof(VarDecl));

    StringCopy(pVarDecl->szVarName, sizeof(pVarDecl->szVarName), szName);
    pVarDecl->iType = iVarType;

    return pVarDecl;
}

static void destroyVarDecl(VarDecl* pVarDecl) {
    free(pVarDecl);
}

static void destroyVarDeclVoidPtr(void* pVoidPtr) {
    destroyVarDecl((VarDecl *)pVoidPtr);
}

static FuncDecl* createFuncDecl(const char* szName, int iNumParams) {
    FuncDecl* pFuncDecl = (FuncDecl *)malloc(sizeof(FuncDecl));

    StringCopy(pFuncDecl->szFuncName, sizeof(pFuncDecl->szFuncName), szName);
    pFuncDecl->iNumParams       = iNumParams;
    pFuncDecl->pListVariables   = vlNewList();
    pFuncDecl->iOpCodeStartPos  = -1;

    return pFuncDecl;
}

static void destroyFuncDecl(FuncDecl* pFuncDecl) {
    vlDestroy(pFuncDecl->pListVariables, destroyVarDeclVoidPtr);
    free(pFuncDecl);
}

static void destroyFuncDeclVoidPtr(void* pVoidPtr) {
    destroyFuncDecl((FuncDecl *)pVoidPtr);
}

static void initControlFlowLabel(CtrlFlowLabel* pCtrlLabel, const AstNode* pAstNode) {
    switch (pAstNode->iType) {
        case AST_FUNCTION_DECLARE: {
            pCtrlLabel->iType = CF_FUNCTION;
            pCtrlLabel->uData.sFunction.iEndPos = -1;
            break;
        }
        case AST_IF: {
            int i, iNumElseIf = pAstNode->uData.sIf.pListElseIf->size;
            pCtrlLabel->iType = CF_IF;
            pCtrlLabel->uData.sIf.pArrElseIfEndPos = NULL;
            pCtrlLabel->uData.sIf.iThenEndPos = -1;
            pCtrlLabel->uData.sIf.iEndPos = -1;
            if (iNumElseIf > 0) {
                pCtrlLabel->uData.sIf.pArrElseIfEndPos = (KLabelOpCodePos *)malloc(iNumElseIf * sizeof(KLabelOpCodePos));
                for (i = 0; i < iNumElseIf; ++i) {
                    pCtrlLabel->uData.sIf.pArrElseIfEndPos[i] = -1;
                }
            }
            break;
        }
        case AST_WHILE: {
            pCtrlLabel->iType = CF_WHILE;
            pCtrlLabel->uData.sWhile.iCondPos = -1;
            pCtrlLabel->uData.sWhile.iEndPos = -1;
            break;
        }
        case AST_DO_WHILE: {
            pCtrlLabel->iType = CF_DO_WHILE;
            pCtrlLabel->uData.sDoWhile.iStartPos = -1;
            pCtrlLabel->uData.sDoWhile.iCondPos = -1;
            pCtrlLabel->uData.sDoWhile.iEndPos = -1;
            break;
        }
        case AST_FOR: {
            pCtrlLabel->iType = CF_FOR;
            pCtrlLabel->uData.sFor.iCondPos = -1;
            pCtrlLabel->uData.sFor.iIncreasePos = -1;
            pCtrlLabel->uData.sFor.iEndPos = -1;
            break;
        }
        default: {
            break;
        }
    }
}

static void cleanUpControlFlowLabel(CtrlFlowLabel* pCtrlLabel) {
    if (pCtrlLabel->iType == CF_IF) {
        if (pCtrlLabel->uData.sIf.pArrElseIfEndPos) {
            free(pCtrlLabel->uData.sIf.pArrElseIfEndPos);   
        }
    }
}

static Context* initContext(const AstNode* pAstProgram) {
    Context* pContext = (Context *)malloc(sizeof(Context));

    pContext->pListGlobalVariables  = vlNewList();
    pContext->pListFunctions        = vlNewList();
    pContext->pListOpCodes          = vlNewList();
    pContext->pListLabels           = vlNewList();
    pContext->pCurrentFunc          = NULL;
    pContext->iStringPoolSize       = 0;
    pContext->iNumCtrlFlowLabels    = pAstProgram->uData.sProgram.iNumControl;
    if (pContext->iNumCtrlFlowLabels > 0) {
        pContext->pCtrlFlowLabels = (CtrlFlowLabel *)malloc(pContext->iNumCtrlFlowLabels * sizeof(CtrlFlowLabel));
    } else {
        pContext->pCtrlFlowLabels = NULL;
    }
    return pContext;
}

void KompilerContext_Destroy(KbCompilerContext* pContext) {
    vlDestroy(pContext->pListGlobalVariables, destroyVarDeclVoidPtr);
    vlDestroy(pContext->pListFunctions, destroyFuncDeclVoidPtr);
    vlDestroy(pContext->pListOpCodes, free);
    vlDestroy(pContext->pListLabels, free);
    if (pContext->pCtrlFlowLabels) {
        int i;
        for (i = 0; i < pContext->iNumCtrlFlowLabels; ++i) {
            cleanUpControlFlowLabel(pContext->pCtrlFlowLabels + i);
        }
        free(pContext->pCtrlFlowLabels);
    }
    free(pContext);
}

static FuncDecl* findFunc(const Context* pContext, const char* szToFind) {
    VlistNode* pListNode;

    for (pListNode = pContext->pListFunctions->head; pListNode; pListNode = pListNode->next) {
        FuncDecl* pFuncDecl = (FuncDecl *)pListNode->data;
        if (IsStringEqual(pFuncDecl->szFuncName, szToFind)) {
            return pFuncDecl;
        }
    }

    return NULL;
}

static FuncDecl* appendFunc(Context* pContext, const char* szName, int iNumParams) {
    FuncDecl* pFuncDecl = createFuncDecl(szName, iNumParams);
    pFuncDecl->iIndex = pContext->pListFunctions->size;
    vlPushBack(pContext->pListFunctions, pFuncDecl);
    return pFuncDecl;
}

static KBool contextIsInFunc(Context* pContext) {
    return pContext->pCurrentFunc != NULL;
}

static int contextGetOpCodeListSize(const Context* pContext) {
    return pContext->pListOpCodes->size;
}

static VarDecl* findVar(Context* pContext, KBool bIsLocal, const char* szName) {
    Vlist*      pListVar    = bIsLocal ? pContext->pCurrentFunc->pListVariables : pContext->pListGlobalVariables;
    VlistNode*  pListNode   = NULL;
    for (pListNode = pListVar->head; pListNode; pListNode = pListNode->next) {
        VarDecl* pVarDecl = (VarDecl *)pListNode->data;
        if (IsStringEqual(pVarDecl->szVarName, szName)) {
            return pVarDecl;
        }
    }
    return NULL;
}

static VarDecl* appendVar(Context* pContext, KBool bIsLocal, const char* szName, VarDeclTypeId iVarType) {
    Vlist* pListVar = bIsLocal ? pContext->pCurrentFunc->pListVariables : pContext->pListGlobalVariables;
    VarDecl* pVarDecl = createVarDecl(szName, iVarType);
    pVarDecl->iIndex = pListVar->size;
    vlPushBack(pListVar, pVarDecl);
    return pVarDecl;
}

static GotoLabel* findLabel(Context* pContext,const char* szLabel) {
    VlistNode* pListNode = NULL;
    for (pListNode = pContext->pListLabels->head; pListNode; pListNode = pListNode->next) {
        GotoLabel* pLabel = (GotoLabel *)pListNode->data;
        if (IsStringEqual(pLabel->szLabelName, szLabel)) {
            return pLabel;
        }
    }
    return NULL;
}

static GotoLabel* appendLabel(Context* pContext,const char* szLabel) {
    GotoLabel* pLabel = (GotoLabel *)malloc(sizeof(GotoLabel));
    
    StringCopy(pLabel->szLabelName, sizeof(pLabel->szLabelName), szLabel);
    pLabel->iOpCodePos = -1;
    pLabel->pFuncDeclScope = pContext->pCurrentFunc;
    vlPushBack(pContext->pListLabels, pLabel);

    return pLabel;
}

static CtrlFlowLabel* getCtrlLabel(Context* pContext, const AstNode* pAstNode) {
    return &pContext->pCtrlFlowLabels[pAstNode->iControlId - 1];
}

static OpCode* appendOpCodePushNum(Context* pContext, KFloat fLiteral) {
    OpCode* pOpCode = (OpCode *)malloc(sizeof(OpCode));
    memset(pOpCode, 0, sizeof(OpCode));

    pOpCode->dwOpCodeId         = K_OPCODE_PUSH_NUM;
    pOpCode->uParam.fLiteral    = fLiteral;
    vlPushBack(pContext->pListOpCodes, pOpCode);

    return pOpCode;
}

static OpCode* appendOpCodePushStr(Context* pContext, KDword dwStringPoolPos) {
    OpCode* pOpCode = (OpCode *)malloc(sizeof(OpCode));
    memset(pOpCode, 0, sizeof(OpCode));

    pOpCode->dwOpCodeId                 = K_OPCODE_PUSH_STR;
    pOpCode->uParam.dwStringPoolPos     = dwStringPoolPos;
    vlPushBack(pContext->pListOpCodes, pOpCode);
    
    return pOpCode;
}

static OpCode* appendOpCodeOperator(Context* pContext, OpCodeId iOpCodeId, OperatorId iOperatorId) {
    OpCode* pOpCode = (OpCode *)malloc(sizeof(OpCode));
    memset(pOpCode, 0, sizeof(OpCode));

    pOpCode->dwOpCodeId             = iOpCodeId;
    pOpCode->uParam.dwOperatorId    = iOperatorId;
    vlPushBack(pContext->pListOpCodes, pOpCode);

    return pOpCode;
}

static OpCode* appendOpCodeNoParam(Context* pContext, OpCodeId iOpCodeId) {
    OpCode* pOpCode = (OpCode *)malloc(sizeof(OpCode));
    memset(pOpCode, 0, sizeof(OpCode));

    pOpCode->dwOpCodeId = iOpCodeId;
    vlPushBack(pContext->pListOpCodes, pOpCode);

    return pOpCode;
}

static OpCode* appendOpCodeVarReadOrWrite(Context* pContext, OpCodeId iOpCodeId, KBool bIsLocal, int iVarIndex) {
    OpCode* pOpCode = (OpCode *)malloc(sizeof(OpCode));
    memset(pOpCode, 0, sizeof(OpCode));

    pOpCode->dwOpCodeId                  = iOpCodeId;
    pOpCode->uParam.sVarAccess.wIsLocal  = bIsLocal ? 1 : 0;
    pOpCode->uParam.sVarAccess.wVarIndex = iVarIndex;
    vlPushBack(pContext->pListOpCodes, pOpCode);

    return pOpCode;
}

static OpCode* appendOpCodeCallBuiltIn(Context* pContext, int iBuiltFuncId) {
    OpCode* pOpCode = (OpCode *)malloc(sizeof(OpCode));
    memset(pOpCode, 0, sizeof(OpCode));

    pOpCode->dwOpCodeId             = K_OPCODE_CALL_BUILT_IN;
    pOpCode->uParam.dwBuiltFuncId   = iBuiltFuncId;
    vlPushBack(pContext->pListOpCodes, pOpCode);

    return pOpCode;
}

static OpCode* appendOpCodeCallFunction(Context* pContext, int iFuncIndex) {
    OpCode* pOpCode = (OpCode *)malloc(sizeof(OpCode));
    memset(pOpCode, 0, sizeof(OpCode));

    pOpCode->dwOpCodeId         = K_OPCODE_CALL_FUNC;
    pOpCode->uParam.dwFuncIndex = iFuncIndex;
    vlPushBack(pContext->pListOpCodes, pOpCode);

    return pOpCode;
}

static OpCode* appendOpCodeGoto(Context* pContext, OpCodeId iOpCodeId, KLabelOpCodePos* pIntOpCodePos) {
    OpCode* pOpCode = (OpCode *)malloc(sizeof(OpCode));
    memset(pOpCode, 0, sizeof(OpCode));

    pOpCode->dwOpCodeId             = iOpCodeId;
    pOpCode->uParam.pLabelOpCodePos = pIntOpCodePos;
    vlPushBack(pContext->pListOpCodes, pOpCode);

    return pOpCode;
}

static SemanticErrorId buildExpression(
    Context*        pContext,
    const AstNode*  pAstNode
) {
    switch (pAstNode->iType) {
        default:
            return SEM_UNRECOGNIZED_AST;
        case AST_UNARY_OPERATOR: {
            SemanticErrorId iSemErrorId;
            /* 编译操作数 */
            iSemErrorId = buildExpression(pContext, pAstNode->uData.sUnaryOperator.pAstOperand);
            if (iSemErrorId != SEM_NO_ERROR) return iSemErrorId;
            /* 添加操作符 opcode */
            appendOpCodeOperator(pContext, K_OPCODE_UNARY_OPERATOR, pAstNode->uData.sUnaryOperator.iOperatorId);
            break;
        }
        case AST_BINARY_OPERATOR: {
            SemanticErrorId iSemErrorId;
            /* 编译左操作数 */
            iSemErrorId = buildExpression(pContext, pAstNode->uData.sBinaryOperator.pAstLeftOperand);
            if (iSemErrorId != SEM_NO_ERROR) return iSemErrorId;
            /* 编译右操作数 */
            iSemErrorId = buildExpression(pContext, pAstNode->uData.sBinaryOperator.pAstRightOperand);
            if (iSemErrorId != SEM_NO_ERROR) return iSemErrorId;
            /* 添加操作符 opcode */
            appendOpCodeOperator(pContext, K_OPCODE_BINARY_OPERATOR, pAstNode->uData.sBinaryOperator.iOperatorId);
            break;
        }
        case AST_PAREN: {
            return buildExpression(pContext, pAstNode->uData.sParen.pAstExpr);
            break;
        }
        case AST_LITERAL_NUMERIC: {
            appendOpCodePushNum(pContext, pAstNode->uData.sLiteralNumeric.fValue);
            break;
        }
        case AST_LITERAL_STRING: {
            int iDestLen = StringLength(pAstNode->uData.sLiteralString.szValue) + 1;
            int iStringPoolPos = pContext->iStringPoolSize;
            /* 检查是否超出了字符串池的最大限制 */
            if (iDestLen + iStringPoolPos >= KB_CONTEXT_STRING_POOL_MAX) {
                return SEM_STR_POOL_EXCEED;
            }
            /* 写入字符串池 */
            StringCopy(pContext->szStringPool + iStringPoolPos, KB_CONTEXT_STRING_POOL_MAX - iStringPoolPos, pAstNode->uData.sLiteralString.szValue);
            pContext->iStringPoolSize += iDestLen;
            /* 添加 opcode */
            appendOpCodePushStr(pContext, iStringPoolPos);
            break;
        }
        case AST_VARIABLE: {
            const char* szVarName   = pAstNode->uData.sVariable.szName;
            KBool       bIsLocal    = contextIsInFunc(pContext);
            VarDecl*    pVarDecl    = NULL;
            /* 寻找指定变量 */
            pVarDecl = findVar(pContext, bIsLocal, szVarName);
            /* 局部没找到，全局寻找 */
            if (bIsLocal && !pVarDecl) {
                bIsLocal = KB_FALSE;
                pVarDecl = findVar(pContext, bIsLocal, szVarName);
            }
            if (!pVarDecl) {
                return SEM_VAR_NOT_FOUND;
            }
            /* 创建 opcode */
            appendOpCodeVarReadOrWrite(pContext, K_OPCODE_PUSH_VAR, bIsLocal, pVarDecl->iIndex);
            break;
        }
        case AST_ARRAY_ACCESS: {
            SemanticErrorId iSemErrorId; 
            const char* szArrayName = pAstNode->uData.sArrayAccess.szArrayName;
            KBool       bIsLocal    = contextIsInFunc(pContext);
            VarDecl*    pVarDecl    = NULL;
            /* 寻找指定变量 */
            pVarDecl = findVar(pContext, bIsLocal, szArrayName);
            /* 局部没找到，全局寻找 */
            if (bIsLocal && !pVarDecl) {
                bIsLocal = KB_FALSE;
                pVarDecl = findVar(pContext, bIsLocal, szArrayName);
            }
            if (!pVarDecl) {
                return SEM_VAR_NOT_FOUND;
            }
            /* 检查变量是否声明为数组 */
            if (pVarDecl->iType != VARDECL_ARRAY) {
                return SEM_VAR_IS_NOT_ARRAY;
            }
            /* 编译下标 */
            iSemErrorId = buildExpression(pContext, pAstNode->uData.sArrayAccess.pAstSubscript);
            if (iSemErrorId != SEM_NO_ERROR) return iSemErrorId;
            /* 创建 opcode */
            appendOpCodeVarReadOrWrite(pContext, K_OPCODE_ARR_GET, bIsLocal, pVarDecl->iIndex);
            break;
        }
        case AST_FUNCTION_CALL: {
            const char*         szFuncName  = pAstNode->uData.sFunctionCall.szFunction;
            int                 iNumArg     = pAstNode->uData.sFunctionCall.pListArguments->size;
            const BuiltInFunc*  pBuiltFunc  = NULL;
            FuncDecl*           pFuncDecl   = NULL;
            VlistNode*          pListNode   = NULL;
            /* 先查找是否为用户定义的函数 */
            pFuncDecl = findFunc(pContext, szFuncName);
            if (pFuncDecl) {
                /* 参数数量不匹配 */
                if (iNumArg != pFuncDecl->iNumParams) {
                    return SEM_FUNC_ARG_LIST_MISMATCH;
                }
            }
            /* 查找是否为系统函数 */
            else {
                pBuiltFunc = findBuiltInFunc(szFuncName);
                /* 找不到 */
                if (!pBuiltFunc) {
                    return SEM_FUNC_NOT_FOUND;
                }
                /* 参数数量不匹配 */
                if (iNumArg != pBuiltFunc->iNumParams) {
                    return SEM_FUNC_ARG_LIST_MISMATCH;
                }
            }
            /* 编译参数列表*/
            for (
                pListNode = pAstNode->uData.sFunctionCall.pListArguments->head;
                pListNode != NULL;
                pListNode = pListNode->next
            ) {
                AstNode* pAstArg = (AstNode *)pListNode->data;
                SemanticErrorId iSemErrorId = buildExpression(pContext, pAstArg);
                if (iSemErrorId != SEM_NO_ERROR) {
                    return iSemErrorId;
                }
            }
            /* 创建函数调用 opCode */
            if (pFuncDecl) {
                appendOpCodeCallFunction(pContext, pFuncDecl->iIndex);
            }
            else {
                appendOpCodeCallBuiltIn(pContext, pBuiltFunc->iFuncId);
            }
            break;
        }
    }
    return SEM_NO_ERROR;
}

#define returnStatementError(semErrId, pAstStop) {  \
    *pPtrAstStop = (pAstStop);                      \
    *pIntSemanticError = (semErrId);                \
    return KB_FALSE;                                \
} NULL

KBool buildStatements(
    Context*            pContext,
    const Vlist*        pListStatements, /* <AstNode> */
    SemanticErrorId*    pIntSemanticError,
    const AstNode**     pPtrAstStop
) {
    const VlistNode*    pListNode   = NULL;
    const AstNode*      pAstNode    = NULL;
    /* 扫描 Statements 下的所有语句*/
    for (pListNode = pListStatements->head; pListNode; pListNode = pListNode->next) {
        pAstNode = (const AstNode *)pListNode->data;
        switch (pAstNode->iType) {
            default:
            case AST_PROGRAM: {
                returnStatementError(SEM_UNRECOGNIZED_AST, pAstNode);
                break;
            }
            case AST_EMPTY: {
                break;
            }
            case AST_FUNCTION_DECLARE: {
                const char*         szFuncName = pAstNode->uData.sFunctionDeclare.szFunction;
                const VlistNode*    pListVarNode = NULL;
                KBool               bSuccess;
                CtrlFlowLabel*      pCtrlLabel = getCtrlLabel(pContext, pAstNode);
                FuncDecl*           pFuncDecl;
                OpCode*             pOpCodeTail;
                /* 设置上下文的当前函数 */
                pFuncDecl = findFunc(pContext, szFuncName);
                pContext->pCurrentFunc = pFuncDecl;
                /* 添加一个 GOTO 无条件跳转到函数结束 */
                appendOpCodeGoto(pContext, K_OPCODE_GOTO, &pCtrlLabel->uData.sFunction.iEndPos);
                /* 更新函数定义的 opCode 起点位置 */
                pFuncDecl->iOpCodeStartPos = contextGetOpCodeListSize(pContext);
                /* 遍历每一个函数的参数 */
                for (
                    pListVarNode = pAstNode->uData.sFunctionDeclare.pListParameters->head;
                    pListVarNode != NULL;
                    pListVarNode = pListVarNode->next
                ) {
                    const char* szParamName = (const char *)pListVarNode->data;
                    /* 检查参数名字长度 */
                    if (StringLength(szParamName) > KB_IDENTIFIER_LEN_MAX) {
                        returnStatementError(SEM_VAR_NAME_TOO_LONG, pAstNode);
                    }
                    /* 检查变量是否名称重复 */
                    if (findVar(pContext, KB_TRUE, szParamName)) {
                        returnStatementError(SEM_VAR_DUPLICATED, pAstNode);
                    }
                    /* 参数作为变量加入函数的局部变量列表 */
                    appendVar(pContext, KB_TRUE, szParamName, VARDECL_PRIMITIVE);
                }
                /* 编译函数的所有语句 */
                bSuccess = buildStatements(
                    pContext,
                    pAstNode->uData.sFunctionDeclare.pListStatements,
                    pIntSemanticError,
                    pPtrAstStop
                );
                if (!bSuccess) return KB_FALSE;
                /* 检查 opCode 最后一条是不是 RETURN */
                pOpCodeTail = (OpCode *)pContext->pListOpCodes->tail->data;
                if (pOpCodeTail->dwOpCodeId != K_OPCODE_RETURN) {
                    /* 添加一个 return 0 */
                    appendOpCodePushNum(pContext, 0);
                    appendOpCodeNoParam(pContext, K_OPCODE_RETURN);
                }
                /* 清除上下文的当前函数 */
                pContext->pCurrentFunc = NULL;
                /* 更新 end func 的位置 */
                pCtrlLabel->uData.sFunction.iEndPos = contextGetOpCodeListSize(pContext);
                break;
            }
            case AST_IF_GOTO: {
                SemanticErrorId iBuildExprErrorId;
                GotoLabel* pLabel = findLabel(pContext, pAstNode->uData.sIfGoto.szLabelName);
                if (!pLabel) {
                    returnStatementError(SEM_GOTO_LABEL_NOT_FOUND, pAstNode);
                }
                if (pLabel->pFuncDeclScope != pContext->pCurrentFunc) {
                    returnStatementError(SEM_GOTO_LABEL_SCOPE_MISMATCH, pAstNode);
                }
                /* 编译条件表达式 */
                iBuildExprErrorId = buildExpression(pContext, pAstNode->uData.sIfGoto.pAstCondition);
                if (iBuildExprErrorId != SEM_NO_ERROR) {
                    returnStatementError(iBuildExprErrorId, pAstNode);
                }
                /* 创建 goto opcode */
                appendOpCodeGoto(pContext, K_OPCODE_IF_GOTO, &pLabel->iOpCodePos);
                break;
            }
            case AST_IF: {
                SemanticErrorId iBuildExprErrorId;
                CtrlFlowLabel*  pCtrlLabel = getCtrlLabel(pContext, pAstNode);
                VlistNode*      pListNodeElseIf = NULL;
                KBool           bSuccess;
                int             iElseIfIndex;
                /* 编译 if 条件 */
                iBuildExprErrorId = buildExpression(pContext, pAstNode->uData.sIf.pAstCondition);
                if (iBuildExprErrorId != SEM_NO_ERROR) {
                    returnStatementError(iBuildExprErrorId, pAstNode);
                }
                /* 添加一个 UNLESS_GOTO，不符合的条件跳过 then 部分 */
                appendOpCodeGoto(pContext, K_OPCODE_UNLESS_GOTO, &pCtrlLabel->uData.sIf.iThenEndPos);
                /* 编译 then 部分 */
                bSuccess = buildStatements(
                    pContext,
                    pAstNode->uData.sIf.pAstThen->uData.sThen.pListStatements,
                    pIntSemanticError,
                    pPtrAstStop
                );
                if (!bSuccess) return KB_FALSE;
                /* 执行完 then 部分，跳转到 end if 标签 */
                appendOpCodeGoto(pContext, K_OPCODE_GOTO, &pCtrlLabel->uData.sIf.iEndPos);
                /* 更新 then 的结束位置标签 */
                pCtrlLabel->uData.sIf.iThenEndPos = contextGetOpCodeListSize(pContext);
                /* 编译 elseif 部分 */
                for (
                    iElseIfIndex = 0, pListNodeElseIf = pAstNode->uData.sIf.pListElseIf->head;
                    pListNodeElseIf != NULL;
                    iElseIfIndex++, pListNodeElseIf = pListNodeElseIf->next
                ) {
                    const AstNode* pAstElseIf = (const AstNode* )pListNodeElseIf->data;
                    /* 编译 elseif 条件 */
                    iBuildExprErrorId = buildExpression(pContext, pAstElseIf->uData.sElseIf.pAstCondition);
                    if (iBuildExprErrorId != SEM_NO_ERROR) {
                        returnStatementError(iBuildExprErrorId, pAstNode);
                    }
                    /* 添加一个 UNLESS_GOTO，不符合的条件跳过此 elseif 部分 */
                    appendOpCodeGoto(pContext, K_OPCODE_UNLESS_GOTO, &pCtrlLabel->uData.sIf.pArrElseIfEndPos[iElseIfIndex]);
                    /* 编译 elseif 的语句 */
                    bSuccess = buildStatements(
                        pContext,
                        pAstElseIf->uData.sElseIf.pListStatements,
                        pIntSemanticError,
                        pPtrAstStop
                    );
                    if (!bSuccess) return KB_FALSE;
                    /* 执行完 elseif 部分，跳转到 end if 标签 */
                    appendOpCodeGoto(pContext, K_OPCODE_GOTO, &pCtrlLabel->uData.sIf.iEndPos);
                    /* 更新此 elseif 的结束位置标签 */
                    pCtrlLabel->uData.sIf.pArrElseIfEndPos[iElseIfIndex] = contextGetOpCodeListSize(pContext);
                }
                /* 编译 else 部分 */
                if (pAstNode->uData.sIf.pAstElse) {
                    bSuccess = buildStatements(
                        pContext,
                        pAstNode->uData.sIf.pAstElse->uData.sElse.pListStatements,
                        pIntSemanticError,
                        pPtrAstStop
                    );
                    if (!bSuccess) return KB_FALSE;
                }
                /* 更新 end if 的标签位置 */
                pCtrlLabel->uData.sIf.iEndPos = contextGetOpCodeListSize(pContext);
                break;
            }
            case AST_THEN: {
                /* 在 AST_IF 中处理了 */
                break;
            }
            case AST_ELSEIF:{
                /* 在 AST_IF 中处理了 */
                break;
            }
            case AST_ELSE:{
                /* 在 AST_IF 中处理了 */
                break;
            }
            case AST_WHILE: {
                SemanticErrorId iBuildExprErrorId;
                CtrlFlowLabel*  pCtrlLabel = getCtrlLabel(pContext, pAstNode);
                KBool           bSuccess;
                /* 更新条件判断的位置 */
                pCtrlLabel->uData.sWhile.iCondPos = contextGetOpCodeListSize(pContext);
                /* 编译 while 条件 */
                iBuildExprErrorId = buildExpression(pContext, pAstNode->uData.sWhile.pAstCondition);
                if (iBuildExprErrorId != SEM_NO_ERROR) {
                    returnStatementError(iBuildExprErrorId, pAstNode);
                }
                /* 添加一个 UNLESS_GOTO，不符合的条件跳过 while */
                appendOpCodeGoto(pContext, K_OPCODE_UNLESS_GOTO, &pCtrlLabel->uData.sWhile.iEndPos);
                /* 编译 while 的所有语句 */
                bSuccess = buildStatements(
                    pContext,
                    pAstNode->uData.sWhile.pListStatements,
                    pIntSemanticError,
                    pPtrAstStop
                );
                if (!bSuccess) return KB_FALSE;
                /* 添加一个 opcode 跳转到 while 的条件 */
                appendOpCodeGoto(pContext, K_OPCODE_GOTO, &pCtrlLabel->uData.sWhile.iCondPos);
                /* 更新 end while 的标签位置 */
                pCtrlLabel->uData.sWhile.iEndPos = contextGetOpCodeListSize(pContext);
                break;
            }
            case AST_DO_WHILE: {
                SemanticErrorId iBuildExprErrorId;
                CtrlFlowLabel*  pCtrlLabel = getCtrlLabel(pContext, pAstNode);
                KBool           bSuccess;
                /* 更新循环开始的位置 */
                pCtrlLabel->uData.sDoWhile.iStartPos = contextGetOpCodeListSize(pContext);
                /* 编译 do...while 的所有语句 */
                bSuccess = buildStatements(
                    pContext,
                    pAstNode->uData.sDoWhile.pListStatements,
                    pIntSemanticError,
                    pPtrAstStop
                );
                if (!bSuccess) return KB_FALSE;
                /* 更新循环条件的位置 */
                pCtrlLabel->uData.sDoWhile.iCondPos = contextGetOpCodeListSize(pContext);
                /* 编译 do...while 条件 */
                iBuildExprErrorId = buildExpression(pContext, pAstNode->uData.sDoWhile.pAstCondition);
                if (iBuildExprErrorId != SEM_NO_ERROR) {
                    returnStatementError(iBuildExprErrorId, pAstNode);
                }
                /* 添加一个 IF_GOTO，条件为真，回到循环开头 */
                appendOpCodeGoto(pContext, K_OPCODE_IF_GOTO, &pCtrlLabel->uData.sDoWhile.iStartPos);
                /* 更新 do..while 结束的标签位置 */
                pCtrlLabel->uData.sDoWhile.iEndPos = contextGetOpCodeListSize(pContext);
                break;
            }
            case AST_FOR: {
                SemanticErrorId iBuildExprErrorId;
                CtrlFlowLabel*  pCtrlLabel  = getCtrlLabel(pContext, pAstNode);
                const char*     szVarName   = pAstNode->uData.sFor.szVariable;
                KBool           bIsLocal    = contextIsInFunc(pContext);
                KBool           bSuccess    = KB_FALSE;
                VarDecl*        pVarDecl    = NULL;
                /* 寻找指定变量 */
                pVarDecl = findVar(pContext, bIsLocal, szVarName);
                /* 局部没找到，全局寻找 */
                if (bIsLocal && !pVarDecl) {
                    bIsLocal = KB_FALSE;
                    pVarDecl = findVar(pContext, bIsLocal, szVarName);
                }
                if (!pVarDecl) {
                    returnStatementError(SEM_VAR_NOT_FOUND, pAstNode);
                }
                /* 变量不是基础类型 */
                if (pVarDecl->iType != VARDECL_PRIMITIVE) {
                    returnStatementError(SEM_VAR_IS_NOT_PRIMITIVE, pAstNode);
                }
                /* 编译 for 表达式 : 初始赋值 */
                iBuildExprErrorId = buildExpression(pContext, pAstNode->uData.sFor.pAstRangeFrom);
                if (iBuildExprErrorId != SEM_NO_ERROR) {
                    returnStatementError(iBuildExprErrorId, pAstNode);
                }
                appendOpCodeVarReadOrWrite(pContext, K_OPCODE_SET_VAR, bIsLocal, pVarDecl->iIndex);
                /* 更新标签循环条件的位置 */
                pCtrlLabel->uData.sFor.iCondPos = contextGetOpCodeListSize(pContext);
                /* 编译 for 表达式 : 循环条件 */
                iBuildExprErrorId = buildExpression(pContext, pAstNode->uData.sFor.pAstRangeTo);
                if (iBuildExprErrorId != SEM_NO_ERROR) {
                    returnStatementError(iBuildExprErrorId, pAstNode);
                }
                /* 添加比较指令 */
                appendOpCodeVarReadOrWrite(pContext, K_OPCODE_PUSH_VAR, bIsLocal, pVarDecl->iIndex);
                appendOpCodeOperator(pContext, K_OPCODE_BINARY_OPERATOR, OPR_GTEQ);
                /* 添加条件失败跳转结束的指令 */
                appendOpCodeGoto(pContext, K_OPCODE_UNLESS_GOTO, &pCtrlLabel->uData.sFor.iEndPos);
                /* 编译 for 的所有语句 */
                bSuccess = buildStatements(
                    pContext,
                    pAstNode->uData.sFor.pListStatements,
                    pIntSemanticError,
                    pPtrAstStop
                );
                if (!bSuccess) return KB_FALSE;
                /* 更新变量增长的标签位置 */
                pCtrlLabel->uData.sFor.iIncreasePos = contextGetOpCodeListSize(pContext);
                /* 编译 for 表达式 : 变量增长 */
                if (pAstNode->uData.sFor.pAstStep) {
                    /* 有 step 表达式 */
                    iBuildExprErrorId = buildExpression(pContext, pAstNode->uData.sFor.pAstStep);
                    if (iBuildExprErrorId != SEM_NO_ERROR) {
                        returnStatementError(iBuildExprErrorId, pAstNode);
                    }
                }
                else {
                    /* 无 step 表达式，变量增长 1 */
                    appendOpCodePushNum(pContext, 1);
                }
                appendOpCodeVarReadOrWrite(pContext, K_OPCODE_PUSH_VAR, bIsLocal, pVarDecl->iIndex);
                appendOpCodeOperator(pContext, K_OPCODE_BINARY_OPERATOR, OPR_ADD);
                appendOpCodeVarReadOrWrite(pContext, K_OPCODE_SET_VAR, bIsLocal, pVarDecl->iIndex);
                /* 添加 opcode 跳转到条件比较 */
                appendOpCodeGoto(pContext, K_OPCODE_GOTO, &pCtrlLabel->uData.sFor.iCondPos);
                /* 更新 next 的标签位置 */
                pCtrlLabel->uData.sFor.iEndPos = contextGetOpCodeListSize(pContext);
                break;
            }
            case AST_BREAK: {
                AstNode*        pAstFindLoop    = NULL;
                CtrlFlowLabel*  pCtrlLabel      = NULL;
                /* 向上寻找最近的循环 */
                for (
                    pAstFindLoop = pAstNode->pAstParent;
                    pAstFindLoop != NULL;
                    pAstFindLoop = pAstFindLoop->pAstParent
                ) {
                    if (pAstFindLoop->iType == AST_FOR || pAstFindLoop->iType == AST_WHILE  || pAstFindLoop->iType == AST_DO_WHILE) {
                        break;
                    }
                }
                pCtrlLabel = getCtrlLabel(pContext, pAstFindLoop);
                /* 添加一个 GOTO 指令跳转到循环结束 */
                switch (pCtrlLabel->iType) {
                    case CF_WHILE:
                        appendOpCodeGoto(pContext, K_OPCODE_GOTO, &pCtrlLabel->uData.sWhile.iEndPos);
                        break;
                    case CF_DO_WHILE:
                        appendOpCodeGoto(pContext, K_OPCODE_GOTO, &pCtrlLabel->uData.sDoWhile.iEndPos);
                        break;
                    case CF_FOR:
                        appendOpCodeGoto(pContext, K_OPCODE_GOTO, &pCtrlLabel->uData.sFor.iEndPos);
                        break;
                    default:
                        break;
                }
                break;
            }
            case AST_CONTINUE: {
                AstNode*        pAstFindLoop    = NULL;
                CtrlFlowLabel*  pCtrlLabel      = NULL;
                /* 向上寻找最近的循环 */
                for (
                    pAstFindLoop = pAstNode->pAstParent;
                    pAstFindLoop != NULL;
                    pAstFindLoop = pAstFindLoop->pAstParent
                ) {
                    if (pAstFindLoop->iType == AST_FOR || pAstFindLoop->iType == AST_WHILE || pAstFindLoop->iType == AST_DO_WHILE) {
                        break;
                    }
                }
                pCtrlLabel = getCtrlLabel(pContext, pAstFindLoop);
                /* 添加一个 GOTO 指令跳转到下一次循环 */
                switch (pCtrlLabel->iType) {
                    case CF_WHILE:
                        appendOpCodeGoto(pContext, K_OPCODE_GOTO, &pCtrlLabel->uData.sWhile.iCondPos);
                        break;
                    case CF_DO_WHILE:
                        appendOpCodeGoto(pContext, K_OPCODE_GOTO, &pCtrlLabel->uData.sDoWhile.iCondPos);
                        break;
                    case CF_FOR:
                        appendOpCodeGoto(pContext, K_OPCODE_GOTO, &pCtrlLabel->uData.sFor.iIncreasePos);
                        break;
                    default:
                        break;
                }
                break;
            }
            case AST_EXIT: {
                SemanticErrorId iBuildExprErrorId;
                /* 带返回值的情况，编译表达式 */
                if (pAstNode->uData.sExit.pAstExpression) {
                    iBuildExprErrorId = buildExpression(pContext, pAstNode->uData.sExit.pAstExpression);
                    if (iBuildExprErrorId != SEM_NO_ERROR) {
                        returnStatementError(iBuildExprErrorId, pAstNode);
                    }
                }
                /* 不带返回值，返回 0 */
                else {
                    appendOpCodePushNum(pContext, 0);
                }
                /* 创建 return opcode */
                appendOpCodeNoParam(pContext, K_OPCODE_STOP);
                break;
            }
            case AST_RETURN: {
                SemanticErrorId iBuildExprErrorId;
                /* 带返回值的情况，编译表达式 */
                if (pAstNode->uData.sReturn.pAstExpression) {
                    iBuildExprErrorId = buildExpression(pContext, pAstNode->uData.sReturn.pAstExpression);
                    if (iBuildExprErrorId != SEM_NO_ERROR) {
                        returnStatementError(iBuildExprErrorId, pAstNode);
                    }
                }
                /* 不带返回值，返回 0 */
                else {
                    appendOpCodePushNum(pContext, 0);
                }
                /* 创建 return opcode */
                appendOpCodeNoParam(pContext, K_OPCODE_RETURN);
                break;
            }
            case AST_GOTO: {
                GotoLabel* pLabel = findLabel(pContext, pAstNode->uData.sGoto.szLabelName);
                if (!pLabel) {
                    returnStatementError(SEM_GOTO_LABEL_NOT_FOUND, pAstNode);
                }
                if (pLabel->pFuncDeclScope != pContext->pCurrentFunc) {
                    returnStatementError(SEM_GOTO_LABEL_SCOPE_MISMATCH, pAstNode);
                }
                /* 创建 goto opcode */
                appendOpCodeGoto(pContext, K_OPCODE_GOTO, &pLabel->iOpCodePos);
                break;
            }
            case AST_DIM: {
                KBool       bIsLocal    = contextIsInFunc(pContext);
                VarDecl*    pVarDecl    = NULL;
                const char* szName      = pAstNode->uData.sDim.szVariable;
                /* 检查变量名是否过长 */
                if (StringLength(szName) > KB_IDENTIFIER_LEN_MAX) {
                    returnStatementError(SEM_VAR_NAME_TOO_LONG, pAstNode);
                }
                /* 检查变量是否重复 */
                pVarDecl = findVar(pContext, bIsLocal, szName);
                if (pVarDecl != NULL) {
                    returnStatementError(SEM_VAR_DUPLICATED, pAstNode);
                }
                /* 创建新变量 */
                pVarDecl = appendVar(pContext, bIsLocal, szName, VARDECL_PRIMITIVE);
                /* 编译初始化表达式 */
                if (pAstNode->uData.sDim.pAstInitializer) {
                    SemanticErrorId iBuildExprErrorId;
                    iBuildExprErrorId = buildExpression(pContext, pAstNode->uData.sDim.pAstInitializer);
                    if (iBuildExprErrorId != SEM_NO_ERROR) {
                        returnStatementError(iBuildExprErrorId, pAstNode);
                    }
                    /* 创建一个赋值的 opcode */
                    appendOpCodeVarReadOrWrite(pContext, K_OPCODE_SET_VAR, bIsLocal, pVarDecl->iIndex);
                }
                break;
            }
            case AST_DIM_ARRAY: {
                SemanticErrorId  iBuildExprErrorId;
                KBool       bIsLocal    = contextIsInFunc(pContext);
                VarDecl*    pVarDecl    = NULL;
                const char* szName      = pAstNode->uData.sDimArray.szArrayName;
                /* 检查变量名是否过长 */
                if (StringLength(szName) > KB_IDENTIFIER_LEN_MAX) {
                    returnStatementError(SEM_VAR_NAME_TOO_LONG, pAstNode);
                }
                /* 检查变量是否重复 */
                pVarDecl = findVar(pContext, bIsLocal, szName);
                if (pVarDecl != NULL) {
                    returnStatementError(SEM_VAR_DUPLICATED, pAstNode);
                }
                /* 创建新变量 */
                pVarDecl = appendVar(pContext, bIsLocal, szName, VARDECL_ARRAY);
                /* 编译数组尺寸表达式 */
                iBuildExprErrorId = buildExpression(pContext, pAstNode->uData.sDimArray.pAstDimension);
                if (iBuildExprErrorId != SEM_NO_ERROR) {
                    returnStatementError(iBuildExprErrorId, pAstNode);
                }
                /* 创建一个设置变量为数组 opcode */
                appendOpCodeVarReadOrWrite(pContext, K_OPCODE_SET_VAR_AS_ARRAY, bIsLocal, pVarDecl->iIndex);
                break;
            }
            case AST_REDIM: {
                SemanticErrorId  iBuildExprErrorId;
                const char* szVarName   = pAstNode->uData.sRedim.szArrayName;
                KBool       bIsLocal    = contextIsInFunc(pContext);
                VarDecl*    pVarDecl    = NULL;
                /* 寻找指定变量 */
                pVarDecl = findVar(pContext, bIsLocal, szVarName);
                /* 局部没找到，全局寻找 */
                if (bIsLocal && !pVarDecl) {
                    bIsLocal = KB_FALSE;
                    pVarDecl = findVar(pContext, bIsLocal, szVarName);
                }
                if (!pVarDecl) {
                    returnStatementError(SEM_VAR_NOT_FOUND, pAstNode);
                }
                /* 不是数组 */
                if (pVarDecl->iType != VARDECL_ARRAY) {
                    returnStatementError(SEM_VAR_IS_NOT_ARRAY, pAstNode);
                }
                /* 编译数组尺寸表达式 */
                iBuildExprErrorId = buildExpression(pContext, pAstNode->uData.sRedim.pAstDimension);
                if (iBuildExprErrorId != SEM_NO_ERROR) {
                    returnStatementError(iBuildExprErrorId, pAstNode);
                }
                /* 创建一个设置变量为数组 opcode */
                appendOpCodeVarReadOrWrite(pContext, K_OPCODE_SET_VAR_AS_ARRAY, bIsLocal, pVarDecl->iIndex);
                break;
            }
            case AST_ASSIGN: {
                SemanticErrorId iBuildExprErrorId;
                const char* szVarName   = pAstNode->uData.sAssign.szVariable;
                KBool       bIsLocal    = contextIsInFunc(pContext);
                VarDecl*    pVarDecl    = NULL;
                /* 寻找指定变量 */
                pVarDecl = findVar(pContext, bIsLocal, szVarName);
                /* 局部没找到，全局寻找 */
                if (bIsLocal && !pVarDecl) {
                    bIsLocal = KB_FALSE;
                    pVarDecl = findVar(pContext, bIsLocal, szVarName);
                }
                if (!pVarDecl) {
                    returnStatementError(SEM_VAR_NOT_FOUND, pAstNode);
                }
                /* 变量不是基础类型 */
                if (pVarDecl->iType != VARDECL_PRIMITIVE) {
                    returnStatementError(SEM_VAR_IS_NOT_PRIMITIVE, pAstNode);
                }
                /* 编译右值 */
                iBuildExprErrorId = buildExpression(pContext, pAstNode->uData.sAssign.pAstValue);
                if (iBuildExprErrorId != SEM_NO_ERROR) {
                    returnStatementError(iBuildExprErrorId, pAstNode);
                }
                /* 创建一个赋值的 opcode */
                appendOpCodeVarReadOrWrite(pContext, K_OPCODE_SET_VAR, bIsLocal, pVarDecl->iIndex);
                break;
            }
            case AST_ASSIGN_ARRAY: {
                SemanticErrorId iBuildExprErrorId;
                const char* szVarName   = pAstNode->uData.sAssignArray.szArrayName;
                KBool       bIsLocal    = contextIsInFunc(pContext);
                VarDecl*    pVarDecl    = NULL;
                /* 寻找指定变量 */
                pVarDecl = findVar(pContext, bIsLocal, szVarName);
                /* 局部没找到，全局寻找 */
                if (bIsLocal && !pVarDecl) {
                    bIsLocal = KB_FALSE;
                    pVarDecl = findVar(pContext, bIsLocal, szVarName);
                }
                if (!pVarDecl) {
                    returnStatementError(SEM_VAR_NOT_FOUND, pAstNode);
                }
                if (pVarDecl->iType != VARDECL_ARRAY) {
                    returnStatementError(SEM_VAR_IS_NOT_ARRAY, pAstNode);
                }
                /* 编译下标 */
                iBuildExprErrorId = buildExpression(pContext, pAstNode->uData.sAssignArray.pAstSubscript);
                if (iBuildExprErrorId != SEM_NO_ERROR) {
                    returnStatementError(iBuildExprErrorId, pAstNode);
                }
                /* 编译右值 */
                iBuildExprErrorId = buildExpression(pContext, pAstNode->uData.sAssignArray.pAstValue);
                if (iBuildExprErrorId != SEM_NO_ERROR) {
                    returnStatementError(iBuildExprErrorId, pAstNode);
                }
                /* 创建一个赋值的 opcode */
                appendOpCodeVarReadOrWrite(pContext, K_OPCODE_ARR_SET, bIsLocal, pVarDecl->iIndex);
                break;
            }
            case AST_LABEL_DECLARE: {
                /* 更新跳转标签的opCode位置 */
                GotoLabel* pLabel = findLabel(pContext, pAstNode->uData.sLabel.szLabelName);
                pLabel->iOpCodePos = contextGetOpCodeListSize(pContext);
                break;
            }
            /* 以下是表达式 */
            case AST_UNARY_OPERATOR:
            case AST_BINARY_OPERATOR:
            case AST_PAREN:
            case AST_LITERAL_NUMERIC:
            case AST_LITERAL_STRING:
            case AST_VARIABLE:
            case AST_ARRAY_ACCESS:
            case AST_FUNCTION_CALL: {
                SemanticErrorId iBuildExprErrorId = buildExpression(pContext, pAstNode);
                if (iBuildExprErrorId != SEM_NO_ERROR) {
                    returnStatementError(iBuildExprErrorId, pAstNode);
                }
                /* 创建一个 pop opCode，释放表达式栈的残留值 */
                appendOpCodeNoParam(pContext, K_OPCODE_POP);
                break;
            }
        }
    }
    return KB_TRUE;
}

static KBool scanLabel(
    Context*            pContext,
    const Vlist*        pListStatements, /* <AstNode> */
    SemanticErrorId*    pIntSemanticError,
    const AstNode**     pPtrAstStop
) {
    const VlistNode*    pListNode   = NULL;
    const AstNode*      pAstNode    = NULL;
    KBool               bSuccess;

    /* 扫描 Statements 下的所有语句*/
    for (pListNode = pListStatements->head; pListNode; pListNode = pListNode->next) {
        pAstNode = (const AstNode *)pListNode->data;
        switch (pAstNode->iType) {
            case AST_LABEL_DECLARE: {
                const char* szLabelName = pAstNode->uData.sLabel.szLabelName;
                GotoLabel*  pLabel;
                /* 检查长度 */
                if (StringLength(szLabelName) > KB_IDENTIFIER_LEN_MAX) {
                    returnStatementError(SEM_LABEL_NAME_TOO_LONG, pAstNode);
                }
                /* 检查是否重复定义 */
                pLabel = findLabel(pContext, szLabelName);
                if (pLabel) {
                    returnStatementError(SEM_LABEL_DUPLICATED, pAstNode);
                }
                /* 创建标签 */
                pLabel = appendLabel(pContext, szLabelName);
                break;
            }
            case AST_FUNCTION_DECLARE: {
                /* 初始化控制结构标签 */
                initControlFlowLabel(getCtrlLabel(pContext, pAstNode), pAstNode);
                /* 设置上下文的当前函数 */
                pContext->pCurrentFunc = findFunc(pContext, pAstNode->uData.sFunctionDeclare.szFunction);
                /* 扫描函数的所有语句 */
                bSuccess = scanLabel(
                    pContext,
                    pAstNode->uData.sFunctionDeclare.pListStatements,
                    pIntSemanticError,
                    pPtrAstStop
                );
                if (!bSuccess) return KB_FALSE;
                pContext->pCurrentFunc = NULL;
                break;
            }
            case AST_IF: {
                const VlistNode* pListNodeElseIf;
                /* 初始化控制结构标签 */
                initControlFlowLabel(getCtrlLabel(pContext, pAstNode), pAstNode);
                /* 扫描 then 部分 */
                bSuccess = scanLabel(
                    pContext,
                    pAstNode->uData.sIf.pAstThen->uData.sThen.pListStatements,
                    pIntSemanticError,
                    pPtrAstStop
                );
                if (!bSuccess) return KB_FALSE;
                /* 扫描 elseif 部分 */
                for (
                    pListNodeElseIf = pAstNode->uData.sIf.pListElseIf->head;
                    pListNodeElseIf != NULL;
                    pListNodeElseIf = pListNodeElseIf->next
                ) {
                    const AstNode* pAstElseIf = (const AstNode* )pListNodeElseIf->data;
                    bSuccess = scanLabel(
                        pContext,
                        pAstElseIf->uData.sElseIf.pListStatements,
                        pIntSemanticError,
                        pPtrAstStop
                    );
                    if (!bSuccess) return KB_FALSE;
                }
                /* 扫描 else 部分 */
                if (pAstNode->uData.sIf.pAstElse) {
                    bSuccess = scanLabel(
                        pContext,
                        pAstNode->uData.sIf.pAstElse->uData.sElse.pListStatements,
                        pIntSemanticError,
                        pPtrAstStop
                    );
                    if (!bSuccess) return KB_FALSE;
                }
                break;
            }
            case AST_WHILE: {
                /* 初始化控制结构标签 */
                initControlFlowLabel(getCtrlLabel(pContext, pAstNode), pAstNode);
                /* 扫描 while 的所有语句 */
                bSuccess = scanLabel(
                    pContext,
                    pAstNode->uData.sWhile.pListStatements,
                    pIntSemanticError,
                    pPtrAstStop
                );
                if (!bSuccess) return KB_FALSE;
                break;
            }
            case AST_DO_WHILE: {
                /* 初始化控制结构标签 */
                initControlFlowLabel(getCtrlLabel(pContext, pAstNode), pAstNode);
                /* 扫描 while 的所有语句 */
                bSuccess = scanLabel(
                    pContext,
                    pAstNode->uData.sDoWhile.pListStatements,
                    pIntSemanticError,
                    pPtrAstStop
                );
                if (!bSuccess) return KB_FALSE;
                break;
            }
            case AST_FOR: {
                /* 初始化控制结构标签 */
                initControlFlowLabel(getCtrlLabel(pContext, pAstNode), pAstNode);
                /* 扫描 for 的所有语句 */
                bSuccess = scanLabel(
                    pContext,
                    pAstNode->uData.sFor.pListStatements,
                    pIntSemanticError,
                    pPtrAstStop
                );
                if (!bSuccess) return KB_FALSE;
                break;
            }
            default: {
                break;
            }
        }
    }
    return KB_TRUE;
}

#define returnProgramError(semErrId, pAstStop) {\
    *pPtrAstStop = (pAstStop);                  \
    *pIntSemanticError = (semErrId);            \
    return pContext;                            \
} NULL

KbCompilerContext* KompilerContext_Build(
    const KbAstNode*    pAstProgram,
    SemanticErrorId*    pIntSemanticError,
    const KbAstNode**   pPtrAstStop
) {
    Context*            pContext    = NULL;
    const VlistNode*    pListNode   = NULL;
    VlistNode*          pListNodeOp = NULL;
    KBool               bSuccess    = KB_TRUE;

    *pIntSemanticError = SEM_NO_ERROR;

    if (pAstProgram->iType != AST_PROGRAM) {
        returnProgramError(SEM_NOT_A_PROGRAM, pAstProgram);
    }

    pContext = initContext(pAstProgram);

    /* 扫描全部函数声明 */
    for (pListNode = pAstProgram->uData.sProgram.pListStatements->head; pListNode; pListNode = pListNode->next) {
        const AstNode* pAstNode = (const AstNode *)pListNode->data;
        const char* szFuncName;
        /* 跳过 */
        if (pAstNode->iType != AST_FUNCTION_DECLARE) {
            continue;
        }
        szFuncName = pAstNode->uData.sFunctionDeclare.szFunction;
        /* 检查是否函数名过长 */
        if (StringLength(szFuncName) > KB_IDENTIFIER_LEN_MAX) {
            returnProgramError(SEM_FUNC_NAME_TOO_LONG, pAstNode);
        }
        /* 检查是不是函数名重复 */
        if (findFunc(pContext, szFuncName) != NULL) {
            returnProgramError(SEM_FUNC_DUPLICATED, pAstNode);
        }
        /* 添加函数到上下文 */
        appendFunc(pContext, szFuncName, pAstNode->uData.sFunctionCall.pListArguments->size);
    }

    /* 扫描全部控制结构、跳转标签 */
    bSuccess = scanLabel(
        pContext,
        pAstProgram->uData.sProgram.pListStatements,
        pIntSemanticError,
        pPtrAstStop
    );
    if (!bSuccess) return pContext;

    /* 编译所有语句 */
    bSuccess = buildStatements(
        pContext,
        pAstProgram->uData.sProgram.pListStatements,
        pIntSemanticError,
        pPtrAstStop
    );
    if (!bSuccess) return pContext;

    /* 手动添加一个 STOP 命令 */
    appendOpCodePushNum(pContext, 0);
    appendOpCodeNoParam(pContext, K_OPCODE_STOP);

    /* 更新所有的 GOTO / IF_GOTO / UNLESS_GOTO opCode 的跳转位置 */
    for (pListNodeOp = pContext->pListOpCodes->head; pListNodeOp != NULL; pListNodeOp = pListNodeOp->next) {
        OpCode*     pOpCode = (OpCode *)pListNodeOp->data;
        const int*  pLabelOpCodePos  = NULL;
    
        switch (pOpCode->dwOpCodeId) {
            case K_OPCODE_GOTO:
            case K_OPCODE_IF_GOTO:
            case K_OPCODE_UNLESS_GOTO:
                pLabelOpCodePos = pOpCode->uParam.pLabelOpCodePos;
                pOpCode->uParam.dwOpCodePos = *pLabelOpCodePos;
        }
    }

    return pContext;
}

KBool KompilerContext_Serialize(
    const KbCompilerContext*    pContext,
    KByte**                     pPtrByteRaw,
    KDword*                     pDwRawLength
) {
/*
          序列化后的内存布局
    ---------- layout ----------   <--- 0
    |                          |
    |        * header *        |      + dwHeaderSize
    |                          |
    ----------------------------   <--- dwFuncBlockStart
    |                          |
    |         * func *         |      + dwByteLengthFunc
    |                          |
    ----------------------------   <--- dwOpCodeBlockStart
    |                          |
    |        * opcode *        |      + dwByteLengthOpCode
    |                          |
    ----------------------------   <--- dwStringPoolStart
    |                          |
    |     * string pool *      |      + dwStringAlignedLength
    |                          |
    ----------------------------
*/
    VlistNode*  pListNode   = NULL;
    int         i           = 0;

    KDword dwHeaderSize         = sizeof(BinHeader);
    KDword dwFuncBlockStart     = dwHeaderSize;
    KDword dwByteLengthFunc     = pContext->pListFunctions->size * sizeof(BinFuncInfo);
    KDword dwOpCodeBlockStart   = dwFuncBlockStart + dwByteLengthFunc;
    KDword dwByteLengthOpCode   = pContext->pListOpCodes->size * sizeof(OpCode);
    KDword dwStringPoolStart    = dwOpCodeBlockStart + dwByteLengthOpCode;
    KDword dwStringPoolLength   = pContext->iStringPoolSize;
    KDword dwStringAlignedSize  = dwStringPoolLength % 16 == 0 ? dwStringPoolLength : (dwStringPoolLength / 16 + 1) * 16;
    KDword dwSizeTotal          = dwHeaderSize + dwByteLengthFunc + dwByteLengthOpCode + dwStringAlignedSize;

    KByte*          pByteRaw        = (KByte *)malloc(dwSizeTotal);
    BinHeader*      pHeader         = (BinHeader *)pByteRaw;
    BinFuncInfo*    pWriterFunc     = NULL;
    OpCode*         pWriterOpCode   = NULL;
    char*           pWriterStrPool  = NULL;

    memset(pByteRaw, 0, dwSizeTotal);

    /* 写入文件头 */
    pHeader->uHeaderMagic.bVal[0]   = K_HEADER_MAGIC_BYTE_0;
    pHeader->uHeaderMagic.bVal[1]   = K_HEADER_MAGIC_BYTE_1;
    pHeader->uHeaderMagic.bVal[2]   = K_HEADER_MAGIC_BYTE_2;
    pHeader->uHeaderMagic.bVal[3]   = K_HEADER_MAGIC_BYTE_3;
    pHeader->dwIsLittleEndian       = isLittleEndian();
    pHeader->dwNumVariables         = pContext->pListGlobalVariables->size;
    pHeader->dwNumFunc              = pContext->pListFunctions->size;
    pHeader->dwFuncBlockStart       = dwFuncBlockStart;
    pHeader->dwOpCodeBlockStart     = dwOpCodeBlockStart;
    pHeader->dwNumOpCode            = pContext->pListOpCodes->size;
    pHeader->dwStringPoolStart      = dwStringPoolStart;
    pHeader->dwStringPoolLength     = dwStringPoolLength;
    pHeader->dwStringAlignedSize    = dwStringAlignedSize;

    /* 设置写入用的指针头 */
    pWriterFunc     = (BinFuncInfo *)(pByteRaw + pHeader->dwFuncBlockStart);
    pWriterOpCode   = (OpCode *)(pByteRaw + pHeader->dwOpCodeBlockStart);
    pWriterStrPool  = (char *)(pByteRaw + pHeader->dwStringPoolStart);

    /* 写入函数 */
    for (
        i = 0, pListNode = pContext->pListFunctions->head;
        pListNode != NULL;
        ++i, pListNode = pListNode->next
    ) {
        FuncDecl*       pFuncDecl   = (FuncDecl *)pListNode->data;
        BinFuncInfo*    pBinFunc    = pWriterFunc + i;

        pBinFunc->dwNumParams = pFuncDecl->iNumParams;
        pBinFunc->dwNumVars   = pFuncDecl->pListVariables->size;
        pBinFunc->dwOpCodePos = pFuncDecl->iOpCodeStartPos;
        StringCopy(pBinFunc->szName, sizeof(pBinFunc->szName), pFuncDecl->szFuncName);
    }

    /* 写入 OpCode */
    for (
        i = 0, pListNode = pContext->pListOpCodes->head;
        pListNode != NULL;
        ++i, pListNode = pListNode->next
    ) {
        OpCode* pOpReader   = (OpCode *)pListNode->data;
        OpCode* pOpWriter   = pWriterOpCode + i;
        memcpy(pOpWriter, pOpReader, sizeof(OpCode));
    }
    
    /* 写入 String Pool */
    memcpy(pWriterStrPool, pContext->szStringPool, pContext->iStringPoolSize);

    *pPtrByteRaw    = pByteRaw;
    *pDwRawLength   = dwSizeTotal;

    return KB_TRUE;
}