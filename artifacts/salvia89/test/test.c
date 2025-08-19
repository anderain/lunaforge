#include <stdio.h>
#include <string.h>
#include "../salvia.h"

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"

struct {
    const char* szFormat;
    int         iValue;
} IntCases[] = {
    { "|%-+8.5d|",      123 },
    { "|%-+8.5d|",      -123 },
    { "|%+08d|",        123 },
    { "|%+08d|",        -123 },
    { "|%+08.5d|",      123 },
    { "|%+08.5d|",      -123 },
    { "|% +5d|",        123 },
    { "|% +5d|",        -123 },
    { "|%-05d|",        123 },
    { "|%-05d|",        -123 },
    { "|%5.0d|",        0 },
    { "|%0-+12.8d|",    123 },
    { "|%0-+12.8d|",    -123 }
};

int NumIntCases = sizeof(IntCases) / sizeof(IntCases[0]);

struct {
    const char* szFormat;
    const char* szValue;
} StrCases[] = {
    { "|%15s|",     "HelloWorld" },  
    { "|%-15s|",    "HelloWorld" },  
    { "|%5s|",      "HelloWorld" },  
    { "|%.5s|",     "HelloWorld" },
    { "|%15.5s|",   "HelloWorld" },  
    { "|%-15.5s|",  "HelloWorld" },  
    { "|%015s|",    "HelloWorld" },  
    { "|%015.5s|",  "HelloWorld" },  
    { "|%10s|",     "" },   
    { "|%.5s|",     "" },  
    { "|%10.0s|",   "HelloWorld" }
};

int NumStrCases = sizeof(StrCases) / sizeof(StrCases[0]);

void testIntCases() {
    char strSalviaBuf[100];
    char strStdBuf[100];
    int i;
    int bIsPassed;

    for (i = 0; i < NumIntCases; ++i) {
        Salvia_Format(strSalviaBuf, IntCases[i].szFormat, IntCases[i].iValue);
        sprintf(strStdBuf, IntCases[i].szFormat, IntCases[i].iValue);
        bIsPassed = strcmp(strSalviaBuf, strStdBuf) == 0;
        printf("%s[%s]%s Case %d, format = \"%s\"\n",
            bIsPassed ? KGRN : KRED,
            bIsPassed ? "Passed" : "Failed",
            KNRM,
            i + 1,
            IntCases[i].szFormat
        );
        printf("    Salvia = %s\n", strSalviaBuf);
        printf("    stdio  = %s\n", strStdBuf);
    }
}

void testStrCases() {
    char strSalviaBuf[100];
    char strStdBuf[100];
    int i;
    int bIsPassed;

    for (i = 0; i < NumStrCases; ++i) {
        Salvia_Format(strSalviaBuf, StrCases[i].szFormat, StrCases[i].szValue);
        sprintf(strStdBuf, StrCases[i].szFormat, StrCases[i].szValue);
        bIsPassed = strcmp(strSalviaBuf, strStdBuf) == 0;
        printf("%s[%s]%s Case %d, format = \"%s\"\n",
            bIsPassed ? KGRN : KRED,
            bIsPassed ? "Passed" : "Failed",
            KNRM,
            i + 1,
            StrCases[i].szFormat
        );
        printf("    Salvia = %s\n", strSalviaBuf);
        printf("    stdio  = %s\n", strStdBuf);
    }
}

int main(int argc, char* argv[]) {
    testIntCases();
    testStrCases();
    return 0;
}