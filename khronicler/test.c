#include <stdio.h>
#include <stdlib.h>
#include "kbasic.h"
#include "k_utils.h"
#include "krt.h"

#define RESULT_SUCCESS      1
#define RESULT_FAILED       0
#define DIVIDER             "--------------------"

typedef struct {
    int excepted;
    char* name;
    char* source;
} SyntaxTestCase;

SyntaxTestCase SyntaxTestCases[] = {
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
        "func too_many(v0,v1,v2,v3,v4,v5,v6,v7,v8,v9,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z)"
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
    },
    { RESULT_FAILED, NULL, NULL }
};

void TestAllSyntaxError() {
    KbBuildError    errorRet = { KBE_NO_ERROR, 0, { '\0' }};
    KbContext*      context;
    int             ret;
    char            *textBuf;
    char            *textPtr;
    int             lineCount;
    char            buf[100];
    int             i;
    int             numPassCount = 0;

    for (i = 0; SyntaxTestCases[i].name; i++) {
        int isSuccess = 0, isPassed = 0;
        SyntaxTestCase* pCase = SyntaxTestCases + i;
        textBuf = pCase->source;
        errorRet.errorType = KBE_NO_ERROR;
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
        isSuccess = 1;
compileEnd:
        if (pCase->excepted == isSuccess) {
            isPassed = 1;
            numPassCount++;
        }
        kbFormatBuildError(&errorRet, buf, sizeof(buf));
        printf("Case %d - %s\n", i + 1, pCase->name);
        printf("Source Code: \n%s\n", pCase->source);
        printf("Result: [%d] %s\n%s\n", lineCount, buf, isPassed ? "PASSED" : "NOT PASSED");
        puts(DIVIDER);
        contextDestroy(context);
    }
    printf("Syntax test completed, %d/%d passed.\n", numPassCount, i);
}

typedef struct {
    int         exceptedType;
    KB_FLOAT    exceptedNum;
    char*       exceptedStr;
    char*       name;
    char*       source;
} RuntimeValueTestCase;

RuntimeValueTestCase RuntimeValueTestCases[] = {
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
    { RVT_NIL, 0, NULL, NULL }
};

int runBinary(const KByte* pRaw, KbRuntimeValue** ppRtValueGot) {
    KbRuntimeError  errorRet;
    int             ret;
    KbMachine*      app;

    app = machineCreate(pRaw);

    if (!app) {
        return 0;
    }

    ret = machineExec(app, 0, &errorRet);
    if (!ret) {
        char error_message[200];
        formatExecError(&errorRet, error_message, sizeof(error_message));
        printf("Runtime Error: %s\n", error_message);
        machineDestroy(app);
        return 0;
    }

    *ppRtValueGot = app->variables[0];

    machineDestroy(app);
    return 1;
}

void TestRuntimeValue() {
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
    RuntimeValueTestCase* pCase;

    for (i = 0; RuntimeValueTestCases[i].name; i++) {
        int casePassed = 0;
        pCase = RuntimeValueTestCases + i;
        textBuf = pCase->source;
        context = kbCompileStart();
        contextCtrlResetCounter(context);
        printf("Case %d - %s\n", i + 1, pCase->name);
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
        printf("Source Code: \n%s\n", pCase->source);
    
        if (!isSuccess) {
            char buf[200];
            kbFormatBuildError(&errorRet, buf, sizeof(buf));
            printf("(%d) ERROR: %s\n", lineCount, buf);
            contextDestroy(context);
            printf("Result: ERROR; NOT PASSED\n");
        }
        else {
            int runResult = 0;
            KbRuntimeValue* rtVarGot;
            char *szExcepted, *szVarGot;
            ret = kbSerialize(context, &pSerializedRaw, &resultByteLength);

            if (ret) {
                runResult = runBinary(pSerializedRaw, &rtVarGot);
                free(pSerializedRaw);
            }
            contextDestroy(context);

            if (runResult) {
                KbRuntimeValue tempRtValue;
                tempRtValue.type = pCase->exceptedType;
                if (tempRtValue.type == RVT_STRING) {
                    tempRtValue.data.sz = pCase->exceptedStr;
                }
                else if (tempRtValue.type == RVT_NUMBER) {
                    tempRtValue.data.num = pCase->exceptedNum;
                }
                szExcepted = rtvalueStringify(&tempRtValue);
                szVarGot = rtvalueStringify(rtVarGot);
                if (pCase->exceptedType != rtVarGot->type) {
                    casePassed = 0;
                }
                else {
                    switch (pCase->exceptedType) {
                    case RVT_NUMBER:
                        casePassed = pCase->exceptedNum == rtVarGot->data.num;
                        break;
                    case RVT_STRING:
                        casePassed = StringEqual(pCase->exceptedStr, rtVarGot->data.sz);
                        break;
                    default:
                        casePassed = 0;
                        break;
                    }
                }
                if (casePassed) {
                    numPassCount++;
                }
                printf("Result: Expected %s, Got %s. %s\n", szExcepted, szVarGot, casePassed ? "PASSED" : "NOT PASSED");
                free(szExcepted);
                free(szVarGot);
            }
            else {
                printf("Result: ERROR, NOT PASSED\n");
            }
        }
        puts(DIVIDER);
    }
    printf("Runtime test completed, %d/%d passed.\n", numPassCount, i);
}

int main(int argc, char** argv) {
    TestAllSyntaxError(); 
    TestRuntimeValue();
    return 0;
}