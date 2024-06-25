%{
//-- don't change *any* of these: if you do, you'll break the compiler.
#include <algorithm>
#include <memory>
#include <cstring>
#include <cdk/compiler.h>
#include <cdk/types/types.h>
#include ".auto/all_nodes.h"
#define LINE                         compiler->scanner()->lineno()
#define yylex()                      compiler->scanner()->scan()
#define yyerror(compiler, s)         compiler->scanner()->error(s)
//-- don't change *any* of these --- END!
%}

%parse-param {std::shared_ptr<cdk::compiler> compiler}

%union {
  //--- don't change *any* of these: if you do, you'll break the compiler.
  YYSTYPE() : type(cdk::primitive_type::create(0, cdk::TYPE_VOID)) {}
  ~YYSTYPE() {}
  YYSTYPE(const YYSTYPE &other) { *this = other; }
  YYSTYPE& operator=(const YYSTYPE &other) { type = other.type; return *this; }

  std::shared_ptr<cdk::basic_type> type;        /* expression type */
  //-- don't change *any* of these --- END!

  int                   i;          /* integer value */
  double                d;
  std::string          *s;          /* symbol name or string literal */
  cdk::basic_node      *node;       /* node pointer */
  cdk::sequence_node   *sequence;
  cdk::expression_node *expression; /* expression nodes */
  cdk::lvalue_node     *lvalue;
  til::block_node      *block;
};

%token <i> tINTEGER
%token <s> tIDENTIFIER tSTRING tREC
%token <d> tDOUBLE
%token tWHILE tIF tPRINT tREAD tBEGIN tEND tPRINTLN tRETURN tLOOP tPROGRAM tBLOCK tNEXT tSTOP tSIZEOF
%token tTYPE_INT tTYPE_STRING tTYPE_VAR tTYPE_DOUBLE tTYPE_VOID tTYPE_POINTER tNULL
%token tEXTERNAL tFORWARD tPUBLIC tPRIVATE tFUNCTION
%token tOBJECTS tINDEX

%right tSET
%left tGE tLE tEQ tNE tAND tOR '>' '<' 
%left '+' '-'
%left '*' '/' '%'
%nonassoc tUNARY '~'

%type <node> stmt var_dec arg_dec program
%type <type> data_type 
%type <sequence> stmts
%type <sequence> exprs var_decs arg_decs
%type <expression> expr opt_initializer fundef
%type <lvalue> lval
%type <block> block

%{
//-- The rules below will be included in yyparse, the main parsing function.
%}
%%

file : var_decs program    { compiler->ast(new cdk::sequence_node(LINE, $2, $1)); }
     | var_decs           { compiler->ast($1); }
     |        program    { compiler->ast(new cdk::sequence_node(LINE, $1)); }
     | /* empty */       { compiler->ast(new cdk::sequence_node(LINE)); }
     ;

program : '(' tPROGRAM block ')' { $$ = new til::program_node(LINE, $3); }


fundef : '(' tFUNCTION '(' data_type arg_decs ')' block ')'  { $$ = new til::function_definition_node(LINE, tPRIVATE, $4, $5, $7); }
       | '(' tFUNCTION '(' '(' data_type ')' ')' block ')' { $$ = new til::function_definition_node(LINE, tPRIVATE, $5, nullptr, $8); }
       ;

arg_decs :                    { $$ = new cdk::sequence_node(LINE); }
         | arg_dec            { $$ = new cdk::sequence_node(LINE, $1); }
         | arg_decs arg_dec   { $$ = new cdk::sequence_node(LINE, $2, $1); }
         ;

arg_dec : '(' data_type tIDENTIFIER ')' { $$ = new til::variable_declaration_node(LINE, tPRIVATE, $2, *$3, nullptr); }
        | '(' '(' data_type ')' tIDENTIFIER ')' { $$ = new til::variable_declaration_node(LINE, tPRIVATE, $3, *$5, nullptr); }
        ;
         
var_dec : '('             data_type tIDENTIFIER                 ')'         { $$ = new til::variable_declaration_node(LINE, tPRIVATE, $2, *$3, nullptr); }
        | '(' tPUBLIC     data_type tIDENTIFIER                 ')'         { $$ = new til::variable_declaration_node(LINE, tPUBLIC, $3, *$4, nullptr);  }
        | '('             data_type tIDENTIFIER opt_initializer ')'         { $$ = new til::variable_declaration_node(LINE, tPRIVATE, $2, *$3, $4);      }
        | '(' tPUBLIC     data_type tIDENTIFIER opt_initializer ')'         { $$ = new til::variable_declaration_node(LINE, tPUBLIC, $3, *$4, $5);       }
        | '('             tTYPE_VAR tIDENTIFIER opt_initializer ')'         { $$ = new til::variable_declaration_node(LINE, tPRIVATE, cdk::primitive_type::create(0, cdk::TYPE_UNSPEC), *$3, $4); }
        | '(' tPUBLIC     tTYPE_VAR tIDENTIFIER opt_initializer ')'         { $$ = new til::variable_declaration_node(LINE, tPUBLIC, cdk::primitive_type::create(0, cdk::TYPE_UNSPEC), *$4, $5);  }
        | '(' tEXTERNAL  '(' data_type ')' tIDENTIFIER          ')'         { $$ = new til::variable_declaration_node(LINE, tEXTERNAL, $4 , *$6, nullptr); }
        | '(' tFORWARD   '(' data_type ')' tIDENTIFIER          ')'         { $$ = new til::variable_declaration_node(LINE, tFORWARD, $4 , *$6, nullptr); }
        | '('            '(' data_type ')' tIDENTIFIER opt_initializer')'   { $$ = new til::variable_declaration_node(LINE, tPRIVATE, $3 , *$5, $6); }
        | '(' tPUBLIC tIDENTIFIER expr ')'                                  { $$ = new til::variable_declaration_node(LINE, tPUBLIC, cdk::primitive_type::create(0, cdk::TYPE_VOID) , *$3, $4); }
        | '(' '(' data_type '(' '(' data_type')' data_type ')'  ')' tIDENTIFIER opt_initializer ')' { $$ = new til::variable_declaration_node(LINE, tPRIVATE, $3, *$11, $12); }
        ;

var_decs : var_dec             { $$ = new cdk::sequence_node(LINE, $1);     }
         | var_decs var_dec    { $$ = new cdk::sequence_node(LINE, $2, $1); }
         ;

data_type    : data_type tTYPE_POINTER          { $$ = cdk::reference_type::create(4, $1); } 
             | tTYPE_STRING                     { $$ = cdk::primitive_type::create(4, cdk::TYPE_STRING);  }
             | tTYPE_INT                        { $$ = cdk::primitive_type::create(4, cdk::TYPE_INT);     }
             | tTYPE_DOUBLE                     { $$ = cdk::primitive_type::create(8, cdk::TYPE_DOUBLE);  }
             | tTYPE_VOID                       { $$ = cdk::primitive_type::create(0, cdk::TYPE_VOID);    }
             | data_type '(' data_type ')'      { $$ = cdk::primitive_type::create(4, cdk::TYPE_INT); }
             ;

opt_initializer  : /* empty */                  { $$ = nullptr; /* must be nullptr, not NIL */ }
                 | expr                         { $$ = $1; }
                 ;
       
stmts : stmt                { $$ = new cdk::sequence_node(LINE, $1); }
      | stmts stmt          { $$ = new cdk::sequence_node(LINE, $2, $1); }
      ;

stmt :     expr                                        { $$ = new til::evaluation_node(LINE, $1); }
     | '(' tPRINTLN exprs ')'                          { $$ = new til::print_node(LINE, $3, true); }
     | '(' tPRINT exprs ')'                            { $$ = new til::print_node(LINE, $3, false); }
     | '(' tRETURN expr ')'                            { $$ = new til::return_node(LINE, $3); }
     | '(' tREAD ')'                                   { $$ = new til::read_node(LINE); }
     | '(' tLOOP '(' expr ')' '(' tBLOCK block ')' ')' { $$ = new til::while_node(LINE, $4, $8); }
     | '(' tIF expr stmt ')'                           { $$ = new til::if_node(LINE, $3, $4); }
     | '(' tIF expr stmt stmt ')'                      { $$ = new til::if_else_node(LINE, $3, $4, $5); }
     | '(' tSTOP ')'                                   { $$ = new til::stop_node(LINE); }             
     | '(' tSTOP tINTEGER ')'                          { $$ = new til::stop_node(LINE, $3); }
     | '(' tNEXT ')'                                   { $$ = new til::next_node(LINE); }
     | '(' tNEXT tINTEGER ')'                          { $$ = new til::next_node(LINE, $3); }
     | '(' tBLOCK stmts ')'                            { $$ = new til::block_node(LINE, $3, new cdk::sequence_node(LINE)); }
     ;

block :                       { $$ = new til::block_node(LINE, new cdk::sequence_node(LINE), new cdk::sequence_node(LINE)); }
      |  var_decs             { $$ = new til::block_node(LINE, $1, new cdk::sequence_node(LINE)); }
      |  stmts                { $$ = new til::block_node(LINE, new cdk::sequence_node(LINE), $1); }
      |  var_decs stmts       { $$ = new til::block_node(LINE, $1, $2); }
      ;

expr : tINTEGER              { $$ = new cdk::integer_node(LINE, $1); }
     | tSTRING               { $$ = new cdk::string_node(LINE, $1); delete $1;}
     | tDOUBLE               { $$ = new cdk::double_node(LINE, $1); }
     | tNULL                 { $$ = new til::null_node(LINE); }
     | '(' expr exprs ')'    { $$ = new til::function_call_node(LINE, $2, $3); }
     | '(' tREC exprs ')'    { $$ = new til::function_call_node(LINE, nullptr, $3); }
     | '(' tREC ')'          { $$ = new til::function_call_node(LINE, nullptr, new cdk::sequence_node(LINE)); }
     | '-' expr %prec tUNARY { $$ = new cdk::unary_minus_node(LINE, $2); }
     | '+' expr %prec tUNARY { $$ = new cdk::unary_plus_node(LINE, $2); }
     | '+' expr expr         { $$ = new cdk::add_node(LINE, $2, $3); }
     | '-' expr expr         { $$ = new cdk::sub_node(LINE, $2, $3); }
     | '*' expr expr         { $$ = new cdk::mul_node(LINE, $2, $3); }
     | '/' expr expr         { $$ = new cdk::div_node(LINE, $2, $3); }
     | '%' expr expr         { $$ = new cdk::mod_node(LINE, $2, $3); }
     | '<' expr expr         { $$ = new cdk::lt_node(LINE, $2, $3); }
     | '>' expr expr         { $$ = new cdk::gt_node(LINE, $2, $3); }
     | tGE expr expr         { $$ = new cdk::ge_node(LINE, $2, $3); }
     | tLE expr expr         { $$ = new cdk::le_node(LINE, $2, $3); }
     | tNE expr expr         { $$ = new cdk::ne_node(LINE, $2, $3); }
     | tEQ expr expr         { $$ = new cdk::eq_node(LINE, $2, $3); }
     | '~' expr              { $$ = new cdk::not_node(LINE, $2); }
     | tAND expr expr        { $$ = new cdk::and_node(LINE, $2, $3); }
     | tOR expr expr         { $$ = new cdk::or_node(LINE, $2, $3); }
     | '(' expr ')'          { $$ = $2; }
     | lval                  { $$ = new cdk::rvalue_node(LINE, $1); }
     | '(' tSET lval expr ')'{ $$ = new cdk::assignment_node(LINE, $3, $4); }
     | '(' tOBJECTS expr ')' { $$ = new til::memory_alloc_node(LINE, $3); }
     | tSIZEOF expr          { $$ = new til::sizeof_node(LINE, $2); }
     | '(' '?' lval ')'      { $$ = new til::memory_address_node(LINE, $3); }
     | fundef                { $$ = $1; }
     ;

exprs     : expr             { $$ = new cdk::sequence_node(LINE, $1); }
          | exprs expr       { $$ = new cdk::sequence_node(LINE, $2, $1); }
          ;

lval : tIDENTIFIER           { $$ = new cdk::variable_node(LINE, $1); }
     | '(' tINDEX expr expr ')' { $$ = new til::indexing_node(LINE, $3, $4); }
     ;

%%
