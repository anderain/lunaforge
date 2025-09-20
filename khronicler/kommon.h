#ifndef _KOMMON_H_
#define _KOMMON_H_

#define isDigit(c)      ((c) >= '0' && (c) <= '9')
#define isUppercase(c)  ((c) >= 'A' && (c) <= 'Z')
#define isLowercase(c)  ((c) >= 'a' && (c) <= 'z')
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

typedef unsigned int    KDword;
typedef unsigned short  KWord;
typedef unsigned char   KByte;
typedef int             KBool;
typedef float           KFloat;
typedef int             KLabelOpCodePos;

/* 布尔值 */
#define KB_TRUE                         1
#define KB_FALSE                        0

/* Parser 的行缓冲最大长度 */
#define KB_PARSER_LINE_MAX              500

/* Parser 中 Token 的最大长度  */
#define KB_TOKEN_LENGTH_MAX             100

/* 标志符（函数名、变量名）允许的最大长度 */
#define KB_IDENTIFIER_LEN_MAX           15

/* 全局变量或者局部变量最大数值 */
#define KB_CONTEXT_VAR_MAX              32

/* 二进制文件中字符串常量池的最大长度 */
#define KB_CONTEXT_STRING_POOL_MAX      500

/* 脚本使用的拓展 Id */
#define KB_HEADER_EXT_ID_MAX_LENGTH     15

/* 数字格式化为字符串的缓冲区大小 */
#define K_NUMERIC_STRINGIFY_BUF_MAX     40

typedef enum tagVarDeclTypeId {
    VARDECL_PRIMITIVE = 0,
    VARDECL_ARRAY
} VarDeclTypeId;

typedef enum {
    OPR_NONE = 0,
    OPR_NEG,
    /* String */
    OPR_CONCAT,
    /* Basic: + , - , * , / , ^ */
    OPR_ADD, OPR_SUB, OPR_MUL, OPR_DIV, OPR_POW,
    /* Integer: INTDIV , MOD */
    OPR_INTDIV, OPR_MOD,
    /* Logic: ! , && , ||*/
    OPR_NOT, OPR_AND, OPR_OR,
    /* Comparison */
    OPR_EQUAL, OPR_APPROX_EQ, OPR_NEQ, OPR_GT, OPR_LT,
    OPR_GTEQ, OPR_LTEQ 
} OperatorId;

typedef enum {
    KBUILT_IN_FUNC_NONE = 0,
    KBUILT_IN_FUNC_P,           /* 打印内容 */
    KBUILT_IN_FUNC_SIN,         /* 数学函数 sin */
    KBUILT_IN_FUNC_COS,         /* 数学函数 cos */
    KBUILT_IN_FUNC_TAN,         /* 数学函数 tan */
    KBUILT_IN_FUNC_SQRT,        /* 数学函数 平方根 */
    KBUILT_IN_FUNC_EXP,         /* 数学函数 指数 */
    KBUILT_IN_FUNC_ABS,         /* 数学函数 绝对值 */
    KBUILT_IN_FUNC_LOG,         /* 数学函数 对数 */
    KBUILT_IN_FUNC_FLOOR,       /* 数学函数 向上取整 */
    KBUILT_IN_FUNC_CEIL,        /* 数学函数 向下取整 */
    KBUILT_IN_FUNC_RAND,        /* 随机值，范围 [0, 1) */
    KBUILT_IN_FUNC_LEN,         /* 字符串 求长度 */
    KBUILT_IN_FUNC_VAL,         /* 字符串 转数字 */
    KBUILT_IN_FUNC_CHR,         /* 数字转字符 */
    KBUILT_IN_FUNC_ASC,         /* 字符串 第一个字符的ASCII值 */
} BuiltFuncId;

typedef enum tagOpCodeId {
    /* OpCode memory layout        [---------- DWORD ----------] */
    K_OPCODE_NONE = 0,          /* [            n/a            ] */
    K_OPCODE_PUSH_NUM,          /* [      literal_numeric      ] */
    K_OPCODE_PUSH_STR,          /* [      string_pool_pos      ] */
    K_OPCODE_BINARY_OPERATOR,   /* [        operator_id        ] */
    K_OPCODE_UNARY_OPERATOR,    /* [        operator_id        ] */
    K_OPCODE_POP,               /* [            n/a            ] */
    K_OPCODE_PUSH_VAR,          /* [  is_local  ][  var_index  ] */
    K_OPCODE_SET_VAR,           /* [  is_local  ][  var_index  ] */
    K_OPCODE_SET_VAR_AS_ARRAY,  /* [  is_local  ][  var_index  ] */
    K_OPCODE_ARR_GET,           /* [  is_local  ][  var_index  ] */
    K_OPCODE_ARR_SET,           /* [  is_local  ][  var_index  ] */
    K_OPCODE_CALL_BUILT_IN,     /* [     built_in_func_id      ] */
    K_OPCODE_GOTO,              /* [         opcode_pos        ] */
    K_OPCODE_IF_GOTO,           /* [         opcode_pos        ] */
    K_OPCODE_UNLESS_GOTO,       /* [         opcode_pos        ] */
    K_OPCODE_CALL_FUNC,         /* [       function_index      ] */
    K_OPCODE_RETURN,            /* [            n/a            ] */
    K_OPCODE_STOP,              /* [            n/a            ] */
} OpCodeId;

typedef struct tagOpCode {
    KDword dwOpCodeId;
    union {
        struct {
            KWord wIsLocal;
            KWord wVarIndex;
        } sVarAccess;
        KFloat fLiteral;
        KDword dwOperatorId;
        KDword dwStringPoolPos;
        KDword dwBuiltFuncId;
        KDword dwFuncIndex;
        KDword dwOpCodePos;
        KLabelOpCodePos* pLabelOpCodePos;
    } uParam;
} OpCode;

/* 文件头魔法数字，用于校验 */
#define K_HEADER_MAGIC_BYTE_0       'k'
#define K_HEADER_MAGIC_BYTE_1       'b'
#define K_HEADER_MAGIC_BYTE_2       's'
#define K_HEADER_MAGIC_BYTE_3       '3'

typedef struct tagKbBinaryFunctionInfo {
    char szName[KB_IDENTIFIER_LEN_MAX + 1];
    KDword dwNumParams;
    KDword dwNumVars;
    KDword dwOpCodePos;
} KbBinaryFunctionInfo;

typedef struct tagKbBinaryHeader {
    /* 文件头魔法数字，用于校验 */
    union {
        KByte bVal[4];
        KDword dwVal;
    } uHeaderMagic;

    /* 是否是小端序 */
    KDword dwIsLittleEndian;

    /* 拓展标志 */
    char szExtensionId[KB_HEADER_EXT_ID_MAX_LENGTH + 1];

    /* 全局变量数 */
    KDword dwNumVariables;

    /* 导出的函数列表 */
    KDword dwFuncBlockStart;     /* 函数部分开始字节位置 */
    KDword dwNumFunc;            /* 函数数量 */

    /* opCode 部分 */
    KDword dwOpCodeBlockStart;  /* opCode 部分开始字节位置  */
    KDword dwNumOpCode;         /* opCode 数量 */

    /* 字符串部分 */
    KDword dwStringPoolStart;   /* 字符串部分开始字节位置 */
    KDword dwStringPoolLength;  /* 字符串部分长度 */
    KDword dwStringAlignedSize; /* 对齐后的字符串长度 */

} KbBinaryHeader;

const char* Kommon_GetOpCodeName            (OpCodeId iOpCodeId);
int         Kommon_GetOperatorPriorityById  (OperatorId iOprId);
const char* Kommon_GetOperatorNameById      (OperatorId iOprId);
const char* Kommon_GetVarDeclTypeName       (VarDeclTypeId iTypeId);

#endif