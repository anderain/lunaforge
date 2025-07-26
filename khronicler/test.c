#include <stdio.h>
#include <stdlib.h>
#include "kbasic.h"
#include "k_utils.h"
#include "krt.h"

#define RESULT_SUCCESS      1
#define RESULT_FAILED       0

typedef struct {
    int excepted;
    char* name;
    char* source;
} SyntaxTest;

SyntaxTest SyntaxTestCases[] = {
    {
        RESULT_SUCCESS,
        "empty_input",
        "# Comment Only"
    },
    {
        RESULT_FAILED,
        "function_nested_in_function",
        "func outside()\n"
        "func nested()"
    },
    {
        RESULT_FAILED,
        "function_nested_in_control_flow",
        "if 1\n"
        "func nested()"
    },
    {
        RESULT_FAILED,
        "function_missing_name",
        "func 1"
    },
    {
        RESULT_FAILED,
        "function_name_too_long",
        "func l234512345123456()"
    },
    {
        RESULT_FAILED,
        "function_invalid_parameters",
        "func parameters(a,b c)"
    },
    {
        RESULT_FAILED,
        "function_duplicated_parameters",
        "func parameters(a,b,a)"
    },
    {
        RESULT_FAILED,
        "function_too_many_parameters",
        "func too_many(v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w)"
    },
    {
        RESULT_FAILED,
        "if_missing_expression",
        "if "
    },
    {
        RESULT_FAILED,
        "if_redundant_statement",
        "if 1 + 1 2"
    },
    {
        RESULT_FAILED,
        "if_goto_missing_label",
        "if 1 > 1 goto"
    },
    {
        RESULT_FAILED,
        "elseif_missing_expression",
        "if 0\n"
        "elseif"
    },
    {
        RESULT_FAILED,
        "elseif_redundant_statement",
        "if 0\n"
        "elseif 0 goto"
    },
    {
        RESULT_FAILED,
        "elseif_not_after_if",
        "elseif 1 > 1"
    },
    {
        RESULT_FAILED,
        "else_redundant_statement",
        "else 1"
    },
    {
        RESULT_FAILED,
        "else_not_after_if",
        "else"
    },
    {
        RESULT_FAILED,
        "while_missing_expression",
        "while"
    },
    {
        RESULT_FAILED,
        "while_redundant",
        "while 1 > 1 2"
    },
    {
        RESULT_FAILED,
        "break_redundant",
        "break 1"
    },
    {
        RESULT_FAILED,
        "break_not_in_loop",
        "break"
    },
    {
        RESULT_FAILED,
        "end_invalid_context",
        "end goto"
    },
    {
        RESULT_FAILED,
        "endif_not_after_if",
        "while 1\n"
        "end if"
    },
    {
        RESULT_FAILED,
        "endwhile_not_after_while",
        "if 0 > 1\n"
        "end while"
    },
    {
        RESULT_FAILED,
        "exit_non_empty",
        "exit 12"
    },
    {
        RESULT_FAILED,
        "return_missing_expression",
        "return"
    },
    {
        RESULT_FAILED,
        "return_redundant",
        "return 1 123"
    },
    {
        RESULT_FAILED,
        "goto_missing_label",
        "goto"
    },
    {
        RESULT_FAILED,
        "goto_redundant",
        "goto labelName 123"
    },
    {
        RESULT_FAILED,
        "label_missing_name",
        ":123"
    },
    {
        RESULT_FAILED,
        "label_redundant",
        ":labelName 123"
    },
    {
        RESULT_FAILED,
        "label_name_too_long",
        ":l234567890123456"
    },
    {
        RESULT_FAILED,
        "dim_missing_variable",
        "dim 2"
    },
    {
        RESULT_FAILED,
        "dim_missing_expression",
        "dim a ="
    },
    {
        RESULT_FAILED,
        "dim_redundant_case1",
        "dim a = 1 2"
    },
    {
        RESULT_FAILED,
        "dim_redundant_case2",
        "dim a 2"
    },
    {
        RESULT_FAILED,
        "assignment_missing_expression",
        "a = "
    },
    {
        RESULT_FAILED,
        "assignment_redundant",
        "a = 1 2"
    },
    {
        RESULT_FAILED,
        "expression_invalid_case1",
        "1 2"
    },
    {
        RESULT_FAILED,
        "expression_invalid_case2",
        "a >"
    },
    {
        RESULT_FAILED,
        "expression_invalid_case3",
        "= b"
    },
    {
        RESULT_SUCCESS,
        "expression_plain",
        "a > b"
    },
    {
        RESULT_SUCCESS,
        "all_correct",
        "dim a = 1\n"
        "while a < 100\n"
        "  if a < 10\n"
        "    p(\"loop \" + a)\n"
        "  else\n"
        "    break\n"
        "  end if\n"
        "  a = a + 1\n"
        "end while\n"
        "p(\"loop exit\")"
    }
};

#define SYNTAX_TEST_NUM (sizeof(SyntaxTestCases) / sizeof(SyntaxTestCases[0]))

typedef struct {
    int isPassed;
    char errMsg[200];
} SyntaxTestResult;

SyntaxTestResult syntaxTestResults[SYNTAX_TEST_NUM];

int numSyntaxTestPassed = 0;

int TestAllSyntaxError() {
    KbBuildError    errorRet = { KBE_NO_ERROR, 0, { '\0' }};
    KbContext*      context;
    int             ret;
    char            *textBuf;
    char            *textPtr;
    int             lineCount;
    char            errBuf[100];
    int             i;
    int             numPassCount = 0;

    for (i = 0; i < SYNTAX_TEST_NUM; i++) {
        int                 isSyntaxCheckSuccess = 0;
        SyntaxTest*         pCase = SyntaxTestCases + i;
        SyntaxTestResult*   pCaseResult = syntaxTestResults + i;

        textBuf = pCase->source;
        errorRet.errorType = KBE_NO_ERROR;
        StringCopy(pCaseResult->errMsg, sizeof(pCaseResult->errMsg), "");
        pCaseResult->isPassed = 0;
    
        context = kbCompileStart();
        contextCtrlResetCounter(context);

        for (textPtr = textBuf, lineCount = 1; *textPtr; lineCount++) {
            char line[1000];
            textPtr += getLineTrimRemarks(textPtr, line);
            ret = kbScanLineSyntax(line, context, &errorRet);
            if (!ret) {
                goto compileEnd;
            }
        }
    
        if (context->pCurrentFunc != NULL) {
            errorRet.errorPos = 0;
            errorRet.errorType = KBE_INCOMPLETE_FUNC;
            errorRet.message[0] = '\0';
            isSyntaxCheckSuccess = 0;
            goto compileEnd;
        }
    
        if (context->ctrlFlow.stack->size > 0) {
            errorRet.errorPos = 0;
            errorRet.errorType = KBE_INCOMPLETE_CTRL_FLOW;
            errorRet.message[0] = '\0';
            isSyntaxCheckSuccess = 0;
            goto compileEnd;
        }
        isSyntaxCheckSuccess = 1;
    
compileEnd:
        if (pCase->excepted == isSyntaxCheckSuccess) {
            pCaseResult->isPassed = 1;
            numPassCount++;
        }

        kbFormatBuildError(&errorRet, errBuf, sizeof(errBuf));
        StringCopy(pCaseResult->errMsg, sizeof(pCaseResult->errMsg), errBuf);
        contextDestroy(context);
    }

    return numPassCount;
}

typedef struct {
    int         exceptedType;
    KB_FLOAT    exceptedNum;
    char*       exceptedStr;
    char*       name;
    char*       source;
} RuntimeValueTest;

RuntimeValueTest RuntimeValueTestCases[] = {
    {
        RVT_NUMBER, 1, NULL,
        "relative equality",
        "dim result = floatRelEqual()\n"
        "func floatRelEqual()\n"
        "    dim a = 1.0000001\n"
        "    dim b = 1.0000002\n"
        "    return a ~= b\n"
        "end func"
    },
    {
        RVT_NUMBER, 120, NULL,
        "factorial",
        "dim result = factorial(5)\n"
        "func factorial(n)\n"
        "    if n = 0\n"
        "        return 1\n"
        "    else\n"
        "        return n * factorial(n-1)\n"
        "    end if\n"
        "end func"
    },
    {
        RVT_NUMBER, 8, NULL,
        "fibonacci",
        "dim result = fib(6)\n"
        "func fib(n)\n"
        "    if n <= 0\n"
        "        return 0\n"
        "    elseif n = 1\n"
        "        return 1\n"
        "    else\n"
        "        return fib(n - 1) + fib(n - 2)\n"
        "    end if\n"
        "end func"
    },
    {
        RVT_NUMBER, 1, NULL,
        "random range test",
        "dim result = chkRandRange()\n"
        "func chkRandRange()\n"
        "    dim i = 0\n"
        "    dim sc = 0\n"
        "    dim r = 0\n"
        "    while i < 100\n"
        "        r = rand()\n"
        "        if r > 0 && r < 1\n"
        "            sc = sc + 1\n"
        "        end if\n"
        "        i = i + 1\n"
        "    end while\n"
        "    return sc = 100\n"
        "end func"
    },
    {
        RVT_NUMBER, 1, NULL,
        "mutual prime test",
        "dim result = gcd(28463, 79867)\n"
        "func gcd(a, b)\n"
        "    dim temp = 0\n"
        "    while b <> 0\n"
        "        temp = b\n"
        "        b = a % b\n"
        "        a = temp\n"
        "    end while\n"
        "    return a\n"
        "end func"
    },
    {
        RVT_NUMBER, 1024, NULL,
        "fast power algorithm",
        "dim result = fastPow(2,10)\n"
        "func fastPow(x, n)\n"
        "    dim half\n"
        "    if n = 0\n"
        "        return 1\n"
        "    elseif n % 2 = 0\n"
        "        half = fastPow(x, n / 2)\n"
        "        return half * half\n"
        "    else\n"
        "        return x * fastPow(x, n - 1)\n"
        "    end if\n"
        "end func"
    },
    {
        RVT_STRING, 0, "A->C;A->B;C->B;A->C;B->A;B->C;A->C;",
        "hanoi",
        "dim actions = hanoi(3, \"A\", \"C\", \"B\")\n"
        "func hanoi(n, s, t, a)\n"
        "    if n > 0\n"
        "        dim move = s & \"->\" & t & \";\"\n"
        "        return hanoi(n - 1, s, a, t) & move & hanoi(n - 1, a, t, s)\n"
        "    end if\n"
        "    return \"\"\n"
        "end func"
    },
    {
        RVT_NUMBER, 1, NULL,
        "string functions",
        "dim checkResult = checkLen() && checkVal() && checkAsc()\n"
        "# String length\n"
        "func checkLen()\n"
        "    return len(\"hello world\") ~= 11\n"
        "end func\n"
        "# String to numeric\n"
        "func checkVal()\n"
        "    return val(\"123.456\") ~= 123.456\n"
        "end func\n"
        "# Character to ASCII code\n"
        "func checkAsc()\n"
        "    return asc(\"ASCII\") ~= 65\n"
        "end func\n"
    }
};
#define RUNTIME_VALUE_TEST_NUM (sizeof(RuntimeValueTestCases) / sizeof(RuntimeValueTestCases[0]))

typedef struct {
    int isPassed;
    char errMsg[200];
    char szExcepted[200];
    char szActualGot[200];
} RuntimeTestResult;

RuntimeTestResult RuntimeTestResults[RUNTIME_VALUE_TEST_NUM];

int numRuntimeTestPassed = 0;

KbRuntimeValue* runBinaryAndDumpFirstVar(const KByte* pRaw, char* errMsg, int errMsgLength) {
    KbRuntimeError  errorRet;
    int             ret;
    KbMachine*      app;
    KbRuntimeValue* rtValueRet;

    app = machineCreate(pRaw);

    if (!app) {
        return NULL;
    }

    ret = machineExec(app, 0, &errorRet);
    if (!ret) {
        formatExecError(&errorRet, errMsg, errMsgLength);
        machineDestroy(app);
        return NULL;
    }

    rtValueRet = rtvalueDuplicate(app->variables[0]);

    machineDestroy(app);
    return rtValueRet;
}

int TestRuntimeValue() {
    KbBuildError    errorRet;
    KbContext*      context;
    int             ret;
    char            *textBuf;
    char            *textPtr;
    int             isSuccess = 1;
    int             lineCount = 1;
    int             resultByteLength;
    KByte*          pSerializedRaw;
    int             i;
    int             numPassCount = 0;


    for (i = 0; i < RUNTIME_VALUE_TEST_NUM; i++) {
        int isCasePassed = 0;
        char* szExcepted = NULL;
        char* szActualGot = NULL;
        char  errBuf[200] = "";
    
        RuntimeValueTest*   pCase = RuntimeValueTestCases + i;
        RuntimeTestResult*  pCaseResult = RuntimeTestResults + i;

        KbRuntimeValue tempExpectValue;
        tempExpectValue.type = pCase->exceptedType;
        if (tempExpectValue.type == RVT_STRING) {
            tempExpectValue.data.sz = pCase->exceptedStr;
        }
        else if (tempExpectValue.type == RVT_NUMBER) {
            tempExpectValue.data.num = pCase->exceptedNum;
        }
        szExcepted = rtvalueStringify(&tempExpectValue);

        StringCopy(pCaseResult->errMsg, sizeof(pCaseResult->errMsg), "");
        StringCopy(pCaseResult->szActualGot, sizeof(pCaseResult->szActualGot), "N/A");

        textBuf = pCase->source;
        context = kbCompileStart();
        contextCtrlResetCounter(context);

        isSuccess = 1; 
        for (textPtr = textBuf, lineCount = 1; *textPtr; lineCount++) {
            char line[1000];
            textPtr += getLineTrimRemarks(textPtr, line);
            ret = kbScanLineSyntax(line, context, &errorRet);
            if (!ret) {
                isSuccess = 0;
                goto compileEnd;
            }
        }
        --lineCount;
        if (context->pCurrentFunc != NULL) {
            errorRet.errorPos = 0;
            errorRet.errorType = KBE_INCOMPLETE_FUNC;
            errorRet.message[0] = '\0';
            isSuccess = 0;
            goto compileEnd;
        }
        if (context->ctrlFlow.stack->size > 0) {
            errorRet.errorPos = 0;
            errorRet.errorType = KBE_INCOMPLETE_CTRL_FLOW;
            errorRet.message[0] = '\0';
            isSuccess = 0;
            goto compileEnd;
        }
        contextCtrlResetCounter(context);
        for (textPtr = textBuf, lineCount = 1; *textPtr; lineCount++) {
            char line[1000];
            textPtr += getLineTrimRemarks(textPtr, line);
            ret = kbCompileLine(line, context, &errorRet);
            if (!ret) {
                isSuccess = 0;
                goto compileEnd;
            }
        }
        kbCompileEnd(context);

compileEnd:
    
        if (!isSuccess) {
            kbFormatBuildError(&errorRet, errBuf, sizeof(errBuf));
            contextDestroy(context);
        }
        else {
            KbRuntimeValue* rtActualGot = NULL;
            
            ret = kbSerialize(context, &pSerializedRaw, &resultByteLength);
            contextDestroy(context);
            if (ret) {
                rtActualGot = runBinaryAndDumpFirstVar(pSerializedRaw, errBuf, sizeof(errBuf));
                free(pSerializedRaw);
            }

            if (rtActualGot) {

                szActualGot = rtvalueStringify(rtActualGot);
                if (pCase->exceptedType != rtActualGot->type) {
                    isCasePassed = 0;
                }
                else {
                    switch (pCase->exceptedType) {
                    case RVT_NUMBER: {
                        isCasePassed = kFloatEqualRel(pCase->exceptedNum, rtActualGot->data.num);
                        break;
                    }
                    case RVT_STRING:
                        isCasePassed = StringEqual(pCase->exceptedStr, rtActualGot->data.sz);
                        break;
                    default:
                        isCasePassed = 0;
                        break;
                    }
                }
                rtvalueDestroy(rtActualGot);
                if (isCasePassed) {
                    numPassCount++;
                }
            }
        }

        StringCopy(pCaseResult->errMsg, sizeof(pCaseResult->errMsg), errBuf);
        StringCopy(pCaseResult->szActualGot, sizeof(pCaseResult->szActualGot), szActualGot ? szActualGot : "N/A");
        StringCopy(pCaseResult->szExcepted, sizeof(pCaseResult->szExcepted), szExcepted ? szExcepted : "N/A");
        pCaseResult->isPassed = isCasePassed;
    
        if (szExcepted) free(szExcepted);
        if (szActualGot) free(szActualGot);
    }
    return numPassCount;
}

void printStringInJson(FILE* fp, const char *str) {
    int i;
    for (i = 0; str[i]; ++i) {
        char c = str[i];
        if (c == '\n') {
            fprintf(fp, "\\n");
        }
        else if (c == '"') {
            fprintf(fp, "\\\"");
        }
        else if (c == '\t') {
            fprintf(fp, "\t");
        }
        else if (c == '\r') {
            fprintf(fp, "\r");
        }
        else {
            fprintf(fp, "%c", c);
        }
    }
}

void printAsJson(FILE* fp) {
    int i;

    fprintf(fp, "{\n");
    fprintf(fp, "  \"syntax_test\": [\n");

    for (i = 0; i < SYNTAX_TEST_NUM; i++) {
        SyntaxTest*         pCase = SyntaxTestCases + i;
        SyntaxTestResult*   pCaseResult = syntaxTestResults + i;

        fprintf(fp, "    {\n");
        fprintf(fp, "      \"id\": \"SYN-%03d\",\n", i + 1);
        fprintf(fp, "      \"name\": \"", i + 1); printStringInJson(fp, pCase->name); fprintf(fp, "\",\n");
        fprintf(fp, "      \"src\": \"", i + 1); printStringInJson(fp, pCase->source); fprintf(fp, "\",\n");
        fprintf(fp, "      \"expect_success\": %s,\n", pCase->excepted ? "true" : "false");
        fprintf(fp, "      \"err_msg\": \"", i + 1); printStringInJson(fp, pCaseResult->errMsg); fprintf(fp, "\",\n");
        fprintf(fp, "      \"passed\": %s\n", pCaseResult->isPassed ? "true" : "false");
        fprintf(fp, "    }");
        if (i >= SYNTAX_TEST_NUM - 1) {
            fprintf(fp, "\n");
        }
        else {
            fprintf(fp, ",\n");
        }
    }
    fprintf(fp, "  ],\n");
    fprintf(fp, "  \"runtime_test\": [\n");
    for (i = 0; i < RUNTIME_VALUE_TEST_NUM; i++) {
        RuntimeValueTest*   pCase = RuntimeValueTestCases + i;
        RuntimeTestResult*  pCaseResult = RuntimeTestResults + i;
    
        fprintf(fp, "    {\n");
        fprintf(fp, "      \"id\": \"RT-%03d\",\n", i + 1);
        fprintf(fp, "      \"name\": \"", i + 1); printStringInJson(fp, pCase->name); fprintf(fp, "\",\n");
        fprintf(fp, "      \"src\": \"", i + 1); printStringInJson(fp, pCase->source); fprintf(fp, "\",\n");
        fprintf(fp, "      \"expected\": \"", i + 1); printStringInJson(fp, pCaseResult->szExcepted); fprintf(fp, "\",\n");
        fprintf(fp, "      \"actual_got\": \"", i + 1); printStringInJson(fp, pCaseResult->szActualGot); fprintf(fp, "\",\n");
        fprintf(fp, "      \"err_msg\": \"", i + 1); printStringInJson(fp, pCaseResult->errMsg); fprintf(fp, "\",\n");
        fprintf(fp, "      \"passed\": %s\n", pCaseResult->isPassed ? "true" : "false");
        fprintf(fp, "    }");
        if (i >= RUNTIME_VALUE_TEST_NUM - 1) {
            fprintf(fp, "\n");
        }
        else {
            fprintf(fp, ",\n");
        }
    }
    fprintf(fp, "  ]\n");
    fprintf(fp, "}\n");
}

int printHighlightToken(int isPrevSpaceAfter, FILE* fp, Token* token, const char* szStart, int len) {
    int i;
    const char* htmlClassName = NULL;
    int isSpaceBefore = 0;
    int isSpaceAfter = 0;

    switch (token->type)
    {
    case TOKEN_NUM:
        htmlClassName = "tk-num";
        break;
    case TOKEN_ID:
        htmlClassName = "tk-id";
        break;
    case TOKEN_OPR:
        isSpaceBefore = 1;
        isSpaceAfter = 1;
        htmlClassName = "tk-opr";
        break;
    case TOKEN_FNC:
        htmlClassName = "tk-fnc";
        break;
    case TOKEN_BKT:
        htmlClassName = "tk-bkt";
        break;
    case TOKEN_CMA:
        htmlClassName = "tk-cma";
        break;
    case TOKEN_STR:
        htmlClassName = "tk-str";
        break;
    case TOKEN_LABEL:
        htmlClassName = "tk-lbl";
        break;
    case TOKEN_KEY:
        isSpaceAfter = 1;
        if (!isPrevSpaceAfter && StringEqual("goto", token->content)) {
            isSpaceBefore = 1;
        }
        htmlClassName = "tk-key";
        break;
    default:
        break;
    }
    if (isSpaceBefore) {
        fputc(' ', fp);
    }
    fprintf(fp, "<span class=\"%s\">", htmlClassName);
    for (i = 0; i < len; ++i) {
        fputc(szStart[i], fp);
    }
    if (isSpaceAfter) {
        fputc(' ', fp);
    }
    fprintf(fp, "</span>");
    return isSpaceAfter;
}

void printScriptWithHighlightHtml(FILE* fp, const char* script) {
    const char* textPtr;
    int         lineCount = 0;
    char        line[1000];
    char*       pRemark;
    char        szRemark[200];
    Analyzer    analyzer;
    int         isSpaceAfter;

    fprintf(fp, "<pre class=\"kbcode\">\n");
    for (textPtr = script, lineCount = 1; *textPtr; lineCount++) {
        char* linePtr;
        /* 读取一行 */
        textPtr += getLineOnly(textPtr, line);
        linePtr = line;
        /* 先打印空白内容 */
        isSpaceAfter = 1;
        while (*linePtr && isSpace(*linePtr)) {
            fprintf(fp, "%c", *linePtr);
            ++linePtr;
        }
        /* 寻找注释 */
        pRemark = strchr(linePtr, '#');
        if (pRemark) {
            strcpy(szRemark, pRemark);
            *pRemark = '\0';
        }
        /* 获取其他token */
        analyzer.expr = linePtr;
        resetToken(&analyzer);
        while (1) {
            Token* token = nextToken(&analyzer);
            if (!token || token->type == TOKEN_END) {
                break;
            }
            isSpaceAfter = printHighlightToken(isSpaceAfter, fp, token, linePtr + token->sourceStartPos, token->sourceLength);
        }
        /* 打印注释 */
        if (pRemark) {
            if (!isSpaceAfter) fputc(' ', fp);
            fprintf(fp, "<span class=\"tk-remark\">%s</span>", szRemark);
        }
        /* 本行结束 */
        fprintf(fp, "\n");
    }
    fprintf(fp, "</pre>\n");
}

void printAsHtml(FILE* fp) {
    int i;

    fputs(
        "<!DOCTYPE html>"
        "<html lang=\"en-US\">"
        "<head>"
        "<meta charset=\"UTF-8\">"
        "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
        "<title> KBasic Test Report </title>"
        "</head>"
        "<body>"
        "<div class=\"report-container\">",
        fp
    );

    fputs("<h1>KBasic Khronicler</h1>", fp);
    fprintf(fp, "<p>Comipler / KRT Version %s</p>\n", KBASIC_VERSION);

    fputs("<h2>Overall Test Summary</h2>\n", fp);
    fputs("<table>\n", fp);
    fputs(
        "<tr>"
        "<th>Test Category</th>"
        "<th>Total Cases</th>"
        "<th>Passed</th>"
        "<th>Failed</th>"
        "<th>Pass Rate</th>"
        "</tr>",
        fp
    );
    fprintf(fp, "<tr>");
    fprintf(fp, "<td> Syntax Tests </td>");
    fprintf(fp, "<td class=\"numeric\"> %d </td>", SYNTAX_TEST_NUM);
    fprintf(fp, "<td class=\"numeric\"> %d </td>", numSyntaxTestPassed);
    fprintf(fp, "<td class=\"numeric\"> %d </td>", SYNTAX_TEST_NUM - numSyntaxTestPassed);
    fprintf(fp, "<td class=\"numeric\"> %d%% </td>", numSyntaxTestPassed * 100 / SYNTAX_TEST_NUM);
    fprintf(fp, "</tr>");
    fprintf(fp, "<tr>");
    fprintf(fp, "<td> Runtime Tests </td>");
    fprintf(fp, "<td class=\"numeric\"> %d </td>", RUNTIME_VALUE_TEST_NUM);
    fprintf(fp, "<td class=\"numeric\"> %d </td>", numRuntimeTestPassed);
    fprintf(fp, "<td class=\"numeric\"> %d </td>", RUNTIME_VALUE_TEST_NUM - numRuntimeTestPassed);
    fprintf(fp, "<td class=\"numeric\"> %d%% </td>", numRuntimeTestPassed * 100 / RUNTIME_VALUE_TEST_NUM);
    fprintf(fp, "</tr>");
    fputs("</table>\n", fp);

    fputs("<h2>Syntax Tests</h2>\n", fp);
    fputs("<table>\n", fp);
    fputs(
        "<tr>"
        "<th class=\"case-id\">Case ID</th>"
        "<th class=\"case-name\">Case Name</th>"
        "<th class=\"source-code\">Source Code</th>"
        "<th class=\"text-expected\">Expected</th>"
        "<th class=\"err-msg\">Error Message</th>"
        "<th class=\"test-result\">Result</th>"
        "</tr>",
        fp
    );
    for (i = 0; i < SYNTAX_TEST_NUM; i++) {
        SyntaxTest*         pCase = SyntaxTestCases + i;
        SyntaxTestResult*   pCaseResult = syntaxTestResults + i;

        fputs("<tr>", fp);
        fprintf(fp, "<td class=\"case-id\"> SYN-%03d </td>", i + 1);
        fprintf(fp, "<td class=\"case-name\"> %s </td>", pCase->name);
        fprintf(fp, "<td class=\"source-code\">");
        printScriptWithHighlightHtml(fp, pCase->source);
        fprintf(fp, "</td>");
        fprintf(fp, "<td class=\"text-expected\"> %s </td>", pCase->excepted ? "Success" : "Fail");
        fprintf(fp, "<td class=\"err-msg\"> %s </td>", pCaseResult->errMsg);
        fprintf(fp, "<td class=\"test-result\"><span class=\"%s\"> %s </span></td>", pCaseResult->isPassed ? "test-status success" : "test-status failed", pCaseResult->isPassed ? "PASSED" : "NOT PASSED");
        fputs("</tr>", fp);
    }
    fputs("<table>\n", fp);

    fputs("<h2>Runtime Value Tests</h2>\n", fp);
    fputs(
        "<tr>"
        "<th class=\"case-id\">Case ID</th>"
        "<th class=\"case-name\">Case Name</th>"
        "<th class=\"source-code\">Source Code</th>"
        "<th class=\"text-expected\">Expected Output</th>"
        "<th class=\"text-got\">Actual Output</th>"
        "<th class=\"err-msg\">Error Message</th>"
        "<th class=\"test-result\">Result</th>"
        "</tr>",
        fp
    );
    for (i = 0; i < RUNTIME_VALUE_TEST_NUM; i++) {
        RuntimeValueTest*   pCase = RuntimeValueTestCases + i;
        RuntimeTestResult*  pCaseResult = RuntimeTestResults + i;

        fputs("<tr>", fp);
        fprintf(fp, "<td class=\"case-id\"> RT-%03d </td>", i + 1);
        fprintf(fp, "<td class=\"case-name\"> %s </td>", pCase->name);
        fprintf(fp, "<td class=\"source-code\">");
        printScriptWithHighlightHtml(fp, pCase->source);
        fprintf(fp, "</td>");
        fprintf(fp, "<td class=\"text-expected\"> %s </td>", pCaseResult->szExcepted);
        fprintf(fp, "<td class=\"text-got\"> %s </td>", pCaseResult->szActualGot);
        fprintf(fp, "<td class=\"err-msg\"> %s </td>", pCaseResult->errMsg);
        fprintf(fp, "<td class=\"test-result\"><span class=\"%s\"> %s </span></td>", pCaseResult->isPassed ? "test-status success" : "test-status failed", pCaseResult->isPassed ? "PASSED" : "NOT PASSED");
        fputs("</tr>", fp);
    }

    fputs(
        "</div>"
        "</body>\n"
        "<style>"
        "td.case-id{color:#999;font-weight:bold}"
        "body,html{color: #333;padding:0;margin:0;overflow-x:hidden;font-family:Arial,Helvetica,sans-serif}"
        ".report-container{width:100%;max-width:1200px;margin:0 auto;padding:10px}"
        "table{table-layout:auto;width:100%;font-size:12px;border-collapse:collapse}"
        "h1,h2,h3,h4,h5,h6{color:#14509a}"
        "h1{padding-bottom:4px;border-bottom:2px solid #5d93d5}"
        "tr{border-bottom:1px solid #ddd}"
        "td{padding:4px 4px}"
        "th{color:#fff;background:linear-gradient(#14509a,#5d93d5);padding:8px 4px;background: -ms-linear-gradient(top,#14509a 0%,#5d93d5 100%)}"
        "pre{text-align:left;margin: 0;font-family:\"Consolas\",\"Monaco\",\"Lucida Console\",\"Courier New\",monospace;font-size:12px;line-height:1.25;tab-size:4}"
        "pre.kbcode{color:#1E1E1E;background-color:#fff}"
        ".kbcode .tk-num{color:#098658}"
        ".kbcode .tk-id{color:#495057}"
        ".kbcode .tk-opr{color:#9D7E14}"
        ".kbcode .tk-fnc{color:#1E1E1E}"
        ".kbcode .tk-bkt{color:#1E1E1E}"
        ".kbcode .tk-cma{color:#1E1E1E}"
        ".kbcode .tk-str{color:#A31515}"
        ".kbcode .tk-lbl{color:#AF00DB}"
        ".kbcode .tk-key{color:#0000FF}"
        ".kbcode .tk-remark{color:#008000}"
        "th.case-id{width:100px}"
        "td.case-id{text-align:center}"
        "td.text-expected,td.text-got{text-align:center;max-width:100px}"
        "td.numeric{text-align:right}"
        ".test-status{font-weight:700;white-space:nowrap;padding:2px 8px;border-radius:4px;color:#fff;display:block;margin:0 auto;width:fit-content}"
        ".test-status.success{background-color:green}"
        ".test-status.failed{background-color:red}"
        "</style>\n"
        "</html>",
        fp
    );
}

int main(int argc, char** argv) {
    numSyntaxTestPassed = TestAllSyntaxError();
    numRuntimeTestPassed = TestRuntimeValue();
    printAsHtml(stdout);
    return 0;
}
