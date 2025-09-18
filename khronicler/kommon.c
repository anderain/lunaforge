#include "kommon.h"

const struct {
    const char* szOperatorName;
    int         iPriority;
} OperatorIdMetaMap[] = {
    { "NONE",       0   },
    { "NEG",        500 },
    { "CONCAT",     90  },
    { "ADD",        100 },
    { "SUB",        100 },
    { "MUL",        200 },
    { "DIV",        200 },
    { "POW",        200 },
    { "INTDIV",     150 },
    { "MOD",        200 },
    { "NOT",        50  },
    { "AND",        40  },
    { "OR",         30  },
    { "EQUAL",      50  },
    { "APPROX_EQ",  50  },
    { "NEQ",        50  },
    { "GT",         60  },
    { "LT",         60  },
    { "GTEQ",       60  },
    { "LTEQ",       60  }
};

const char* Kommon_GetOpCodeName(OpCodeId iOpCodeId) {
    static const char* SZ_OPCODE_NAME[] = {
        "NONE",
        "PUSH_NUM",
        "PUSH_STR",
        "BINARY_OPERATOR",
        "UNARY_OPERATOR",
        "POP",
        "PUSH_VAR",
        "SET_VAR",
        "SET_VAR_AS_ARRAY",
        "ARR_GET",
        "ARR_SET",
        "CALL_BUILT_IN",
        "GOTO",
        "IF_GOTO",
        "UNLESS_GOTO",
        "CALL_FUNC",
        "RETURN",
        "STOP"
    };
    return SZ_OPCODE_NAME[iOpCodeId];
}

int Kommon_GetOperatorPriorityById(OperatorId iOprId) {
    return OperatorIdMetaMap[iOprId].iPriority;
}

const char* Kommon_GetOperatorNameById(OperatorId iOprId) {
    return OperatorIdMetaMap[iOprId].szOperatorName;
}