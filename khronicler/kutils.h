#ifndef _KUTILS_H_
#define _KUTILS_H_

#include "kommon.h"

int     KUtils_StringCopy       (char *dest, int max, const char *src);
char*   KUtils_StringDump       (const char *str);
char*   KUtils_StringConcat     (const char* szLeft, const char* szRight);
KBool   KUtils_IsStringEqual    (const char* szA, const char* szB); 
KBool   KUtils_IsStringEndWith  (const char *str, const char *token);
double  KUtils_Atof             (const char *str);
char*   KUtils_Ftoa             (double f, char * buf, int precision);
long    KUtils_Atoi             (const char* S);
char*   KUtils_Itoa             (int num, char* str, int base);
KBool   KUtils_FloatEqualRel    (KFloat a, KFloat b);
KBool   KUtils_IsLittleEndian   ();

#define K_DEFAULT_FTOA_PRECISION    8

typedef struct tagVlistNode {
    struct tagVlistNode *prev, *next;
    void *data;
} VlistNode;

typedef struct {
    VlistNode *head, *tail;
    int size;
} Vlist;

Vlist*      vlNewList       ();
Vlist*      vlPushBack      (Vlist* _self, void *data);
void*       vlPopFront      (Vlist* _self);
void*       vlPopBack       (Vlist* _self);
void        vlDestroy       (Vlist* _self, void (* releaseData)(void *));

#define vlPeek(_self)       ((_self)->tail->data)

typedef Vlist VQueue;
#define vqNewQueue                  vlNewList
#define vqPush                      vlPushBack
#define vqPop                       vlPopFront
#define vqIsEmpty(q)                ((q)->size <= 0)
#define vqDestroy(q, releaseData)   vlDestroy(q, releaseData)

#endif