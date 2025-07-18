#ifndef _KOMMON_H_
#define _KOMMON_H_

#define isDigit(c)      ((c) >= '0' && (c) <= '9')
#define isUppercase(c)  ((c) >= 'a' && (c) <= 'z')
#define isLowercase(c)  ((c) >= 'A' && (c) <= 'Z')
#define isAlpha(c)      (isUppercase(c) || isLowercase(c) || (c) == '_')
#define isAlphaNum(c)   ((isDigit(c)) || (isAlpha(c)))
#define isSpace(c)      ((c) == ' ' || (c) == '\t' || (c) == '\n' || (c) == '\r')

#if defined(_SH3) || defined(_SH4)
#   ifndef _FX_9860_
#       define _FX_9860_
#   endif
#endif

typedef unsigned int KDword;
typedef unsigned char KByte;

#define KB_FLOAT                        float
#define KB_ERROR_MESSAGE_MAX            200
#define KB_TOKEN_LENGTH_MAX             50
#define KB_CONTEXT_VAR_MAX              32
#define KB_CONTEXT_STRING_BUFFER_MAX    300

// Node of List
typedef struct tagVlistNode {
    struct tagVlistNode *prev, *next;
    void *data;
} VlistNode;

// List
typedef struct {
    VlistNode *head, *tail;
    int size;
} Vlist;

// List API
Vlist*      vlNewList     ();
Vlist*      vlPushBack    (Vlist* _self, void *data);
void*       vlPopFront    (Vlist* _self);
void*       vlPopBack     (Vlist* _self);
void        vlDestroy     (Vlist* _self, void (* releaseData)(void *));

// Queue API
typedef Vlist VQueue;
#define vqNewQueue                  vlNewList
#define vqPush                      vlPushBack
#define vqPop                       vlPopFront
#define vqIsEmpty(q)                ((q)->size <= 0)
#define vqDestroy(q, releaseData)   vlDestroy(q, releaseData)

// Command

typedef struct {
    unsigned int op;
    union {
        KB_FLOAT        number;
        unsigned int    index;
        void *          ptr;
    } param;
} KbOpCommand;

#define opCommandRelease free

typedef enum {
    KBO_NUL = 0,
    KBO_PUSH_VAR,       // push / pop actions
    KBO_PUSH_NUM,
    KBO_PUSH_STR,
    KBO_POP,
    KBO_OPR_NEG,        // operators
    KBO_OPR_CONCAT,
    KBO_OPR_ADD,
    KBO_OPR_SUB,
    KBO_OPR_MUL,
    KBO_OPR_DIV,
    KBO_OPR_INTDIV,
    KBO_OPR_MOD,
    KBO_OPR_NOT,
    KBO_OPR_AND,
    KBO_OPR_OR,
    KBO_OPR_EQUAL,
    KBO_OPR_NEQ,
    KBO_OPR_GT,
    KBO_OPR_LT,
    KBO_OPR_GTEQ,
    KBO_OPR_LTEQ,
    KBO_CALL_BUILT_IN,  // call built-in function
    KBO_ASSIGN_VAR,     // assign to variable
    KBO_GOTO,           // goto commands
    KBO_IFGOTO,
    KBO_PUSH_OFFSET,    // push_offset + goto = gosub
    KBO_RETURN,         // return
    KBO_STOP            // stop command
} KB_OPCODE;

typedef enum {
    KFID_NONE = 0,
    KFID_P,
    KFID_SIN,
    KFID_COS,
    KFID_TAN,
    KFID_SQRT,
    KFID_EXP,
    KFID_ABS,
    KFID_LOG,
    KFID_RAND,
    KFID_ZEROPAD
} KB_BUILT_IN_FUNCTION_ID;

extern const char *_KOCODE_NAME[];

#define HEADER_MAGIC_FLAG 0x424B

typedef struct {
    // file header
    KDword headerMagic;

    // number of variables
    KDword numVariables;

    // cmd block def
    KDword cmdBlockStart;       // cmd start position
    KDword numCmd;              // number of cmd

    // string block def
    KDword stringBlockStart;    // string start position
    KDword stringBlockLength;   // length of string block
} KBASIC_BINARY_HEADER;

void dbgPrintHeader(KBASIC_BINARY_HEADER *);

#define endianSwapDword(l) (unsigned int)((((l) & 0x000000ff) << 24) | (((l) & 0x0000ff00) << 8) | (((l) & 0xff000000) >> 24) | (((l) & 0x00ff0000) >> 8))

#endif