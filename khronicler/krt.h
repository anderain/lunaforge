#ifndef _KRT_H_
#define _KRT_H_

#include "kommon.h"
#include "kutils.h"

typedef enum tagRuntimeErrorId {
    RUNTIME_NONE,
    RUNTIME_STACK_UNDERFLOW,
    RUNTIME_TYPE_MISMATCH,
    RUNTIME_UNKNOWN_OPCODE,
    RUNTIME_UNKNOWN_OPERATOR,
    RUNTIME_UNKNOWN_BUILT_IN_FUNC,
    RUNTIME_UNKNOWN_USER_FUNC,
    RUNTIME_DIVISION_BY_ZERO,
    RUNTIME_NOT_IN_USER_FUNC,
    RUNTIME_ARRAY_INVALID_SIZE,
    RUNTIME_ARRAY_OUT_OF_BOUNDS,
    RUNTIME_NOT_ARRAY
} RuntimeErrorId;

typedef enum tagRuntimeValueTypeId {
    RT_VALUE_NIL = 0,
    RT_VALUE_NUMBER,
    RT_VALUE_STRING,
    RT_VALUE_ARRAY,
    RT_VALUE_ARRAY_REF
} RuntimeValueTypeId;

struct tagKbRuntimeValue;

typedef struct {
    struct tagKbRuntimeValue** pArrPtrElements;
    int iSize;
} KbRuntimeArray;

typedef struct tagKbRuntimeValue {
    int iType;
    union {
        struct {
            union {
                const char* pReadOnly;
                char* pReadWrite;
            } uContent;
            KBool bIsRef;
        } sString;
        KbRuntimeArray  sArray;
        KbRuntimeArray* pArrRef;
        KFloat fNumber;
    } uData;
} KbRuntimeValue;

typedef struct {
    int iPrevOpCodePos;
    int iNumVar;
    int iNumParams;
    KbRuntimeValue** pArrPtrLocalVars;
} KbCallEnv;

typedef struct tagKbVirtualMachine {
    const KbBinaryHeader*       pBinHeader;
    Vlist*                      pStackOperand;      /* <KbRuntimeValue> */
    const OpCode *              pOpCodeCur;
    const KByte*                pByteRaw;
    KbRuntimeValue**            pArrPtrGlobalVars;
    Vlist*                      pStackCallEnv;      /* <KbCallEnv> */
    const KbBinaryFunctionInfo* pArrFuncInfo;
    int                         iStopValue;
} KbVirtualMachine;

const char*         KRuntimeValue_GetTypeNameById   (RuntimeValueTypeId iRtTypeId);
const char*         KRuntimeValue_Stringify         (KbRuntimeValue* pRtValue, KBool *pBoolNeedDispose);
const char*         KRuntimeError_GetNameById       (RuntimeErrorId iRuntimeErrorId);
const char*         KRuntimeError_GetMessageById    (RuntimeErrorId iRuntimeErrorId);
KBool               KRuntime_MachineExecute         (KbVirtualMachine* pMachine, int iStartPos, RuntimeErrorId* pIntRtErrId, const OpCode** ppStopOpCode);
KbVirtualMachine*   KRuntime_CreateMachine          (const KByte* pSerializedRaw);
void                KRuntime_DestroyMachine         (KbVirtualMachine* pMachine);

#endif