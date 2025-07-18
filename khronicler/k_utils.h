#ifndef _K_UTILS_H_
#define _K_UTILS_H_

#include <string.h>

#define     StringEqual(s1, s2) (((s1) != NULL) && ((s2) != NULL) && strcmp((s1), (s2)) == 0)
int         StringCopy          (char *dest, int max, const char *src);
char *      StringDump          (const char *str);
int         StringEndWith       (const char *str, const char *token);

#define DEFAUL_FTOA_PRECISION   8

double  kAtof(const char *s);
char *  kFtoa(double f, char * buf, int precision);
char*   kItoa(int num, char* str, int base);
long    kAtoi(const char* S);

#endif