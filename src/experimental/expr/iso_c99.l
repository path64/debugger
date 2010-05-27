
%{

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "expr_node.h"
#include "expr_c99.h"

int yyc99input(const char*, int);
#undef YY_INPUT
#define YY_INPUT(b, r, ms)  (r = yyc99input(b, ms))

#define YYSTYPE ex_node_t*
#include "yyc99.tab.h"

static int new_ident(const char*);
int is_type(const char*);

#define BUFSIZE 16384
#define ALLOC_TRIM(SRC,DEST,NUM)     \
   char DEST[BUFSIZE];               \
   long len = strlen(SRC);           \
   strncpy(DEST, (SRC), len-(NUM));  \
   (DEST)[len-(NUM)] = '\0';
%}

%option never-interactive

WS      [ \t]*
IDENT   [a-zA-Z_][a-zA-Z0-9_]*

F_MAN1  [0-9]+
F_MAN2  [0-9]*\.[0-9]+
F_MAN3  [0-9]+\.[0-9]*
F_EXP   [eE][-+]?[0-9]+
F_FUL1  ({F_MAN1})({F_EXP})
F_FUL2  ({F_MAN2})({F_EXP})?
F_FUL3  ({F_MAN3})({F_EXP})?
FLOAT   ({F_FUL1}|{F_FUL2}|{F_FUL3})

OCTAL   [0][0-7]+
HEXAD   [0][xX][0-9a-fA-F]+
DECIM   [0-9]+

%%

[ \t]+ /* ignore whitespace */;

void     { return VOID; }
int      { return INT; }
char     { return CHAR; }

bool     { return BOOL; }
short    { return SHORT; }
long     { return LONG; }
float    { return FLOAT; }
double   { return DOUBLE; }
complex  { return COMPLEX; }
signed   { return SIGNED; }
unsigned { return UNSIGNED; }

_Bool    { return BOOL; }
_Complex { return COMPLEX; }

const    { return CONST; }
volatile { return VOLATILE; }
restrict { return RESTRICT; }

sizeof   { return SIZEOF; }
__typeof { return TYPEOF; }
typeof   { return TYPEOF; }

struct   { return STRUCT; }
union    { return UNION; }
class    { return CLASS; }
enum     { return ENUM; }

__extension__ { return EXTN; }

dynamic_cast { return DCAST; }
static_cast { return SCAST; }
const_cast { return CCAST; }
reinterpret_cast { return ICAST; }

true  { 
    yyc99lval = ex_new_bool(1);
    return K_BOOL;
}

false {
    yyc99lval = ex_new_bool(0);
    return K_BOOL;
}

["]([^"\n]|[\\]["])*["] {
   ALLOC_TRIM(yytext+1, val, 1)
   yyc99lval = ex_c99_new_str(val);
   return K_STR;
   }

[']([^'\n]|[\\]['])+['] {
   ALLOC_TRIM(yytext+1, val, 1)
   yyc99lval = ex_c99_new_char(val);
   return K_CHAR;
   }

{FLOAT}[fF] {
   ALLOC_TRIM(yytext, val, 1)
   yyc99lval = ex_parse_float(val);
   return K_FLOAT;
   }

{FLOAT}[lL] {
   ALLOC_TRIM(yytext, val, 1)
   yyc99lval = ex_parse_ldoubl(val);
   return K_LDOUBL;
   }

{FLOAT} {
   yyc99lval = ex_parse_doubl(yytext);
   return K_DOUBL;
   }

{OCTAL}[uU][lL][lL] {
   ALLOC_TRIM(yytext, val, 3)
   yyc99lval = ex_parse_llong(val,8,0);
   return K_ULLONG;
   }

{OCTAL}[uU][lL] {
   ALLOC_TRIM(yytext, val, 2)
   yyc99lval = ex_parse_long(val,8,0);
   return K_ULONG;
   }

{OCTAL}[uU] {
   ALLOC_TRIM(yytext, val, 1)
   yyc99lval = ex_parse_int(val,8,0);
   return K_UINT;
   }

{OCTAL}[lL][lL] {
   ALLOC_TRIM(yytext, val, 2)
   yyc99lval = ex_parse_llong(val,8,1);
   return K_LLONG;
   }

{OCTAL}[lL] {
   ALLOC_TRIM(yytext, val, 1)
   yyc99lval = ex_parse_long(val,8,1);
   return K_LONG;
   }

{OCTAL} {
   yyc99lval = ex_parse_int(yytext,8,1);
   return K_INT;
   }

{HEXAD}[uU][lL][lL] {
   ALLOC_TRIM(yytext, val, 3)
   yyc99lval = ex_parse_llong(val,16,0);
   return K_ULLONG;
   }

{HEXAD}[uU][lL] {
   ALLOC_TRIM(yytext, val, 2)
   yyc99lval = ex_parse_long(val,16,0);
   return K_ULONG;
   }

{HEXAD}[uU] {
   ALLOC_TRIM(yytext, val, 1)
   yyc99lval = ex_parse_int(val,16,0);
   return K_UINT;
   }

{HEXAD}[lL][lL] {
   ALLOC_TRIM(yytext, val, 2)
   yyc99lval = ex_parse_llong(val,16,1);
   return K_LLONG;
   }

{HEXAD}[lL] {
   ALLOC_TRIM(yytext, val, 1)
   yyc99lval = ex_parse_long(val,16,1);
   return K_LONG;
   }

{HEXAD} {
   yyc99lval = ex_parse_int(yytext,16,1);
   return K_INT;
   }

{DECIM}[uU][lL][lL] {
   ALLOC_TRIM(yytext, val, 3)
   yyc99lval = ex_parse_llong(val,10,0);
   return K_ULLONG;
   }

{DECIM}[uU][lL] {
   ALLOC_TRIM(yytext, val, 2)
   yyc99lval = ex_parse_long(val,10,0);
   return K_ULONG;
   }

{DECIM}[uU] {
   ALLOC_TRIM(yytext, val, 1)
   yyc99lval = ex_parse_int(val,10,0);
   return K_UINT;
   }

{DECIM}[lL][lL] {
   ALLOC_TRIM(yytext, val, 2)
   yyc99lval = ex_parse_llong(val,10,1);
   return K_LLONG;
   }

{DECIM}[lL] {
   ALLOC_TRIM(yytext, val, 1)
   yyc99lval = ex_parse_long(val,10,1);
   return K_LONG;
   }

{DECIM} {
   yyc99lval = ex_parse_int(yytext,10,1);
   return K_INT;
   }

(::{WS})?{IDENT}({WS}::{WS}{IDENT})* {
   return new_ident(yytext);
   }

[.][.][.] { return ELLP; }
[<][<][=] { return ASHL; }
[>][>][=] { return ASHL; }
[-][>][*] { return PPTR; }

[.][*]  { return MPTR; }
[&][=]  { return AAND; }
[|][=]  { return AOOR; }
[\^][=] { return AXOR; }

[+][=]  { return AADD; }
[-][=]  { return ASUB; }
[*][=]  { return AMUL; }
[/][=]  { return ADIV; }
[%][=]  { return AMOD; }

[<][<]  { return BSHL; }
[>][>]  { return BSHR; }
[+][+]  { return INCR; }
[-][-]  { return DECR; }
[-][>]  { return ARRW; }
[<][=]  { return CMLE; }
[>][=]  { return CMGE; }
[!][=]  { return CNEQ; }
[=][=]  { return CEEQ; }
[&][&]  { return LAND; }
[|][|]  { return LLOR; }

[=]  { return AEQL; }
[.]  { return PDOT; }
[+]  { return PLUS; }
[-]  { return MINU; }
[/]  { return DIVI; }
[%]  { return MODL; }
[*]  { return MULT; }
[~]  { return TLDE; }
[!]  { return EXCL; }
[&]  { return AMPR; }
[|]  { return PIPE; }
[?]  { return QEST; }
[:]  { return COLN; }
[,]  { return COMA; }
[;]  { return SCLN; }
[{]  { return LCRL; }
[}]  { return RCRL; }
[(]  { return LPAR; }
[)]  { return RPAR; }
[<]  { return LANG; }
[>]  { return RANG; }
[\^] { return CART; }
[\[] { return LBRC; }
[\]] { return RBRC; }

\n   { return EXEC; }
.    { return yytext[0]; };

%%

static int new_ident(const char* s) {
   char *c, m[BUFSIZE];
   int i, length;

   length = strlen(s) + 1; 
   for (c=m,i=0; i<length; i++) {
      if (!isspace(s[i])) {
        *c++ = s[i];
      }
   }

   yyc99lval = ex_new_str(m, strlen(m));

   if (is_type(m)) {
      return TYPE;
   } else {
      return NAME;
   }
}

int is_type(const char* s) {
   /* XXX: needs real impl. */
   return (*s == 's');
}
