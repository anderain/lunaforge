import subprocess
import json
from datetime import date
from jinja2 import Environment, FileSystemLoader, select_autoescape

# 配置
TestProgram = "./ktest.exe"
TestReportFile = "test_report.html"

# 语法测试用例
SyntaxTestCases = [
  {
    "caseId": "FuncDeclNested",
    "source": "func outside()\n  func nested()",
    "expected": "SYN_FUNC_NESTED",
  },
  {
    "caseId": "FuncDeclNameMissing",
    "source": "func (",
    "expected": "SYN_FUNC_MISSING_NAME",
  },
  {
    "caseId": "FuncDeclMissingLeftParen",
    "source": "func missingParen",
    "expected": "SYN_FUNC_MISSING_LEFT_PAREN",
  },
  {
    "caseId": "FuncDeclInvalidParam1",
    "source": "func missingParameter(",
    "expected": "SYN_FUNC_INVALID_PARAMETERS",
  },
  {
    "caseId": "FuncDeclInvalidParam2",
    "source": "func invalidList(+",
    "expected": "SYN_FUNC_INVALID_PARAMETERS",
  },
  {
    "caseId": "FuncDeclInvalidParam3",
    "source": "func invalidList(a,+)",
    "expected": "SYN_FUNC_INVALID_PARAMETERS",
  },
  {
    "caseId": "FuncDeclInvalidParam4",
    "source": "func invalidList(a,b[)",
    "expected": "SYN_FUNC_INVALID_PARAMETERS",
  },
  {
    "caseId": "FuncDeclLineNotEnd",
    "source": "func invalidList(a,b),",
    "expected": "SYN_EXPECT_LINE_END",
  },
  {"caseId": "IfMissingCondition", "source": "if", "expected": "SYN_EXPR_INVALID"},
  {
    "caseId": "IfGotoMissingLabel",
    "source": "if cond goto",
    "expected": "SYN_IF_GOTO_MISSING_LABEL",
  },
  {
    "caseId": "IfGotoNotEnd",
    "source": "if cond goto myLabel notEnd",
    "expected": "SYN_EXPECT_LINE_END",
  },
  {
    "caseId": "IfNotEnd",
    "source": "if cond notEnd",
    "expected": "SYN_EXPECT_LINE_END",
  },
  {
    "caseId": "ElseIfNotMatch",
    "source": "func funcDecl()\n  elseif",
    "expected": "SYN_ELSEIF_NOT_MATCH",
  },
  {
    "caseId": "ElseIfMissingCondition",
    "source": "if 1 = 1\nelseif",
    "expected": "SYN_EXPR_INVALID",
  },
  {
    "caseId": "ElseIfNotEnd",
    "source": "if 1 = 2\nelseif 1 = 1 notEnd",
    "expected": "SYN_EXPECT_LINE_END",
  },
  {
    "caseId": "ElseNotMatch",
    "source": "func funcDecl()\n  else",
    "expected": "SYN_ELSE_NOT_MATCH",
  },
  {
    "caseId": "ElseNotEnd",
    "source": "if 1 = 1\nelse notEnd",
    "expected": "SYN_EXPECT_LINE_END",
  },
  {
    "caseId": "WhileMissingCondition",
    "source": "while",
    "expected": "SYN_EXPR_INVALID",
  },
  {
    "caseId": "WhileNotEnd",
    "source": "while 1 = 1 notEnd",
    "expected": "SYN_EXPECT_LINE_END",
  },
  {
    "caseId": "DoWhileMissingCondition",
    "source": "do\nwhile",
    "expected": "SYN_EXPR_INVALID",
  },
  {
    "caseId": "DoWhileNotEnd",
    "source": "do\nwhile 1 = 1 notEnd",
    "expected": "SYN_EXPECT_LINE_END",
  },
  {
    "caseId": "DoWhileNotEnd",
    "source": "do\nwhile 1 = 1 notEnd",
    "expected": "SYN_EXPECT_LINE_END",
  },
  {
    "caseId": "DoNotEnd",
    "source": "do notEnd",
    "expected": "SYN_EXPECT_LINE_END"
  },
  {
    "caseId": "ForMissingVar",
    "source": "for 1 to 100",
    "expected": "SYN_FOR_MISSING_VARIABLE",
  },
  {
    "caseId": "ForMissingEqual",
    "source": "for i 1 to 100",
    "expected": "SYN_FOR_MISSING_EQUAL",
  },
  {
    "caseId": "ForInvalidRangeFrom",
    "source": "for i = to 100",
    "expected": "SYN_EXPR_INVALID",
  },
  {
    "caseId": "ForMissingTo",
    "source": "for i = 1 100",
    "expected": "SYN_FOR_MISSING_TO",
  },
  {
    "caseId": "ForInvalidRangeTo",
    "source": "for i = 1 to",
    "expected": "SYN_EXPR_INVALID",
  },
  {
    "caseId": "ForInvalidStepExpr",
    "source": "for i = 1 to 100 step /",
    "expected": "SYN_EXPR_INVALID",
  },
  {
    "caseId": "ForNotEnd",
    "source": "for i = 1 to 100 step 1 notEnd",
    "expected": "SYN_EXPECT_LINE_END",
  },
  {
    "caseId": "NextNotMatch",
    "source": "if 1 = 1\nnext i",
    "expected": "SYN_NEXT_NOT_MATCH",
  },
  {
    "caseId": "NextVarNotMatch",
    "source": "for i=0 to 10\nnext j",
    "expected": "SYN_FOR_VAR_MISMATCH",
  },
  {
    "caseId": "NextNotEnd1",
    "source": "for i=0 to 10\nnext /",
    "expected": "SYN_EXPECT_LINE_END",
  },
  {
    "caseId": "NextNotEnd1",
    "source": "for i=0 to 10\nnext i /",
    "expected": "SYN_EXPECT_LINE_END",
  },
  {
    "caseId": "BreakNotInLoop",
    "source": "if 1 = 1\nbreak",
    "expected": "SYN_BREAK_OUTSIDE_LOOP",
  },
  {
    "caseId": "BreakNotEnd",
    "source": "while 1 = 1\nbreak notEnd",
    "expected": "SYN_EXPECT_LINE_END",
  },
  {
    "caseId": "ContinueNotInLoop",
    "source": "if 1 = 1\ncontinue",
    "expected": "SYN_CONTINUE_OUTSIDE_LOOP",
  },
  {
    "caseId": "ContinueNotEnd",
    "source": "while 1 = 1\ncontinue notEnd",
    "expected": "SYN_EXPECT_LINE_END",
  },
  {
    "caseId": "EndNotMatch1",
    "source": "while 1 = 1\nend if",
    "expected": "SYN_END_KEYWORD_NOT_MATCH",
  },
  {
    "caseId": "EndNotMatch2",
    "source": "if 1 = 1\nelse\nend while",
    "expected": "SYN_END_KEYWORD_NOT_MATCH",
  },
  {
    "caseId": "EndNotMatch3",
    "source": "func funcDecl()\nend if",
    "expected": "SYN_END_KEYWORD_NOT_MATCH",
  },
  {
    "caseId": "EndNotEnd",
    "source": "while 1 = 1\nend while /",
    "expected": "SYN_EXPECT_LINE_END",
  },
  {
    "caseId": "ExitInvalidValue",
    "source": "exit 1=",
    "expected": "SYN_EXPR_INVALID"
  },
  {
    "caseId": "ExitNotEnd",
    "source": "exit 1 notEnd",
    "expected": "SYN_EXPECT_LINE_END",
  },
  {
    "caseId": "ReturnOutsideFunction",
    "source": "if 1=1\n  return",
    "expected": "SYN_RETURN_OUTSIDE_FUNC",
  },
  {
    "caseId": "ReturnInvalidValue",
    "source": "func testReturn()\n  return 1=",
    "expected": "SYN_EXPR_INVALID",
  },
  {
    "caseId": "ReturnNotEnd",
    "source": "func testReturn()\n  return 2 notEnd",
    "expected": "SYN_EXPECT_LINE_END",
  },
  {
    "caseId": "GotoMissingLabel",
    "source": "goto/",
    "expected": "SYN_GOTO_MISSING_LABEL",
  },
  {
    "caseId": "GotoNotEnd",
    "source": "goto label/",
    "expected": "SYN_EXPECT_LINE_END",
  },
  {
    "caseId": "DimMissingVariable",
    "source": "dim 3",
    "expected": "SYN_DIM_MISSING_VARIABLE",
  },
  {
    "caseId": "DimMissingInitializer",
    "source": "dim a=",
    "expected": "SYN_EXPR_INVALID",
  },
  {
    "caseId": "DimWithInitializerInvalidExpr",
    "source": "dim a=1+",
    "expected": "SYN_EXPR_INVALID",
  },
  {
    "caseId": "DimWithInitializerNotEnd",
    "source": "dim a=1+2 notEnd",
    "expected": "SYN_EXPECT_LINE_END",
  },
  {
    "caseId": "DimArrayInvalidDimension",
    "source": "dim a[",
    "expected": "SYN_EXPR_INVALID",
  },
  {
    "caseId": "DimArrayMissingRightBracket",
    "source": "dim a[10",
    "expected": "SYN_DIM_ARRAY_MISSING_BRACKET_R",
  },
  {
    "caseId": "DimArrayNotEnd",
    "source": "dim a[10]+",
    "expected": "SYN_EXPECT_LINE_END",
  },
  {
    "caseId": "RedimMissingVar",
    "source": "redim [",
    "expected": "SYN_REDIM_MISSING_VARIABLE",
  },
  {
    "caseId": "RedimMissingLeftBracket",
    "source": "redim a",
    "expected": "SYN_REDIM_MISSING_BRACKET_L",
  },
  {
    "caseId": "RedimMissingInvalidDimension",
    "source": "redim a[1+",
    "expected": "SYN_EXPR_INVALID",
  },
  {
    "caseId": "RedimMissingRightBracket",
    "source": "redim a[1+2",
    "expected": "SYN_REDIM_MISSING_BRACKET_R",
  },
  {
    "caseId": "RedimNotEnd",
    "source": "redim a[1+2]+",
    "expected": "SYN_EXPECT_LINE_END",
  },
  {
    "caseId": "LabelNotEnd",
    "source": "labelName:/",
    "expected": "SYN_EXPECT_LINE_END",
  },
  {
    "caseId": "AssignInvalidExpr",
    "source": "var =",
    "expected": "SYN_EXPR_INVALID"},
  {
    "caseId": "AssignInvalidExpr",
    "source": "var =",
    "expected": "SYN_EXPR_INVALID"
  },
  {
    "caseId": "AssignNotEnd",
    "source": "var = 1 + 2 notEnd",
    "expected": "SYN_EXPECT_LINE_END",
  },
  {
    "caseId": "AssignArrayInvalidExpr",
    "source": "arr[1] =",
    "expected": "SYN_EXPR_INVALID",
  },
  {
    "caseId": "AssignArrayNotEnd",
    "source": "arr[1] = 2 notEnd",
    "expected": "SYN_EXPECT_LINE_END",
  },
  {
    "caseId": "ExprFuncCallMissingComma",
    "source": "call(1+2 2",
    "expected": "SYN_EXPR_INVALID",
  },
  {
    "caseId": "ExprArrayAccessMissingRightBracket",
    "source": "arr[1",
    "expected": "SYN_EXPR_INVALID",
  },
  {
    "caseId": "ExprInvalidOperator",
    "source": "1 ! 2",
    "expected": "SYN_EXPR_INVALID",
  },
  {
    "caseId": "ExprMissingOperand1",
    "source": "1 +",
    "expected": "SYN_EXPR_INVALID"
  },
  {
    "caseId": "ExprMissingRightParen",
    "source": "(1",
    "expected": "SYN_EXPR_INVALID"
  },
  {
    "caseId": "ExprInvalidOperand1",
    "source": "/",
    "expected": "SYN_EXPR_INVALID"
  },
  {
    "caseId": "ExprInvalidOperand2",
    "source": ")",
    "expected": "SYN_EXPR_INVALID"
  },
]

# 抽象语法树测试用例
AstTestCases = [
  {
    "caseId": "AstIfEmpty",
    "source": "if 1\nend if",
    "expected": {
      "astType": "Program",
      "numOfControl": 1,
      "statements": [
        {
          "astType": "If",
          "lineNumber": 1,
          "controlId": 1,
          "condition": {
            "astType": "LiteralNumeric",
            "value": 1
          },
          "then": {
            "astType": "Then",
            "statements": []
          },
          "elseif": [],
          "else": None
        }
      ]
    }
  },
  {
    "caseId": "AstIfElseIf",
    "source": "if 0\nelseif 1\nend if",
    "expected": {
      "astType": "Program",
      "numOfControl": 1,
      "statements": [
        {
          "astType": "If",
          "lineNumber": 1,
          "controlId": 1,
          "condition": {
            "astType": "LiteralNumeric",
            "value": 0
          },
          "then": {
            "astType": "Then",
            "statements": []
          },
          "elseif": [
            {
              "astType": "ElseIf",
              "lineNumber": 2,
              "condition": {
                "astType": "LiteralNumeric",
                "value": 1
              },
              "statements": []
            }
          ],
          "else": None
        }
      ]
    }
  },
  {
    "caseId": "AstIfElse",
    "source": "if 0\nelse\nend if",
    "expected": {
      "astType": "Program",
      "numOfControl": 1,
      "statements": [
        {
          "astType": "If",
          "lineNumber": 1,
          "controlId": 1,
          "condition": {
            "astType": "LiteralNumeric",
            "value": 0
          },
          "then": {
            "astType": "Then",
            "statements": []
          },
          "elseif": [
          ],
          "else": {
            "astType": "Else",
            "lineNumber": 2,
            "statements": []
          }
        }
      ]
    }
  },
  {
    "caseId": "AstIfElseIfElse",
    "source": "if 0\nelseif 1\nelseif 2\nelse\nend if",
    "expected": {
      "astType": "Program",
      "numOfControl": 1,
      "statements": [
        {
          "astType": "If",
          "lineNumber": 1,
          "controlId": 1,
          "condition": {
            "astType": "LiteralNumeric",
            "value": 0
          },
          "then": {
            "astType": "Then",
            "statements": []
          },
          "elseif": [
            {
              "astType": "ElseIf",
              "lineNumber": 2,
              "condition": {
                "astType": "LiteralNumeric",
                "value": 1
              },
              "statements": []
            },
            {
              "astType": "ElseIf",
              "lineNumber": 3,
              "condition": {
                "astType": "LiteralNumeric",
                "value": 2
              },
              "statements": []
            }
          ],
          "else": {
            "astType": "Else",
            "lineNumber": 4,
            "statements": []
          }
        }
      ]
    } 
  },
  {
    "caseId": "AstWhile",
    "source": "while 1\nend while",
    "expected": {
      "astType": "Program",
      "numOfControl": 1,
      "statements": [
        {
          "astType": "While",
          "lineNumber": 1,
          "controlId": 1,
          "condition": {
            "astType": "LiteralNumeric",
            "value": 1
          },
          "statements": []
        }
      ]
    }
  },
  {
    "caseId": "AstDoWhile",
    "source": "do\nwhile 1",
    "expected": {
      "astType": "Program",
      "numOfControl": 1,
      "statements": [
        {
          "astType": "DoWhile",
          "lineNumber": 1,
          "controlId": 1,
          "statements": [],
          "condition": {
            "astType": "LiteralNumeric",
            "value": 1
          }
        }
      ]
    }
  },
  {
    "caseId": "AstFor",
    "source": "for i=1 to 10\nnext i",
    "expected": {
      "astType": "Program",
      "numOfControl": 1,
      "statements": [
        {
          "astType": "For",
          "lineNumber": 1,
          "controlId": 1,
          "variable": "i",
          "rangeFrom": {
            "astType": "LiteralNumeric",
            "value": 1
          },
          "rangeTo": {
            "astType": "LiteralNumeric",
            "value": 10
          },
          "statements": []
        }
      ]
    }
  },
  {
    "caseId": "AstFuncDeclare",
    "source": "func declare(a[],b,c)\n  dim e = 0\n  p(e)\nend func",
    "expected": {
      "astType": "Program",
      "numOfControl": 1,
      "statements": [
        {
          "astType": "FunctionDeclare",
          "lineNumber": 1,
          "controlId": 1,
          "name": "declare",
          "parameters": [
            {
              "name": "a",
              "type": "ARRAY"
            },
            {
              "name": "b",
              "type": "PRIMITIVE"
            },
            {
              "name": "c",
              "type": "PRIMITIVE"
            }
          ],
          "statements": [
            {
              "astType": "Dim",
              "lineNumber": 2,
              "variable": "e",
              "initializer": {
                "astType": "LiteralNumeric",
                "value": 0
              }
            },
            {
              "astType": "FunctionCall",
              "lineNumber": 3,
              "name": "p",
              "arguments": [
                {
                  "astType": "Variable",
                  "variable": "e"
                }
              ]
            }
          ]
        }
      ]
    }
  },
]

# 语义错误测试用例
SemanticTestCases = [
  {
    "caseId": "ExprVarNotFound",
    "source": "1=a",
    "expected": "SEM_VAR_NOT_FOUND",
  },
  {
    "caseId": "ExprArrayNotFound",
    "source": "1=arr[0]",
    "expected": "SEM_VAR_NOT_FOUND",
  },
  {
    "caseId": "ExprVarIsNotArray",
    "source": "dim a = 0\na[1]=2",
    "expected": "SEM_VAR_IS_NOT_ARRAY",
  },
  {
    "caseId": "ExprFuncArgListMismatch1",
    "source": "p(1,2,3,4)",
    "expected": "SEM_FUNC_ARG_LIST_MISMATCH",
  },
  {
    "caseId": "ExprFuncNotFound",
    "source": "p_(1)",
    "expected": "SEM_FUNC_NOT_FOUND",
  },
  {
    "caseId": "ExprFuncArgListMismatch2",
    "source": "func callMe(arg1, arg2)\nend func\ncallMe(2)",
    "expected": "SEM_FUNC_ARG_LIST_MISMATCH",
  },
  {
    "caseId": "ExprFuncArgListMismatch2",
    "source": "func callMe(arg1, arg2)\nend func\ncallMe(2)",
    "expected": "SEM_FUNC_ARG_LIST_MISMATCH",
  },
  {
    "caseId": "FuncDeclParamTooLong",
    "source": "func _(_1234567890abcde)\nend func",
    "expected": "SEM_VAR_NAME_TOO_LONG",
  },
  {
    "caseId": "FuncDeclParamDuplicated",
    "source": "func _(a1,a1)\nend func",
    "expected": "SEM_VAR_DUPLICATED",
  },
  {
    "caseId": "GotoLabelNotFound",
    "source": "goto undefinedLabel",
    "expected": "SEM_GOTO_LABEL_NOT_FOUND",
  },
  {
    "caseId": "GotoCrossScope",
    "source": "goto insideFunc\nfunc _()\n  insideFunc:\nend func",
    "expected": "SEM_GOTO_LABEL_SCOPE_MISMATCH",
  },
  {
    "caseId": "ForVarNotFound",
    "source": "for i=0 to 10\nnext i",
    "expected": "SEM_VAR_NOT_FOUND",
  },
  {
    "caseId": "ForVarIsNotPrimitive",
    "source": "dim arr[10]\nfor arr=0 to 10\nnext arr",
    "expected": "SEM_VAR_IS_NOT_PRIMITIVE",
  },
  {
    "caseId": "DimVarTooLong",
    "source": "dim _1234567890abcde",
    "expected": "SEM_VAR_NAME_TOO_LONG",
  },
  {
    "caseId": "DimVarDuplicated",
    "source": "dim a1\ndim a1",
    "expected": "SEM_VAR_DUPLICATED",
  },
  {
    "caseId": "DimArrayTooLong",
    "source": "dim _1234567890abcde[10]",
    "expected": "SEM_VAR_NAME_TOO_LONG",
  },
  {
    "caseId": "DimArrayDuplicated",
    "source": "dim a1\ndim a1[10]",
    "expected": "SEM_VAR_DUPLICATED",
  },
  {
    "caseId": "RedimNotFound",
    "source": "redim nf[100]",
    "expected": "SEM_VAR_NOT_FOUND",
  },
  {
    "caseId": "RedimNotArray",
    "source": "dim a = 0;redim a[100]",
    "expected": "SEM_VAR_IS_NOT_ARRAY",
  },
  {
    "caseId": "AssignNotFound",
    "source": "a = 0",
    "expected": "SEM_VAR_NOT_FOUND",
  },
  {
    "caseId": "AssignNotPrimitive",
    "source": "dim a[10]\na = 0",
    "expected": "SEM_VAR_IS_NOT_PRIMITIVE",
  },
  {
    "caseId": "AssignArrayNotFound",
    "source": "a[1] = 0",
    "expected": "SEM_VAR_NOT_FOUND",
  },
  {
    "caseId": "AssignArrayNotArray",
    "source": "dim a\na[1] = 0",
    "expected": "SEM_VAR_IS_NOT_ARRAY",
  },
  {
    "caseId": "LabelTooLong",
    "source": "_1234567890abcde:",
    "expected": "SEM_LABEL_NAME_TOO_LONG",
  },
  {
    "caseId": "LabelDuplicated",
    "source": "a:\na:",
    "expected": "SEM_LABEL_DUPLICATED",
  },
]

# 运行时错误测试用例
RuntimeTestCases = [
  {
    "caseId": "TypeMismatch1",
    "source": "1+\"2\"",
    "expected": "RUNTIME_TYPE_MISMATCH",
  },
  {
    "caseId": "DivisionByZero1",
    "source": "1/0",
    "expected": "RUNTIME_DIVISION_BY_ZERO",
  },
  {
    "caseId": "DivisionByZero2",
    "source": "1\\0",
    "expected": "RUNTIME_DIVISION_BY_ZERO",
  },
  {
    "caseId": "ArrayInvalidSize",
    "source": "dim a[-1]",
    "expected": "RUNTIME_ARRAY_INVALID_SIZE",
  },
  {
    "caseId": "ArrayOutOfBounds1",
    "source": "dim a[5]\na[10]=0",
    "expected": "RUNTIME_ARRAY_OUT_OF_BOUNDS",
  },
  {
    "caseId": "ArrayOutOfBounds2",
    "source": "dim a[5]\na[-1]",
    "expected": "RUNTIME_ARRAY_OUT_OF_BOUNDS",
  },
  {
    "caseId": "TypeMismatch2",
    "source": "len(1)",
    "expected": "RUNTIME_TYPE_MISMATCH",
  },
  {
    "caseId": "TypeMismatch3",
    "source": "val(1)",
    "expected": "RUNTIME_TYPE_MISMATCH",
  },
  {
    "caseId": "TypeMismatch4",
    "source": "asc(0)",
    "expected": "RUNTIME_TYPE_MISMATCH",
  },
  {
    "caseId": "TypeMismatch5",
    "source": "chr(\"A\")",
    "expected": "RUNTIME_TYPE_MISMATCH",
  },
  {
     "caseId": "NotArray1",
     "source": "func assignArrEl(a[])\n  a[0]=1\nend func\nassignArrEl(0)",
     "expected": "RUNTIME_NOT_ARRAY",
  },
  {
     "caseId": "NotArray2",
     "source": "func accessArrEl(a[])\n  p(a[0])\nend func\naccessArrEl(0)",
     "expected": "RUNTIME_NOT_ARRAY",
  },
]

# 运算值测试用例
SourceFloatRelEqual = """
dim result = floatRelEqual()
func floatRelEqual()
  dim a = 1.0000001
  dim b = 1.0000002
  return a ~= b
end func
"""

SourceFactorial = """
dim result = factorial(5)
func factorial(n)
  if n = 0
    return 1
  else
    return n * factorial(n-1)
  end if
end func
"""

SourceFibonacci = """
dim result = fib(10)
func fib(n)
  if n <= 0
    return 0
  elseif n = 1
    return 1
  else
    return fib(n - 1) + fib(n - 2)
  end if
end func
"""

SourceMutualPrime = """	
dim result = gcd(28463, 79867)
func gcd(a, b)
  dim temp = 0
  while b <> 0
    temp = b
    b = a % b
    a = temp
  end while
  return a
end func
"""

SourceFastPower = """
dim result = fastPow(2,10)
func fastPow(x, n)
  dim half
  if n = 0
    return 1
  elseif n % 2 = 0
    half = fastPow(x, n / 2)
    return half * half
  else
    return x * fastPow(x, n - 1)
  end if
end func
"""

SourceHanoi = """
dim actions = hanoi(3, "A", "C", "B")
func hanoi(n, s, t, a)
  if n > 0
    dim move = s & "->" & t & ";"
    return hanoi(n - 1, s, a, t) & move & hanoi(n - 1, a, t, s)
  end if
  return ""
end func
"""

SourceStringFunctions = """
dim checkResult = checkLen() && checkVal() && checkAsc()
func checkLen()
  return len("hello world") ~= 11
end func
func checkVal()
  return val("123.456") ~= 123.456
end func
func checkAsc()
  return asc("ASCII") ~= 65
end func
"""

SourceBubbleSort = """
dim result = ""
func bubbleSort(a[])
  dim changeFlag
  dim i
  dim size = len(a)
  do
    changeFlag = 0
    for i = 0 to size - 2
      if a[i] < a[i + 1]
        dim temp = a[i]
        a[i] = a[i + 1]
        a[i + 1] = temp
        changeFlag = 1
      end if
    next i
  while changeFlag
end func
dim array[5]
dim i
for i = 0 to len(array) - 1
  array[i] = (i + 1) * 10;
next i
bubbleSort(array)
for i = 0 to len(array) - 1
  result = result & array[i]
next i
"""

ValueTestCases = [
  {
    "caseId": "FloatRelEqual",
    "source": SourceFloatRelEqual,
    "expected": {
      "type": "number",
      "stringified": "1"
    }
  },
  {
    "caseId": "Factorial",
    "source": SourceFactorial,
    "expected": {
      "type": "number",
      "stringified": "120"
    }
  },
  {
    "caseId": "Fibonacci",
    "source": SourceFibonacci,
    "expected": {
      "type": "number",
      "stringified": "55"
    }
  },
  {
    "caseId": "MutualPrime",
    "source": SourceMutualPrime,
    "expected": {
      "type": "number",
      "stringified": "1"
    }
  },
  {
    "caseId": "FastPower",
    "source": SourceFastPower,
    "expected": {
      "type": "number",
      "stringified": "1024"
    }
  },
  {
    "caseId": "Hanoi",
    "source": SourceHanoi,
    "expected": {
      "type": "string",
      "stringified": "A->C;A->B;C->B;A->C;B->A;B->C;A->C;"
    }
  },
  {
    "caseId": "StringFunctions",
    "source": SourceStringFunctions,
    "expected": {
      "type": "number",
      "stringified": "1"
    }
  },
  {
    "caseId": "BubbleSort",
    "source": SourceBubbleSort,
    "expected": {
      "type": "string",
      "stringified": "5040302010"
    }
  },
]

# 测试结果合集
testResults = []
numCases = 0
numPassed = 0

def runErrorCheckingCase(cases):
  global numCases
  global numPassed
  for testCase in cases:
    # 进行测试
    result = subprocess.run(
        [TestProgram, "check", testCase["source"]], capture_output=True, text=True
    )
    # 解析获得的 JSON 格式的命令行输出
    output = json.loads(result.stdout)
    # 是否通过测试
    isPassed = output["error"] and output["errorId"] == testCase["expected"]
    numCases = numCases + 1
    if isPassed:
        numPassed = numPassed + 1
    # 输出结果到命令行
    print(("PASSED" if isPassed else "FAILED") + " - " + testCase["caseId"])
    # 测试结果添加到合集
    testResults.append(
      {
        "caseId": testCase["caseId"],
        "source": testCase["source"],
        "passed": isPassed,
        "expected": testCase["expected"],
        "actualGot": output["errorId"]
      }
    )

def runAstCheckingCase(cases):
  global numCases
  global numPassed
  for testCase in cases:
    # 进行测试
    result = subprocess.run(
        [TestProgram, "ast", testCase["source"]], capture_output=True, text=True
    )
    # 解析获得的 JSON 格式的命令行 AST 输出
    output = json.loads(result.stdout)
    # 是否通过测试
    isPassed = output == testCase["expected"]
    numCases = numCases + 1
    if isPassed:
        numPassed = numPassed + 1
    # 输出结果到命令行
    print(("PASSED" if isPassed else "FAILED") + " - " + testCase["caseId"])
    # 测试结果添加到合集
    testResults.append(
      {
        "caseId": testCase["caseId"],
        "source": testCase["source"],
        "passed": isPassed,
        "expected": {
          "isJson": True,
          "content": json.dumps(testCase["expected"], indent=2)
        },
        "actualGot": {
          "isJson": True,
          "content": json.dumps(output, indent=2)
        }
      }
    )

def runValueCheckingCase(cases):
  global numCases
  global numPassed
  for testCase in cases:
    # 进行测试
    result = subprocess.run(
        [TestProgram, "check", testCase["source"]], capture_output=True, text=True
    )
    # 解析获得的 JSON 格式的命令行输出
    output = json.loads(result.stdout)
    # 是否通过测试
    isPassed = output["target"] and output["target"] == testCase["expected"]
    numCases = numCases + 1
    if isPassed:
        numPassed = numPassed + 1
    # 输出结果到命令行
    print(("PASSED" if isPassed else "FAILED") + " - " + testCase["caseId"])
    # 测试结果添加到合集
    testResults.append(
      {
        "caseId": testCase["caseId"],
        "source": testCase["source"],
        "passed": isPassed,
        "expected": testCase["expected"],
        "actualGot": output["target"]
      }
    )

runErrorCheckingCase(SyntaxTestCases)
runAstCheckingCase(AstTestCases)
runErrorCheckingCase(SemanticTestCases)
runErrorCheckingCase(RuntimeTestCases)
runValueCheckingCase(ValueTestCases)

htmlTemplate = """
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <title> KBasic Test Report </title>
  <style>
    body, html { padding: 0; margin: 0; }
    .container { width: 100%; max-width: 1200px; margin: 20px auto; font-size: 12px; }
    table { border-collapse: collapse; width: 100%; }
    h1, h2, h3, h4, h5, h6 { color: #1c8080; }
    td { padding: 2px 4px; text-align: left; }
    tr { border: 1px solid rgba(0, 0, 0, 0.1); }
    th { padding: 8px 2px; background: linear-gradient(0deg, #1c8080, #2b6060); color: white; text-align: center; }
    .pass-badge { transform: scale(0.8); display: inline-block; color: white; padding: 4px 8px; border-radius: 4px; line-height: 12px; }
    .pass { background-color: green; }
    .fail { background-color: red; }
    .col-id { text-align: center; }
    .col-pass-status { text-align: center; }
    .col-source { min-width: 120px; }
    pre, code {
      font-family: ui-monospace,      /* macOS / iOS modern system monospace keyword */
        SFMono-Regular,  /* macOS SF Mono */
        Menlo, Monaco,   /* macOS classics */
        Consolas,        /* Windows Consolas */
      "Segoe UI Mono", /* Windows monospace */
      "Liberation Mono", "Ubuntu Mono", /* Linux common monospace */
      "Courier New", monospace; /* fallback */
      font-size: 12px;
      line-height: 1.4;
      white-space: pre-wrap;
      word-break: break-word;
      color: #666;
    }
    .report-footer { max-width: 400px; margin: 0px auto; text-align: center; margin-top: 20px; }
    .report-footer h4 { margin-bottom: 8px; padding-bottom: 8px; border-bottom: 1px solid #ddd; }
    .report-footer p { margin: 0; }
    .type-badge { text-transform: capitalize; transform: scale(0.8);font-size: 12px; line-height: 1; padding: 4px 8px; border-radius: 2px; color: white; background: #999; margin-right: 4px; display: inline-block; }
  </style>
</head>
<body>
  <div class="container">
    <h1> KBasic Interpreter Test Report </h1>
    <p> Generated on: {{date}} </p>
    <h2> Cases </h2>
    <table>
      <tr>
        <th>Case ID</th>
        <th>Status</th>
        <th>Source</th>
        <th>Expected</th>
        <th>Actual</th>
      </tr>
      {% for case in results %}
        <tr>
          <td class="col-case-id">{{ case.caseId }}</td>
            <td class="col-pass-status">
              <span class="pass-badge {{ 'pass' if case.passed else 'fail' }}">
                {{ "PASSED" if case.passed else "FAILED" }}
              </span>
            </td>
            <td class="col-source"><pre>{{ case.source }}</pre></td>
            <td class="col-expected">
              {% if case.expected is string %}
                {{ case.expected }}
              {% elif case.expected.isJson %}
                <pre> {{ case.expected.content }} </pre>
              {% else %}
                <span class="type-badge">{{ case.expected.type }} </span>
                {{ case.expected.stringified }}
              {% endif %}
            </td>
            <td class="col-actual-got">
              {% if case.actualGot is string %}
                {{ case.actualGot }}
              {% elif case.actualGot.isJson %}
                <pre> {{ case.actualGot.content }} </pre>
              {% else %}
                <span class="type-badge">{{ case.actualGot.type }} </span>
                {{ case.actualGot.stringified }}
              {% endif %}
            </td>
          </tr>
        {% endfor %}
      </table>
      <div class="report-footer">
        <h4> KBASIC INTERPRETER </h4>
        <p> {{date}} </p>
      </div>
  </div>
</body>
</html>
"""

# 设置 Jinja2 环境
jinjaEnv = Environment(loader=FileSystemLoader("."), autoescape=select_autoescape())

# 从字符串加载模板
template = jinjaEnv.from_string(htmlTemplate)

# 渲染
htmlOutput = template.render(results=testResults, date=date.today())

# 输出到文件
with open(TestReportFile, "w", encoding="utf-8") as f:
    f.write(htmlOutput)

print(
    "Report generated: {file} ({passed}/{total}).".format(
        file=TestReportFile, passed=numPassed, total=numCases
    )
)
