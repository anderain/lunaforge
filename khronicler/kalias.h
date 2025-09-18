#ifndef _KALIAS_H_
#define _KALIAS_H_

#define StringCopy          KUtils_StringCopy
#define StringDump          KUtils_StringDump
#define StringLength        strlen
#define StringConcat        KUtils_StringConcat
#define IsStringEqual       KUtils_IsStringEqual 
#define IsStringEndWith     KUtils_IsStringEndWith
#define Atof                KUtils_Atof
#define Ftoa                KUtils_Ftoa
#define Atoi                KUtils_Atoi
#define Itoa                KUtils_Itoa
#define FloatEqualRel       KUtils_FloatEqualRel
#define isLittleEndian      KUtils_IsLittleEndian

#define nextToken           KAnalyzer_NextToken
#define rewindToken         KAnalyzer_RewindToken
#define resetToken          KAnalyzer_ResetToken
#define initAnalyzer        KAnalyzer_Initialize
#define getCurrentPtr       KAnalyzer_GetCurrentPtr
#define setCurrentPtr       KAnalyzer_SetCurrentPtr
#define getTokenTypeName    KToken_GetTypeName
#define Token               KbToken
#define Analyzer            KbLineAnalyzer

#define getSyntaxErrMsg     KSyntaxError_GetMessageById
#define getSyntaxErrName    KSyntaxError_GetNameById
#define getStatementName    KStatement_GetNameById
#define getAstTypeNameById  KAst_GetNameById
#define destroyAst          KAstNode_Destroy
#define initParser          KSourceParser_Initialize
#define parseAsAst          KSourceParser_Parse
#define AstNode             KbAstNode
#define Parser              KbSourceParser

#define getSemanticErrMsg   KSemanticError_GetMessageById
#define getSemanticErrName  KSemanticError_GetNameById
#define destroyContext      KompilerContext_Destroy
#define buildContext        KompilerContext_Build
#define serializeContext    KompilerContext_Serialize
#define Context             KbCompilerContext
#define FuncDecl            KbFunctionDeclaration
#define VarDecl             KbVariableDeclaration
#define GotoLabel           KbGotoLabel
#define CtrlFlowLabel       KbControlFlowLabel

#define getOpCodeName           Kommon_GetOpCodeName
#define getOperatorPriorityById Kommon_GetOperatorPriorityById
#define getOperatorNameById     Kommon_GetOperatorNameById
#define BinHeader               KbBinaryHeader
#define BinFuncInfo             KbBinaryFunctionInfo

#define getRtValueTypeName  KRuntimeValue_GetTypeNameById
#define stringifyRtValue    KRuntimeValue_Stringify
#define getRuntimeErrMsg    KRuntimeError_GetMessageById
#define getRuntimeErrName   KRuntimeError_GetNameById
#define createMachine       KRuntime_CreateMachine
#define destroyMachine      KRuntime_DestroyMachine
#define executeMachine      KRuntime_MachineExecute
#define Machine             KbVirtualMachine
#define RtValue             KbRuntimeValue
#define CallEnv             KbCallEnv

#endif