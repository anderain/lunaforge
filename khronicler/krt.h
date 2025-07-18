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
    KBRE_INVALID_IMAGE_INDEX,
    KBRE_UNKNOWN_BUILT_IN_FUNC,
    KBRE_UNKNOWN_CMD
} KB_RT_ERROR_TYPE;

typedef enum {
    RVT_NIL = 0,
    RVT_NUMBER,
    RVT_STRING,
    RVT_INTEGER
} KB_RT_VALUE_TYPE;

typedef struct {
    int type;
    union {
        char *sz;
        KB_FLOAT num;
        int intVal;
    } data;
} KbRuntimeValue;

void rtvalueDestroy                (KbRuntimeValue* val);
void rtvalueDestoryVoidPointer   (void* p);

typedef struct {
    KBASIC_BINARY_HEADER*   header; // pointer of raw
    Vlist*                  stack;  // <KbRuntimeValue>
    KbOpCommand *           cmdPtr;
    unsigned char *         raw;
    KbRuntimeValue **       variables;
} KbMachine;

typedef struct {
    int     type;
    int     cmdOp;
    char    message[KB_ERROR_MESSAGE_MAX];
} KbRuntimeError;

KbMachine*  machineCreate       (unsigned char * raw);
void        machineCommandReset (KbMachine* machine);
int         machineExec         (KbMachine* machine, KbRuntimeError *errorRet);
void        machineDestroy      (KbMachine* machine);
int         machineVarAssignNum (KbMachine* machine, int varIndex, KB_FLOAT num);
void        formatExecError     (const KbRuntimeError *errorRet, char *message, int messageLength);

#endif