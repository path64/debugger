
%{

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "expr_node.h"
#include "expr_f90.h"

int yyf90input(const char*, int);
#define YY_INPUT(b, r, ms) ( r = yyf90input(b, ms) )

#define YYSTYPE ex_node_t*
#include "yyf90.tab.h"

#define BUFSIZE 16384
#define ALLOC_TRIM(SRC,DEST,NUM)     \
   char DEST[BUFSIZE];               \
   long len = strlen(SRC);           \
   strncpy(DEST, (SRC), len-(NUM));  \
   (DEST)[len-(NUM)] = '\0';

%}

%option never-interactive

WS      [ \t]*

IDENT   [a-zA-Z][a-zA-Z0-9_]*

SQSTR   [']([^'\n]|[']['])*['] 
DQSTR   ["]([^"\n]|["]["])*["] 

F_MAN1  [0-9]+
F_MAN2  [0-9]*\.[0-9]+
F_MAN3  [0-9]+\.[0-9]*

F_EXP   [eEdD][-+]?[0-9]+
F_FUL1  ({F_MAN1})({F_EXP})
F_FUL2  ({F_MAN2})({F_EXP})?
F_FUL3  ({F_MAN3})({F_EXP})?
FLOAT   ({F_FUL1}|{F_FUL2}|{F_FUL3})

BINAR   [bB]({SQSTR}|{DQSTR})
OCTAL   [oO]({SQSTR}|{DQSTR})
HEXAD   [zZ]({SQSTR}|{DQSTR})
DECIM   [0-9]+

%%

[ \t]+ /* ignore whitespace */;

{SQSTR} {
   ALLOC_TRIM(yytext+1, val, 1)
   yyf90lval = ex_f90_new_str(val, 1);
   return K_STR;
   }

{DQSTR} {
   ALLOC_TRIM(yytext+1, val, 1)
   yyf90lval = ex_f90_new_str(val, 0);
   return K_STR;
   }

{DECIM}/([.][a-zA-Z][.a-zA-Z]) {
   yyf90lval = ex_parse_long(yytext,10,1);
   return K_INTEG;
   }

{FLOAT}/[_] {
   yyf90lval = ex_f90_new_float(yytext,1);
   return K_FLOAT;
   }

{FLOAT} {
   yyf90lval = ex_f90_new_float(yytext,0);
   return K_FLOAT;
   }

{BINAR} {
   yyf90lval = ex_f90_new_integ(yytext,2,0);
   yyf90lval = ex_new_op1(EX_OP_KST_DATA, yyf90lval);
   return K_DATA;
   }

{OCTAL} {
   yyf90lval = ex_f90_new_integ(yytext,8,0);
   yyf90lval = ex_new_op1(EX_OP_KST_DATA, yyf90lval);
   return K_DATA;
   }

{HEXAD} {
   yyf90lval = ex_f90_new_integ(yytext,16,0);
   yyf90lval = ex_new_op1(EX_OP_KST_DATA, yyf90lval);
   return K_DATA;
   }

{DECIM}/[_] {
   yyf90lval = ex_parse_llong(yytext,10,0);
   return K_INTEG;
   }

{DECIM} {
   yyf90lval = ex_parse_long(yytext,10,0);
   return K_INTEG;
   }

[rR][eE][aA][lL] {
   return REAL;
   }

[iI][nN][tT][eE][gG][eE][rR] {
   return INTEGER;
   }

[cC][oO][mM][pP][lL][eE][xX] {
   return COMPLEX;
   }

[lL][oO][gG][iI][cC][aA][lL] {
   return LOGICAL;
   }

{IDENT}[_]/['"] {
   ALLOC_TRIM(yytext, val, 1)
   yyf90lval = ex_new_str(val,strlen(val));
   yyless(strlen(val));
   return NAME;
   }

{IDENT} {
   yyf90lval = ex_new_str(yytext,strlen(yytext));
   return NAME;
   }

[.][tT][rR][uU][eE][.] {
   yyf90lval = ex_new_bool(1);
   return K_BOOL;
   }

[.][fF][aA][lL][sS][eE][.] {
   yyf90lval = ex_new_bool(0);
   return K_BOOL;
   }

[.][nN][oO][tT][.] { return LNOT; }
[.][aA][nN][dD][.] { return LAND; }
[.][oO][rR][.]     { return LLOR; }

[.][nN][eE][qQ][vV][.] { return LNEQ; }
[.][eE][qQ][vV][.] { return LEQV; }

[.][eE][qQ][.] { return CEEQ; }
[.][nN][eE][.] { return CNEQ; }
[.][lL][tT][.] { return CMLT; }
[.][lL][eE][.] { return CMLE; }
[.][gG][tT][.] { return CMGT; }
[.][gG][eE][.] { return CMGT; }

[.][a-zA-Z]+[.] {
   ALLOC_TRIM(yytext+1, val, 1)
   yyf90lval = ex_new_str(val,strlen(val));
   return OPER;
   }

[-][>] { return INTO; }
[=][>] { return PTRE; }
[*][*] { return EXPP; }
[=][=] { return CEEQ; }
[/][=] { return CNEQ; }

[(][/] { return LARR; }
[/][)] { return RARR; }
[/][/] { return SCAT; }
[<][=] { return CMLE; }
[>][=] { return CMGE; }

[=]  { return AEQL; }
[+]  { return PLUS; }
[-]  { return MINU; }
[/]  { return DIVI; }
[*]  { return MULT; }
[,]  { return COMA; }
[:]  { return COLN; }
[%]  { return PERC; }
[&]  { return AMPR; }
[(]  { return LPAR; }
[)]  { return RPAR; }
[<]  { return CMLT; }
[>]  { return CMGT; }
[_]  { return UNDR; }

\n   { return EXEC; }
.    { return yytext[0]; };

%%

