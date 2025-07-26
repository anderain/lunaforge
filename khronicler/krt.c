#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "krt.h"
#include "k_utils.h"

#if defined(_FX_9860_) || defined(VER_PLATFORM_WIN32_CE)
#   define DISABLE_P_FUNCTION
#endif

#define KB_RT_STRINGIFY_BUF_SIZE 200

char stringifyBuf[KB_RT_STRINGIFY_BUF_SIZE];

static const char * DBG_RT_VALUE_NAME[] = {
    "nil", "number", "string"
};

KbRuntimeValue* rtvalueCreateNumber(const KB_FLOAT num) {
    KbRuntimeValue* v = (KbRuntimeValue *)malloc(sizeof(KbRuntimeValue));
    v->type = RVT_NUMBER;
    v->data.num = num;
    return v;
}

KbRuntimeValue* rtvalueCreateString(const char* sz) {
    KbRuntimeValue* v = (KbRuntimeValue *)malloc(sizeof(KbRuntimeValue));
    v->type = RVT_STRING;
    v->data.sz = StringDump(sz);
    return v;
}

KbRuntimeValue* rtvalueCreateFromConcat(const char* szLeft, const char* szRight) {
    char *strNew, *p;
    int newLength = strlen(szLeft) + strlen(szRight) + 1;

    KbRuntimeValue* v = (KbRuntimeValue *)malloc(sizeof(KbRuntimeValue));
    v->type = RVT_STRING;


    p = strNew = (char *)malloc(newLength);
    p += StringCopy(p, newLength, szLeft);
    p += StringCopy(p, newLength, szRight);

    v->data.sz = strNew;
    return v;
}

KbRuntimeValue* rtvalueDuplicate(const KbRuntimeValue* v) {
    if (v->type == RVT_NUMBER) {
        return rtvalueCreateNumber(v->data.num);
    }
    else if (v->type == RVT_STRING) {
        return rtvalueCreateString(v->data.sz);
    }
    return NULL;
}

char* rtvalueStringify(const KbRuntimeValue* v) {
    if (v->type == RVT_NUMBER) {
        char _buf[KB_RT_STRINGIFY_BUF_SIZE];
        kFtoa(v->data.num, _buf, DEFAUL_FTOA_PRECISION);
        return StringDump(_buf);
    }
    else if (v->type == RVT_STRING) {
        return StringDump(v->data.sz);
    }
    return StringDump("<unknown-value>");
}

int rtvalueIsTrue(const KbRuntimeValue *v) {
    if (v == NULL) {
        return 0;
    }
    /* 数字不为零是真 */
    else if (v->type == RVT_NUMBER) {
        return (int)v->data.num;
    }
    /* 字符串不为空是真 */
    else if (v->type == RVT_STRING && strlen(v->data.sz) > 0) {
        return 1;
    }
    return 0;
}

void rtvalueDestroy(KbRuntimeValue* val) {
    if (val->type == RVT_STRING) {
        free(val->data.sz);
    }
    free(val);
}

void rtvalueDestroyVoidPointer(void* p) {
    rtvalueDestroy((KbRuntimeValue *)p);
}

KbCallEnv* callEnvCreate(int prevCmdPos, const KbExportedFunction* pExportedFunc) {
    KbCallEnv* pEnv = (KbCallEnv *)malloc(sizeof(KbCallEnv));
    int i;
    pEnv->numArg = pExportedFunc->numArg;
    pEnv->numVar = pExportedFunc->numVar;
    pEnv->prevCmdPos = prevCmdPos;
    /* 前numArg个变量从栈上取得，不需要初始化 */
    pEnv->variables  = (KbRuntimeValue **)malloc(sizeof(KbRuntimeValue *) * pEnv->numVar);
    /* 其他的初始化为0 */
    for (i = pEnv->numArg; i < pEnv->numVar; ++i) {
        pEnv->variables[i] = rtvalueCreateNumber(0);
    }
    return pEnv;
}

void callEnvDestroy(KbCallEnv * pEnv) {
    int numVar = pEnv->numVar, i;
    for (i = 0; i < numVar; ++i) {
        rtvalueDestroy(pEnv->variables[i]);
    }
    free(pEnv->variables);
    free(pEnv);
}

void callEnvDestroyVoidPtr(void* pEnv) {
    callEnvDestroy(pEnv);
}

void machineCommandReset(KbMachine* machine) {
    machine->cmdPtr = (KbOpCommand *)(machine->raw + machine->header->cmdBlockStart);
}

KbMachine* machineCreate(const KByte* raw) {
    KbMachine* machine;
    int numVar, i;

    machine = (KbMachine *)malloc(sizeof(KbMachine));

    machine->raw            = raw;
    machine->header         = (const KbBinaryHeader *)raw;
    machine->pExportedFunc  = (const KbExportedFunction *)(raw + machine->header->funcBlockStart);
    machine->stack          = vlNewList();
    machine->callEnvStack   = vlNewList();

    /* 全部以数字0初始化全局变量 */
    numVar = machine->header->numVariables;
    machine->variables  = (KbRuntimeValue **)malloc(sizeof(KbRuntimeValue *) * numVar);
    for (i = 0; i < numVar; ++i) {
       machine->variables[i] = rtvalueCreateNumber(0);
    }    

    return machine;
}

void machineDestroy(KbMachine* machine) {
    int i;
    int numVar = machine->header->numVariables;
    for (i = 0; i < numVar; ++i) {
        rtvalueDestroy(machine->variables[i]);
    }
    free(machine->variables);
    vlDestroy(machine->stack, rtvalueDestroyVoidPointer);
    vlDestroy(machine->callEnvStack, callEnvDestroyVoidPtr);
    free(machine);
}

int machineVarAssignNum(KbMachine* machine, int varIndex, KB_FLOAT num) {
    KbRuntimeValue* varValue;
    int numVar = machine->header->numVariables;

    if (varIndex < 0 || varIndex >= numVar) {
        return 0;
    }

    varValue = machine->variables[varIndex];
    if (varValue->type == RVT_NUMBER) {
        varValue->data.num = num;
    }
    else {
        rtvalueDestroy(varValue);
        machine->variables[varIndex] = rtvalueCreateNumber(num);
    }

    return 1;
}

const KbExportedFunction* machineGetFunctionByIndex(KbMachine* machine, int userFuncIndex) {
    if (userFuncIndex < 0 || userFuncIndex >= machine->header->numFunc) {
        return NULL;
    }
    return machine->pExportedFunc + userFuncIndex;
}

extern const char *_KOCODE_NAME[];

#define formatExecErrorAppend(newPart) (p += StringCopy(p, sizeof(buf) - (p - buf), newPart))

void formatExecError(const KbRuntimeError *errorRet, char *message, int messageLength) {
    char buf[KB_ERROR_MESSAGE_MAX];
    char *p = buf;

    switch(errorRet->type) {
    case KBRE_OPERAND_TYPE_MISMATCH:
        formatExecErrorAppend("Type mismatch error occurred. The operand was a '");
        formatExecErrorAppend(errorRet->message);
        formatExecErrorAppend("' when performing ");
        formatExecErrorAppend(_KOCODE_NAME[errorRet->cmdOp]);
        break;
    case KBRE_STACK_OVERFLOW:
        formatExecErrorAppend("Stack overflow occurred. PTR = ");
        formatExecErrorAppend(errorRet->message);
        break;
    case KBRE_UNKNOWN_BUILT_IN_FUNC:
        formatExecErrorAppend("Unknown built function: ");
        formatExecErrorAppend(errorRet->message);
        break;
    case KBRE_UNKNOWN_CMD:
        formatExecErrorAppend("Unknown opcode: ");
        formatExecErrorAppend(errorRet->message);
        break;
    case KBRE_ILLEGAL_RETURN:
        formatExecErrorAppend("Invalid RETURN command. ");
        formatExecErrorAppend(errorRet->message);
        break;
    case KBRE_NOT_IN_USER_FUNC:
        formatExecErrorAppend("Cannot execute this command out of function.");
        formatExecErrorAppend(errorRet->message);
        break;
    default:
        formatExecErrorAppend("Other problems occurred during runtime. ");
        if (!StringEqual(errorRet->message, "")) {
             formatExecErrorAppend(errorRet->message);
        }
        break;
    }

    StringCopy(message, messageLength, buf);
}

void dbgPrintContextCommand(const KbOpCommand *cmd);

#define machineCmdStartPointer(machine) ((const KbOpCommand *)((machine)->raw + (machine)->header->cmdBlockStart))

#define execReturnErrorWithMessage(errorCode, msg) NULL;            \
    errorRet->type = errorCode;                                     \
    errorRet->cmdOp = machine->cmdPtr->op;                          \
    StringCopy(errorRet->message, sizeof(errorRet->message), msg);  \
    return 0

#define execReturnErrorWithInt(errorCode, intval) NULL;             \
    errorRet->type = errorCode;                                     \
    errorRet->cmdOp = machine->cmdPtr->op;                          \
    kItoa(intval, errorRet->message, 10);                           \
    return 0

#define execPopAndCheckType(oprIndex, valType)  NULL;               \
    operand[oprIndex] = (KbRuntimeValue *)vlPopBack(machine->stack);\
    if (operand[oprIndex] == NULL) {                                \
        execReturnErrorWithInt(                                     \
            KBRE_STACK_OVERFLOW,                                    \
            machine->cmdPtr - machineCmdStartPointer(machine)       \
        );                                                          \
    }                                                               \
    if (operand[oprIndex]->type != valType) {                       \
        execReturnErrorWithMessage(                                 \
            KBRE_OPERAND_TYPE_MISMATCH,                             \
            DBG_RT_VALUE_NAME[operand[oprIndex]->type]              \
        );                                                          \
    } NULL

int machineExecCallBuiltInFunc(int funcId, KbMachine* machine, KbRuntimeError *errorRet, KbRuntimeValue ** operand);

int machineGetUserFuncIndex(KbMachine* machine, const char* funcName) {
    int i = 0;
    for (i = 0; i < machine->header->numFunc; ++i) {
        const KbExportedFunction* pExpFunc = machine->pExportedFunc + i;
        if (StringEqual(pExpFunc->funcName, funcName)) {
            return i;
        }
    }
    return -1;
}

void machineMovePushValue(KbMachine* machine, KbRuntimeValue* pRtValue) {
    vlPushBack(machine->stack, pRtValue);
}

int machineExecCallUserFuncByIndex(KbMachine* machine, int funcIndex, KbRuntimeError *errorRet) {
    int currentPos = machine->header->numCmd;
    int i;
    const KbExportedFunction* pExpFunc = machineGetFunctionByIndex(machine, funcIndex);
    KbCallEnv *pEnv = callEnvCreate(currentPos, pExpFunc);
    for (i = 0; i < pEnv->numArg; ++i) {
        pEnv->variables[pEnv->numArg - 1 - i] = (KbRuntimeValue *)vlPopBack(machine->stack);
    }
    vlPushBack(machine->callEnvStack, pEnv);
    return machineExec(machine, pExpFunc->pos, errorRet);
}

int machineExec(KbMachine* machine, int startPos, KbRuntimeError *errorRet) {
    const KbOpCommand*  cmdBlockPtr = machineCmdStartPointer(machine);
    int                 numCmd = machine->header->numCmd;
    KbRuntimeValue*     operand[10];
    KB_FLOAT            numResult;

    errorRet->type = KBRE_NO_ERROR;
    errorRet->message[0] = '\0';

    machineCommandReset(machine);
    machine->cmdPtr += startPos;

    while (machine->cmdPtr - cmdBlockPtr < numCmd) {
        const KbOpCommand* cmd = machine->cmdPtr;
        
        /* printf("        [%04d] ", machine->cmdPtr - cmdBlockPtr); dbgPrintContextCommand(cmd); printf("\n");*/

        switch(cmd->op) {
            case KBO_NOP: {
	            break;
            }
            case KBO_PUSH_VAR: {
                KbRuntimeValue *varValue = machine->variables[cmd->param.index];
                vlPushBack(machine->stack, rtvalueDuplicate(varValue));
	            break;
            }
            case KBO_PUSH_LOCAL: {
                KbRuntimeValue* varValue;
                KbCallEnv* pCallEnv;
                if (machine->callEnvStack->size <= 0) {
                    execReturnErrorWithInt(KBRE_NOT_IN_USER_FUNC, cmd->op);
                }
                pCallEnv = (KbCallEnv *)machine->callEnvStack->tail->data;
                varValue = pCallEnv->variables[cmd->param.index];
                vlPushBack(machine->stack, rtvalueDuplicate(varValue));
	            break;
            }
            case KBO_PUSH_NUM: {
                numResult = cmd->param.number;
                vlPushBack(machine->stack, rtvalueCreateNumber(numResult));
	            break;
            }
            case KBO_PUSH_STR: {
                const char *sz = (const char *)(machine->raw + machine->header->stringBlockStart + cmd->param.index);
                vlPushBack(machine->stack, rtvalueCreateString(sz));
	            break;
            }
            case KBO_POP: {
                KbRuntimeValue *popped = (KbRuntimeValue *)vlPopBack(machine->stack);
                rtvalueDestroy(popped);
	            break;
            }
            case KBO_OPR_NEG: {
                execPopAndCheckType(0, RVT_NUMBER);

                numResult = -operand[0]->data.num;

                vlPushBack(machine->stack, rtvalueCreateNumber(numResult));
                rtvalueDestroy(operand[0]);
	            break;
            }
            case KBO_OPR_CONCAT: {
                char *concatLeft, *concatRight;

                operand[1] = (KbRuntimeValue *)vlPopBack(machine->stack);
                operand[0] = (KbRuntimeValue *)vlPopBack(machine->stack);

                concatLeft = rtvalueStringify(operand[0]);
                concatRight = rtvalueStringify(operand[1]);

                rtvalueDestroy(operand[0]);
                rtvalueDestroy(operand[1]);

                vlPushBack(machine->stack, rtvalueCreateFromConcat(concatLeft, concatRight));

                free(concatLeft);
                free(concatRight);
	            break;
            }
            case KBO_OPR_ADD: {
                execPopAndCheckType(1, RVT_NUMBER);
                execPopAndCheckType(0, RVT_NUMBER);

                numResult = operand[0]->data.num + operand[1]->data.num;

                vlPushBack(machine->stack, rtvalueCreateNumber(numResult));
                rtvalueDestroy(operand[0]);
                rtvalueDestroy(operand[1]);
	            break;
            }
            case KBO_OPR_SUB: {
                execPopAndCheckType(1, RVT_NUMBER);
                execPopAndCheckType(0, RVT_NUMBER);

                numResult = operand[0]->data.num - operand[1]->data.num;

                vlPushBack(machine->stack, rtvalueCreateNumber(numResult));
                rtvalueDestroy(operand[0]);
                rtvalueDestroy(operand[1]);
	            break;
            }
            case KBO_OPR_MUL: {
                execPopAndCheckType(1, RVT_NUMBER);
                execPopAndCheckType(0, RVT_NUMBER);

                numResult = operand[0]->data.num * operand[1]->data.num;

                vlPushBack(machine->stack, rtvalueCreateNumber(numResult));
                rtvalueDestroy(operand[0]);
                rtvalueDestroy(operand[1]);
	            break;
            }
            case KBO_OPR_DIV: {
                execPopAndCheckType(1, RVT_NUMBER);
                execPopAndCheckType(0, RVT_NUMBER);

                numResult = operand[0]->data.num / operand[1]->data.num;
                vlPushBack(machine->stack, rtvalueCreateNumber(numResult));
                rtvalueDestroy(operand[0]);
                rtvalueDestroy(operand[1]);
	            break;
            }
            case KBO_OPR_INTDIV: {
                execPopAndCheckType(1, RVT_NUMBER);
                execPopAndCheckType(0, RVT_NUMBER);

                numResult = (KB_FLOAT)((int)operand[0]->data.num / (int)operand[1]->data.num);

                vlPushBack(machine->stack, rtvalueCreateNumber(numResult));
                rtvalueDestroy(operand[0]);
                rtvalueDestroy(operand[1]);
	            break;
            }
            case KBO_OPR_MOD: {
                execPopAndCheckType(1, RVT_NUMBER);
                execPopAndCheckType(0, RVT_NUMBER);

                numResult = (KB_FLOAT)((int)operand[0]->data.num % (int)operand[1]->data.num);

                vlPushBack(machine->stack, rtvalueCreateNumber(numResult));
                rtvalueDestroy(operand[0]);
                rtvalueDestroy(operand[1]);
	            break;
            }
            case KBO_OPR_NOT: {
                /* 旧逻辑，只处理数字 */
                /*
                execPopAndCheckType(0, RVT_NUMBER);

                numResult = (KB_FLOAT)(!(int)operand[0]->data.num);

                vlPushBack(machine->stack, rtvalueCreateNumber(numResult));
                rtvalueDestroy(operand[0]);
                */
                /* 新逻辑，除了数字，增加了字符串的逻辑 */
                KbRuntimeValue* popped = (KbRuntimeValue *)vlPopBack(machine->stack);
                int isTrue = rtvalueIsTrue(popped);
                rtvalueDestroy(popped);
                /* 入栈一个相反的数字值 */
                vlPushBack(machine->stack, rtvalueCreateNumber((KB_FLOAT)!isTrue));
	            break;
            }
            case KBO_OPR_AND: {
                execPopAndCheckType(1, RVT_NUMBER);
                execPopAndCheckType(0, RVT_NUMBER);

                numResult = (KB_FLOAT)((int)operand[0]->data.num && (int)operand[1]->data.num);

                vlPushBack(machine->stack, rtvalueCreateNumber(numResult));
                rtvalueDestroy(operand[0]);
                rtvalueDestroy(operand[1]);
	            break;
            }
            case KBO_OPR_OR: {
                execPopAndCheckType(1, RVT_NUMBER);
                execPopAndCheckType(0, RVT_NUMBER);

                numResult = (KB_FLOAT)((int)operand[0]->data.num || (int)operand[1]->data.num);

                vlPushBack(machine->stack, rtvalueCreateNumber(numResult));
                rtvalueDestroy(operand[0]);
                rtvalueDestroy(operand[1]);
                break;
            }
            case KBO_OPR_EQUAL: {
                execPopAndCheckType(1, RVT_NUMBER);
                execPopAndCheckType(0, RVT_NUMBER);

                numResult = (KB_FLOAT)(operand[0]->data.num == operand[1]->data.num);

                vlPushBack(machine->stack, rtvalueCreateNumber(numResult));
                rtvalueDestroy(operand[0]);
                rtvalueDestroy(operand[1]);
	            break;
            }
            case KBO_OPR_EQUAL_REL: {
                execPopAndCheckType(1, RVT_NUMBER);
                execPopAndCheckType(0, RVT_NUMBER);

                numResult = (KB_FLOAT)kFloatEqualRel(operand[0]->data.num, operand[1]->data.num);

                vlPushBack(machine->stack, rtvalueCreateNumber(numResult));
                rtvalueDestroy(operand[0]);
                rtvalueDestroy(operand[1]);
	            break;
            }
            case KBO_OPR_NEQ: {
                execPopAndCheckType(1, RVT_NUMBER);
                execPopAndCheckType(0, RVT_NUMBER);

                numResult = (KB_FLOAT)(operand[0]->data.num != operand[1]->data.num);

                vlPushBack(machine->stack, rtvalueCreateNumber(numResult));
                rtvalueDestroy(operand[0]);
                rtvalueDestroy(operand[1]);
                break;
            }
            case KBO_OPR_GT: {
                execPopAndCheckType(1, RVT_NUMBER);
                execPopAndCheckType(0, RVT_NUMBER);

                numResult = (KB_FLOAT)(operand[0]->data.num > operand[1]->data.num);

                vlPushBack(machine->stack, rtvalueCreateNumber(numResult));
                rtvalueDestroy(operand[0]);
                rtvalueDestroy(operand[1]);
	            break;
            }
            case KBO_OPR_LT: {
                execPopAndCheckType(1, RVT_NUMBER);
                execPopAndCheckType(0, RVT_NUMBER);

                numResult = (KB_FLOAT)(operand[0]->data.num < operand[1]->data.num);

                vlPushBack(machine->stack, rtvalueCreateNumber(numResult));
                rtvalueDestroy(operand[0]);
                rtvalueDestroy(operand[1]);
	            break;
            }
            case KBO_OPR_GTEQ: {
                execPopAndCheckType(1, RVT_NUMBER);
                execPopAndCheckType(0, RVT_NUMBER);

                numResult = (KB_FLOAT)(operand[0]->data.num >= operand[1]->data.num);

                vlPushBack(machine->stack, rtvalueCreateNumber(numResult));
                rtvalueDestroy(operand[0]);
                rtvalueDestroy(operand[1]);
	            break;
            }
            case KBO_OPR_LTEQ: {
                execPopAndCheckType(1, RVT_NUMBER);
                execPopAndCheckType(0, RVT_NUMBER);

                numResult = (KB_FLOAT)(operand[0]->data.num <= operand[1]->data.num);
                
                vlPushBack(machine->stack, rtvalueCreateNumber(numResult));
                rtvalueDestroy(operand[0]);
                rtvalueDestroy(operand[1]);
	            break;
            }
            case KBO_CALL_BUILT_IN: {
                int isCallSuccess = machineExecCallBuiltInFunc(cmd->param.index, machine, errorRet, operand);
                if (!isCallSuccess) {
                    return 0;
                }
	            break;
            }
            case KBO_CALL_USER: {
                int i;
                int currentPos = machine->cmdPtr - cmdBlockPtr;
                const KbExportedFunction* pExpFunc = machineGetFunctionByIndex(machine, cmd->param.index);
                KbCallEnv *pEnv = callEnvCreate(currentPos, pExpFunc);
                for (i = 0; i < pEnv->numArg; ++i) {
                    pEnv->variables[pEnv->numArg - 1 - i] = (KbRuntimeValue *)vlPopBack(machine->stack);
                }
                vlPushBack(machine->callEnvStack, pEnv);
                machine->cmdPtr = cmdBlockPtr + pExpFunc->pos;
                continue;
            }
            case KBO_ASSIGN_VAR: {
                KbRuntimeValue *varValue = machine->variables[cmd->param.index];
                KbRuntimeValue *popped = (KbRuntimeValue *)vlPopBack(machine->stack);
                rtvalueDestroy(varValue);
                machine->variables[cmd->param.index] = popped;
	            break;
            }
            case KBO_ASSIGN_LOCAL: {
                KbCallEnv* pCallEnv;
                if (machine->callEnvStack->size <= 0) {
                    execReturnErrorWithInt(KBRE_NOT_IN_USER_FUNC, cmd->op);
                }
                pCallEnv = (KbCallEnv *)machine->callEnvStack->tail->data;
                KbRuntimeValue *varValue = pCallEnv->variables[cmd->param.index];
                KbRuntimeValue *popped = (KbRuntimeValue *)vlPopBack(machine->stack);
                rtvalueDestroy(varValue);
                pCallEnv->variables[cmd->param.index] = popped;
	            break;
            }
            case KBO_GOTO: {
                machine->cmdPtr = cmdBlockPtr + cmd->param.index;
	            continue;
            }
            case KBO_IF_GOTO: {
                KbRuntimeValue *popped = (KbRuntimeValue *)vlPopBack(machine->stack);
                int isTrue = rtvalueIsTrue(popped);
                rtvalueDestroy(popped);
                if (isTrue) {
                    machine->cmdPtr = cmdBlockPtr + cmd->param.index;
                    continue;
                }
                break;
            }
            case KBO_UNLESS_GOTO: {
                KbRuntimeValue *popped = (KbRuntimeValue *)vlPopBack(machine->stack);
                int isTrue = rtvalueIsTrue(popped);
                rtvalueDestroy(popped);
                if (!isTrue) {
                    machine->cmdPtr = cmdBlockPtr + cmd->param.index;
                    continue;
                }
                break;
            }
            case KBO_RETURN: {
                KbCallEnv* pCallEnv;
                if (machine->callEnvStack->size <= 0) {
                    execReturnErrorWithInt(KBRE_NOT_IN_USER_FUNC, cmd->op);
                }
                pCallEnv = (KbCallEnv *)vlPopBack(machine->callEnvStack);
                machine->cmdPtr = cmdBlockPtr + pCallEnv->prevCmdPos + 1;
                callEnvDestroy(pCallEnv);
                continue;
            }
            case KBO_STOP: {
	            goto stopExec;
            }
            default: {
                execReturnErrorWithInt(KBRE_UNKNOWN_CMD, cmd->op);
                break;
            }
        }
        ++machine->cmdPtr;
    }

stopExec:

    return 1;
}

#define machineExecCallBuiltInFuncSingleArg(mathFuncPtr) NULL;      \
    execPopAndCheckType(0, RVT_NUMBER);                             \
    numResult = (KB_FLOAT)mathFuncPtr(operand[0]->data.num);        \
    vlPushBack(machine->stack, rtvalueCreateNumber(numResult));     \
    rtvalueDestroy(operand[0])

int machineExecCallBuiltInFunc (int funcId, KbMachine* machine, KbRuntimeError *errorRet, KbRuntimeValue ** operand) {
    KB_FLOAT numResult;
    switch (funcId) {
        case KFID_P: {
            KbRuntimeValue *popped = (KbRuntimeValue *)vlPopBack(machine->stack);
            char *sz = rtvalueStringify(popped);
            rtvalueDestroy(popped);
#ifndef DISABLE_P_FUNCTION
            printf("%s\n", sz);
#endif
            free(sz);
            vlPushBack(machine->stack, rtvalueCreateNumber(0));
            return 1;
        }
        case KFID_SIN: {
            machineExecCallBuiltInFuncSingleArg(sin);
            return 1;
        }
        case KFID_COS: {
            machineExecCallBuiltInFuncSingleArg(cos);
            return 1;
        }
        case KFID_TAN: {
            machineExecCallBuiltInFuncSingleArg(tan);
            return 1;
        }
        case KFID_SQRT: {
            machineExecCallBuiltInFuncSingleArg(sqrt);
            return 1;
        }
        case KFID_EXP: {
            machineExecCallBuiltInFuncSingleArg(exp);
            return 1;
        }
        case KFID_ABS: {
            machineExecCallBuiltInFuncSingleArg(fabs);
            return 1;
        }
        case KFID_LOG: {
            machineExecCallBuiltInFuncSingleArg(log);
            return 1;
        }
        case KFID_RAND: {
            int randInt = rand() % 1000;
            vlPushBack(machine->stack, rtvalueCreateNumber(1.0f * randInt / 1000));
            return 1;
        }
        case KFID_ZEROPAD: {
            int digits;
            int max = sizeof(stringifyBuf);
            char *buf = stringifyBuf;
            int oldLength;

            execPopAndCheckType(1, RVT_NUMBER);
            execPopAndCheckType(0, RVT_NUMBER);

            digits = (int)operand[1]->data.num;
            kFtoa(operand[0]->data.num, buf, max);
            oldLength = strlen(buf);

            if (digits < max - 1 && oldLength < digits) {
                int i;
                int needed = digits - oldLength;
                for (i = 0; i < oldLength; ++i) {
                    buf[i + needed] = buf[i];
                }
                buf[i + needed] = '\0';
                for (i = 0; i < needed; ++i) {
                    buf[i] = '0';
                }
            }

            vlPushBack(machine->stack, rtvalueCreateString(buf));
            rtvalueDestroy(operand[0]);
            rtvalueDestroy(operand[1]);
            
            return 1;
        }
    }
    execReturnErrorWithInt(KBRE_UNKNOWN_BUILT_IN_FUNC, funcId);
}