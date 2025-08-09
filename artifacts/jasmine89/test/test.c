#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../jasmine.h"
#include "person_schema.h"

const char* JSON_CASE_01 =
"{\n"
"  \"game\": {\n"
"    \"title\": \"Phantom Horizon\",\n"
"    \"version\": \"1.4.2\",\n"
"    \"lastUpdated\": \"2025-08-01T12:34:56Z\"\n"
"  },\n"
"  \"graphics\": {\n"
"    \"general\": {\n"
"      \"resolution\": {\n"
"        \"width\": 2560,\n"
"        \"height\": 1440,\n"
"        \"refreshRate\": 144,\n"
"        \"native\": true\n"
"      },\n"
"      \"fullscreen\": false,\n"
"      \"windowMode\": \"borderless\",\n"
"      \"frameLimit\": 240,\n"
"      \"vSync\": true,\n"
"      \"renderScale\": \"2\"\n"
"    },\n"
"    \"quality\": {\n"
"      \"preset\": \"ultra\",\n"
"      \"textureQuality\": \"high\",\n"
"      \"shadowQuality\": \"very_high\",\n"
"      \"textureAnisotropy\": 16,\n"
"      \"levelOfDetail\": {\n"
"        \"enabled\": true,\n"
"        \"distanceBias\": 1.25,\n"
"        \"lodRanges\": [[0, 50],[0, 100],[0, 200]]\n"
"      }\n"
"    }\n"
"  }\n"
"}\n";

typedef struct {
    char*   name;
    char*   path;
    int     fallback;
    int     excepted;
} TestIntCase;

typedef struct {
    char*           name;
    char*           path;
    JasmineFloat    fallback;
    JasmineFloat    excepted;
} TestFloatCase;

typedef struct {
    char*   name;
    char*   path;
    char*   fallback;
    char*   excepted;
} TestStringCase;

const TestIntCase ConfigIntCases[] = {
    { "int_from_num",       "graphics.general.frameLimit",                      -1, 240 },
    { "int_from_str",       "graphics.general.renderScale",                     -1, 2   },
    { "int_from_true",      "graphics.general.resolution.native",               -1, 1   },
    { "int_from_false",     "graphics.general.fullscreen",                      -1, 0   },
    { "int_from_arr_el",    "graphics.quality.levelOfDetail.lodRanges.2.1",     -1, 200 },
    { "int_from_str_0",     "graphics.quality.preset",                          -1, 0   },
    { "int_fallback",       "game.not_exist",                                   -1, -1  }
};

const TestFloatCase ConfigFloatCases[] = {
    { "float_from_num",     "graphics.general.frameLimit",                      -1, 240 },
    { "float_from_str",     "graphics.general.renderScale",                     -1, 2   },
    { "float_from_true",    "graphics.general.resolution.native",               -1, 1   },
    { "float_from_false",   "graphics.general.fullscreen",                      -1, 0   },
    { "float_from_arr_el",  "graphics.quality.levelOfDetail.lodRanges.1.1",     -1, 100 },
    { "float_from_str_0",   "graphics.quality.preset",                          -1, 0   },
    { "float_fallback",     "game.not_exist",                                   -1, -1  }
};

const TestStringCase ConfigStringCases[] = {
    { "str_from_num",       "graphics.general.frameLimit",                      "x", "240"      },
    { "str_from_str",       "graphics.general.renderScale",                     "x", "2"        },
    { "str_from_true",      "graphics.general.resolution.native",               "x", "true"     },
    { "str_from_false",     "graphics.general.fullscreen",                      "x", "false"    },
    { "str_from_arr_el",    "graphics.quality.levelOfDetail.lodRanges.1.1",     "x", "100"      },
    { "str_from_str_c1",    "graphics.quality.preset",                          "x", "ultra"    },
    { "str_from_str_c2",    "game.version",                                     "x", "1.4.2"    },
    { "str_fallback",       "game.not_exist",                                   "x", "x"        }
};

#define NUM_INT_CASES       (sizeof(ConfigIntCases) / sizeof(ConfigIntCases[0]))
#define NUM_FLOAT_CASES     (sizeof(ConfigFloatCases) / sizeof(ConfigFloatCases[0]))
#define NUM_STRING_CASES    (sizeof(ConfigStringCases) / sizeof(ConfigStringCases[0]))

struct {
    int intCasesNum;
    int intCasesPassed;
    int intIsPassed[NUM_INT_CASES];
    int intActualGot[NUM_INT_CASES];

    int floatCasesNum;
    int floatCasesPassed;
    int floatIsPassed[NUM_FLOAT_CASES];
    JasmineFloat floatActualGot[NUM_FLOAT_CASES];

    int stringCasesNum;
    int stringCasesPassed;
    int stringIsPassed[NUM_STRING_CASES];
    char stringActualGot[NUM_STRING_CASES][100];

    int schemaPassed;
} TestReport = {
    NUM_INT_CASES,      0,  {}, {},
    NUM_FLOAT_CASES,    0,  {}, {},
    NUM_STRING_CASES,   0,  {}, {},
    0
};

void performConfigTest() {
    int             i;
    const char*     json        = JSON_CASE_01;
    JasmineNode*    rootNode    = NULL;
    JasmineParser*  parser      = NULL;

    for (i = 0; i < TestReport.intCasesNum; ++i) {
        TestReport.intActualGot[i] = -9999;
        TestReport.intIsPassed[i] = 0;
    }

    for (i = 0; i < TestReport.floatCasesNum; ++i) {
        TestReport.floatActualGot[i] = -9999;
        TestReport.floatIsPassed[i] = 0;
    }

    for (i = 0; i < TestReport.stringCasesNum; ++i) {
        strcpy(TestReport.stringActualGot[i], "N/A");
        TestReport.stringIsPassed[i] = 0;
    }

    rootNode = Jasmine_Parse(json, &parser);

    for (i = 0; i < TestReport.intCasesNum; ++i) {
        const TestIntCase* pCase = ConfigIntCases + i;
        int actual = JasmineConfig_LoadInteger(rootNode, pCase->path, pCase->fallback);
        if (actual == pCase->excepted) {
            TestReport.intCasesPassed++;
            TestReport.intIsPassed[i] = 1;
        }
        TestReport.intActualGot[i] = actual;
    }

    for (i = 0; i < TestReport.floatCasesNum; ++i) {
        const TestFloatCase* pCase = ConfigFloatCases + i;
        JasmineFloat actual = JasmineConfig_LoadFloat(rootNode, pCase->path, pCase->fallback);
        if (fabs(actual - pCase->excepted) < 1e-8) {
            TestReport.floatCasesPassed++;
            TestReport.floatIsPassed[i] = 1;
        }
        TestReport.floatActualGot[i] = actual;
    }

    for (i = 0; i < TestReport.stringCasesNum; ++i) {
        const TestStringCase* pCase = ConfigStringCases + i;
        char* actual = JasmineConfig_LoadStringDumped(rootNode, pCase->path, pCase->fallback);
        if (strcmp(actual, pCase->excepted) == 0) {
            TestReport.stringCasesPassed++;
            TestReport.stringIsPassed[i] = 1;
        }
        strcpy(TestReport.stringActualGot[i], actual);
    }

    JasmineNode_Dispose(rootNode);
    JasmineParser_Dispose(parser);
}

const char* JSON_CASE_02 =
"{\n"
"  \"id\": 29,\n"
"  \"firstName\": \"Aaliyah\",\n"
"  \"lastName\": \"Foster\",\n"
"  \"email\": \"6b7qk@zztvi.com\",\n"
"  \"active\": false,\n"
"  \"tags\": [\n"
"    \"boat\",\n"
"    \"bill\"\n"
"  ],\n"
"  \"details\": {\n"
"    \"age\": 22,\n"
"    \"city\": \"Lagos\",\n"
"    \"country\": \"Indonesia\",\n"
"    \"phoneNumber\": \"(214) 990-1798\",\n"
"    \"startDate\": \"2010-09-02T09:04:57.533Z\",\n"
"    \"endDate\": \"2016-07-25T06:32:28.885Z\"\n"
"  },\n"
"  \"educationBackground\": [\n"
"    {\n"
"      \"institution\": \"Jasmine No. 1 Experimental Primary School\",\n"
"      \"period\": \"2006-09 to 2012-06\",\n"
"      \"degree\": \"Primary School\"\n"
"    },\n"
"    {\n"
"      \"institution\": \"Jasmine No. 1 Middle School\",\n"
"      \"period\": \"2012-09 to 2015-06\",\n"
"      \"degree\": \"Junior High School\",\n"
"    },\n"
"    {\n"
"      \"institution\": \"Jasmine Senior High School\",\n"
"      \"period\": \"2015-09 to 2018-06\",\n"
"      \"degree\": \"High School\"\n"
"    },\n"
"    {\n"
"      \"institution\": \"Jasmine University\",\n"
"      \"period\": \"2018-09 to 2022-06\",\n"
"      \"degree\": \"University\"\n"
"    }\n"
"  ]\n"
"}";

void performSchemaTest() {
    const char*     json        = JSON_CASE_02;
    JasmineNode*    rootNode    = NULL;
    JasmineParser*  parser      = NULL;
    int             validateResult;
    char            errMessage[200] = "No error";

    rootNode = Jasmine_Parse(json, &parser);

    validateResult = JasmineSchema_Validate("root node", rootNode, &PersonSchema, errMessage);
    TestReport.schemaPassed = validateResult;

    JasmineParser_Dispose(parser);
    JasmineNode_Dispose(rootNode);
}

void printAsHtml(FILE* fp) {
    int i;

    fputs(
        "<!DOCTYPE html>"
        "<html lang=\"en-US\">"
        "<head>"
        "<meta charset=\"UTF-8\">"
        "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
        "<title> Jasmine89 Test Report </title>"
        "</head>"
        "<body>"
        "<div class=\"report-container\">",
        fp
    );

    fputs("<h1>Jasmine89 Test Report</h1>", fp);

    fputs("<h2>Config Test</h2>\n", fp);
    fputs("<table>", fp);
    fputs(
        "<tr>"
        "<th class=\"case-id\">Case ID</th>"
        "<th class=\"case-name\">Case Name</th>"
        "<th class=\"source-code\">Path</th>"
        "<th class=\"text-expected\">Expected Output</th>"
        "<th class=\"text-got\">Actual Output</th>"
        "<th class=\"test-result\">Result</th>"
        "</tr>",
        fp
    );

    for (i = 0; i < TestReport.intCasesNum; ++i) {
        const TestIntCase* pCase = ConfigIntCases + i;
        fputs("<tr>", fp);
        fprintf(fp, "<td class=\"case-id\"> INT-%03d </td>", i + 1);
        fprintf(fp, "<td class=\"case-name\"> %s </td>", pCase->name);
        fprintf(fp, "<td class=\"source-code\"><pre>%s</pre></td>", pCase->path);
        fprintf(fp, "<td class=\"text-expected\">%d</td>", pCase->excepted);
        fprintf(fp, "<td class=\"text-got\">%d</td>", TestReport.intActualGot[i]);
        fprintf(fp, "<td class=\"test-result\"><span class=\"%s\"> %s </span></td>", TestReport.intIsPassed[i] ? "test-status success" : "test-status failed", TestReport.intIsPassed[i] ? "PASSED" : "NOT PASSED");
        fputs("</tr>", fp);
    }

    for (i = 0; i < TestReport.floatCasesNum; ++i) {
        const TestFloatCase* pCase = ConfigFloatCases + i;
        char pBufExcepted[100];
        char pBufActual[100];
        Jasmine_Ftoa(pCase->excepted, pBufExcepted);
        Jasmine_Ftoa(TestReport.floatActualGot[i], pBufActual);
        fputs("<tr>", fp);
        fprintf(fp, "<td class=\"case-id\"> FLOAT-%03d </td>", i + 1);
        fprintf(fp, "<td class=\"case-name\"> %s </td>", pCase->name);
        fprintf(fp, "<td class=\"source-code\"><pre>%s</pre></td>", pCase->path);
        fprintf(fp, "<td class=\"text-expected\">%s</td>", pBufExcepted);
        fprintf(fp, "<td class=\"text-got\">%s</td>", pBufActual);
        fprintf(fp, "<td class=\"test-result\"><span class=\"%s\"> %s </span></td>", TestReport.floatIsPassed[i] ? "test-status success" : "test-status failed", TestReport.floatIsPassed[i] ? "PASSED" : "NOT PASSED");
        fputs("</tr>", fp);
    }

    for (i = 0; i < TestReport.stringCasesNum; ++i) {
        const TestStringCase* pCase = ConfigStringCases + i;
        fputs("<tr>", fp);
        fprintf(fp, "<td class=\"case-id\"> STRING-%03d </td>", i + 1);
        fprintf(fp, "<td class=\"case-name\"> %s </td>", pCase->name);
        fprintf(fp, "<td class=\"source-code\"><pre>%s</pre></td>", pCase->path);
        fprintf(fp, "<td class=\"text-expected\">%s</td>", pCase->excepted);
        fprintf(fp, "<td class=\"text-got\">%s</td>", TestReport.stringActualGot[i]);
        fprintf(fp, "<td class=\"test-result\"><span class=\"%s\"> %s </span></td>", TestReport.stringIsPassed[i] ? "test-status success" : "test-status failed", TestReport.stringIsPassed[i] ? "PASSED" : "NOT PASSED");
        fputs("</tr>", fp);
    }
    fputs("<table>\n", fp);

    fputs("<h2>Schema Test</h2>\n", fp);
    fputs("<table>", fp);
    fputs(
        "<tr>"
        "<th class=\"case-id\">Case ID</th>"
        "<th class=\"source-code\">JSON</th>"
        "<th class=\"test-result\">Result</th>"
        "</tr>",
        fp
    );
    fputs("<tr>", fp);
    fprintf(fp, "<td class=\"case-id\"> SCHEMA-001 </td>");
    fprintf(fp, "<td class=\"source-code\"><pre>%s</pre></td>", JSON_CASE_01);
    fprintf(fp, "<td class=\"test-result\"><span class=\"%s\"> %s </span></td>", TestReport.schemaPassed ? "test-status success" : "test-status failed", TestReport.schemaPassed ? "PASSED" : "NOT PASSED");
    fputs("</tr>", fp);
    fputs("<table>\n", fp);

    fputs(
        "</div>"
        "</body>\n"
        "<style>"
        "td.case-id{color:#999;font-weight:bold}"
        "body,html{color: #333;padding:0;margin:0;overflow-x:hidden;font-family:Arial,Helvetica,sans-serif}"
        ".report-container{width:100%;max-width:1200px;margin:0 auto;padding:10px}"
        "table{table-layout:auto;width:100%;font-size:12px;border-collapse:collapse}"
        "h1,h2,h3,h4,h5,h6{color: #149a96}"
        "h1{padding-bottom:4px;border-bottom:2px solid #96cac8}"
        "tr{border-bottom:1px solid #ddd}"
        "td{padding:4px 4px}"
        "th{color:#fff;background:linear-gradient(#12918d, #4fb6b2);padding:8px 4px;}"
        "pre{text-align:left;margin: 0;font-family:\"Consolas\",\"Monaco\",\"Lucida Console\",\"Courier New\",monospace;font-size:12px;line-height:1.25;tab-size:4}"
        "th.case-id{width:100px}"
        "td.case-id{text-align:center}"
        "td.text-expected,td.text-got{text-align:center;max-width:100px}"
        "td.numeric{text-align:right}"
        ".test-status{font-weight:700;white-space:nowrap;padding:2px 8px;border-radius:4px;color:#fff;display:block;margin:0 auto;width:fit-content}"
        ".test-status.success{background-color:green}"
        ".test-status.failed{background-color:red}"
        "</style>\n"
        "</html>",
        fp
    );
}

int main(int argc, char *argv[]) {
    performConfigTest();
    performSchemaTest();
    printAsHtml(stdout);
    return 0;
}