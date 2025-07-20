#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../jasmine.h"
#include "person_schema.h"

#define TEST_PRINT_ALL_NODES    1
#define TEST_GET_FLOAT          1
#define TEST_GET_INTEGER        1
#define TEST_GET_STRING         1
#define TEST_SCHEMA_VALIDATE    1

const char* JSON_TEST_CASE_0 =
"{"
"  \"seeing\": \"immediately\","
"  \"sleep\": \"effect\","
"  \"rabbit\": ["
"    ["
"      \"somewhere\","
"      {"
"        \"sharp\": \"right\","
"        \"definition\": \"slipped\","
"        \"here\": 1086565867.734243,"
"        \"college\": true"
"      },"
"      {"
"        \"such\": \"knew\","
"        \"vast\": false,"
"        \"entire\": \"written\","
"        \"having\": ["
"          -1065160677.1831651,"
"          ["
"            \"born\","
"            753335367.0710559,"
"            \"leave\","
"            \"hurry\""
"          ],"
"          \"shorter\","
"          false"
"        ]"
"      },"
"      \"wooden\""
"    ],"
"    1675130641,"
"    true,"
"    true"
"  ],"
"  \"brief\": false"
"}";

void TestPrintNode(JasmineNode* node, int depth, int skipSelf) {
    int i;
    if (node == NULL) {
        printf("Failed to print (NULL);\n");
        return;
    }

    if (!skipSelf) {
        for (i = 0; i < 2 * depth; ++i) {
            putchar(' ');
        }
    }

    if (JasmineNode_IsObject(node)) {
        JasmineLinkedListNode *ln = node->data.listObj->head;
        printf("[OBJ] length = %d\n", node->data.listObj->size);
        while (ln != NULL) {
            JasmineKeyValue *kv = (JasmineKeyValue *)ln->data;
            for (i = 0; i < 2 * (depth + 1); ++i) {
                putchar(' ');
            }
            printf("KEY [%s]: ", kv->keyName);
            TestPrintNode(kv->jasmineNode, depth + 1, 1);
            ln = ln->next;
        }
    }
    else if (JasmineNode_IsArray(node)) {
        JasmineLinkedListNode *ln = node->data.listArray->head;
        printf("[ARR] length = %d\n", node->data.listArray->size);
        while (ln != NULL) {
            TestPrintNode((JasmineNode *)ln->data, depth + 1, 0);
            ln = ln->next;
        }
    }
    else if (JasmineNode_IsString(node)) {
        printf("[STR] \"%s\"\n", node->data.pString);
    }
    else if (JasmineNode_IsNumber(node)) {
        printf("[NUM] \"%f\"\n", node->data.fNumber);
    }
    else if (JasmineNode_IsBoolean(node)) {
        printf("[BOOL] \"%s\"\n", node->data.iBoolean ? "true" : "false");
    }
    else if (JasmineNode_IsNull(node)) {
        printf("[NULL] \"null\"\n");
    }
}

const char* JSON_SCHEMA_PERSON_TEST =
"{"
"  \"id\": 29,"
"  \"firstName\": \"Aaliyah\","
"  \"lastName\": \"Foster\","
"  \"email\": \"6b7qk@zztvi.com\","
"  \"active\": false,"
"  \"tags\": ["
"    \"boat\","
"    \"bill\""
"  ],"
"  \"details\": {"
"    \"age\": 22,"
"    \"city\": \"Lagos\","
"    \"country\": \"Indonesia\","
"    \"phoneNumber\": \"(214) 990-1798\","
"    \"startDate\": \"2010-09-02T09:04:57.533Z\","
"    \"endDate\": \"2016-07-25T06:32:28.885Z\""
"  }"
"}";

void TestSchema() {
    const char*     json    = JSON_SCHEMA_PERSON_TEST;
    JasmineNode*    node    = NULL;
    JasmineParser*  parser  = NULL;
    int             validateResult;
    char            errMessage[200] = "No error";

    node = Jasmine_Parse(json, &parser);

    if (TEST_PRINT_ALL_NODES) {
        TestPrintNode(node, 0, 0);
    }

    validateResult = JasmineSchema_Validate("root node", node, &PersonSchema, errMessage);
    if (validateResult) {
        puts("Success!");        
    }
    else {
        printf("Error: %s\n", errMessage);
    }

    JasmineParser_Dispose(parser);
    JasmineNode_Dispose(node);
}

int main(int argc, char *argv[]) {
    const char*     json    = JSON_TEST_CASE_0;
    JasmineNode*    node    = NULL;
    JasmineParser*  parser  = NULL;

    node = Jasmine_Parse(json, &parser);

    if (parser) {
        printf(
            "[Error = %d] Line %d: '%s'\n",
            parser->errorCode,
            parser->lineNumber,
            parser->errorString
        );
    }

    if (TEST_PRINT_ALL_NODES) {
        TestPrintNode(node, 0, 0);
    }
    
    if (TEST_GET_FLOAT) {
        const char* paths[] = {
            "rabbit.0.0",
            "sleep",
            "rabbit.1",
            "rabbit.0.2.having.0",
            "rabbit.3"
        };
        int i;
        for (i = 0; i < sizeof(paths) / sizeof(paths[0]); ++i) {
            printf("[Float] \"%s\" = %lf\n", paths[i], JasmineConfig_LoadFloat(node, paths[i], -1));
        }
    }

    if (TEST_GET_INTEGER) {
        const char* paths[] = {
            "rabbit",
            "sleep",
            "rabbit.1",
            "rabbit.0.2.having.0",
            "rabbit.3"
        };
        int i;
        for (i = 0; i < sizeof(paths) / sizeof(paths[0]); ++i) {
            printf("[Integer] \"%s\" = %d\n", paths[i], JasmineConfig_LoadInteger(node, paths[i], -1));
        }
    }

    if (TEST_GET_STRING) {
        const char* paths[] = {
            "rabbit",
            "sleep",
            "rabbit.1",
            "rabbit.0.2.having.0",
            "rabbit.3"
        };
        char* buf;
        int i;
        for (i = 0; i < sizeof(paths) / sizeof(paths[0]); ++i) {
            printf("[String] \"%s\" = %s\n", paths[i], buf = JasmineConfig_LoadStringDumped(node, paths[i], "nullstring"));
        }
        free(buf);
    }

    JasmineParser_Dispose(parser);
    JasmineNode_Dispose(node);

    if (TEST_SCHEMA_VALIDATE) {
        TestSchema();
    }
    
    return 0;
}