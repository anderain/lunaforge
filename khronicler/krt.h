#ifndef _KRT_H_
#define _KRT_H_

#if defined(_SH3) || defined(_SH4)
#   ifndef _FX_9860_
#       define _FX_9860_
#   endif
#endif

#ifdef _FX_9860_
#   include "../core/kommon.h"
#else
#   include "kommon.h"
#endif

typedef enum {
    KBRE_NO_ERROR,
    KBRE_OPERAND_TYPE_MISMATCH,
    KBRE_STACK_OVERFLOW,
    KBRE_ILLEGAL_RETURN,
    KBRE_UNKNOWN_BUILT_IN_FUNC,
    KBRE_UNKNOWN_CMD,
    KBRE_NOT_IN_USER_FUNC,
} KB_RT_ERROR_TYPE;

typedef enum {
    RVT_NIL = 0,
    RVT_NUMBER,
    RVT_STRING
} KB_RT_VALUE_TYPE;

typedef struct {
    int type;
    union {
        char *sz;
        KB_FLOAT num;
    } data;
} KbRuntimeValue;

KbRuntimeValue* rtvalueCreateNumber         (const KB_FLOAT num);
KbRuntimeValue* rtvalueCreateString         (const char* sz);
void            rtvalueDestroy              (KbRuntimeValue* val);
void            rtvalueDestoryVoidPointer   (void* p);
char*           rtvalueStringify            (const KbRuntimeValue* v);

typedef struct {
    int                 prevCmdPos;
    int                 numVar, numArg;
    KbRuntimeValue**    variables;
} KbCallEnv;

typedef struct {
    const KbBinaryHeader*       header;
    Vlist*                      stack;          /* <KbRuntimeValue> */
    const KbOpCommand *         cmdPtr;
    const unsigned char*        raw;
    KbRuntimeValue**            variables;
    Vlist*                      callEnvStack;   /* <KbCallEnv> */
    const KbExportedFunction*   pExportedFunc;
} KbMachine;

typedef struct {
    int     type;
    int     cmdOp;
    char    message[KB_ERROR_MESSAGE_MAX];
} KbRuntimeError;

KbMachine*  machineCreate                   (const unsigned char * raw);
void        machineCommandReset             (KbMachine* machine);
int         machineGetLabelPos              (KbMachine* machine, const char* labelName);
int         machineExec                     (KbMachine* machine, int startPos, KbRuntimeError *errorRet);
void        machineDestroy                  (KbMachine* machine);
int         machineVarAssignNum             (KbMachine* machine, int varIndex, KB_FLOAT num);
void        machineMovePushValue            (KbMachine* machine, KbRuntimeValue* pRtValue);
int         machineGetUserFuncIndex         (KbMachine* machine, const char* funcName);
int         machineExecCallUserFuncByIndex  (KbMachine* machine, int funcIndex, KbRuntimeError *errorRet);
#define     machinePopUserFuncRetValue(app) ((KbRuntimeValue *)vlPopBack((app)->stack))
void        formatExecError                 (const KbRuntimeError *errorRet, char *message, int messageLength);

#endif