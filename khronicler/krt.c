#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include "krt.h"
#include "kalias.h"

static const struct {
    const char* szName;
    const char* szMessage;
} RUNTIME_ERROR_DETAIL[] = {
    { "RUNTIME_NONE",                   "No runtime error occurred" },
    { "RUNTIME_STACK_UNDERFLOW",        "Stack underflow: attempted to pop from an empty stack" },
    { "RUNTIME_TYPE_MISMATCH",          "Type mismatch: operation applied to incompatible types" },
    { "RUNTIME_UNKNOWN_OPCODE",         "Unknown opcode encountered during execution" },
    { "RUNTIME_UNKNOWN_OPERATOR",       "Unknown operator encountered during expression evaluation" },
    { "RUNTIME_UNKNOWN_BUILT_IN_FUNC",  "Attempted to call an undefined built-in function" },
    { "RUNTIME_UNKNOWN_USER_FUNC",      "Attempted to call an undefined user function" },
    { "RUNTIME_DIVISION_BY_ZERO",       "Division by zero is not allowed" },
    { "RUNTIME_NOT_IN_USER_FUNC",       "Return statement encountered outside of a function context" },
    { "RUNTIME_ARRAY_INVALID_SIZE",     "Invalid array size specified during allocation" },
    { "RUNTIME_ARRAY_OUT_OF_BOUNDS",    "Array index out of bounds" },
    { "RUNTIME_NOT_ARRAY",              "Attempted to perform array operation on a non-array value" }
};

const char* KRuntimeError_GetNameById(RuntimeErrorId iRuntimeErrorId) {
    if (iRuntimeErrorId < 0 || iRuntimeErrorId > RUNTIME_NOT_ARRAY) return "n/a";
    return RUNTIME_ERROR_DETAIL[iRuntimeErrorId].szName;
}

const char* KRuntimeError_GetMessageById(RuntimeErrorId iRuntimeErrorId) {
    if (iRuntimeErrorId < 0 || iRuntimeErrorId > RUNTIME_NOT_ARRAY) return "n/a";
    return RUNTIME_ERROR_DETAIL[iRuntimeErrorId].szMessage;
}

const char* KRuntimeValue_GetTypeNameById(RuntimeValueTypeId iRtTypeId) {
    static const char * SZ_RT_VALUE_NAME[] = {
        "nil", "number", "string", "array", "array_ref"
    };
    return SZ_RT_VALUE_NAME[iRtTypeId];
}

static void destroyRtValue(RtValue* pRtValue) {
    switch (pRtValue->iType) {
        /* 不需要释放值 */
        case RT_VALUE_NIL:
        case RT_VALUE_NUMBER:
        case RT_VALUE_ARRAY_REF:
        default:
            break;
        /* 释放字符串 */
        case RT_VALUE_STRING:
            if (!pRtValue->uData.sString.bIsRef) {
                free(pRtValue->uData.sString.uContent.pReadWrite);
            }
            break;
        /* 释放数组 */
        case RT_VALUE_ARRAY:
            free(pRtValue->uData.sArray.pArrPtrElements);
            pRtValue->uData.sArray.iSize = 0;
            break;
    }
    free(pRtValue);
}

static void destroyRtValueVoidPtr(void* pVoidPtr) {
    destroyRtValue((RtValue *)pVoidPtr);
}

static RtValue* createNumericRtValue(KFloat fValue) {
    RtValue* pRtValue = (RtValue *)malloc(sizeof(RtValue));
    pRtValue->iType = RT_VALUE_NUMBER;
    pRtValue->uData.fNumber = fValue;
    return pRtValue;
}

static RtValue* createStringRtValue(char* szValue) {
    RtValue* pRtValue = (RtValue *)malloc(sizeof(RtValue));
    pRtValue->iType = RT_VALUE_STRING;
    pRtValue->uData.sString.bIsRef = KB_FALSE;
    pRtValue->uData.sString.uContent.pReadWrite = szValue;
    return pRtValue;
}

static RtValue* createStringRefRtValue(const char* szValue) {
    RtValue* pRtValue = (RtValue *)malloc(sizeof(RtValue));
    pRtValue->iType = RT_VALUE_STRING;
    pRtValue->uData.sString.bIsRef = KB_TRUE;
    pRtValue->uData.sString.uContent.pReadOnly = szValue;
    return pRtValue;
}

static RtValue* createArrayRtValue(int iArraySize) {
    RtValue* pRtValue = (RtValue *)malloc(sizeof(RtValue));
    int i;
    pRtValue->iType = RT_VALUE_ARRAY;
    pRtValue->uData.sArray.iSize = iArraySize;
    pRtValue->uData.sArray.pArrPtrElements = (RtValue **)malloc(sizeof(RtValue* ) * iArraySize);
    for (i = 0; i < iArraySize; ++i) {
        pRtValue->uData.sArray.pArrPtrElements[i] = createNumericRtValue(0);
    }
    return pRtValue;
}

const char* KRuntimeValue_Stringify(KbRuntimeValue* pRtValue, KBool *pBoolNeedDispose) {
    *pBoolNeedDispose = KB_FALSE;
    switch (pRtValue->iType) {
        default:
            return "![n/a]";
        case RT_VALUE_NIL:
            return "![nil]";
        case RT_VALUE_NUMBER: {
            char szBuf[K_NUMERIC_STRINGIFY_BUF_MAX];
            Ftoa(pRtValue->uData.fNumber, szBuf, K_DEFAULT_FTOA_PRECISION);
            *pBoolNeedDispose = KB_TRUE;
            return StringDump(szBuf);
        }
        case RT_VALUE_STRING: {
            if (pRtValue->uData.sString.bIsRef) {
                return pRtValue->uData.sString.uContent.pReadOnly;
            }
            else {
                return pRtValue->uData.sString.uContent.pReadWrite;
            }
        }
        case RT_VALUE_ARRAY: {
            return "![array]";
        }
        case RT_VALUE_ARRAY_REF: {
            return "![arrayRef]";
        }
    }
}

static RtValue* createStringRtValueFromConcat(RtValue* pRtLeft, RtValue* pRtRight) {
    RtValue*    pRtValue = (RtValue *)malloc(sizeof(RtValue));
    const char  *szLeft, *szRight;
    KBool       bLeftNeedDispose, bRightNeedDispose;

    szLeft = stringifyRtValue(pRtLeft, &bLeftNeedDispose);
    szRight = stringifyRtValue(pRtRight, &bRightNeedDispose);

    pRtValue->iType = RT_VALUE_STRING;
    pRtValue->uData.sString.bIsRef = KB_FALSE;
    pRtValue->uData.sString.uContent.pReadWrite = StringConcat(szLeft, szRight);

    if (bLeftNeedDispose) free((void *)szLeft);
    if (bRightNeedDispose) free((void *)szRight);
    
    return pRtValue;
}

static RtValue* createRefRtValue(RtValue* pRtValue) {
    switch (pRtValue->iType) {
        default:
        case RT_VALUE_NIL:
            return createNumericRtValue(0);
        case RT_VALUE_NUMBER: 
            return createNumericRtValue(pRtValue->uData.fNumber);
        case RT_VALUE_STRING:
            return createStringRefRtValue(pRtValue->uData.sString.uContent.pReadOnly);
        case RT_VALUE_ARRAY: {
            RtValue* pRtNew = (RtValue *)malloc(sizeof(RtValue));
            pRtNew->iType = RT_VALUE_ARRAY_REF;
            pRtNew->uData.pArrRef = &pRtValue->uData.sArray;
            return pRtNew;
        }
        case RT_VALUE_ARRAY_REF: {
            RtValue* pRtNew = (RtValue *)malloc(sizeof(RtValue));
            pRtNew->iType = RT_VALUE_ARRAY_REF;
            pRtNew->uData.pArrRef = pRtValue->uData.pArrRef;
            return pRtNew;
        }
    }
    return NULL;
}

static KBool canBeConsideredAsTrue(RtValue* pRtValue) {
    switch (pRtValue->iType) {
        default:
        case RT_VALUE_NIL:
            return KB_FALSE;
        case RT_VALUE_NUMBER:
            return !!(int)pRtValue->uData.fNumber;
        case RT_VALUE_STRING: {
            const char* szValue = pRtValue->uData.sString.uContent.pReadOnly;
            return szValue && StringLength(szValue) > 0;
        }
        case RT_VALUE_ARRAY: {
            return KB_TRUE;
        }
        case RT_VALUE_ARRAY_REF: {
            return KB_TRUE;
        }
    }
}

static KBool canBeConsideredEqual(RtValue* pRtLeft, RtValue* pRtRight) {
    if (pRtLeft->iType == RT_VALUE_NIL && pRtRight->iType == RT_VALUE_NIL) {
        return KB_TRUE;
    }
    else if (pRtLeft->iType == RT_VALUE_NUMBER && pRtRight->iType == RT_VALUE_NUMBER) {
        return pRtLeft->uData.fNumber == pRtRight->uData.fNumber;
    }
    else if (pRtLeft->iType == RT_VALUE_STRING && pRtRight->iType == RT_VALUE_STRING) {
        const char* szLeft = pRtLeft->uData.sString.uContent.pReadOnly;
        const char* szRight = pRtRight->uData.sString.uContent.pReadOnly;
        return IsStringEqual(szLeft, szRight);
    }
    return KB_FALSE;
}

CallEnv* createCallEnv(int iPrevPos, const BinFuncInfo* pFuncInfo) {
    CallEnv* pEnv = (CallEnv *)malloc(sizeof(CallEnv));
    int i;

    pEnv->iNumParams        = pFuncInfo->dwNumParams;
    pEnv->iNumVar           = pFuncInfo->dwNumVars;
    pEnv->iPrevOpCodePos    = iPrevPos;
    pEnv->pArrPtrLocalVars  = (RtValue **)malloc(sizeof(RtValue *) * pEnv->iNumVar);

    /* 前 numArg 个变量从栈上取得，不需要初始化, 其他的初始化为0 */
    for (i = pEnv->iNumParams; i < pEnv->iNumVar; ++i) {
        pEnv->pArrPtrLocalVars[i] = createNumericRtValue(0);
    }
    return pEnv;
}

void destroyCallEnv(CallEnv * pEnv) {
    int i;
    for (i = 0; i < pEnv->iNumParams; ++i) {
        destroyRtValue(pEnv->pArrPtrLocalVars[i]);
    }
    free(pEnv->pArrPtrLocalVars);
    free(pEnv);
}

void destroyCallEnvVoidPtr(void* pEnv) {
    destroyCallEnv((CallEnv *)pEnv);
}

KbVirtualMachine* KRuntime_CreateMachine(const KByte* pSerializedRaw) {
    Machine* pMachine = (Machine *)malloc(sizeof(Machine));
    int iNumVar, i;

    pMachine->pByteRaw      = pSerializedRaw;
    pMachine->pBinHeader    = (const BinHeader *)pSerializedRaw;
    pMachine->pArrFuncInfo  = (const BinFuncInfo *)(pSerializedRaw + pMachine->pBinHeader->dwFuncBlockStart);
    pMachine->pStackOperand = vlNewList();
    pMachine->pStackCallEnv = vlNewList();
    pMachine->iStopValue    = 0;

    /* 全部以数字0初始化全局变量 */
    iNumVar = pMachine->pBinHeader->dwNumVariables;
    pMachine->pArrPtrGlobalVars = (RtValue **)malloc(sizeof(RtValue *) * iNumVar);
    for (i = 0; i < iNumVar; ++i) {
       pMachine->pArrPtrGlobalVars[i] = createNumericRtValue(0);
    }

    return pMachine;
}

void KRuntime_DestroyMachine(KbVirtualMachine* pMachine) {
    int i, iNumVar = pMachine->pBinHeader->dwNumVariables;
    for (i = 0; i < iNumVar; ++i) {
        destroyRtValue(pMachine->pArrPtrGlobalVars[i]);
    }
    free(pMachine->pArrPtrGlobalVars);
    vlDestroy(pMachine->pStackOperand, destroyRtValueVoidPtr);
    vlDestroy(pMachine->pStackCallEnv, destroyCallEnvVoidPtr);
    free(pMachine);
}

static void machineOpCodePosReset(Machine* pMachine) {
    pMachine->pOpCodeCur = (OpCode *)(pMachine->pByteRaw + pMachine->pBinHeader->dwOpCodeBlockStart);
}

static void cleanUpOperandsWithArraySize(RtValue** pArrPtrOperands, int iSize) {
    int i;
    for (i = 0; i < iSize; ++i) {
        if (pArrPtrOperands[i]) {
            destroyRtValue(pArrPtrOperands[i]);
        }
        pArrPtrOperands[i] = NULL;
    }
}

#define popRtValue(toStore) {                       \
    (toStore) = NULL;                               \
    if (pMachine->pStackOperand->size <= 0) {       \
        returnExecError(RUNTIME_STACK_UNDERFLOW);   \
    }                                               \
    toStore = vlPopBack(pMachine->pStackOperand);   \
} NULL

#define checkRtValueTypeIs(pRtValue, iTypExptd) {   \
    if (pRtValue->iType != iTypExptd) {             \
        returnExecError(RUNTIME_TYPE_MISMATCH);     \
    }                                               \
} NULL

#define pushNumericOperand(num) \
    vlPushBack(pMachine->pStackOperand, createNumericRtValue(num));

#define getCurrentCallEnv(pCallEnv)  {                          \
    if (pMachine->pStackCallEnv->size <= 0) {                   \
        returnExecError(RUNTIME_NOT_IN_USER_FUNC);              \
    }                                                           \
    pCallEnv = (KbCallEnv *)pMachine->pStackCallEnv->tail->data;\
} NULL

#define getVariable(pPtrVar) {                          \
    if (pOpCode->uParam.sVarAccess.wIsLocal) {          \
        CallEnv* pCallEnv;                              \
        getCurrentCallEnv(pCallEnv);                    \
        pPtrVar = (pCallEnv->pArrPtrLocalVars         \
             + pOpCode->uParam.sVarAccess.wVarIndex);   \
    } else {                                            \
        pPtrVar = (pMachine->pArrPtrGlobalVars        \
             + pOpCode->uParam.sVarAccess.wVarIndex);   \
    }                                                   \
} NULL

#define cleanUpOperands() (cleanUpOperandsWithArraySize(pArrPtrOperands, sizeof(pArrPtrOperands) / sizeof(pArrPtrOperands[0])))
#define pRtOperandLeft      (pArrPtrOperands[0])
#define pRtOperandRight     (pArrPtrOperands[1])

#define callMathFunc(mathFunc) {                        \
    popRtValue(pRtOperandLeft);                         \
    fResult = mathFunc(pRtOperandLeft->uData.fNumber);  \
    cleanUpOperands();                                  \
    pushNumericOperand(fResult);                        \
} NULL

#define returnExecError(rtErrId) {                  \
    *pIntRtErrId = (rtErrId);                       \
    *ppStopOpCode = pOpCode;                        \
    cleanUpOperands();                              \
    return KB_FALSE;                                \
} NULL

KBool KRuntime_MachineExecute(
    KbVirtualMachine*   pMachine,
    int                 iStartPos,
    RuntimeErrorId*     pIntRtErrId,
    const OpCode**      ppStopOpCode
) {
    int             iNumOpCode          = pMachine->pBinHeader->dwNumOpCode;
    const OpCode*   pOpCodeStart        = (OpCode *)(pMachine->pByteRaw + pMachine->pBinHeader->dwOpCodeBlockStart);
    KFloat          fResult             = 0;
    RtValue*        pArrPtrOperands[]   = { NULL, NULL };

    srand(time(NULL));

    *pIntRtErrId = RUNTIME_NONE;
    pMachine->iStopValue = 0;
    
    machineOpCodePosReset(pMachine);
    pMachine->pOpCodeCur += iStartPos;
    
    while (pMachine->pOpCodeCur - pOpCodeStart < iNumOpCode) {
        const OpCode* pOpCode = pMachine->pOpCodeCur;
        /* 
        printf("%03d | %-18s \n", pMachine->pOpCodeCur - pOpCodeStart, getOpCodeName(pOpCode->dwOpCodeId));
         */
        switch(pOpCode->dwOpCodeId) {
            default: {
                returnExecError(RUNTIME_UNKNOWN_OPCODE);
                break;
            }
            case K_OPCODE_PUSH_NUM: {
                pushNumericOperand(pOpCode->uParam.fLiteral);
                break;
            }
            case K_OPCODE_PUSH_STR: {
                vlPushBack(
                    pMachine->pStackOperand,
                    createStringRefRtValue(
                        (const char *)pMachine->pBinHeader + 
                        pMachine->pBinHeader->dwStringPoolStart + 
                        pOpCode->uParam.dwStringPoolPos
                    )
                );
                break;
            }
            case K_OPCODE_BINARY_OPERATOR: {
                popRtValue(pRtOperandRight);
                popRtValue(pRtOperandLeft);

                switch (pOpCode->uParam.dwOperatorId) {
                    default: {
                        returnExecError(RUNTIME_UNKNOWN_OPERATOR);
                        break;
                    }
                    case OPR_CONCAT: {
                        vlPushBack(pMachine->pStackOperand, createStringRtValueFromConcat(pRtOperandLeft, pRtOperandRight));
                        break;
                    }
                    case OPR_ADD: {
                        checkRtValueTypeIs(pRtOperandLeft, RT_VALUE_NUMBER);
                        checkRtValueTypeIs(pRtOperandRight, RT_VALUE_NUMBER);
                        fResult = pRtOperandLeft->uData.fNumber + pRtOperandRight->uData.fNumber;
                        pushNumericOperand(fResult);
                        break;
                    }
                    case OPR_SUB: {
                        checkRtValueTypeIs(pRtOperandLeft, RT_VALUE_NUMBER);
                        checkRtValueTypeIs(pRtOperandRight, RT_VALUE_NUMBER);
                        fResult = pRtOperandLeft->uData.fNumber - pRtOperandRight->uData.fNumber;
                        pushNumericOperand(fResult);
                        break;
                    }
                    case OPR_MUL: {
                        checkRtValueTypeIs(pRtOperandLeft, RT_VALUE_NUMBER);
                        checkRtValueTypeIs(pRtOperandRight, RT_VALUE_NUMBER);
                        fResult = pRtOperandLeft->uData.fNumber * pRtOperandRight->uData.fNumber;
                        pushNumericOperand(fResult);
                        break;
                    }
                    case OPR_DIV: {
                        checkRtValueTypeIs(pRtOperandLeft, RT_VALUE_NUMBER);
                        checkRtValueTypeIs(pRtOperandRight, RT_VALUE_NUMBER);

                        if (pRtOperandRight->uData.fNumber == 0) {
                            returnExecError(RUNTIME_DIVISION_BY_ZERO);
                        }

                        fResult = pRtOperandLeft->uData.fNumber / pRtOperandRight->uData.fNumber;
                        pushNumericOperand(fResult);
                        break;
                    }
                    case OPR_POW: {
                        checkRtValueTypeIs(pRtOperandLeft, RT_VALUE_NUMBER);
                        checkRtValueTypeIs(pRtOperandRight, RT_VALUE_NUMBER);
                        fResult = pow(pRtOperandLeft->uData.fNumber, pRtOperandRight->uData.fNumber);
                        pushNumericOperand(fResult);
                        break;
                    }
                    case OPR_INTDIV: {
                        checkRtValueTypeIs(pRtOperandLeft, RT_VALUE_NUMBER);
                        checkRtValueTypeIs(pRtOperandRight, RT_VALUE_NUMBER);

                        if (pRtOperandRight->uData.fNumber == 0) {
                            returnExecError(RUNTIME_DIVISION_BY_ZERO);
                        }

                        fResult = (int)(pRtOperandLeft->uData.fNumber / pRtOperandRight->uData.fNumber);
                        pushNumericOperand(fResult);
                        break;
                    }
                    case OPR_MOD: {
                        checkRtValueTypeIs(pRtOperandLeft, RT_VALUE_NUMBER);
                        checkRtValueTypeIs(pRtOperandRight, RT_VALUE_NUMBER);
                        fResult = ((int)pRtOperandLeft->uData.fNumber) % ((int)pRtOperandRight->uData.fNumber);
                        pushNumericOperand(fResult);
                        break;
                    }
                    case OPR_AND: {
                        fResult = canBeConsideredAsTrue(pRtOperandLeft) && canBeConsideredAsTrue(pRtOperandRight);
                        pushNumericOperand(fResult);
                        break;
                    }
                    case OPR_OR: {
                        fResult = canBeConsideredAsTrue(pRtOperandLeft) || canBeConsideredAsTrue(pRtOperandRight);
                        pushNumericOperand(fResult);
                        break;
                    }
                    case OPR_EQUAL: {
                        fResult = canBeConsideredEqual(pRtOperandLeft, pRtOperandRight);
                        pushNumericOperand(fResult);
                        break;
                    }
                    case OPR_APPROX_EQ: {
                        checkRtValueTypeIs(pRtOperandLeft, RT_VALUE_NUMBER);
                        checkRtValueTypeIs(pRtOperandRight, RT_VALUE_NUMBER);
                        fResult = FloatEqualRel(pRtOperandLeft->uData.fNumber, pRtOperandRight->uData.fNumber);
                        pushNumericOperand(fResult);
                        break;
                    }
                    case OPR_NEQ: {
                        fResult = !canBeConsideredEqual(pRtOperandLeft, pRtOperandRight);
                        pushNumericOperand(fResult);
                        break;
                    }
                    case OPR_GT: {
                        checkRtValueTypeIs(pRtOperandLeft, RT_VALUE_NUMBER);
                        checkRtValueTypeIs(pRtOperandRight, RT_VALUE_NUMBER);
                        fResult = pRtOperandLeft->uData.fNumber > pRtOperandRight->uData.fNumber;
                        pushNumericOperand(fResult);
                        break;
                    }
                    case OPR_LT: {
                        checkRtValueTypeIs(pRtOperandLeft, RT_VALUE_NUMBER);
                        checkRtValueTypeIs(pRtOperandRight, RT_VALUE_NUMBER);
                        fResult = pRtOperandLeft->uData.fNumber < pRtOperandRight->uData.fNumber;
                        pushNumericOperand(fResult);
                        break;
                    }
                    case OPR_GTEQ: {
                        checkRtValueTypeIs(pRtOperandLeft, RT_VALUE_NUMBER);
                        checkRtValueTypeIs(pRtOperandRight, RT_VALUE_NUMBER);
                        fResult = pRtOperandLeft->uData.fNumber >= pRtOperandRight->uData.fNumber;
                        pushNumericOperand(fResult);
                        break;
                    }
                    case OPR_LTEQ: {
                        checkRtValueTypeIs(pRtOperandLeft, RT_VALUE_NUMBER);
                        checkRtValueTypeIs(pRtOperandRight, RT_VALUE_NUMBER);
                        fResult = pRtOperandLeft->uData.fNumber <= pRtOperandRight->uData.fNumber;
                        pushNumericOperand(fResult);
                        break;
                    }
                }

                cleanUpOperands();
                break;
            }
            case K_OPCODE_UNARY_OPERATOR: {
                popRtValue(pRtOperandLeft);
                
                switch (pOpCode->uParam.dwOperatorId) {
                    default: {
                        returnExecError(RUNTIME_UNKNOWN_OPERATOR);
                        break;
                    }
                    case OPR_NEG: {
                        checkRtValueTypeIs(pRtOperandLeft, RT_VALUE_NUMBER);
                        pushNumericOperand(-pRtOperandLeft->uData.fNumber);
                        break;
                    }
                    case OPR_NOT: {
                        pushNumericOperand(!canBeConsideredAsTrue(pRtOperandLeft));
                        break;
                    }
                }

                cleanUpOperands();
                break;
            }
            case K_OPCODE_POP: {
                if (pMachine->pStackOperand->size <= 0) {
                    returnExecError(RUNTIME_STACK_UNDERFLOW);
                }
                destroyRtValue(vlPopBack(pMachine->pStackOperand));
                break;
            }
            case K_OPCODE_PUSH_VAR: {
                RtValue** pPtrVar = NULL;
                getVariable(pPtrVar);
                vlPushBack(pMachine->pStackOperand, createRefRtValue(*pPtrVar));
                break;
            }
            case K_OPCODE_SET_VAR: {
                RtValue** pPtrVar = NULL;
                /* 获取变量指针 */
                getVariable(pPtrVar);
                /* 弹出栈顶的值 */
                popRtValue(pRtOperandLeft);
                /* 释放变量旧值 */
                destroyRtValue(*pPtrVar);
                /* 出栈的值写入变量位置 */
                *pPtrVar = pRtOperandLeft;
                /* 不释放弹出的值 */
                pRtOperandLeft = NULL;
                break;
            }
            case K_OPCODE_SET_VAR_AS_ARRAY: {
                RtValue** pPtrVar = NULL;
                int iArraySize = 0;
                /* 获取变量指针 */
                getVariable(pPtrVar);
                /* 弹出数组尺寸 */
                popRtValue(pRtOperandLeft);
                /* 检查尺寸是否是数值 */
                checkRtValueTypeIs(pRtOperandLeft, RT_VALUE_NUMBER);
                /* 检查尺寸是否合法 */
                iArraySize = (int)pRtOperandLeft->uData.fNumber;
                if (iArraySize <= 0) {
                    returnExecError(RUNTIME_ARRAY_INVALID_SIZE);
                }
                /* 释放弹出的值 */
                cleanUpOperands();
                /* 创建的数组写入变量 */
                *pPtrVar = createArrayRtValue(iArraySize);
                break;
            }
            case K_OPCODE_ARR_GET: {
                RtValue**       pPtrVar = NULL;
                int             iSubscript;
                RuntimeArray*   pArray;
                /* 获取变量指针 */
                getVariable(pPtrVar);
                /* 变量是数组 */
                if ((*pPtrVar)->iType == RT_VALUE_ARRAY) {
                    pArray = &(*pPtrVar)->uData.sArray;
                }
                /* 变量是数组引用 */
                else if ((*pPtrVar)->iType == RT_VALUE_ARRAY_REF) {
                    pArray = (*pPtrVar)->uData.pArrRef;
                }
                /* 变量不是数组 */
                else {
                    returnExecError(RUNTIME_NOT_ARRAY);
                }
                /* 弹出下标 */
                popRtValue(pRtOperandLeft);
                /* 检查下标是否是数值 */
                checkRtValueTypeIs(pRtOperandLeft, RT_VALUE_NUMBER);
                /* 检查下标是否合法 */
                iSubscript = (int)pRtOperandLeft->uData.fNumber;
                if (iSubscript < 0 || iSubscript >= pArray->iSize) {
                    returnExecError(RUNTIME_ARRAY_OUT_OF_BOUNDS);
                }
                /* 释放弹出的值 */
                cleanUpOperands();
                /* 数组元素入栈 */
                vlPushBack(pMachine->pStackOperand, createRefRtValue(pArray->pArrPtrElements[iSubscript]));
                break;
            }
            case K_OPCODE_ARR_SET: {
                RtValue**       pPtrVar = NULL;
                int             iSubscript;
                RuntimeArray*   pArray;
                /* 获取变量指针 */
                getVariable(pPtrVar);
                /* 变量是数组 */
                if ((*pPtrVar)->iType == RT_VALUE_ARRAY) {
                    pArray = &(*pPtrVar)->uData.sArray;
                }
                /* 变量是数组引用 */
                else if ((*pPtrVar)->iType == RT_VALUE_ARRAY_REF) {
                    pArray = (*pPtrVar)->uData.pArrRef;
                }
                /* 变量不是数组 */
                else {
                    returnExecError(RUNTIME_NOT_ARRAY);
                }
                /* 弹出右值 */
                popRtValue(pRtOperandRight);
                /* 弹出下标 */
                popRtValue(pRtOperandLeft);
                /* 检查下标是否是数值 */
                checkRtValueTypeIs(pRtOperandLeft, RT_VALUE_NUMBER);
                /* 检查下标是否合法 */
                iSubscript = (int)pRtOperandLeft->uData.fNumber;
                if (iSubscript < 0 || iSubscript >= pArray->iSize) {
                    returnExecError(RUNTIME_ARRAY_OUT_OF_BOUNDS);
                }
                /* 释放数组元素旧值 */
                destroyRtValue(pArray->pArrPtrElements[iSubscript]);
                /* 出栈的值赋值给数组元素 */
                pArray->pArrPtrElements[iSubscript] = pRtOperandRight;
                /* 不释放右值，已经赋值给元素了 */
                pRtOperandRight = NULL;
                /* 释放弹出的值 */
                cleanUpOperands();
                break;
            }
            case K_OPCODE_CALL_BUILT_IN: {
                switch (pOpCode->uParam.dwBuiltFuncId) {
                    default: {
                        returnExecError(RUNTIME_UNKNOWN_BUILT_IN_FUNC);
                    }
                    case KBUILT_IN_FUNC_P: {
                        const char* szStringified;
                        KBool bNeedDispose;
                        /* 弹出要打印的值 */
                        popRtValue(pRtOperandLeft);
                        /* 字符串化 */
                        szStringified = stringifyRtValue(pRtOperandLeft, &bNeedDispose);
                        printf("%s", szStringified);
                        /* 释放临时字符串 */
                        if (bNeedDispose) free((void *)szStringified);
                        /* 释放弹出的值 */
                        cleanUpOperands();
                        /* 添加返回值 0 */
                        pushNumericOperand(0);
                        break; 
                    }
                    case KBUILT_IN_FUNC_SIN: {
                        callMathFunc(sinf);
                        break;
                    }
                    case KBUILT_IN_FUNC_COS: {
                        callMathFunc(cosf);
                        break;
                    }
                    case KBUILT_IN_FUNC_TAN: {
                        callMathFunc(tanf);
                        break;
                    }
                    case KBUILT_IN_FUNC_SQRT: {
                        callMathFunc(sqrtf);
                        break;
                    }
                    case KBUILT_IN_FUNC_EXP: {
                        callMathFunc(expf);
                        break;
                    }
                    case KBUILT_IN_FUNC_ABS: {
                        callMathFunc(fabsf);
                        break;
                    }
                    case KBUILT_IN_FUNC_LOG: {
                        callMathFunc(logf);
                        break;
                    }
                    case KBUILT_IN_FUNC_FLOOR: {
                        callMathFunc(floorf);
                        break;
                    }
                    case KBUILT_IN_FUNC_CEIL: {
                        callMathFunc(ceilf);
                        break;
                    }
                    case KBUILT_IN_FUNC_RAND: {
                        const int iMax = 10000;
                        const int iRandVal = rand() % iMax;
                        fResult = ((KFloat)iRandVal) / ((KFloat) iMax);
                        pushNumericOperand(fResult);
                        break;
                    }
                    case KBUILT_IN_FUNC_LEN: {
                        int iLength = 0;
                        /* 弹出需要求长度的值 */
                        popRtValue(pRtOperandLeft);
                        /* 根据不同类型计算长度 */
                        switch (pRtOperandLeft->iType) {
                            default:
                                returnExecError(RUNTIME_TYPE_MISMATCH);
                                break;
                            case RT_VALUE_ARRAY:
                                iLength = pRtOperandLeft->uData.sArray.iSize;
                                break;
                            case RT_VALUE_ARRAY_REF:
                                iLength = pRtOperandLeft->uData.pArrRef->iSize;
                                break;
                            case RT_VALUE_STRING:
                                iLength = strlen(pRtOperandLeft->uData.sString.uContent.pReadOnly);
                                break;
                        }
                        /* 清理弹出的值 */
                        cleanUpOperands(); 
                        /* 长度入栈 */
                        pushNumericOperand(iLength);
                        break;
                    }
                    case KBUILT_IN_FUNC_VAL: {
                        /* 弹出字符串 */
                        popRtValue(pRtOperandLeft);
                        /* 检查是不是字符串 */
                        checkRtValueTypeIs(pRtOperandLeft, RT_VALUE_STRING);
                        /* 转换为数值 */
                        fResult = (KFloat)Atof(pRtOperandLeft->uData.sString.uContent.pReadOnly);
                        /* 释放弹出的值 */
                        cleanUpOperands();
                        /* 转换的数值入栈 */
                        pushNumericOperand(fResult);
                        break;
                    }
                    case KBUILT_IN_FUNC_CHR: {
                        char szAscStr[] = { 0, 0 };
                        /* 弹出要要转为字符串的 ASCII 值 */
                        popRtValue(pRtOperandLeft);
                        /* 检查是否是数值 */
                        checkRtValueTypeIs(pRtOperandLeft, RT_VALUE_NUMBER);
                        /* 转为字符串 */
                        szAscStr[0] = (int)pRtOperandLeft->uData.fNumber;
                        /* 释放弹出的值 */
                        cleanUpOperands();
                        /* 生成的字符串作为返回值 */
                        vlPushBack(pMachine->pStackOperand, createStringRtValue(StringDump(szAscStr)));
                        break; 
                    }
                    case KBUILT_IN_FUNC_ASC: {
                        /* 弹出字符串 */
                        popRtValue(pRtOperandLeft);
                        /* 检查是否是字符串 */
                        checkRtValueTypeIs(pRtOperandLeft, RT_VALUE_STRING);
                        /* 字符串第一个字符转数字 */
                        fResult = pRtOperandLeft->uData.sString.uContent.pReadOnly[0];
                        /* 释放弹出的值 */
                        cleanUpOperands();
                        /* 生成的字符串作为返回值 */
                        pushNumericOperand(fResult);
                        break; 
                    }
                }
                break;
            }
            case K_OPCODE_GOTO: {
                pMachine->pOpCodeCur = pOpCodeStart + pOpCode->uParam.dwOpCodePos;
                continue;
            }
            case K_OPCODE_IF_GOTO: {
                KBool bCondition = KB_FALSE;
                /* 弹出条件值 */
                popRtValue(pRtOperandLeft);
                /* 条件值能否被视为 True */
                bCondition = canBeConsideredAsTrue(pRtOperandLeft);
                /* 释放条件值 */
                cleanUpOperands();
                /* 跳转 */
                if (bCondition) {
                    pMachine->pOpCodeCur = pOpCodeStart + pOpCode->uParam.dwOpCodePos;
                    continue;
                }
                break;
            }
            case K_OPCODE_UNLESS_GOTO: {
                KBool bCondition = KB_FALSE;
                /* 弹出条件值 */
                popRtValue(pRtOperandLeft);
                /* 条件值能否被视为 True */
                bCondition = canBeConsideredAsTrue(pRtOperandLeft);
                /* 释放条件值 */
                cleanUpOperands();
                /* 跳转 */
                if (!bCondition) {
                    pMachine->pOpCodeCur = pOpCodeStart + pOpCode->uParam.dwOpCodePos;
                    continue;
                }
                break;
            }
            case K_OPCODE_CALL_FUNC: {
                int                 i;
                int                 iCurrentPos = pMachine->pOpCodeCur - pOpCodeStart;
                const BinFuncInfo*  pFuncInfo   = pMachine->pArrFuncInfo + pOpCode->uParam.dwFuncIndex;
                CallEnv*            pCallEnv    = createCallEnv(iCurrentPos, pFuncInfo);
                
                /* 新调用环境入调用环境栈 */
                vlPushBack(pMachine->pStackCallEnv, pCallEnv);
                
                /* 操作数出栈作为函数调用的参数 */
                for (i = 0; i < pCallEnv->iNumParams; ++i) {
                    RtValue* pRtValue;
                    popRtValue(pRtValue);
                    pCallEnv->pArrPtrLocalVars[pCallEnv->iNumParams - 1 - i] = pRtValue;
                }
                
                /* opCode 跳转 */
                pMachine->pOpCodeCur = pOpCodeStart + pFuncInfo->dwOpCodePos;
                continue;
            }
            case K_OPCODE_RETURN: {
                CallEnv* pCallEnv;
                if (pMachine->pStackCallEnv->size <= 0) {
                    returnExecError(RUNTIME_NOT_IN_USER_FUNC);
                }
                pCallEnv = (CallEnv *)vlPopBack(pMachine->pStackCallEnv);

                /* 回去原来的位置 */
                pMachine->pOpCodeCur = pOpCodeStart + pCallEnv->iPrevOpCodePos + 1;

                /* 销毁调用环境 */
                destroyCallEnv(pCallEnv);
                continue;
            }
            case K_OPCODE_STOP: {
                /* 弹出停止数值 */
                popRtValue(pRtOperandLeft);
                /* 检查类型是否是数字 */
                checkRtValueTypeIs(pRtOperandLeft, RT_VALUE_NUMBER);
                /* 停止数值写入机器 */
                pMachine->iStopValue = (int)pRtOperandLeft->uData.fNumber;
                /* 释放弹出的值 */
                cleanUpOperands();
                /* 结束运行 */
                return KB_TRUE;
            }
        }
        pMachine->pOpCodeCur++;
    }

    return KB_TRUE;
}