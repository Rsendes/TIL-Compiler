%option c++ prefix="til_scanner_" outfile="til_scanner.cpp"
%option stack noyywrap yylineno 8bit debug
%{ 
// make relevant includes before including the parser's tab file
#include <cstring>
#include <vector>
#include <sstream>
#include <string>
#include <cdk/ast/sequence_node.h>
#include <cdk/ast/expression_node.h>
#include <cdk/ast/lvalue_node.h>
#include "til_parser.tab.h"

static std::ostringstream strlit;

// don't change this
#define yyerror LexerError
%}
%x X_STRING X_COMMENT X_HEX_INT
%%
  yydebug=1; set_debug(1);

";".*                 ; /* ignore comments */

"/*"                   yy_push_state(X_COMMENT);
<X_COMMENT>"*/"        yy_pop_state();
<X_COMMENT>.|\n        ;  /* to parse multiple lines */

"int"                  return tTYPE_INT;
"string"               return tTYPE_STRING;
"var"                  return tTYPE_VAR;
"double"               return tTYPE_DOUBLE;
"void"                 return tTYPE_VOID;
"!"                    return tTYPE_POINTER;  

">="                   return tGE; 
"<="                   return tLE; 
"=="                   return tEQ; 
"!="                   return tNE; 
"&&"                   return tAND;
"||"                   return tOR;


"next"                 return tNEXT;
"stop"                 return tSTOP;
"while"                return tWHILE; 
"if"                   return tIF;
"print"                return tPRINT;
"return"               return tRETURN;
"println"              return tPRINTLN;
"read"                 return tREAD;  
"external"             return tEXTERNAL;
"forward"              return tFORWARD;
"public"               return tPUBLIC;
"set"                  return tSET;
"loop"                 return tLOOP;
"block"                return tBLOCK;
"program"              return tPROGRAM;
"sizeof"               return tSIZEOF;
"function"             return tFUNCTION;
"@"                    return tREC;
"objects"              return tOBJECTS;
"index"                return tINDEX;
"null"                 return tNULL;



[A-Za-z][A-Za-z0-9_]*  yylval.s = new std::string(yytext); return tIDENTIFIER;

\"                     yy_push_state(X_STRING); yylval.s = new std::string("");
<X_STRING>\"           yy_pop_state(); return tSTRING;
<X_STRING>\\\'         *yylval.s += yytext + 1;
<X_STRING>.            *yylval.s += yytext;
<X_STRING>\\t          *yylval.s += '\t';
<X_STRING>\\n          *yylval.s += '\n';
<X_STRING>\\r          *yylval.s += '\r';
<X_STRING>\\[0-7]{1,3} {
                          int i = std::stoi(yytext + 1, nullptr, 8);
                          if (i > 255) yyerror("octal escape sequence out of range");
                          *yylval.s += (char) i;
                       }
<X_STRING>\n           yyerror("newline in string");

"0x"                   yy_push_state(X_HEX_INT);

<X_HEX_INT>[[:xdigit:]]+      { yylval.i = strtoul(yytext, NULL, 16); yy_pop_state(); return tINTEGER; }
<X_HEX_INT>.|\n               std::cerr << "WARNING: line " << lineno() << ": bad hexadecimal data!" << std::endl; exit(1);


[0-9]+                 yylval.i = strtol(yytext, nullptr, 10); return tINTEGER;



([0-9]*\.[0-9]+|[0-9]+\.[0-9]*)([eE][-+]?[0-9]+)? { yylval.d = strtod(yytext, NULL); return tDOUBLE; }

[-()<>=+*/%;{}.~?]       return *yytext;

[ \t\n]+               ; /* ignore whitespace */

.                      yyerror("Unknown character");

%%
