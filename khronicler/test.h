#ifndef _TEST_H_
#define _TEST_H_

#include "kbasic.h"

void printAsXml                 (const char* szOutputFile, KbAstNode* pAstNode);
void printAsJson                (const char* szOutputFile, KbAstNode* pAstNode);
void dumpKbasicBinary           (const char* szOutputFile, const KByte* pRawSerialized);
void formatSyntaxErrorMessage   (char* szBuf, int iStopLineNumber, StatementId iStopStatement, SyntaxErrorId iSyntaxErrorId);
void formatSemanticErrorMessage (char* szBuf, const AstNode* pAstSemStop, SemanticErrorId iSemanticErrorId);
void formatRuntimeErrorMessage  (char* szBuf, const OpCode* pStopOpCode, RuntimeErrorId iRuntimeErrorId);
#endif