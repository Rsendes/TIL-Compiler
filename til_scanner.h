#pragma once

#undef yyFlexLexer
#define yyFlexLexer til_scanner_FlexLexer
#include <FlexLexer.h>

typedef til_scanner_FlexLexer til_scanner;
