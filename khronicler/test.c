#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kbasic.h"
#include "kalias.h"

static void printStringEscaped(FILE* fp, const unsigned char* szToPrint) {
    int i, ch;
    fprintf(fp, "\"");
    for (i = 0; (ch = szToPrint[i]) != 0; ++i) {
        switch (ch) {
            case '\\':  fprintf(fp, "\\");      break;
            case '\n':  fprintf(fp, "\\n");     break;
            case '\r':  fprintf(fp, "\\r");     break;
            case '\t':  fprintf(fp, "\\t");     break;
            case '"':   fprintf(fp, "\\\"");    break;
            default:
                if (ch >= ' ' && ch <= '~')
                    fprintf(fp, "%c", ch);
                else
                    fprintf(fp, "\\x%02x", ch);
        }
    }
    fprintf(fp, "\"");
}

void dumpKbasicBinary(const char* szOutputFile, const KByte* pRawSerialized) {
    FILE*               fp          = NULL;
    const BinHeader*    pHeader     = (const BinHeader *)pRawSerialized;
    const BinFuncInfo*  pFuncInfo   = (const BinFuncInfo *)(pRawSerialized + pHeader->dwFuncBlockStart);
    const OpCode*       pOpCodes    = (const OpCode *)(pRawSerialized + pHeader->dwOpCodeBlockStart);
    const char*         pStrPool    = (const char *)(pRawSerialized + pHeader->dwStringPoolStart);
    int                 i, s;
    char                szNumBuf[K_NUMERIC_STRINGIFY_BUF_MAX];

    if (szOutputFile == NULL) {
        fp = stdout;
    }

    fprintf(fp, "--------------- Header ---------------\n");
    fprintf(fp, "Header Magic        = 0x%x\n", pHeader->uHeaderMagic.dwVal);
    fprintf(fp, "Little Endian       = %s\n", pHeader->dwIsLittleEndian ? "Yes" : "No");
    fprintf(fp, "Extension Id        = %s\n", pHeader->szExtensionId);
    fprintf(fp, "Num Variables       = %d\n", pHeader->dwNumVariables);
    fprintf(fp, "Func Block Start    = %d\n", pHeader->dwFuncBlockStart);
    fprintf(fp, "Num Func            = %d\n", pHeader->dwNumFunc);
    fprintf(fp, "OpCode Block Start  = %d\n", pHeader->dwOpCodeBlockStart);
    fprintf(fp, "Num OpCode          = %d\n", pHeader->dwNumOpCode);
    fprintf(fp, "String Pool Start   = %d\n", pHeader->dwStringPoolStart);
    fprintf(fp, "String Pool Length  = %d\n", pHeader->dwStringPoolLength);
    fprintf(fp, "String Aligned Size = %d\n", pHeader->dwStringAlignedSize);

    fprintf(fp, "-------------- Function --------------\n");
    for (i = 0; i < pHeader->dwNumFunc; ++i) {
        const BinFuncInfo* pFunc = pFuncInfo + i;
        fprintf(fp, "%3d | %-18s | Param=%2d | Var=%2d | Start=%3d\n", i, pFunc->szName, pFunc->dwNumParams, pFunc->dwNumVars, pFunc->dwOpCodePos);
    }
    
    fprintf(fp, "--------------- OpCode ---------------\n");
    for (i = 0; i < pHeader->dwNumOpCode; ++i) {
        const OpCode* pOpCode = pOpCodes + i;
        fprintf(fp, "%03d | %-18s | ", i, getOpCodeName(pOpCode->dwOpCodeId));
        switch (pOpCode->dwOpCodeId) {
            case K_OPCODE_PUSH_NUM:
                Ftoa(pOpCode->uParam.fLiteral, szNumBuf, K_DEFAULT_FTOA_PRECISION);
                fprintf(fp, "%s", szNumBuf);
                break;
            case K_OPCODE_PUSH_STR:
                printStringEscaped(fp, (const unsigned char *)(pStrPool + pOpCode->uParam.dwStringPoolPos));
                break;
            case K_OPCODE_BINARY_OPERATOR:
            case K_OPCODE_UNARY_OPERATOR:
                fprintf(fp, "%s", getOperatorNameById(pOpCode->uParam.dwOperatorId));
                break;
            case K_OPCODE_POP:
                break;
            case K_OPCODE_PUSH_VAR:
            case K_OPCODE_SET_VAR:
            case K_OPCODE_SET_VAR_AS_ARRAY:
            case K_OPCODE_ARR_GET:
            case K_OPCODE_ARR_SET:
                fprintf(fp, "<%s> %d", pOpCode->uParam.sVarAccess.wIsLocal ? "LOCAL" : "GLOBAL", pOpCode->uParam.sVarAccess.wVarIndex);
                break;
            case K_OPCODE_CALL_BUILT_IN:
                fprintf(fp, "%d", pOpCode->uParam.dwBuiltFuncId);
                break;
            case K_OPCODE_GOTO:
            case K_OPCODE_IF_GOTO:
            case K_OPCODE_UNLESS_GOTO:
                fprintf(fp, "%d", pOpCode->uParam.dwOpCodePos);
                break;
            case K_OPCODE_CALL_FUNC:
                fprintf(fp, "%d", pOpCode->uParam.dwFuncIndex);
                break;
        }
        fprintf(fp, "\n");
    }

    fprintf(fp, "------------ String Pool -------------\n");
    for (i = 0, s = 0; s < pHeader->dwStringPoolLength; ++i, s += strlen(pStrPool + s) + 1) {
        fprintf(fp, "%3d | ", i);
        printStringEscaped(fp, (const unsigned char *)(pStrPool + s));
        fprintf(fp, "\n");
    }
}

static void printTab(FILE* fp, int iTabLevel) {
    int i;
    for (i = 0 ; i < iTabLevel * 2; ++i) {
        fputc(' ', fp);
    }
}

static void printAstNodeAsXml(FILE* fp, AstNode* pAstNode, int iTabLevel) {
    VlistNode* pVlNode;
    const char* szTagName = getAstTypeNameById(pAstNode->iType);

    printTab(fp, iTabLevel); fprintf(fp, "<%s>\n", szTagName);

    if (pAstNode->iLineNumber >= 0) {
        printTab(fp, iTabLevel + 1); fprintf(fp, "<LineNumber> %d </LineNumber>\n", pAstNode->iLineNumber);
    }
    
    switch (pAstNode->iType) {
        case AST_EMPTY:
            break;
        case AST_PROGRAM:
            printTab(fp, iTabLevel + 1); fprintf(fp, "<NumberOfControl> %d </NumberOfControl>\n", pAstNode->uData.sProgram.iNumControl);
            printTab(fp, iTabLevel + 1); fprintf(fp, "<Statements>\n");
            for (pVlNode = pAstNode->uData.sProgram.pListStatements->head; pVlNode; pVlNode = pVlNode->next) {
                printAstNodeAsXml(fp, (AstNode *)pVlNode->data, iTabLevel + 2);
            }
            printTab(fp, iTabLevel + 1); fprintf(fp, "</Statements>\n");
            break;
        case AST_FUNCTION_DECLARE:
            printTab(fp, iTabLevel + 1); fprintf(fp, "<ControlId> %d </ControlId>\n", pAstNode->iControlId);
            printTab(fp, iTabLevel + 1); fprintf(fp, "<Name> %s </Name>\n", pAstNode->uData.sFunctionDeclare.szFunction);
            printTab(fp, iTabLevel + 1); fprintf(fp, "<Parameters>\n");
            for (pVlNode = pAstNode->uData.sFunctionDeclare.pListParameters->head; pVlNode; pVlNode = pVlNode->next) {
                AstFuncParam* pFuncParam = (AstFuncParam *)pVlNode->data;
                printTab(fp, iTabLevel + 2); fprintf(fp, "<Parameter name=\"%s\" type=\"%s\" />\n", pFuncParam->szName, getVarDeclTypeNameById(pFuncParam->iType));
            }
            printTab(fp, iTabLevel + 1); fprintf(fp, "</Parameters>\n");
            printTab(fp, iTabLevel + 1); fprintf(fp, "<Statements>\n");
            for (pVlNode = pAstNode->uData.sFunctionDeclare.pListStatements->head; pVlNode; pVlNode = pVlNode->next) {
                printAstNodeAsXml(fp, (AstNode *)pVlNode->data, iTabLevel + 2);
            }
            printTab(fp, iTabLevel + 1); fprintf(fp, "</Statements>\n");
            break;
         case AST_IF_GOTO:
            printTab(fp, iTabLevel + 1); fprintf(fp, "<LabelName> %s </LabelName>\n", pAstNode->uData.sIfGoto.szLabelName);
            printTab(fp, iTabLevel + 1); fprintf(fp, "<Condition>\n");
            printAstNodeAsXml(fp, pAstNode->uData.sIfGoto.pAstCondition, iTabLevel + 2);
            printTab(fp, iTabLevel + 1); fprintf(fp, "</Condition>\n");
            break;
        case AST_IF:
            printTab(fp, iTabLevel + 1); fprintf(fp, "<ControlId> %d </ControlId>\n", pAstNode->iControlId);
            printTab(fp, iTabLevel + 1); fprintf(fp, "<Condition>\n");
            printAstNodeAsXml(fp, pAstNode->uData.sIf.pAstCondition, iTabLevel + 2);
            printTab(fp, iTabLevel + 1); fprintf(fp, "</Condition>\n");
            printAstNodeAsXml(fp, pAstNode->uData.sIf.pAstThen, iTabLevel + 1);
            for (pVlNode = pAstNode->uData.sIf.pListElseIf->head; pVlNode; pVlNode = pVlNode->next) {
                printAstNodeAsXml(fp, (AstNode *)pVlNode->data, iTabLevel + 1);
            }
            if (pAstNode->uData.sIf.pAstElse) {
                printAstNodeAsXml(fp, pAstNode->uData.sIf.pAstElse, iTabLevel + 1);
            }
            break;
        case AST_THEN:
            printTab(fp, iTabLevel + 1); fprintf(fp, "<Statements>\n");
            for (pVlNode = pAstNode->uData.sThen.pListStatements->head; pVlNode; pVlNode = pVlNode->next) {
                printAstNodeAsXml(fp, (AstNode *)pVlNode->data, iTabLevel + 2);
            }
            printTab(fp, iTabLevel + 1); fprintf(fp, "</Statements>\n");
            break;
        case AST_ELSEIF:
            printTab(fp, iTabLevel + 1); fprintf(fp, "<Condition>\n");
            printAstNodeAsXml(fp, pAstNode->uData.sElseIf.pAstCondition, iTabLevel + 2);
            printTab(fp, iTabLevel + 1); fprintf(fp, "</Condition>\n");
            printTab(fp, iTabLevel + 1); fprintf(fp, "<Statements>\n");
            for (pVlNode = pAstNode->uData.sElseIf.pListStatements->head; pVlNode; pVlNode = pVlNode->next) {
                printAstNodeAsXml(fp, (AstNode *)pVlNode->data, iTabLevel + 2);
            }
            printTab(fp, iTabLevel + 1); fprintf(fp, "</Statements>\n");
            break;
        case AST_ELSE:
            printTab(fp, iTabLevel + 1); fprintf(fp, "<Statements>\n");
            for (pVlNode = pAstNode->uData.sElse.pListStatements->head; pVlNode; pVlNode = pVlNode->next) {
                printAstNodeAsXml(fp, (AstNode *)pVlNode->data, iTabLevel + 2);
            }
            printTab(fp, iTabLevel + 1); fprintf(fp, "</Statements>\n");
            break;
        case AST_WHILE:
            printTab(fp, iTabLevel + 1); fprintf(fp, "<ControlId> %d </ControlId>\n", pAstNode->iControlId);
            printTab(fp, iTabLevel + 1); fprintf(fp, "<Condition>\n");
            printAstNodeAsXml(fp, pAstNode->uData.sWhile.pAstCondition, iTabLevel + 2);
            printTab(fp, iTabLevel + 1); fprintf(fp, "</Condition>\n");
            printTab(fp, iTabLevel + 1); fprintf(fp, "<Statements>\n");
            for (pVlNode = pAstNode->uData.sWhile.pListStatements->head; pVlNode; pVlNode = pVlNode->next) {
                printAstNodeAsXml(fp, (AstNode *)pVlNode->data, iTabLevel + 2);
            }
            printTab(fp, iTabLevel + 1); fprintf(fp, "</Statements>\n");
            break;
        case AST_DO_WHILE:
            printTab(fp, iTabLevel + 1); fprintf(fp, "<ControlId> %d </ControlId>\n", pAstNode->iControlId);
            printTab(fp, iTabLevel + 1); fprintf(fp, "<Statements>\n");
            for (pVlNode = pAstNode->uData.sDoWhile.pListStatements->head; pVlNode; pVlNode = pVlNode->next) {
                printAstNodeAsXml(fp, (AstNode *)pVlNode->data, iTabLevel + 2);
            }
            printTab(fp, iTabLevel + 1); fprintf(fp, "</Statements>\n");
            printTab(fp, iTabLevel + 1); fprintf(fp, "<Condition>\n");
            printAstNodeAsXml(fp, pAstNode->uData.sDoWhile.pAstCondition, iTabLevel + 2);
            printTab(fp, iTabLevel + 1); fprintf(fp, "</Condition>\n");
            break;
        case AST_FOR:
            printTab(fp, iTabLevel + 1); fprintf(fp, "<ControlId> %d </ControlId>\n", pAstNode->iControlId);
            printTab(fp, iTabLevel + 1); fprintf(fp, "<Variable> %s </Variable>\n", pAstNode->uData.sFor.szVariable);
            printTab(fp, iTabLevel + 1); fprintf(fp, "<RangeFrom>\n");
            printAstNodeAsXml(fp, pAstNode->uData.sFor.pAstRangeFrom, iTabLevel + 2);
            printTab(fp, iTabLevel + 1); fprintf(fp, "</RangeFrom>\n");
            printTab(fp, iTabLevel + 1); fprintf(fp, "<RangeTo>\n");
            printAstNodeAsXml(fp, pAstNode->uData.sFor.pAstRangeTo, iTabLevel + 2);
            printTab(fp, iTabLevel + 1); fprintf(fp, "</RangeTo>\n");
            if (pAstNode->uData.sFor.pAstStep) {
                printTab(fp, iTabLevel + 1); fprintf(fp, "<Step>\n");
                printAstNodeAsXml(fp, pAstNode->uData.sFor.pAstStep, iTabLevel + 2);
                printTab(fp, iTabLevel + 1); fprintf(fp, "</Step>\n");
            }
            printTab(fp, iTabLevel + 1); fprintf(fp, "<Statements>\n");
            for (pVlNode = pAstNode->uData.sFor.pListStatements->head; pVlNode; pVlNode = pVlNode->next) {
                printAstNodeAsXml(fp, (AstNode *)pVlNode->data, iTabLevel + 2);
            }
            printTab(fp, iTabLevel + 1); fprintf(fp, "</Statements>\n");
            break;
        case AST_EXIT:
            if (pAstNode->uData.sExit.pAstExpression) {
                printAstNodeAsXml(fp, pAstNode->uData.sExit.pAstExpression, iTabLevel + 1);
            }
            break;
        case AST_RETURN:
            if (pAstNode->uData.sReturn.pAstExpression) {
                printAstNodeAsXml(fp, pAstNode->uData.sReturn.pAstExpression, iTabLevel + 1);
            }
            break;
        case AST_GOTO:
            printTab(fp, iTabLevel + 1); fprintf(fp, "<Label> %s </Label>\n", pAstNode->uData.sGoto.szLabelName);
            break;
        case AST_DIM:
            printTab(fp, iTabLevel + 1); fprintf(fp, "<Variable> %s </Variable>\n", pAstNode->uData.sDim.szVariable);
            if (pAstNode->uData.sDim.pAstInitializer) {
                printTab(fp, iTabLevel + 1); fprintf(fp, "<Initializer>\n");
                printAstNodeAsXml(fp, pAstNode->uData.sDim.pAstInitializer, iTabLevel + 2);
                printTab(fp, iTabLevel + 1); fprintf(fp, "</Initializer>\n");
            }
            break;
        case AST_DIM_ARRAY:
            printTab(fp, iTabLevel + 1); fprintf(fp, "<ArrayName> %s </ArrayName>\n", pAstNode->uData.sDimArray.szArrayName);
            if (pAstNode->uData.sDimArray.pAstDimension) {
                printTab(fp, iTabLevel + 1); fprintf(fp, "<Dimension>\n");
                printAstNodeAsXml(fp, pAstNode->uData.sDimArray.pAstDimension, iTabLevel + 2);
                printTab(fp, iTabLevel + 1); fprintf(fp, "</Dimension>\n");
            }
            break;
        case AST_REDIM:
            printTab(fp, iTabLevel + 1); fprintf(fp, "<ArrayName> %s </ArrayName>\n", pAstNode->uData.sRedim.szArrayName);
            if (pAstNode->uData.sRedim.pAstDimension) {
                printTab(fp, iTabLevel + 1); fprintf(fp, "<Dimension>\n");
                printAstNodeAsXml(fp, pAstNode->uData.sRedim.pAstDimension, iTabLevel + 2);
                printTab(fp, iTabLevel + 1); fprintf(fp, "</Dimension>\n");
            }
            break;
        case AST_ASSIGN:
            printTab(fp, iTabLevel + 1); fprintf(fp, "<Variable> %s </Variable>\n", pAstNode->uData.sAssign.szVariable);
            printTab(fp, iTabLevel + 1); fprintf(fp, "<Value>\n");
            printAstNodeAsXml(fp, pAstNode->uData.sAssign.pAstValue, iTabLevel + 2);
            printTab(fp, iTabLevel + 1); fprintf(fp, "</Value>\n");
            break;
        case AST_ASSIGN_ARRAY:
            printTab(fp, iTabLevel + 1); fprintf(fp, "<Variable> %s </Variable>\n", pAstNode->uData.sAssignArray.szArrayName);
            printTab(fp, iTabLevel + 1); fprintf(fp, "<Subscript>\n");
            printAstNodeAsXml(fp, pAstNode->uData.sAssignArray.pAstSubscript, iTabLevel + 2);
            printTab(fp, iTabLevel + 1); fprintf(fp, "</Subscript>\n");
            printTab(fp, iTabLevel + 1); fprintf(fp, "<Value>\n");
            printAstNodeAsXml(fp, pAstNode->uData.sAssignArray.pAstValue, iTabLevel + 2);
            printTab(fp, iTabLevel + 1); fprintf(fp, "</Value>\n");
            break;
        case AST_LABEL_DECLARE:
            printTab(fp, iTabLevel + 1); fprintf(fp, "<Label> %s </Label>\n", pAstNode->uData.sLabel.szLabelName);
            break;
        case AST_UNARY_OPERATOR:
            printTab(fp, iTabLevel + 1); fprintf(fp, "<Operator> %s </Operator>\n", getOperatorNameById(pAstNode->uData.sUnaryOperator.iOperatorId));
            printTab(fp, iTabLevel + 1); fprintf(fp, "<Operand>\n");
            printAstNodeAsXml(fp, pAstNode->uData.sUnaryOperator.pAstOperand, iTabLevel + 2);
            printTab(fp, iTabLevel + 1); fprintf(fp, "</Operand>\n");
            break;
        case AST_BINARY_OPERATOR:
            printTab(fp, iTabLevel + 1); fprintf(fp, "<Operator> %s </Operator>\n", getOperatorNameById(pAstNode->uData.sBinaryOperator.iOperatorId));
            printTab(fp, iTabLevel + 1); fprintf(fp, "<LeftOperand>\n");
            printAstNodeAsXml(fp, pAstNode->uData.sBinaryOperator.pAstLeftOperand, iTabLevel + 2);
            printTab(fp, iTabLevel + 1); fprintf(fp, "</LeftOperand>\n");
            printTab(fp, iTabLevel + 1); fprintf(fp, "<RightOperand>\n");
            printAstNodeAsXml(fp, pAstNode->uData.sBinaryOperator.pAstRightOperand, iTabLevel + 2);
            printTab(fp, iTabLevel + 1); fprintf(fp, "</RightOperand>\n");
            break;
        case AST_PAREN:
            printAstNodeAsXml(fp, pAstNode->uData.sParen.pAstExpr, iTabLevel + 1);
            break;
        case AST_LITERAL_NUMERIC: {
            char szNumericBuf[K_NUMERIC_STRINGIFY_BUF_MAX];
            Ftoa(pAstNode->uData.sLiteralNumeric.fValue, szNumericBuf, K_DEFAULT_FTOA_PRECISION);
            printTab(fp, iTabLevel + 1); fprintf(fp, "%s\n", szNumericBuf);
            break;
        }
        case AST_LITERAL_STRING:
            printTab(fp, iTabLevel + 1); fprintf(fp, "%s\n", pAstNode->uData.sLiteralString.szValue);
            break;
        case AST_VARIABLE:
            printTab(fp, iTabLevel + 1); fprintf(fp, "%s\n", pAstNode->uData.sVariable.szName);
            break;
        case AST_ARRAY_ACCESS:
            printTab(fp, iTabLevel + 1); fprintf(fp, "<ArrayName> %s </ArrayName>\n", pAstNode->uData.sArrayAccess.szArrayName);
            printTab(fp, iTabLevel + 1); fprintf(fp, "<Subscript>\n");
            printAstNodeAsXml(fp, pAstNode->uData.sArrayAccess.pAstSubscript, iTabLevel + 2);
            printTab(fp, iTabLevel + 1); fprintf(fp, "</Subscript>\n");
            break;
        case AST_FUNCTION_CALL:
            printTab(fp, iTabLevel + 1); fprintf(fp, "<Name> %s </Name>\n", pAstNode->uData.sFunctionCall.szFunction);
            printTab(fp, iTabLevel + 1); fprintf(fp, "<Arguments>\n");
            for (pVlNode = pAstNode->uData.sFunctionCall.pListArguments->head; pVlNode; pVlNode = pVlNode->next) {
                printAstNodeAsXml(fp, (AstNode *)pVlNode->data, iTabLevel + 2);
            }
            printTab(fp, iTabLevel + 1); fprintf(fp, "</Arguments>\n");
            break;
    }

    printTab(fp, iTabLevel); fprintf(fp, "</%s>\n", szTagName);
}

void printAsXml(const char* szOutputFile, KbAstNode* pAstNode) {
    FILE* fp = NULL;
    if (szOutputFile == NULL) {
        fp = stdout;
    }
    if (!pAstNode) return;
    fprintf(fp, "<?xml version=\"1.0\"?>\n");
    printAstNodeAsXml(fp, pAstNode, 0);
}

static void printAstNodeAsJson(FILE* fp, AstNode* pAstNode, int iTabLevel, KBool bSkipFirstTab);

static void printAstNodeStatementsAsJson(
    FILE*   fp,
    Vlist*  pListStatements, /* <AstNode> */ 
    int     iTabLevel
) {
    VlistNode* pVlNode;
    for (
        pVlNode = pListStatements->head;
        pVlNode != NULL;
        pVlNode = pVlNode->next
    ) {
        printAstNodeAsJson(fp, (AstNode *)pVlNode->data, iTabLevel + 1, KB_FALSE);
        if (pVlNode->next != NULL) {
            fprintf(fp, ",\n");
        }
        else {
            fprintf(fp, "\n");
        }
    }
}

static void printAstNodeAsJson(FILE* fp, AstNode* pAstNode, int iTabLevel, KBool bSkipFirstTab) {
    VlistNode* pVlNode;
    if (!bSkipFirstTab) {
        printTab(fp, iTabLevel);
    }
    fprintf(fp, "{\n");
    printTab(fp, iTabLevel + 1); fprintf(fp, "\"astType\": \"%s\",\n", getAstTypeNameById(pAstNode->iType));

    if (pAstNode->iLineNumber >= 0) {
        printTab(fp, iTabLevel + 1); fprintf(fp, "\"lineNumber\": %d,\n", pAstNode->iLineNumber);
    }

    switch (pAstNode->iType) {
        case AST_EMPTY:
            fprintf(fp, "\"isEmpty\": true\n");
            break;
        case AST_PROGRAM:
            printTab(fp, iTabLevel + 1); fprintf(fp, "\"numOfControl\": %d,\n", pAstNode->uData.sProgram.iNumControl);
            printTab(fp, iTabLevel + 1); fprintf(fp, "\"statements\": [\n");
            printAstNodeStatementsAsJson(fp, pAstNode->uData.sProgram.pListStatements, iTabLevel + 1);
            printTab(fp, iTabLevel + 1); fprintf(fp, "]\n");
            break;
        case AST_FUNCTION_DECLARE:
            printTab(fp, iTabLevel + 1); fprintf(fp, "\"controlId\": %d,\n", pAstNode->iControlId);
            printTab(fp, iTabLevel + 1); fprintf(fp, "\"name\": \"%s\",\n", pAstNode->uData.sFunctionDeclare.szFunction);
            printTab(fp, iTabLevel + 1); fprintf(fp, "\"parameters\": [\n");
            for (pVlNode = pAstNode->uData.sFunctionDeclare.pListParameters->head; pVlNode; pVlNode = pVlNode->next) {
                AstFuncParam* pFuncParam = (AstFuncParam *)pVlNode->data;
                printTab(fp, iTabLevel + 2);
                fprintf(fp, "{ \"name\": \"%s\", \"type\": \"%s\" }", pFuncParam->szName, getVarDeclTypeNameById(pFuncParam->iType));
                if (pVlNode->next != NULL) {
                    fprintf(fp, ",\n");
                }
                else {
                    fprintf(fp, "\n");
                }
            }
            printTab(fp, iTabLevel + 1); fprintf(fp, "],\n");
            printTab(fp, iTabLevel + 1); fprintf(fp, "\"statements\": [\n");
            printAstNodeStatementsAsJson(fp, pAstNode->uData.sFunctionDeclare.pListStatements, iTabLevel + 1);
            printTab(fp, iTabLevel + 1); fprintf(fp, "]\n");
            break;
        case AST_IF_GOTO:
            printTab(fp, iTabLevel + 1); fprintf(fp, "\"labelName\": \"%s\",\n", pAstNode->uData.sIfGoto.szLabelName);
            printTab(fp, iTabLevel + 1); fprintf(fp, "\"condition\": ");
            printAstNodeAsJson(fp, pAstNode->uData.sIfGoto.pAstCondition, iTabLevel + 1, KB_TRUE);
            fprintf(fp, "\n");
            break;
        case AST_IF:
            printTab(fp, iTabLevel + 1); fprintf(fp, "\"controlId\": %d,\n", pAstNode->iControlId);
            printTab(fp, iTabLevel + 1); fprintf(fp, "\"condition\": ");
            printAstNodeAsJson(fp, pAstNode->uData.sIf.pAstCondition, iTabLevel + 1, KB_TRUE);
            fprintf(fp, ",\n");
            printTab(fp, iTabLevel + 1); fprintf(fp, "\"then\": ");
            printAstNodeAsJson(fp, pAstNode->uData.sIf.pAstThen, iTabLevel + 1, KB_TRUE);
            fprintf(fp, ",\n");
            printTab(fp, iTabLevel + 1); fprintf(fp, "\"elseif\": [\n");
            printAstNodeStatementsAsJson(fp, pAstNode->uData.sIf.pListElseIf, iTabLevel + 1);
            printTab(fp, iTabLevel + 1); fprintf(fp, "],\n");
            printTab(fp, iTabLevel + 1); fprintf(fp, "\"else\": ");
            if (pAstNode->uData.sIf.pAstElse) {
                printAstNodeAsJson(fp, pAstNode->uData.sIf.pAstElse, iTabLevel + 1, KB_TRUE);
            }
            else {
                fprintf(fp, "null");
            }
            fprintf(fp, "\n");
            break;
        case AST_THEN:
            printTab(fp, iTabLevel + 1); fprintf(fp, "\"statements\": [\n");
            printAstNodeStatementsAsJson(fp, pAstNode->uData.sThen.pListStatements, iTabLevel + 1);
            printTab(fp, iTabLevel + 1); fprintf(fp, "]\n");
            break;
        case AST_ELSEIF:
            printTab(fp, iTabLevel + 1); fprintf(fp, "\"condition\": ");
            printAstNodeAsJson(fp, pAstNode->uData.sElseIf.pAstCondition, iTabLevel + 1, KB_TRUE);
            fprintf(fp, ",\n");
            printTab(fp, iTabLevel + 1); fprintf(fp, "\"statements\": [\n");
            printAstNodeStatementsAsJson(fp, pAstNode->uData.sElseIf.pListStatements, iTabLevel + 1);
            printTab(fp, iTabLevel + 1); fprintf(fp, "]\n");
            break;
        case AST_ELSE:
            printTab(fp, iTabLevel + 1); fprintf(fp, "\"statements\": [\n");
            printAstNodeStatementsAsJson(fp, pAstNode->uData.sElse.pListStatements, iTabLevel + 1);
            printTab(fp, iTabLevel + 1); fprintf(fp, "]\n");
            break;
        case AST_WHILE:
            printTab(fp, iTabLevel + 1); fprintf(fp, "\"controlId\": %d,\n", pAstNode->iControlId);
            printTab(fp, iTabLevel + 1); fprintf(fp, "\"condition\": ");
            printAstNodeAsJson(fp, pAstNode->uData.sWhile.pAstCondition, iTabLevel + 1, KB_TRUE);
            fprintf(fp, ",\n");
            printTab(fp, iTabLevel + 1); fprintf(fp, "\"statements\": [\n");
            printAstNodeStatementsAsJson(fp, pAstNode->uData.sWhile.pListStatements, iTabLevel + 1);
            printTab(fp, iTabLevel + 1); fprintf(fp, "]\n");
            break;
        case AST_DO_WHILE:
            printTab(fp, iTabLevel + 1); fprintf(fp, "\"controlId\": %d,\n", pAstNode->iControlId);
            printTab(fp, iTabLevel + 1); fprintf(fp, "\"statements\": [\n");
            printAstNodeStatementsAsJson(fp, pAstNode->uData.sDoWhile.pListStatements, iTabLevel + 1);
            printTab(fp, iTabLevel + 1); fprintf(fp, "],\n");
            printTab(fp, iTabLevel + 1); fprintf(fp, "\"condition\": ");
            printAstNodeAsJson(fp, pAstNode->uData.sDoWhile.pAstCondition, iTabLevel + 1, KB_TRUE);
            fprintf(fp, "\n");
            break;
        case AST_FOR:
            printTab(fp, iTabLevel + 1); fprintf(fp, "\"controlId\": %d,\n", pAstNode->iControlId);
            printTab(fp, iTabLevel + 1); fprintf(fp, "\"variable\": \"%s\",\n", pAstNode->uData.sFor.szVariable);
            printTab(fp, iTabLevel + 1); fprintf(fp, "\"rangeFrom\": ");
            printAstNodeAsJson(fp, pAstNode->uData.sFor.pAstRangeFrom, iTabLevel + 1, KB_TRUE);
            fprintf(fp, ",\n");
            printTab(fp, iTabLevel + 1); fprintf(fp, "\"rangeTo\": ");
            printAstNodeAsJson(fp, pAstNode->uData.sFor.pAstRangeTo, iTabLevel + 1, KB_TRUE);
            fprintf(fp, ",\n");
            if (pAstNode->uData.sFor.pAstStep) {
                printTab(fp, iTabLevel + 1); fprintf(fp, "\"step\": ");
                printAstNodeAsJson(fp, pAstNode->uData.sFor.pAstStep, iTabLevel + 1, KB_TRUE);
                fprintf(fp, ",\n");
            }
            printTab(fp, iTabLevel + 1); fprintf(fp, "\"statements\": [\n");
            printAstNodeStatementsAsJson(fp, pAstNode->uData.sFor.pListStatements, iTabLevel + 1);
            printTab(fp, iTabLevel + 1); fprintf(fp, "]\n");
            break;
        case AST_EXIT:
            printTab(fp, iTabLevel + 1); fprintf(fp, "\"expression\": ");
            if (pAstNode->uData.sExit.pAstExpression) {
                printAstNodeAsJson(fp, pAstNode->uData.sExit.pAstExpression, iTabLevel + 1, KB_TRUE);
            }
            else {
                fprintf(fp, "null");
            }
            fprintf(fp, "\n");
            break;
        case AST_RETURN:
            printTab(fp, iTabLevel + 1); fprintf(fp, "\"expression\": ");
            if (pAstNode->uData.sReturn.pAstExpression) {
                printAstNodeAsJson(fp, pAstNode->uData.sReturn.pAstExpression, iTabLevel + 1, KB_TRUE);
            }
            else {
                fprintf(fp, "null");
            }
            fprintf(fp, "\n");
            break;
        case AST_GOTO:
            printTab(fp, iTabLevel + 1); fprintf(fp, "\"labelName\": \"%s\"\n", pAstNode->uData.sGoto.szLabelName);
            break;
        case AST_DIM:
            printTab(fp, iTabLevel + 1); fprintf(fp, "\"variable\": \"%s\",\n", pAstNode->uData.sDim.szVariable);
            printTab(fp, iTabLevel + 1); fprintf(fp, "\"initializer\": ");
            if (pAstNode->uData.sDim.pAstInitializer) {
                printAstNodeAsJson(fp, pAstNode->uData.sDim.pAstInitializer, iTabLevel + 1, KB_TRUE);
            }
            else {
                fprintf(fp, "null");
            }
            fprintf(fp, "\n");
            break;
        case AST_DIM_ARRAY:
            printTab(fp, iTabLevel + 1); fprintf(fp, "\"arrayName\": \"%s\"\n", pAstNode->uData.sDimArray.szArrayName);
            printTab(fp, iTabLevel + 1); fprintf(fp, "\"dimension\": ");
            printAstNodeAsJson(fp, pAstNode->uData.sDimArray.pAstDimension, iTabLevel + 1, KB_TRUE);
            fprintf(fp, "\n");
            break;
        case AST_REDIM:
            printTab(fp, iTabLevel + 1); fprintf(fp, "\"arrayName\": \"%s\"\n", pAstNode->uData.sRedim.szArrayName);
            printTab(fp, iTabLevel + 1); fprintf(fp, "\"dimension\": ");
            printAstNodeAsJson(fp, pAstNode->uData.sRedim.pAstDimension, iTabLevel + 1, KB_TRUE);
            fprintf(fp, "\n");
            break;
        case AST_ASSIGN:
            printTab(fp, iTabLevel + 1); fprintf(fp, "\"variable\": \"%s\",\n", pAstNode->uData.sAssign.szVariable);
            printTab(fp, iTabLevel + 1); fprintf(fp, "\"value\": ");
            printAstNodeAsJson(fp, pAstNode->uData.sAssign.pAstValue, iTabLevel + 1, KB_TRUE);
            fprintf(fp, "\n");
            break;
        case AST_ASSIGN_ARRAY:
            printTab(fp, iTabLevel + 1); fprintf(fp, "\"variable\": \"%s\",\n", pAstNode->uData.sAssign.szVariable);
            printTab(fp, iTabLevel + 1); fprintf(fp, "\"subscript\": ");
            printAstNodeAsJson(fp, pAstNode->uData.sAssignArray.pAstSubscript, iTabLevel + 1, KB_TRUE);
            fprintf(fp, ",\n");
            printTab(fp, iTabLevel + 1); fprintf(fp, "\"value\": ");
            printAstNodeAsJson(fp, pAstNode->uData.sAssignArray.pAstValue, iTabLevel + 1, KB_TRUE);
            fprintf(fp, "\n");
            break;
        case AST_LABEL_DECLARE:
            printTab(fp, iTabLevel + 1); fprintf(fp, "\"labelName\": \"%s\"\n", pAstNode->uData.sLabel.szLabelName);
            break;
        case AST_UNARY_OPERATOR:
            printTab(fp, iTabLevel + 1); fprintf(fp, "\"operator\": \"%s\",\n", getOperatorNameById(pAstNode->uData.sUnaryOperator.iOperatorId));
            printTab(fp, iTabLevel + 1); fprintf(fp, "\"operand\": ");
            printAstNodeAsJson(fp, pAstNode->uData.sUnaryOperator.pAstOperand, iTabLevel + 1, KB_TRUE);
            fprintf(fp, "\n");
            break;
        case AST_BINARY_OPERATOR:
            printTab(fp, iTabLevel + 1); fprintf(fp, "\"operator\": \"%s\",\n", getOperatorNameById(pAstNode->uData.sBinaryOperator.iOperatorId));
            printTab(fp, iTabLevel + 1); fprintf(fp, "\"leftOperand\": ");
            printAstNodeAsJson(fp, pAstNode->uData.sBinaryOperator.pAstLeftOperand, iTabLevel + 1, KB_TRUE);
            fprintf(fp, ",\n");
            printTab(fp, iTabLevel + 1); fprintf(fp, "\"rightOperand\": ");
            printAstNodeAsJson(fp, pAstNode->uData.sBinaryOperator.pAstRightOperand, iTabLevel + 1, KB_TRUE);
            fprintf(fp, "\n");
            break;
        case AST_PAREN:
            printTab(fp, iTabLevel + 1); fprintf(fp, "\"expression\": ");
            printAstNodeAsJson(fp, pAstNode->uData.sParen.pAstExpr, iTabLevel + 1, KB_TRUE);
            fprintf(fp, "\n");
            break;
        case AST_LITERAL_NUMERIC: {
            char szNumericBuf[K_NUMERIC_STRINGIFY_BUF_MAX];
            Ftoa(pAstNode->uData.sLiteralNumeric.fValue, szNumericBuf, K_DEFAULT_FTOA_PRECISION);
            printTab(fp, iTabLevel + 1); fprintf(fp, "\"value\": %s", szNumericBuf);
            fprintf(fp, "\n");
            break;
        }
        case AST_LITERAL_STRING:
            printTab(fp, iTabLevel + 1);
            fprintf(fp, "\"value\": ");
            printStringEscaped(fp, (const unsigned char *)pAstNode->uData.sLiteralString.szValue);
            fprintf(fp, "\n");
            break;
        case AST_VARIABLE:
            printTab(fp, iTabLevel + 1); fprintf(fp, "\"variable\": \"%s\"\n", pAstNode->uData.sVariable.szName);
            break;
        case AST_ARRAY_ACCESS:
            printTab(fp, iTabLevel + 1); fprintf(fp, "\"arrayName\": \"%s\",\n", pAstNode->uData.sArrayAccess.szArrayName);
            printTab(fp, iTabLevel + 1); fprintf(fp, "\"subscript\": ");
            printAstNodeAsJson(fp, pAstNode->uData.sArrayAccess.pAstSubscript, iTabLevel + 1, KB_TRUE);
            fprintf(fp, "\n");
            break;
        case AST_FUNCTION_CALL:
            printTab(fp, iTabLevel + 1); fprintf(fp, "\"name\": \"%s\",\n", pAstNode->uData.sFunctionCall.szFunction);
            printTab(fp, iTabLevel + 1); fprintf(fp, "\"arguments\": [\n");
            printAstNodeStatementsAsJson(fp, pAstNode->uData.sFunctionCall.pListArguments, iTabLevel + 1);
            printTab(fp, iTabLevel + 1); fprintf(fp, "]\n");
            break;
    }

    printTab(fp, iTabLevel); fprintf(fp, "}");
}

void formatSyntaxErrorMessage(char* szBuf, int iStopLineNumber, StatementId iStopStatement, SyntaxErrorId iSyntaxErrorId) {
    const char* szSyntaxError = getSyntaxErrMsg(iSyntaxErrorId);
    const char* szStatement = getStatementName(iStopStatement);
    if (iSyntaxErrorId == SYN_EXPECT_LINE_END || iSyntaxErrorId == SYN_EXPR_INVALID) {
        sprintf(szBuf, "Syntax error in '%s' statement: %s", szStatement, szSyntaxError);
    }
    else {
        sprintf(szBuf, "Syntax error: %s", szSyntaxError);
    }
}

void formatSemanticErrorMessage(char* szBuf, const AstNode* pAstSemStop, SemanticErrorId iSemanticErrorId) {
    const char* szSemanticError = getSemanticErrMsg(iSemanticErrorId);
    const char* szAstTypeName = getAstTypeNameById(pAstSemStop->iType);
    sprintf(szBuf, "Semantic error in '%s': %s", szAstTypeName, szSemanticError);
}

void formatRuntimeErrorMessage(char* szBuf, const OpCode* pStopOpCode, RuntimeErrorId iRuntimeErrorId) {
    const char* szRuntimeError = getRuntimeErrMsg(iRuntimeErrorId);
    const char* szOpCodeName = getOpCodeName(pStopOpCode->dwOpCodeId);

    switch (pStopOpCode->dwOpCodeId) {
        case K_OPCODE_UNARY_OPERATOR:
        case K_OPCODE_BINARY_OPERATOR:
            sprintf(szBuf, "Runtime error in '%s.%s': %s", szOpCodeName, getOperatorNameById(pStopOpCode->uParam.dwOperatorId), szRuntimeError);
            break;
        default:
            sprintf(szBuf, "Runtime error in '%s': %s", szOpCodeName, szRuntimeError);
            break;
    }
}

void printAsJson(const char* szOutputFile, KbAstNode* pAstNode) {
    FILE* fp = NULL;
    if (szOutputFile == NULL) {
        fp = stdout;
    }
    if (!pAstNode) return;
    printAstNodeAsJson(fp, pAstNode, 0, KB_FALSE);
    fprintf(fp, "\n");
}

typedef enum tagTestTargetId {
    TEST_CHECK_ERROR = 0,
    TEST_GENERATE_AST
} TestTargetId;


int testMain(int argc, char** argv) {
    const char*     szInputTarget;          /* 命令行传入的测试目标 */
    TestTargetId    iTestTargetId;          /* 测试目标 ID */
    const char*     szSource;               /* 用户输入的源代码 */
    char            szErrorMessage[200];    /* 各种错误的格式化缓冲区 */
    /* Parser 部分 */
    AstNode*        pAstProgram;            /* 源代码解析后生成的 AST 根节点 */
    SyntaxErrorId   iSyntaxErrorId;         /* 语法错误 ID */
    StatementId     iStopStatement;         /* 语法错误停止的语句类型 */
    int             iStopLineNumber;        /* 停止时候的行号 */
    /* Compiler 部分 */
    Context*        pContext;               /* 编译上下文 */
    const AstNode*  pAstSemStop;            /* 编译解析 AST 遇到错误停止时的节点 */
    SemanticErrorId iSemanticErrorId;       /* 语义错误 ID */
    KByte*          pRawSerialized;         /* 序列化后的字节 */
    KDword          dwRawSize;              /* 序列化的字节长度 */
    /* Runtime 部分 */
    Machine*        pMachine;               /* 虚拟机实例 */
    KBool           bExecuteSuccess;        /* 执行是否成功结束 */
    RuntimeErrorId  iRuntimeErrorId;        /* 运行时错误 */
    const OpCode*   pStopOpCode;            /* 运行时错误结束的 opCode */

    if (argc != 3) {
        fprintf(stderr, "Usage: %s 'TestTarget' 'SourceCode'\n", argv[0]);
        fprintf(stderr, "Available targets:\n");
        fprintf(stderr, "  check   - Check syntax, semantic or runtime error.\n");
        fprintf(stderr, "  ast     - Generates an abstract expression tree in JSON format.\n");
        return -1;
    }
    szInputTarget = argv[1];

    if (IsStringEqual(szInputTarget, "check")) {
        iTestTargetId = TEST_CHECK_ERROR;
    }
    else if (IsStringEqual(szInputTarget, "ast")) {
        iTestTargetId = TEST_GENERATE_AST;
    }
    else {
        fprintf(stderr, "Unrecognized target: '%s'\n", szInputTarget);
        return -1;
    }
    
    szSource = argv[2];

    switch (iTestTargetId) {
        case TEST_GENERATE_AST: {
            /* 解析源代码为 AST */
            pAstProgram = parseAsAst(szSource, &iSyntaxErrorId, &iStopStatement, &iStopLineNumber);
            /* 有语法错误 */
            if (iSyntaxErrorId != SYN_NO_ERROR) {
                formatSyntaxErrorMessage(szErrorMessage, iStopLineNumber, iStopStatement, iSyntaxErrorId);
                printf("{\n");
                printf("  \"error\": true,\n");
                printf("  \"errorMessage\": ");
                printStringEscaped(stdout, (const unsigned char *)szErrorMessage);
                printf("\n}\n");
                destroyAst(pAstProgram);
                return 0;
            }
            /* 打印 AST 为 JSON */
            printAsJson(NULL, pAstProgram);
            break;
        }
        case TEST_CHECK_ERROR: {
            /* 解析源代码为 AST */
            pAstProgram = parseAsAst(szSource, &iSyntaxErrorId, &iStopStatement, &iStopLineNumber);
            /* 有语法错误 */
            if (iSyntaxErrorId != SYN_NO_ERROR) {
                formatSyntaxErrorMessage(szErrorMessage, iStopLineNumber, iStopStatement, iSyntaxErrorId);
                printf("{\n");
                printf("  \"error\": true,\n");
                printf("  \"lineNumber\": %d,\n", iStopLineNumber);
                printf("  \"errorId\": \"%s\",\n", getSyntaxErrName(iSyntaxErrorId));
                printf("  \"errorMessage\": ");
                printStringEscaped(stdout, (const unsigned char *)szErrorMessage);
                printf("\n}\n");
                destroyAst(pAstProgram);
                return 0;
            }
            /* 编译 AST 为上下文 */
            pContext = createContext(pAstProgram);
            buildContext(pContext, pAstProgram, &iSemanticErrorId, &pAstSemStop);
            /* 有语义错误 */
            if (iSemanticErrorId != SEM_NO_ERROR) {
                formatSemanticErrorMessage(szErrorMessage, pAstSemStop, iSemanticErrorId);
                printf("{\n");
                printf("  \"error\": true,\n");
                printf("  \"lineNumber\": %d,\n", pAstSemStop->iLineNumber);
                printf("  \"errorId\": \"%s\",\n", getSemanticErrName(iSemanticErrorId));
                printf("  \"errorMessage\": ");
                printStringEscaped(stdout, (const unsigned char *)szErrorMessage);
                printf("\n}\n");
                destroyAst(pAstProgram);
                destroyContext(pContext);
                return 0;
            }
            destroyAst(pAstProgram);

            /* 序列化上下文 */
            serializeContext(pContext, &pRawSerialized, &dwRawSize);
            destroyContext(pContext);

            /* 执行 opCode */
            pMachine = createMachine(pRawSerialized);
            bExecuteSuccess = executeMachine(pMachine, 0, &iRuntimeErrorId, &pStopOpCode);
    
            /* 执行有错误 */
            if (!bExecuteSuccess) {
                formatRuntimeErrorMessage(szErrorMessage, pStopOpCode, iRuntimeErrorId);
                printf("{\n");
                printf("  \"error\": true,\n");
                printf("  \"errorId\": \"%s\",\n", getRuntimeErrName(iRuntimeErrorId));
                printf("  \"errorMessage\": ");
                printStringEscaped(stdout, (const unsigned char *)szErrorMessage);
                printf("\n}\n");
            }
            /* 没有错误 */
            else {
                /* 没有全局变量 */
                if (pMachine->pBinHeader->dwNumVariables <= 0) {
                    printf("{\n");
                    printf("  \"stopValue\": %d\n", pMachine->iStopValue);
                    printf("}\n");
                }
                /* 输出第一个全局变量的值 */
                else {
                    const char* szValueStringified;
                    KBool       bNeedDispose;
                    RtValue*    pRtValue = pMachine->pArrPtrGlobalVars[0];

                    szValueStringified = stringifyRtValue(pRtValue, &bNeedDispose);

                    printf("{\n");
                    printf("  \"stopValue\": %d,\n", pMachine->iStopValue);
                    printf("  \"target\": {\n");
                    printf("    \"type\": \"%s\",\n", getRtValueTypeName(pRtValue->iType));
                    printf("    \"stringified\": ");
                    printStringEscaped(stdout, (const unsigned char *)szValueStringified);
                    printf("\n  }\n");
                    printf("}\n");

                    if (bNeedDispose) {
                        free((void *)szValueStringified);
                    }
                }
            }

            destroyMachine(pMachine);
            free(pRawSerialized);
        }
    }

    return 0;
}

#ifdef IS_TEST_PROGRAM
int main(int argc, char** argv) {
    return testMain(argc, argv);
}
#endif