
%{

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "expr_node.h"
#include "expr_f90.h"

void yyf90error(const char* s);
int yyf90lex();

#define YYSTYPE ex_node_t*

#undef YY_INPUT
#define YY_INPUT (b, r, ms)  (r = yyf90input(b, ms))

%}

%token EXEC NAME TYPE AEQL PLUS SCAT MINU DIVI MULT
%token COLN COMA SCLN LPAR RPAR LARR RARR UNDR EXPP
%token OPER CNEQ CEEQ CMLT CMLE CMGT CMGE LNOT LAND
%token LLOR LEQV LNEQ PERC PTRE AMPR INTO

%token K_STR K_DATA K_BOOL K_INTEG K_FLOAT K_DOUBL 
%token REAL INTEGER COMPLEX LOGICAL

%%

exec :  e_assn EXEC                { ex_f90_chk_syntax($1); 
                                     printf("\nParsed to:\n");
                                     ex_dump($1,0);
                                     putchar('\n');
                                     exit(0); }

e_assn : e_dbinr
   | e_primary AEQL e_dbinr       { $$ = ex_new_op2(EX_OP_MOV_EQ, $1, $3); }
   | e_primary PTRE e_dbinr       { $$ = ex_new_op2(EX_OP_MOV_PTR, $1, $3); }
   ;

e_top : e_dbinr ;

e_dbinr : e_equiv
   | e_dbinr OPER e_equiv         { $$ = ex_new_op3(EX_OP_DEF_BNRY, $1, $3, $2); }
   ;

e_equiv : e_llor
   | e_equiv LEQV e_llor          { $$ = ex_new_op2(EX_OP_CMP_EQ, $1, $3); }
   | e_equiv LNEQ e_llor          { $$ = ex_new_op2(EX_OP_CMP_NE, $1, $3); }
   ; 

e_llor : e_land
   | e_llor LLOR e_land           { $$ = ex_new_op2(EX_OP_LOG_OR, $1, $3); }
   ;

e_land : e_hi_unary
   | e_land LAND e_hi_unary       { $$ = ex_new_op2(EX_OP_LOG_AND, $1, $3); }
   ;

e_hi_unary : e_comp
   | LNOT e_hi_unary              { $$ = ex_new_op1(EX_OP_LOG_NOT, $2); }
   ;

e_comp : e_scat
   | e_comp CEEQ e_ct_unary       { $$ = ex_new_op2(EX_OP_CMP_EQ, $1, $3); }
   | e_comp CNEQ e_ct_unary       { $$ = ex_new_op2(EX_OP_CMP_NE, $1, $3); }
   | e_comp CMLT e_ct_unary       { $$ = ex_new_op2(EX_OP_CMP_LT, $1, $3); }
   | e_comp CMLE e_ct_unary       { $$ = ex_new_op2(EX_OP_CMP_LE, $1, $3); }
   | e_comp CMGT e_ct_unary       { $$ = ex_new_op2(EX_OP_CMP_GT, $1, $3); }
   | e_comp CMGE e_ct_unary       { $$ = ex_new_op2(EX_OP_CMP_GE, $1, $3); }
   ;

e_ct_unary : e_scat
   | LNOT e_scat                  { $$ = ex_new_op1(EX_OP_LOG_NOT, $2); }
   ;

e_scat : e_addi
   | e_scat SCAT e_addi           { $$ = ex_new_op2(EX_OP_STR_CAT, $1, $3); }
   ;

e_addi : e_md_unary
   | e_addi PLUS e_md_unary       { $$ = ex_new_op2(EX_OP_ART_ADD, $1, $3); }
   | e_addi MINU e_md_unary       { $$ = ex_new_op2(EX_OP_ART_SUB, $1, $3); }
   ;

e_md_unary : e_mult
   | PLUS e_md_unary              { $$ = ex_new_op1(EX_OP_SGN_POS, $2); }
   | MINU e_md_unary              { $$ = ex_new_op1(EX_OP_SGN_NEG, $2); }
   | AMPR e_md_unary              { $$ = ex_new_op1(EX_OP_REF_ADDR, $2); }
   ;

e_mult : e_expon
   | e_mult MULT e_expon          { $$ = ex_new_op2(EX_OP_ART_MUL, $1, $3); }
   | e_mult DIVI e_expon          { $$ = ex_new_op2(EX_OP_ART_DIV, $1, $3); }
   ;

e_expon : e_dunary
   | e_dunary EXPP e_expon        { $$ = ex_new_op2(EX_OP_ART_POW, $1, $3); }
   | e_dunary EXPP e_lo_unary     { $$ = ex_new_op2(EX_OP_ART_POW, $1, $3); }
   ;

e_lo_unary : 
     PLUS e_lo_unary              { $$ = ex_new_op1(EX_OP_SGN_POS, $2); }
   | MINU e_lo_unary              { $$ = ex_new_op1(EX_OP_SGN_NEG, $2); }
   | PLUS e_dunary                { $$ = ex_new_op1(EX_OP_SGN_POS, $2); }
   | MINU e_dunary                { $$ = ex_new_op1(EX_OP_SGN_NEG, $2); }
   ; 

e_dunary : e_primary
   | OPER e_primary               { $$ = ex_new_op2(EX_OP_DEF_UNRY, $2, $1); }
   ;

e_primary : 
     LPAR e_top RPAR              { $$ = $2; }
   | e_local
   | e_access
   | e_list
   | e_cmplx
   ;

e_local : NAME                    { $$ = ex_new_op1(EX_OP_LOOKUP, $1); }
   | e_local PERC NAME            { $$ = ex_new_op2(EX_OP_REF_MEMB, $1, $3); }
   ;

e_access : 
     e_local e_arglist            { $$ = ex_new_op2(EX_OP_CALL, $1, $2); }
   | e_cmplx e_arglist            { $$ = ex_new_op2(EX_OP_REF_ARRY, $1, $2); }
   | e_list e_arglist             { $$ = ex_new_op2(EX_OP_REF_ARRY, $1, $2); }
   ;

e_arglist : LPAR RPAR             { $$ = ex_new_op0(EX_OP_PARAM); }
   | LPAR e_param RPAR            { $$ = $2; }
   ;

e_param : e_arg                   { $$ = ex_new_op1(EX_OP_PARAM, $1); }
   | e_param COMA e_arg           { $$ = ex_add_operand($1, $3); }
   ;

e_arg : e_slice
   | NAME AEQL e_top              { $$ = ex_new_op2(EX_OP_NAMED, $1, $3); }
   ;

e_slice : e_top
   | e_top COLN e_top COLN e_top  { $$ = ex_f90_do_range($1, $3, $5); }
   | e_top COLN e_top             { $$ = ex_f90_do_range($1, $3, NULL); }
   | e_top COLN                   { $$ = ex_f90_do_range($1, NULL, NULL); }
   | COLN e_top                   { $$ = ex_f90_do_range(NULL, $2, NULL); }
   | COLN                         { $$ = ex_f90_do_range(NULL, NULL, NULL); }
   ;

e_list :
     LARR e_elem RARR             { $$ = $2; }
   | LARR e_list RARR             { $$ = $2; }
   ;

e_elem : e_litem                  { $$ = ex_new_op1(EX_OP_LIST, $1); }
   | e_list COMA e_list           { $$ = ex_add_operand($1, $3); }
   | e_elem COMA e_list           { $$ = ex_add_operand($1, $3); }
   | e_list COMA e_litem          { $$ = ex_add_operand($1, $3); } 
   | e_elem COMA e_litem          { $$ = ex_add_operand($1, $3); }
   ;

e_litem : e_cmplx
   | PLUS e_cast                  { $$ = ex_new_op1(EX_OP_SGN_POS, $2); }
   | MINU e_cast                  { $$ = ex_new_op1(EX_OP_SGN_NEG, $2); }
   ;

e_cmplx : e_cast
   | LPAR e_iitem COMA e_iitem RPAR  { $$ = ex_new_op2(EX_OP_KST_CMPX, $2, $4); }
   ;

e_iitem: e_cast
   | PLUS e_cast                  { $$ = ex_new_op1(EX_OP_SGN_POS, $2); }
   | MINU e_cast                  { $$ = ex_new_op1(EX_OP_SGN_NEG, $2); }
   ;

e_cast : e_number | e_string
   | e_number UNDR e_number       { $$ = ex_new_op2(EX_OP_CVT_KND, $3, $1); }
   | e_number UNDR e_local        { $$ = ex_f90_kind_cast($3, $1); }
   | e_number UNDR e_string       { $$ = ex_new_op2(EX_OP_CVT_KND, $1, $3); }
   | e_local UNDR e_string        { $$ = ex_f90_kind_cast($1, $3); }
   ;

e_string : K_STR ;

e_number :
     K_DATA
   | K_INTEG
   | K_BOOL
   | K_FLOAT
   | K_DOUBL
   ;

%%

extern FILE* yyf90in;


#define YYF90_BUF 16384
char yyf90_data_buf[YYF90_BUF];
char* yyf90_data_pos;
char* yyf90_data_last;

int yyf90wrap() {
   return 1;
}

void yyf90error(const char* s) {
   fprintf(stderr, "%s\n", s);
   exit(1);
}

int yyf90input(char* buf, int max_size) {
   int left = yyf90_data_last - yyf90_data_pos;
   left = max_size > left ? left : max_size;

   if (left > 0) {
      memcpy(buf,yyf90_data_pos, left);
      yyf90_data_pos += left; 
   }

   return left; 
}

int main()
{

   fgets(yyf90_data_buf, YYF90_BUF, stdin);
   yyf90_data_pos = yyf90_data_buf;

   ex_f90_trim_input(yyf90_data_pos);
   yyf90_data_last = yyf90_data_buf;
   yyf90_data_last += strlen(yyf90_data_last);
   yyf90parse();

   return 0;
}

