#ifndef _KOMPILER_H_
#define _KOMPILER_H_

#include "kparser.h"

typedef struct tagKbVariableDeclaration {
    char            szVarName[KB_IDENTIFIER_LEN_MAX + 1];
    int             iIndex;
    VarDeclTypeId   iType;
} KbVariableDeclaration;

typedef struct tagKbFunctionDeclaration {
    char    szFuncName[KB_IDENTIFIER_LEN_MAX + 1];
    int     iNumParams;
    int     iIndex;
    int     iOpCodeStartPos;
    Vlist*  pListVariables; /* <KbVariableDeclaration> */
} KbFunctionDeclaration;

typedef struct tagKbGotoLabel {
    char szLabelName[KB_IDENTIFIER_LEN_MAX + 1];
    KLabelOpCodePos iOpCodePos; 
    KbFunctionDeclaration* pFuncDeclScope;
} KbGotoLabel;

typedef enum tagControlFlowTypeId {
    CF_NONE = 0,
    CF_FUNCTION,
    CF_IF,
    CF_WHILE,
    CF_DO_WHILE,
    CF_FOR
} ControlFlowTypeId;

typedef struct tagControlFlowLabel {
    ControlFlowTypeId iType;
    union {
        struct {
            KLabelOpCodePos iEndPos;
        } sFunction;
        struct {
            KLabelOpCodePos iThenEndPos;
            KLabelOpCodePos* pArrElseIfEndPos;
            KLabelOpCodePos iEndPos;
        } sIf;
        struct {
            KLabelOpCodePos iCondPos;
            KLabelOpCodePos iEndPos;
        } sWhile;
        struct {
            KLabelOpCodePos iStartPos;
            KLabelOpCodePos iCondPos;
            KLabelOpCodePos iEndPos;
        } sDoWhile;
        struct {
            KLabelOpCodePos iCondPos;
            KLabelOpCodePos iIncreasePos;
            KLabelOpCodePos iEndPos;
        } sFor;
    } uData;
} KbControlFlowLabel;

typedef struct tagKbCompilerContext {
    Vlist*  pListGlobalVariables;   /* <KbVariableDeclaration> */
    Vlist*  pListFunctions;         /* <KbFunctionDeclaration> */
    char    szStringPool[KB_CONTEXT_STRING_POOL_MAX];
    int     iStringPoolSize;
    Vlist*  pListOpCodes;           /* <OpCode> */
    Vlist*  pListLabels;            /* <KbGotoLabel> */
    KbFunctionDeclaration* pCurrentFunc;
    int     iNumCtrlFlowLabels;
    KbControlFlowLabel* pCtrlFlowLabels;
    char    szExtensionId[KB_IDENTIFIER_LEN_MAX + 1];
    Vlist*  pListExtFuncs;          /* <KbExtensionFunction> */
} KbCompilerContext;

typedef enum {
    SEM_NO_ERROR = 0,
    SEM_UNRECOGNIZED_AST,
    SEM_NOT_A_PROGRAM,
    SEM_VAR_NAME_TOO_LONG,
    SEM_VAR_DUPLICATED,
    SEM_VAR_NOT_FOUND,
    SEM_VAR_IS_NOT_ARRAY,
    SEM_VAR_IS_NOT_PRIMITIVE,
    SEM_FUNC_NAME_TOO_LONG,
    SEM_FUNC_DUPLICATED,
    SEM_FUNC_NOT_FOUND,
    SEM_FUNC_ARG_LIST_MISMATCH,
    SEM_LABEL_NAME_TOO_LONG,
    SEM_LABEL_DUPLICATED,
    SEM_GOTO_LABEL_NOT_FOUND,
    SEM_GOTO_LABEL_SCOPE_MISMATCH,
    SEM_STR_POOL_EXCEED
} SemanticErrorId;

const char*         KSemanticError_GetNameById      (SemanticErrorId iSemanticErrorId);
const char*         KSemanticError_GetMessageById   (SemanticErrorId iSemanticErrorId);
const char*         KExtensionError_GetMessageById  (ExtensionErrorId iExtErrorId);
void                KompilerContext_Destroy         (KbCompilerContext* pContext);
KbCompilerContext*  KompilerContext_Create          (const KbAstNode* pAstProgram);
KBool               KompilerContext_Build           (KbCompilerContext* pContext, const KbAstNode* pAstProgram, SemanticErrorId* pIntSemanticError, const KbAstNode** pPtrAstStop);
KBool               KompilerContext_Serialize       (const KbCompilerContext* pContext, KByte** pPtrByteRaw, KDword* pDwRawLength);

#endif