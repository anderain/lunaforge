#ifndef _JASMINE89_H_
#define _JASMINE89_H_

#ifndef NULL
#define NULL ((void *)0)
#endif

#define TOKEN_LENGTH_MAX    200
#define JasmineFloat        double

/* Character / String utilities */
#define JasmineChar_IsDigit(c)      ((c) >= '0' && (c) <= '9')
#define JasmineChar_IsUpperCase(c)  ((c) >= 'a' && (c) <= 'z')
#define JasmineChar_IsLowerCase(c)  ((c) >= 'A' && (c) <= 'Z')
#define JasmineChar_IsAlpha(c)      (JasmineChar_IsUpperCase(c) || JasmineChar_IsLowerCase(c))
#define JasmineChar_IsHexDigit(c)   (JasmineChar_IsDigit(c) || ((c) >= 'a' && (c) <= 'f') || ((c) >= 'A' && (c) <= 'F'))
#define JasmineChar_IsSpace(c)      ((c) == ' ' || (c) == '\t' || (c) == '\n' || (c) == '\r')
#define JasmineChar_IsCtrl(c)       (((c) >= 0 && (c) <= 31) || (c) == 127)

#define JasmineString_Equal(s1, s2) (strcmp((s1), (s2)) == 0)
#define JasmineString_Length(s)     (strlen(s))

int     JasmineString_Copy          (char *dest, int max, const char *src);
int     JasmineString_Append        (char *dest, int max, const char *src);
char*   JasmineString_Dump          (const char *str);

/* Object type defination */
#define JASMINE_NODE_NOT_APPLICABLE -1
#define JASMINE_NODE_NULL            0
#define JASMINE_NODE_OBJECT          1
#define JASMINE_NODE_ARRAY           2
#define JASMINE_NODE_STRING          3
#define JASMINE_NODE_NUMBER          4
#define JASMINE_NODE_BOOLEAN         5

extern const char* JASMINE_NODE_TYPE_NAME_DEBUG[];

#define JasmineNode_TypeString(node) ((node) ? JASMINE_NODE_TYPE_NAME_DEBUG[(node)->type] : "node:nullptr")

/* Node of List */
typedef struct tagJasmineLinkedListNode {
    struct tagJasmineLinkedListNode *prev, *next;
    void *data;
} JasmineLinkedListNode;

/* List */
typedef struct {
    JasmineLinkedListNode *head, *tail;
    int size;
} JasmineLinkedList;

JasmineLinkedListNode*  JasmineLListNode_Create     (void* data);
void                    JasmineLListNode_Dispose    (JasmineLinkedListNode* vln, void (* releaseData)(void *));
JasmineLinkedList*      JasmineLList_Create         ();
JasmineLinkedList*      JasmineLList_PushBack       (JasmineLinkedList* _self, void *data);
void*                   JasmineLList_PopFront       (JasmineLinkedList* _self);
void                    JasmineLList_Dispose        (JasmineLinkedList* _self, void (* releaseData)(void *));

/* Json Node */
typedef struct tagJasmineNode {
    int type;
    union {
        JasmineLinkedList*  listObj;    /* <JasmineKeyValue> */
        JasmineLinkedList*  listArray;  /* <JasmineNode> */
        char*               pString;
        JasmineFloat        fNumber;
        int                 iBoolean;
    } data;
} JasmineNode;

/* Key value */
typedef struct {
    char *keyName;
    struct tagJasmineNode *jasmineNode;
} JasmineKeyValue;

JasmineKeyValue*        JasmineKeyValue_Create          (char* keyName, JasmineNode* jasmineNode, int useDumpKeyName);
void                    JasmineKeyValue_Dispose         (JasmineKeyValue* kv);
void                    JasmineKeyValue_DisposeVoidPtr  (void* kv);

/* Parser */
typedef struct {
    int type;
    char content[TOKEN_LENGTH_MAX];
} JasmineToken;

typedef struct {
    const char *expr;
    const char *eptr;
    JasmineToken token;
    int errorCode;
    char errorString[TOKEN_LENGTH_MAX * 2];
    int lineNumber;
} JasmineParser;

JasmineNode*    Jasmine_ParseObject     (JasmineParser* parser);
JasmineNode*    Jasmine_ParseArray      (JasmineParser* parser);
JasmineNode*    Jasmine_Parse           (const char * szPlainText, JasmineParser** parserArg);
void            JasmineParser_Dispose   (JasmineParser* parser);

void    JasmineNode_Dispose             (JasmineNode* node);
void    JasmineNode_DisposeVoidPtr      (void* node);
#define JasmineNode_IsObject(pNode)     ((pNode)->type == JASMINE_NODE_OBJECT)
#define JasmineNode_IsArray(pNode)      ((pNode)->type == JASMINE_NODE_ARRAY)
#define JasmineNode_IsString(pNode)     ((pNode)->type == JASMINE_NODE_STRING)
#define JasmineNode_IsNumber(pNode)     ((pNode)->type == JASMINE_NODE_NUMBER)
#define JasmineNode_IsBoolean(pNode)    ((pNode)->type == JASMINE_NODE_BOOLEAN)
#define JasmineNode_IsNull(pNode)       ((pNode)->type == JASMINE_NODE_NULL)

#define JASMINE_ERROR_NO_ERROR          0
#define JASMINE_ERROR_START_BRACE       1
#define JASMINE_ERROR_SYNTAX            2

/* jasmine_utils.c */

JasmineFloat        Jasmine_Atof            (const char* str);
int                 Jasmine_Atoi            (const char *s);
char*               Jasmine_Itoa            (int num, char* str, int base);
char*               Jasmine_Ftoa            (double value, char* buffer);
char*               Jasmine_ExtractString   (const char* stringInJson);

/* Find child */
JasmineNode*        JasmineObject_Child    (JasmineNode* _node, const char *key);
JasmineNode*        JasmineArray_At        (JasmineNode* _node, int index);

/* Config utils */
JasmineNode*        JasmineConfig_FindNode          (JasmineNode* node, const char *path);
JasmineFloat        JasmineConfig_LoadFloat         (JasmineNode* node, const char *path, JasmineFloat fallbackValue);
int                 JasmineConfig_LoadInteger       (JasmineNode* node, const char *path, int fallbackValue);
char*               JasmineConfig_LoadStringDumped  (JasmineNode* node, const char *path, const char* fallbackString);

/* Schema */
struct tagJasmineObjectPropertySchema;

typedef struct tagJasmineSchema {
    int type;
    int isOptional;
    const struct tagJasmineObjectPropertySchema* propertySchema;
    int childrenType;
} JasmineSchema;

typedef struct tagJasmineObjectPropertySchema {
    const char*     name;
    JasmineSchema   schema;
} JasmineObjectPropertySchema;

int JasmineSchema_Validate(const char* nodeName, JasmineNode* pNode, const JasmineSchema* pSchema, char *errMessage);

#endif /* _JASMINE89_H_ */