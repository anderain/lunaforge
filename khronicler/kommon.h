#ifndef _KOMMON_H_
#define _KOMMON_H_

#define KBASIC_VERSION  "2.0"

#define isDigit(c)      ((c) >= '0' && (c) <= '9')
#define isUppercase(c)  ((c) >= 'a' && (c) <= 'z')
#define isLowercase(c)  ((c) >= 'A' && (c) <= 'Z')
#define isHexAlphaU(c)  ((c) >= 'A' && (c) <= 'F')
#define isHexAlphaL(c)  ((c) >= 'a' && (c) <= 'f')
#define isHexDigit(c)   (isDigit(c) || isHexAlphaU(c) || isHexAlphaL(c))
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
typedef unsigned short KWord;

#define KB_FLOAT                        float
#define KB_ERROR_MESSAGE_MAX            200
#define KB_TOKEN_LENGTH_MAX             50
#define KB_CONTEXT_VAR_MAX              32
#define KB_CONTEXT_STRING_BUFFER_MAX    1000
#define KB_IDENTIFIER_LEN_MAX           15
#define KB_EXTENDED_BUILT_IN_FUNC_MIN   1000

typedef struct tagVlistNode {
    struct tagVlistNode *prev, *next;
    void *data;
} VlistNode;

typedef struct {
    VlistNode *head, *tail;
    int size;
} Vlist;

Vlist*      vlNewList     ();
Vlist*      vlPushBack    (Vlist* _self, void *data);
void*       vlPopFront    (Vlist* _self);
void*       vlPopBack     (Vlist* _self);
void        vlDestroy     (Vlist* _self, void (* releaseData)(void *));

typedef Vlist VQueue;
#define vqNewQueue                  vlNewList
#define vqPush                      vlPushBack
#define vqPop                       vlPopFront
#define vqIsEmpty(q)                ((q)->size <= 0)
#define vqDestroy(q, releaseData)   vlDestroy(q, releaseData)

typedef struct {
    KDword op;
    union {
        KB_FLOAT    number;
        KDword      index;
        /* 只有第一轮处理在 GOTO / IF_GOTO / UNLESS_GOTO 命令使用这种值 */
        void*       ptr; /* <KbContextLabel *> */
    } param;
} KbOpCommand;

#define opCommandRelease free

typedef enum {
    KBO_NOP = 0,            /* 空命令 */
    /* 基本栈操作命令 */
    KBO_PUSH_VAR,           /* 取全局变量入栈 */
    KBO_PUSH_LOCAL,         /* 取局部变量入栈 */
    KBO_PUSH_NUM,           /* 数字入栈 */
    KBO_PUSH_STR,           /* 字符串入栈 */
    KBO_POP,                /* 出栈 */
    KBO_ASSIGN_VAR,         /* 出栈并且赋值给全局变量 */
    KBO_ASSIGN_LOCAL,       /* 出栈并且赋值给局部变量 */
    /* 运算符命令 */
    KBO_OPR_NEG,            /* 数字取负 */
    KBO_OPR_CONCAT,         /* 字符串连接 */
    KBO_OPR_ADD,            /* 相加 */
    KBO_OPR_SUB,            /* 相减 */
    KBO_OPR_MUL,            /* 相乘 */
    KBO_OPR_DIV,            /* 相除 */
    KBO_OPR_INTDIV,         /* 整数除法 */
    KBO_OPR_MOD,            /* 取模 */
    KBO_OPR_NOT,            /* 逻辑反 */
    KBO_OPR_AND,            /* 逻辑与 */
    KBO_OPR_OR,             /* 逻辑或 */
    KBO_OPR_EQUAL,          /* 相等 */
    KBO_OPR_NEQ,            /* 不相等 */
    KBO_OPR_GT,             /* 大于 */
    KBO_OPR_LT,             /* 小于 */
    KBO_OPR_GTEQ,           /* 大于等于 */
    KBO_OPR_LTEQ,           /* 小于等于 */
    KBO_CALL_BUILT_IN,      /* 调用内置函数 */
    KBO_CALL_USER,          /* 调用 */
    /* 跳转控制命令 */
    KBO_GOTO,               /* 无条件跳转 */
    KBO_IF_GOTO,            /* 出栈，是真后跳转 */
    KBO_UNLESS_GOTO,        /* 出栈，是假后跳转 */
    KBO_RETURN,             /* 函数返回 */
    KBO_STOP                /* 结束执行 */
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

typedef struct {
    int     pos;
    int     numArg;
    int     numVar;
    char    funcName[KB_IDENTIFIER_LEN_MAX + 1];
} KbExportedFunction;

/* 文件头魔法数字，用于校验 */
#define HEADER_MAGIC_FLAG 0x424B

typedef struct {
    /* 文件头魔法数字，用于校验 */
    KDword headerMagic;

    /* 全局变量数 */
    KDword numVariables;

    /* 导出的函数列表 */
    KDword funcBlockStart;     /* 函数部分开始字节位置 */
    KDword numFunc;            /* 函数数量 */

    /* cmd部分 */
    KDword cmdBlockStart;       /* cmd部分开始字节位置  */
    KDword numCmd;              /* cmd数量 */

    /* 字符串部分 */
    KDword stringBlockStart;    /* 字符串部分开始字节位置 */
    KDword stringBlockLength;   /* 字符串部分长度 */
} KbBinaryHeader;

#define endianSwapDword(l) (KDword)((((l) & 0x000000ff) << 24) | (((l) & 0x0000ff00) << 8) | (((l) & 0xff000000) >> 24) | (((l) & 0x00ff0000) >> 8))

#endif