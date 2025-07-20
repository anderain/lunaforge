#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kbasic.h"
#include "k_utils.h"
#include "krt.h"

struct {
    int isDebug;
} builderConfig = { 1 };

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

int writeCompiledFile(const char *fileName, const void * data, int length) {
    FILE *fp = fopen(fileName, "wb");
    if (!fp) {
        return 0;
    }
    fwrite(data, length, 1, fp);
    fclose(fp);
    return 1;
}

int getLineTrimRemarks(const char* textPtr, char* line) {
    char *linePtr = line;
    const char *originalTextPtr = textPtr;

    // get line from file buffer
    do {
        *linePtr++ = *textPtr++;
    } while(*textPtr != '\n' && *textPtr != '\0');

    *linePtr = '\0';

    // remove remarks
    for (linePtr = line; *linePtr && *linePtr != '#'; ++linePtr) NULL;
    *linePtr = '\0';

    // trim ending spaces
    for (--linePtr; linePtr >= line && isSpace(*linePtr); --linePtr) {
        *linePtr = '\0';
    }

    // return how many char read
    return textPtr - originalTextPtr;
}

int buildFromKBasic(const char *fileName, const char * outputFileName) {
    KbBuildError    errorRet;
    KbContext*      context;
    int             ret;
    char            *textBuf;
    char            *textPtr;
    int             isSuccess = 1;
    int             lineCount = 1;
    int             resultByteLength;
    unsigned char*  resultRaw;

    textBuf = readTextFile(fileName);
    if (!textBuf) {
        printf("Failed to load script: '%s'\n", fileName);
        return 0;
    }

    context = kbCompileStart();
    
    /*
        第一遍循环
        1. 检查控制结构是否合法，并且标注出来控制结构需要的跳转标签
        2. 检查所有用户写的标签
     */
    contextCtrlResetCounter(context);    
    for (textPtr = textBuf, lineCount = 1; *textPtr; lineCount++) {
        char line[1000];

        /* 获取行的内容，并且移除尾部的空白和注释 */
        textPtr += getLineTrimRemarks(textPtr, line);

        /* 扫描一行 */
        ret = kbScanControlStructureAndLabel(line, context, &errorRet);

        if (!ret) {
            isSuccess = 0;
            goto compileEnd;
        }
    }
    /*
        检查是不是有没结束的控制结构
     */
    if (context->ctrlStruct.stack->size > 0) {
        errorRet.errorPos = 0;
        errorRet.errorType = KBE_INCOMPLETE_CTRL_STRUCT;
        errorRet.message[0] = '\0';
        isSuccess = 0;
        goto compileEnd;
    }

    /*
        第二遍循环，编译生成命令序列。
        本次循环之后，生成的 GOTO 与 IFGOTO 命令的 `param`.`ptr` 存储的是 KbContextLabel 的指针
     */
    contextCtrlResetCounter(context);
    for (textPtr = textBuf, lineCount = 1; *textPtr; lineCount++) {
        char line[1000];
        
        /* 获取行的内容，并且移除尾部的空白和注释 */
        textPtr += getLineTrimRemarks(textPtr, line);

        /* 编译一行 */
        ret = kbCompileLine(line, context, &errorRet);

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
    free(textBuf);

    if (isSuccess) {
        if (builderConfig.isDebug) {
            puts("-------- STRING --------");
            dbgPrintContextListText(context);

            puts("-------- VARIABLE --------");
            dbgPrintContextVariables(context);

            puts("-------- LABELS --------");
            dbgPrintContextListLabel(context);

            puts("-------- COMMAND --------");
            dbgPrintContextCommandList(context);
        }
        printf("Build success.\n");
    }
    else {
        char buf[200];
        kbFormatBuildError(&errorRet, buf, sizeof(buf));
        printf("(%d) ERROR: %s\n", lineCount, buf);
        goto releaseContext;
    }

    ret = kbSerialize(context, &resultRaw, &resultByteLength);

    if (ret) {
        ret = writeCompiledFile(outputFileName, resultRaw, resultByteLength);
        if (!ret) {
            puts("Failed to write output file");
        }
        else if (builderConfig.isDebug) {
            printf("Output file header:\n");
            dbgPrintHeader((KbBinaryHeader *)resultRaw);
        }
        free(resultRaw);
    } else {
        puts("Failed to serialize");
    }

releaseContext:
    contextDestroy(context);

    return isSuccess;
}

unsigned char *readBinaryFile(const char *filename) {
    FILE *fp;
    int length = 0;
    unsigned char *buf;

    fp = fopen(filename, "rb");

    if (!fp) {
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    length = ftell(fp);

    buf = (unsigned char *)malloc(length);
    memset(buf, 0, length);
    fseek(fp, 0, SEEK_SET);
    fread(buf, length, 1, fp);

    fclose(fp);
    return buf;
}

int debugBinary(const char* binFileName) {
    KbRuntimeError  errorRet;
    int             ret;
    KbMachine*      app;
    const char*     filename = binFileName;
    unsigned char*  raw;
    int             labelPos;

    raw = readBinaryFile(filename);
    app = machineCreate(raw);

    if (!app) {
        printf("Failed to load '%s', file not exist for invalid format\n", filename);
        return 1;
    }
    
    if (builderConfig.isDebug) {
        dbgPrintHeader(app->header);
    }

    ret = machineExec(app, 0, &errorRet);
    if (!ret) {
        char error_message[200];
        formatExecError(&errorRet, error_message, sizeof(error_message));
        printf("Runtime Error: %s\n%s\n", filename, error_message);
    }

    labelPos = machineGetLabelPos(app, "helloWorld");
    if (labelPos < 0) {
        printf("error, label not found\n");
    } else {
        ret = machineExecGoSub(app, labelPos, &errorRet);
        if (!ret) {
            char error_message[200];
            formatExecError(&errorRet, error_message, sizeof(error_message));
            printf("Runtime Error: %s\n%s\n", filename, error_message);
        }
    }


    machineDestroy(app);
    free(raw);
    return 0;
}

int main(int argc, char** argv) {
    return buildFromKBasic("test.kbs", "test.bin") && debugBinary("test.bin");
}