#ifndef _KBASIC_H_
#define _KBASIC_H_

#include "kommon.h"

#define CTRL_LABEL_FORMAT "!_CS%03d_"
#define FUNC_LABEL_FORMAT "!_FC%03d_"

typedef enum {
    KBE_NO_ERROR = 0,
    KBE_SYNTAX_ERROR,
    KBE_UNDEFINED_IDENTIFIER,
    KBE_UNDEFINED_FUNC,
    KBE_INVALID_NUM_ARGS,
    KBE_STRING_NO_SPACE,
    KBE_TOO_MANY_VAR,
    KBE_ID_TOO_LONG,
    KBE_DUPLICATED_LABEL,
    KBE_DUPLICATED_VAR,
    KBE_UNDEFINED_LABEL,
    KBE_INCOMPLETE_CTRL_FLOW,
    KBE_INCOMPLETE_FUNC,
    KBE_OTHER
} KB_BUILD_ERROR;

typedef struct {
    int errorType;
    int errorPos;
    char message[200];
} KbBuildError;

typedef enum {
    KBL_USER = 0,
    KBL_CTRL,
} KbLabelType;

typedef struct {
    KbLabelType type;
    KDword      pos;
    char        name[0];
} KbContextLabel;

typedef enum {
    KBCS_NONE       = 0,
    KBCS_IF_THEN    = 10,
    KBCS_ELSE_IF    = 20,
    KBCS_ELSE       = 30,
    KBCS_WHILE      = 40
} KB_CTRL_STRUCT_TYPE;

typedef struct {
    int csType;
    int iLabelNext;
    int iLabelEnd;
} KbCtrlFlowItem;

typedef struct {
    char    funcName[KN_ID_LEN_MAX + 1];
    int     numArg;
    int     numVar;
    char    varList[KB_CONTEXT_VAR_MAX][KN_ID_LEN_MAX + 1];
    int     iLblFuncBegin;
    int     iLblFuncEnd;
} KbUserFunc;

typedef struct {
    char        varList[KB_CONTEXT_VAR_MAX][KN_ID_LEN_MAX + 1];
    int         numVar;
    char        stringBuffer[KB_CONTEXT_STRING_BUFFER_MAX];
    char*       stringBufferPtr;
    Vlist*      commandList;    /* <KbOpCommand> */ 
    Vlist*      labelList;      /* <KbContextLabel> */
    Vlist*      userFuncList;   /* <KbUserFunc> */
    struct {
        int     labelCounter;
        Vlist*  stack;          /* <KbCtrlFlowItem> */
    } ctrlFlow;
    int         funcLabelCounter;
    KbUserFunc* pCurrentFunc;
} KbContext;

KbContext*  contextCreate           ();
void        contextDestroy          (KbContext *context);
int         contextSetVariable      (KbContext *context, const char * varName);
void        contextCtrlResetCounter (KbContext* context);

int         kbFormatBuildError      (const KbBuildError *errorVal, char *strBuffer, int strLengthMax);
KbContext*  kbCompileStart          ();
int         kbScanLineSyntax        (const char* line, KbContext *context, KbBuildError *errorRet);
int         kbCompileLine           (const char* lineContent, KbContext *context, KbBuildError *pErrorRet);
int         kbCompileEnd            (KbContext* context);
int         kbSerialize             (const KbContext* context, unsigned char** pRaw, int* pByteLength);

void dbgPrintContextCommandList     (const KbContext *context);
void dbgPrintContextVariables       (const KbContext *context);
void dbgPrintContextListText        (const KbContext *context);
void dbgPrintContextListLabel       (const KbContext *context);
void dbgPrintContextListFunction    (const KbContext *context);
void dbgPrintHeader                 (const KbBinaryHeader *);

#endif
