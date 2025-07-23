#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kbasic.h"
#include "k_utils.h"
#include "krt.h"

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
char*           readTextFile    (const char *fileName);
unsigned char*  readBinaryFile  (const char *filename);
int             writeBinaryFile (const char *fileName, const void * data, int length);

int buildFromScript(const char *fileContent, KbContext** retContext, Vlist* pListMapping) {
    KbBuildError    errorRet;
    KbContext*      context;
    int             ret;
    const char      *textBuf;
    const char      *textPtr;
    int             isSuccess = 1;
    int             lineCount = 1;

    textBuf = fileContent;

    *retContext = NULL;
    context = kbCompileStart();
    
    /*
        第一遍循环
        1. 检查是否有语法错误
        2. 检查控制结构、函数定义是否合法
        3. 自动生成出来控制结构、函数需要的跳转标签
        4. 标注所有用户写的标签
     */
    contextCtrlResetCounter(context);    
    for (textPtr = textBuf, lineCount = 1; *textPtr; lineCount++) {
        char line[1000];
        /* 获取行的内容，并且移除尾部的空白和注释 */
        textPtr += getLineTrimRemarks(textPtr, line);
        /* 扫描一行 */
        ret = kbScanLineSyntax(line, context, &errorRet);
        /* 语法错误 */
        if (!ret) {
            isSuccess = 0;
            goto compileEnd;
        }
    }
    /* 回退，修正行号 */
    --lineCount;
    /* 检查是不是有没结束的函数定义 */
    if (context->pCurrentFunc != NULL) {
        errorRet.errorPos = 0;
        errorRet.errorType = KBE_INCOMPLETE_FUNC;
        errorRet.message[0] = '\0';
        isSuccess = 0;
        goto compileEnd;
    }
    /* 检查是不是有没结束的控制结构 */
    if (context->ctrlFlow.stack->size > 0) {
        errorRet.errorPos = 0;
        errorRet.errorType = KBE_INCOMPLETE_CTRL_FLOW;
        errorRet.message[0] = '\0';
        isSuccess = 0;
        goto compileEnd;
    }
    /*
        第二遍循环，编译生成命令序列。
        本次循环之后，生成的 GOTO 与 IFGOTO 命令的
        `param`.`ptr` 存储的是 KbContextLabel 的指针
     */
    contextCtrlResetCounter(context);
    for (textPtr = textBuf, lineCount = 1; *textPtr; lineCount++) {
        char line[1000];
        int cmdPos = context->commandList->size;
        /* 获取行的内容，并且移除尾部的空白和注释 */
        textPtr += getLineTrimRemarks(textPtr, line);
        /* 保存行对应的 cmd 位置*/
        if (pListMapping) {
            CmdLineMapping* pMapping = malloc(sizeof(CmdLineMapping) + strlen(line));
            strcpy(pMapping->lineStr, line);
            pMapping->cmdPos = cmdPos;
            pMapping->lineNum = lineCount;
            vlPushBack(pListMapping, pMapping);
        }
        /* 编译一行 */
        ret = kbCompileLine(line, context, &errorRet);
        /* 编译错误 */
        if (!ret) {
            isSuccess = 0;
            goto compileEnd;
        }
    }
    /*
        执行完成后 GOTO 与 IFGOTO 命令的 `param`.`index`
        将会被替换为 opcode 的 offset
     */
    kbCompileEnd(context);

compileEnd:
    if (!isSuccess) {
        char buf[200];
        kbFormatBuildError(&errorRet, buf, sizeof(buf));
        printf("Line %d: %s\n", lineCount, buf);
        contextDestroy(context);
        return 0;
    }

    *retContext = context;
    return 1;
}

int runBinary(const unsigned char* raw) {
    KbRuntimeError  errorRet;
    int             ret;
    KbMachine*      app;

    app = machineCreate(raw);
    ret = machineExec(app, 0, &errorRet);
    if (!ret) {
        char error_message[200];
        formatExecError(&errorRet, error_message, sizeof(error_message));
        fprintf(stderr, "Runtime Error:\n%s\n", error_message);
    }
    fprintf(stderr, "[Execution Finished] Call stack -> %d, Value stack -> %d\n", app->callEnvStack->size, app->stack->size);

    machineDestroy(app);
    return 1;
}

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

void dumpRawKBinary(const unsigned char* raw, Vlist* pListMapping) {
    FILE*                       fp      = stdout;
    const KbBinaryHeader*       h       = (const KbBinaryHeader *)raw;
    const KbExportedFunction*   pFunc   = (const KbExportedFunction *)(raw + h->funcBlockStart);
    const KbOpCommand*          pCmd    = (const KbOpCommand *)(raw + h->cmdBlockStart);
    const char *                pStr    = (const char *)(raw + h->stringBlockStart);
    int                         i;

    fprintf(fp, "----------------- Header -----------------\n");
    fprintf(fp, "headerMagic       = 0x%x\n",  h->headerMagic);
    fprintf(fp, "numVariables      = %d\n",    h->numVariables);
    fprintf(fp, "funcBlockStart    = %d\n",    h->funcBlockStart);
    fprintf(fp, "numFunc           = %d\n",    h->numFunc);
    fprintf(fp, "cmdBlockStart     = %d\n",    h->cmdBlockStart);
    fprintf(fp, "numCmd            = %d\n",    h->numCmd);
    fprintf(fp, "stringBlockStart  = %d\n",    h->stringBlockStart);
    fprintf(fp, "stringBlockLength = %d\n",    h->stringBlockLength);

    if (h->numFunc > 0) {
        fprintf(fp, "---------------- Function ----------------\n");
        for (i = 0; i < h->numFunc; ++i) {
            const KbExportedFunction* pf = pFunc + i;
            fprintf(fp, "[%3d] %16s, args = %2d, vars = %2d\n", i, pf->funcName, pf->numArg, pf->numVar - pf->numArg);
        }
    }

    if (h->stringBlockLength > 0) {
        fprintf(fp, "----------------- String -----------------\n");
        int pos = 0;
        int index = 0;
        while (pos < h->stringBlockLength) {
            int len = strlen(pStr + pos);
            fprintf(fp, "[%3d] \"%s\"\n", index, pStr + pos);
            pos += len + 1;
            index++;
        }
    }

    if (h->numCmd > 0) {
        const VlistNode*        pNode = NULL;
        const CmdLineMapping*   pMapping = NULL;

        if (pListMapping) {
            pNode = pListMapping->head;
        }

        fprintf(fp, "------------------ Cmd ------------------\n");
        for (i = 0; i < h->numCmd; ++i) {
            const KbOpCommand* cmd = pCmd + i;
            fprintf(fp, "[%04d] ", i);
            dbgPrintContextCommand(cmd);
            /* 打印对应的行 */
            pMapping = NULL;
            while (pNode != NULL) {
                pMapping = (CmdLineMapping*)pNode->data;
                if (pMapping->cmdPos >= i) {
                    break;
                }
                pNode = pNode->next;
            }
            if (pMapping != NULL && pMapping->cmdPos == i) {
                fprintf(fp, "[%04d] %s", pMapping->lineNum, pMapping->lineStr);
            }
            fprintf(fp, "\n");
        }
    }
}

int main(int argc, char** argv) {
    int             runSuccess      = 0;
    int             parseResult     = 0;
    char*           inputText       = NULL;
    unsigned char*  inputBinary     = NULL;

    parseResult = parseCommandLineParameters(argc, argv);
    if (!parseResult) {
        displayUsage(EXE_NAME);
        return 1;
    }
    
    switch (cliParams.target) {
    case TARGET_COMPILE:
    case TARGET_DUMP:
        inputText = readTextFile(cliParams.inputPath);
        if (!inputText) {
            fprintf(stderr, "Failed to load script '%s'\n", cliParams.inputPath);
        }
        break;
    case TARGET_INSPECT:
    case TARGET_EXECUTE:
        inputBinary = readBinaryFile(cliParams.inputPath);
        if (!inputBinary) {
            fprintf(stderr, "Failed to load binary file '%s'\n", cliParams.inputPath);
        }
        break;
    }

    runSuccess = 0;

    if (cliParams.target == TARGET_COMPILE) {
        KbContext*      context;
        int             compileRet;
        int             serializeRet;
        int             resultByteLength;
        unsigned char*  resultRaw;
        int             writeRet;
        /* 编译内容 */
        compileRet = buildFromScript(inputText, &context, NULL);
        if (!compileRet) goto dispose;
        /* 序列化二进制内容 */
        serializeRet = kbSerialize(context, &resultRaw, &resultByteLength);
        contextDestroy(context);
        if (!serializeRet) {
            fprintf(stderr, "Failed to serialize context.");
            goto dispose;
        }
        /* 写入文件 */
        writeRet = writeBinaryFile(cliParams.outputPath, resultRaw, resultByteLength);
        free(resultRaw);
        if (!writeRet) {
            fprintf(stderr, "Failed to write '%s'\n", cliParams.outputPath);
        }
        runSuccess = 1;
    }
    else if (cliParams.target == TARGET_DUMP) {
        KbContext*      context;
        int             compileRet;
        int             serializeRet;
        int             resultByteLength;
        unsigned char*  resultRaw;
        Vlist*          pListMapping = vlNewList();
        /* 编译内容 */
        compileRet = buildFromScript(inputText, &context, pListMapping);
        if (!compileRet) goto dispose;
        /* 序列化二进制内容 */
        serializeRet = kbSerialize(context, &resultRaw, &resultByteLength);
        contextDestroy(context);
        if (!serializeRet) {
            vlDestroy(pListMapping, free);
            fprintf(stderr, "Failed to serialize context.");
            goto dispose;
        }
        /* 输出分析信息 */
        dumpRawKBinary(resultRaw, pListMapping);
        vlDestroy(pListMapping, free);
        free(resultRaw);
        runSuccess = 1;
    }
    else if (cliParams.target == TARGET_INSPECT) {
        dumpRawKBinary(inputBinary, NULL);
        runSuccess = 1;
    }
    else if (cliParams.target == TARGET_EXECUTE) {
        runSuccess = runBinary(inputBinary);
    }

dispose:
    if (inputText) free(inputText);
    if (inputBinary) free(inputBinary);

    return !runSuccess;
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

unsigned char *readBinaryFile(const char *filename) {
    FILE *fp;
    int length = 0;
    unsigned char *buf;

    fp = fopen(filename, "rb");

    if (!fp) return NULL;

    fseek(fp, 0, SEEK_END);
    length = ftell(fp);

    buf = (unsigned char *)malloc(length);
    memset(buf, 0, length);
    fseek(fp, 0, SEEK_SET);
    fread(buf, length, 1, fp);

    fclose(fp);
    return buf;
}