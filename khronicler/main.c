#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kbasic.h"
#include "kalias.h"
#include "test.h"

#define TARGET_NONE         0
#define TARGET_COMPILE      1
#define TARGET_DUMP         2
#define TARGET_INSPECT      3
#define TARGET_EXECUTE      4
#define CLI_COMPILE         "--compile"
#define CLI_COMPILE_S       "-c"
#define CLI_DUMP            "--dump"
#define CLI_DUMP_S          "-d"
#define CLI_INSPECT         "--inspect"
#define CLI_INSPECT_S       "-n"
#define CLI_EXECUTE         "--execute"
#define CLI_EXECUTE_S       "-e"
#define CLI_OUTPUT          "--output"
#define CLI_OUTPUT_S        "-o"
#define ARG_IS(param)       (strcmp((param), argv[argIndex]) == 0)
#define HAVE_ARG()          (argIndex < argc)
#define NEXT_ARG()          (argIndex++)
#define CURRENT_ARG()       (argv[argIndex])
#define EXE_NAME            "khronicler"

char szErrorMessage[200]; /* 各种错误的格式化缓冲区 */

typedef struct {
    int     lineNum;
    int     cmdPos;
    char    lineStr[1];
} CmdLineMapping;

struct {
    int         target;
    const char* inputPath;
    const char* outputPath;
} cliParams = { TARGET_NONE, NULL, NULL };

/* 文件工具函数 */
char*   readTextFile    (const char *fileName);
KByte*  readBinaryFile  (const char *filename);
int     writeBinaryFile (const char *fileName, const void * data, int length);

void displayUsage(const char* exeName) {
    fprintf(
        stderr,
        "Khronicler KBasic Compiler Tool\n"
        "\n"
        "Usage:\n"
        "  %s, %-12s <input>  Compile script to bytecode file\n"
        "  %s, %-12s <file>   Output file (use with --compile)\n"
        "  %s, %-12s <file>   Analyze script and dump bytecode\n"
        "  %s, %-12s <file>   Inspect bytecode file\n"
        "  %s, %-12s <file>   Execute bytecode file\n"
        "\n"
        "Examples:\n"
        "  Compile:  %s %s program.kbs -o bytecode.kbn\n"
        "  Dump:     %s %s program.kbs\n"
        "  Inspect:  %s %s bytecode.kbn\n"
        "  Execute:  %s %s bytecode.kbn\n",
        CLI_COMPILE_S, CLI_COMPILE,
        CLI_OUTPUT_S, CLI_OUTPUT,
        CLI_DUMP_S, CLI_DUMP,
        CLI_INSPECT_S, CLI_INSPECT,
        CLI_EXECUTE_S, CLI_EXECUTE,
        exeName, CLI_COMPILE_S,
        exeName, CLI_DUMP_S,
        exeName, CLI_INSPECT_S,
        exeName, CLI_EXECUTE_S
    );
}

int parseCommandLineParameters(int argc, char** argv) {
    int argIndex = 1;
    
    cliParams.target = TARGET_NONE;
    cliParams.inputPath = NULL;
    cliParams.outputPath = NULL;

    while (HAVE_ARG()) {
        /* 输出文件名 */
        if (ARG_IS(CLI_OUTPUT) || ARG_IS(CLI_OUTPUT_S)) {
            NEXT_ARG();
            if (!HAVE_ARG()) {
                fprintf(stderr, "Invalid parameter: missing output file after -o flag.\n\n");
                return 0;
            }
            cliParams.outputPath = CURRENT_ARG();
        }
        /* 编译字节码模式 */
        else if (ARG_IS(CLI_COMPILE) || ARG_IS(CLI_COMPILE_S)) {
            cliParams.target = TARGET_COMPILE;
        }
        /* 输出编译字节码分析 */
        else if (ARG_IS(CLI_DUMP) || ARG_IS(CLI_DUMP_S)) {
            cliParams.target = TARGET_DUMP;
        }
        /* 分析字节码文件 */
        else if (ARG_IS(CLI_INSPECT) || ARG_IS(CLI_INSPECT_S)) {
            cliParams.target = TARGET_INSPECT;
        }
        /* 执行字节码文件 */
        else if (ARG_IS(CLI_EXECUTE) || ARG_IS(CLI_EXECUTE_S)) {
            cliParams.target = TARGET_EXECUTE;
        }
        /* 输入文件名 */
        else {
            if (cliParams.inputPath != NULL) {
                fprintf(stderr, "Invalid flag: unrecognized flag '%s'.\n\n", CURRENT_ARG());
                return 0;
            }
            cliParams.inputPath = CURRENT_ARG();
        }
        NEXT_ARG();
    }

    if (cliParams.target == TARGET_NONE) {
        fprintf(stderr, "Missing target.\n\n");
        return 0;
    }

    if (cliParams.inputPath == NULL) {
        fprintf(stderr, "Missing input file.\n\n");
        return 0;
    }

    if (cliParams.target == TARGET_COMPILE && cliParams.outputPath == NULL) {
        fprintf(stderr, "Missing output file.\n\n");
        return 0;
    }

    return 1;
}

KBool buildFromScript(const char* szSource, Context** pPtrContext) {
    /* Parser 部分 */
    AstNode*        pAstProgram;            /* 源代码解析后生成的 AST 根节点 */
    SyntaxErrorId   iSyntaxErrorId;         /* 语法错误 ID */
    StatementId     iStopStatement;         /* 语法错误停止的语句类型 */
    int             iStopLineNumber;        /* 停止时候的行号 */
    /* Compiler 部分 */
    Context*        pContext;               /* 编译上下文 */
    const AstNode*  pAstSemStop;            /* 编译解析 AST 遇到错误停止时的节点 */
    SemanticErrorId iSemanticErrorId;       /* 语义错误 ID */

    /* 解析源代码为 AST */
    pAstProgram = parseAsAst(szSource, &iSyntaxErrorId, &iStopStatement, &iStopLineNumber);
    /* 有语法错误 */
    if (iSyntaxErrorId != SYN_NO_ERROR) {
        formatSyntaxErrorMessage(szErrorMessage, iStopLineNumber, iStopStatement, iSyntaxErrorId);
        fprintf(stderr, "[Line %d]\n%s\n", iStopLineNumber, szErrorMessage);
        return KB_FALSE;
    }

    /* 编译 AST 为上下文 */
    pContext = buildContext(pAstProgram, &iSemanticErrorId, &pAstSemStop);
    /* 有语义错误 */
    if (iSemanticErrorId != SEM_NO_ERROR) {
        formatSemanticErrorMessage(szErrorMessage, pAstSemStop, iSemanticErrorId);
        fprintf(stderr, "[Line %d]\n%s\n", pAstSemStop->iLineNumber, szErrorMessage);
        destroyAst(pAstProgram);
        destroyContext(pContext);
        return KB_FALSE;
    }
    destroyAst(pAstProgram);

    *pPtrContext = pContext;
    return KB_TRUE;
}

int main(int argc, char** argv) {
    KBool   bRunSuccess         = KB_FALSE;
    KBool   bParseSuccess       = KB_FALSE;
    char*   szInputText         = NULL;
    KByte*  pByteInputBinary    = NULL;

    bParseSuccess = parseCommandLineParameters(argc, argv);
    if (!bParseSuccess) {
        displayUsage(EXE_NAME);
        return 1;
    }
    
    switch (cliParams.target) {
    case TARGET_COMPILE:
    case TARGET_DUMP:
        szInputText = readTextFile(cliParams.inputPath);
        if (!szInputText) {
            fprintf(stderr, "Failed to load script '%s'\n", cliParams.inputPath);
            goto dispose;
        }
        break;
    case TARGET_INSPECT:
    case TARGET_EXECUTE:
        pByteInputBinary = readBinaryFile(cliParams.inputPath);
        if (!pByteInputBinary) {
            fprintf(stderr, "Failed to load binary file '%s'\n", cliParams.inputPath);
            goto dispose;
        }
        break;
    }

    if (cliParams.target == TARGET_COMPILE) {
        Context*    pContext;
        KDword      dwRawSize;
        KByte*      pRawSerialized;
        KBool       bWriteSuccess;
        /* 编译脚本为上下文 */
        if (!buildFromScript(szInputText, &pContext)) {
            goto dispose;
        }
        /* 序列化上下文 */
        serializeContext(pContext, &pRawSerialized, &dwRawSize);
        destroyContext(pContext);
        /* 写入文件 */
        bWriteSuccess = writeBinaryFile(cliParams.outputPath, pRawSerialized, dwRawSize);
        free(pRawSerialized);
        if (!bWriteSuccess) {
            fprintf(stderr, "Failed to write '%s'\n", cliParams.outputPath);
        }
        bRunSuccess = KB_TRUE;
    }
    else if (cliParams.target == TARGET_DUMP) {
        Context*    pContext;
        KDword      dwRawSize;
        KByte*      pRawSerialized;
        /* 编译脚本为上下文 */
        if (!buildFromScript(szInputText, &pContext)) {
            goto dispose;
        }
        /* 序列化上下文 */
        serializeContext(pContext, &pRawSerialized, &dwRawSize);
        destroyContext(pContext);
        /* 输出分析信息 */
        dumpKbasicBinary(NULL, pRawSerialized);
        free(pRawSerialized);
        bRunSuccess = KB_TRUE;
    }
    else if (cliParams.target == TARGET_INSPECT) {
        dumpKbasicBinary(NULL, pByteInputBinary);
        bRunSuccess = KB_TRUE;
    }
    else if (cliParams.target == TARGET_EXECUTE) {
        Machine*        pMachine;               /* 虚拟机实例 */
        KBool           bExecuteSuccess;        /* 执行是否成功结束 */
        RuntimeErrorId  iRuntimeErrorId;        /* 运行时错误 */
        const OpCode*   pStopOpCode;            /* 运行时错误结束的 opCode */
        
        /* 执行 opCode */
        pMachine = createMachine(pByteInputBinary);
        bExecuteSuccess = executeMachine(pMachine, 0, &iRuntimeErrorId, &pStopOpCode);
    
        /* 执行有错误 */
        if (!bExecuteSuccess) {
            formatRuntimeErrorMessage(szErrorMessage, pStopOpCode, iRuntimeErrorId);
            fputs(szErrorMessage, stderr);
        }

        destroyMachine(pMachine);
    }

dispose:
    if (szInputText) free(szInputText);
    if (pByteInputBinary) free(pByteInputBinary);

    return bRunSuccess ? 0 : -1;
}

/* 文件工具函数 */
char *readTextFile(const char *fileName) {
    FILE *fp;
    int length = 0;
    char *buf;

    fp = fopen(fileName, "rb");

    if (!fp) {
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    length = ftell(fp) + 1;

    buf = (char *)malloc(length);
    memset(buf, 0, length);
    fseek(fp, 0, SEEK_SET);
    fread(buf, length, 1, fp);

    fclose(fp);
    return buf;
}

int writeBinaryFile(const char *fileName, const void * data, int length) {
    FILE *fp = fopen(fileName, "wb");
    if (!fp) {
        return 0;
    }
    fwrite(data, length, 1, fp);
    fclose(fp);
    return 1;
}

KByte* readBinaryFile(const char *filename) {
    FILE*   fp;
    int     length = 0;
    KByte*  buf;

    fp = fopen(filename, "rb");

    if (!fp) return NULL;

    fseek(fp, 0, SEEK_END);
    length = ftell(fp);

    buf = (KByte *)malloc(length);
    memset(buf, 0, length);
    fseek(fp, 0, SEEK_SET);
    fread(buf, length, 1, fp);

    fclose(fp);
    return buf;
}