
%{

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "expr_node.h"
#include "expr_arena.h"
#include "expr_c99.h"
#include "expr_c99_lower.h"

void yyc99error(const char* s);
int yyc99parse(void);
int yyc99lex();

#define YYSTYPE ex_node_t*

%}


%token NAME EXEC PLUS MINU MULT DIVI MODL LPAR
%token RPAR LBRC RBRC ARRW PDOT EXCL TLDE INCR
%token DECR AMPR CMGE CMLE CNEQ CEEQ CART PIPE
%token LAND LLOR QEST COLN AEQL ASHL ASHR AAND
%token AOOR AXOR AADD ASUB AMUL ADIV AMOD COMA
%token LCRL RCRL LANG RANG SCLN BSHR BSHL EXTN
%token ELLP DCAST SCAST ICAST CCAST PPTR MPTR

%token K_STR K_INT K_LONG K_LLONG K_UINT K_ULONG
%token K_ULLONG K_CHAR K_FLOAT K_DOUBL K_LDOUBL
%token K_BOOL

%token VOID BOOL CHAR SHORT INT LONG FLOAT DOUBLE
%token COMPLEX SIGNED UNSIGNED STRUCT UNION CLASS
%token TYPE ENUM CONST VOLATILE RESTRICT SIZEOF
%token TYPEOF

%%

exec :  e_top EXEC                { ex_arena_t* w = ex_arena_new();
                                    $1 = ex_c99_reduce_dlist($1);
                                    ex_c99_chk_syntax($1);
                                    //$1 = ex_c99_lower($1, w);
                                    printf("\nParsed to:\n");
                                    ex_dump($1,0);
                                    putchar('\n');
                                    exit(0); }

e_top : e_list | e_comma ;

e_list :
        LCRL e_list RCRL          { $$ = $2; }
   |    LCRL e_elem RCRL          { $$ = $2; }
   ;

e_elem : e_cond                   { $$ = ex_new_op1(EX_OP_LIST, $1); }
   |    e_list COMA e_list        { $$ = ex_add_operand($1, $3); }
   |    e_elem COMA e_list        { $$ = ex_add_operand($1, $3); }
   |    e_list COMA e_cond        { $$ = ex_add_operand($1, $3); }
   |    e_elem COMA e_cond        { $$ = ex_add_operand($1, $3); }
   ; 

e_comma : e_assn
   |    e_comma COMA e_assn       { $$ = ex_new_op2(EX_OP_COMMA, $1,$3); }
   ;

e_assn : e_cond
   |    e_cond AEQL e_assn        { $$ = ex_new_op2(EX_OP_MOV_EQ, $1,$3); }
   |    e_cond ASHL e_assn        { $$ = ex_new_op2(EX_OP_MOV_SHL, $1,$3); }
   |    e_cond ASHR e_assn        { $$ = ex_new_op2(EX_OP_MOV_SHR, $1,$3); }
   |    e_cond AAND e_assn        { $$ = ex_new_op2(EX_OP_MOV_AND, $1,$3); }
   |    e_cond AOOR e_assn        { $$ = ex_new_op2(EX_OP_MOV_OR, $1,$3); }
   |    e_cond AXOR e_assn        { $$ = ex_new_op2(EX_OP_MOV_XOR, $1,$3); }
   |    e_cond AADD e_assn        { $$ = ex_new_op2(EX_OP_MOV_ADD, $1,$3); }
   |    e_cond ASUB e_assn        { $$ = ex_new_op2(EX_OP_MOV_SUB, $1,$3); }
   |    e_cond AMUL e_assn        { $$ = ex_new_op2(EX_OP_MOV_MUL, $1,$3); }
   |    e_cond ADIV e_assn        { $$ = ex_new_op2(EX_OP_MOV_DIV, $1,$3); }
   |    e_cond AMOD e_assn        { $$ = ex_new_op2(EX_OP_MOV_MOD, $1,$3); }
   ;

e_cond : e_loor
 | e_loor QEST e_top COLN e_cond  { $$ = ex_new_op3(EX_OP_SELECT, $1,$3,$5); }
 | e_loor QEST COLN e_cond        { $$ = ex_new_op2(EX_OP_SELECT, $1,$4); }
   ;

e_loor : e_land
   |    e_loor LLOR e_land        { $$ = ex_new_op2(EX_OP_LOG_OR, $1,$3); }
   ;

e_land : e_boor
   |    e_land LAND e_boor        { $$ = ex_new_op2(EX_OP_LOG_AND, $1,$3); }
   ;

e_boor : e_bxor
   |    e_boor PIPE e_bxor        { $$ = ex_new_op2(EX_OP_BIT_OR, $1,$3); }
   ;

e_bxor : e_band
   |    e_bxor CART e_band        { $$ = ex_new_op2(EX_OP_BIT_XOR, $1,$3); }
   ;

e_band : e_equal
   |    e_band AMPR e_equal       { $$ = ex_new_op2(EX_OP_BIT_AND, $1,$3); }
   ;

e_equal : e_rela
   |    e_equal CNEQ e_rela       { $$ = ex_new_op2(EX_OP_CMP_NE, $1,$3); }
   |    e_equal CEEQ e_rela       { $$ = ex_new_op2(EX_OP_CMP_EQ, $1,$3); }
   ;

e_rela : e_shift
   |    e_rela RANG e_shift       { $$ = ex_new_op2(EX_OP_CMP_GT, $1,$3); }
   |    e_rela LANG e_shift       { $$ = ex_new_op2(EX_OP_CMP_LT, $1,$3); }
   |    e_rela CMGE e_shift       { $$ = ex_new_op2(EX_OP_CMP_GE, $1,$3); }
   |    e_rela CMLE e_shift       { $$ = ex_new_op2(EX_OP_CMP_LE, $1,$3); }
   ;

e_shift : e_addi
   |    e_shift BSHL e_addi       { $$ = ex_new_op2(EX_OP_BIT_SHL, $1,$3); }
   |    e_shift BSHR e_addi       { $$ = ex_new_op2(EX_OP_BIT_SHR, $1,$3); }
   ;

e_addi : e_multp
   |    e_addi PLUS e_multp       { $$ = ex_new_op2(EX_OP_ART_ADD, $1,$3); }
   |    e_addi MINU e_multp       { $$ = ex_new_op2(EX_OP_ART_SUB, $1,$3); }
   ;

e_multp : e_mptr
   |    e_multp MULT e_unary      { $$ = ex_new_op2(EX_OP_ART_MUL, $1,$3); }
   |    e_multp DIVI e_unary      { $$ = ex_new_op2(EX_OP_ART_DIV, $1,$3); }
   |    e_multp MODL e_unary      { $$ = ex_new_op2(EX_OP_ART_MOD, $1,$3); }
   ;

e_mptr : e_unary
   |    e_mptr PPTR e_unary       { $$ = ex_new_op2(EX_OP_REF_PCPP, $1,$3); }
   |    e_mptr MPTR e_unary       { $$ = ex_new_op2(EX_OP_REF_MCPP, $1,$3); }
   ;

e_unary : e_post
   |    e_cast e_unary            { $$ = ex_new_op2(EX_OP_CVT_EXP, $1,$2); }
   |    EXCL e_unary              { $$ = ex_new_op1(EX_OP_LOG_NOT, $2); }
   |    TLDE e_unary              { $$ = ex_new_op1(EX_OP_BIT_NEG, $2); }
   |    INCR e_unary              { $$ = ex_new_op1(EX_OP_INC_PRE, $2); }
   |    DECR e_unary              { $$ = ex_new_op1(EX_OP_DEC_PRE, $2); }
   |    PLUS e_unary              { $$ = ex_new_op1(EX_OP_SGN_POS, $2); }
   |    MINU e_unary              { $$ = ex_new_op1(EX_OP_SGN_NEG, $2); }
   |    MULT e_unary              { $$ = ex_new_op1(EX_OP_REF_STAR, $2); }
   |    AMPR e_unary              { $$ = ex_new_op1(EX_OP_REF_ADDR, $2); }
   ;

e_cast : LPAR t_top RPAR          { $$ = $2; }
   ;

e_post : e_xcast
   |    e_post ARRW NAME          { $$ = ex_new_op2(EX_OP_REF_MPTR, $1,$3); }
   |    e_post PDOT NAME          { $$ = ex_new_op2(EX_OP_REF_MEMB, $1,$3); }
   |    e_post e_loc              { $$ = ex_new_op2(EX_OP_REF_ARRY, $1,$2); }
   |    e_post e_call             { $$ = ex_new_op2(EX_OP_CALL, $1,$2); }
   |    e_post INCR               { $$ = ex_new_op1(EX_OP_INC_POST, $1); }
   |    e_post DECR               { $$ = ex_new_op1(EX_OP_DEC_POST, $1); }
   ; 

e_loc : LBRC e_top RBRC           { $$ = $2; }
   ;

e_call : LPAR RPAR                { $$ = ex_new_op0(EX_OP_PARAM); }
   |    LPAR e_param RPAR         { $$ = $2; }
   ;

e_param : e_assn                  { $$ = ex_new_op1(EX_OP_PARAM, $1); }
   |    e_param COMA e_assn       { $$ = ex_add_operand($1, $3); }
   ;

e_xcast : e_stmt
   |    DCAST e_xtype e_xcarg     { $$ = ex_new_op2(EX_OP_CVT_DYN, $2, $3); }
   |    SCAST e_xtype e_xcarg     { $$ = ex_new_op2(EX_OP_CVT_STC, $2, $3); }
   |    CCAST e_xtype e_xcarg     { $$ = ex_new_op2(EX_OP_CVT_CST, $2, $3); }
   |    ICAST e_xtype e_xcarg     { $$ = ex_new_op2(EX_OP_CVT_ITP, $2, $3); }
   ;

e_xtype : LANG t_top RANG         { $$ = $2; }
   ;

e_xcarg : LPAR e_top RPAR         { $$ = $2; }
   ;

e_stmt : e_sizeof
   |    EXTN LPAR s_top RPAR      { $$ = ex_new_op1(EX_OP_SEXPR, $3); }
   |    EXTN LPAR e_comma RPAR    { $$ = $3; }
   ;

e_sizeof : e_primary
   |    SIZEOF LPAR t_top RPAR    { $$ = ex_new_op1(EX_OP_SIZEOF, $3); } 
   |    SIZEOF e_primary          { $$ = ex_c99_sizeof_expr($2); } 
   ;

e_primary :
        LPAR e_top RPAR           { $$ = $2; }
   |    e_strcat
   |    NAME                      { $$ = ex_new_op1(EX_OP_LOOKUP, $1); }
   |    K_BOOL
   |    K_CHAR
   |    K_INT 
   |    K_LONG
   |    K_LLONG
   |    K_UINT 
   |    K_ULONG
   |    K_ULLONG
   |    K_FLOAT
   |    K_DOUBL
   |    K_LDOUBL
   ; 

e_strcat : K_STR
   |    e_strcat K_STR            { $$ = ex_new_op2(EX_OP_STR_CAT, $1, $2); }
   ;

s_top : s_block ;

s_block : LCRL RCRL               { $$ = ex_new_op0(EX_OP_BLOCK); }
   |    LCRL s_list RCRL          { $$ = $2; }
   ;

s_list : s_expr                   { $$ = ex_new_op1(EX_OP_BLOCK, $1); }
   |    s_list s_expr             { $$ = ex_add_operand($1, $2); }
   ;

s_expr : td_top
   |    e_assn SCLN               { $$ = ex_new_op1(EX_OP_STMT, $1); }
   |    s_block
   ;

td_top :
        t_decl_top td_names SCLN  { $$ = ex_c99_declare_list($1,$2); }
   ;

td_names : td_field               
   |    td_names COMA td_field    { $$ = ex_new_op2(EX_OP_GLUE, $1,$3); }
   ;

td_field : td_name
   |    td_name COLN e_primary    { $$ = ex_new_op2(EX_OP_BITFD,$1,$3); }
   |    td_name AEQL td_init      { $$ = ex_new_op2(EX_OP_INITL,$1,$3); }
   ;

td_init : e_list | e_assn ;

td_name : NAME                    { $$ = ex_new_op2(EX_OP_DECLR,NULL,$1); }
   |    t_ptr_part NAME t_ary_vpart { $$ = ex_c99_decl_ptr_ary($1,$2,$3); }
   |    NAME t_ary_vpart          { $$ = ex_new_op2(EX_OP_DECLR,$2,$1); }
   |    t_ptr_part NAME           { $$ = ex_new_op2(EX_OP_DECLR,$1,$2); }
   ;

t_top : t_func ;

t_func : t_array
   | t_pointer t_ary_over t_args  { $$ = ex_c99_func_pointer($1,$2,$3); }
   ;

t_args :  LPAR RPAR               { $$ = ex_new_op0(EX_OP_PARAM); }
   |    LPAR t_arg_list RPAR      { $$ = $2; }
   ; 

t_arg_list : t_param              { $$ = ex_new_op1(EX_OP_PARAM,$1); }
   |    t_arg_list COMA t_param   { $$ = ex_add_operand($1, $3); }
   ;

t_param : t_top                   { $$ = $1; }
   |    ELLP                      { $$ = ex_new_op0(EX_OP_VARARG); };
   ;

t_array : t_pointer
   |    t_pointer t_ary_top       { $$ = ex_root_tree($1,$2); }
   ;

t_ary_top : t_ary_vpart
   |    t_ary_over t_ary_vpart    { $$ = ex_root_tree($2,$1); }
   ;

t_ary_over : LPAR t_ary_mod RPAR  { $$ = $2; }
   ;

t_ary_mod : t_ptr_part
   |    t_ptr_part t_ary_vpart    { $$ = ex_root_tree($1,$2); }
   ;

t_ary_vpart : t_ary_part
   |    LBRC RBRC                 { $$ = ex_new_op1(EX_OP_VARRAY, NULL); }
   |    t_ary_part LBRC RBRC      { $$ = ex_new_op1(EX_OP_VARRAY, $1); }
   ;

t_ary_part : t_ary_dim            { $$ = ex_new_op2(EX_OP_ARRAY, NULL,$1); }
   |    t_ary_part t_ary_dim      { $$ = ex_new_op2(EX_OP_ARRAY, $1,$2); }
   ;

t_ary_dim : LBRC e_top RBRC       { $$ = $2; }
   ;

t_pointer : t_qual
   |    t_qual t_ptr_part         { $$ = ex_root_tree($1,$2); }
   ;
   
t_ptr_part : MULT                 { $$ = ex_new_op1(EX_OP_POINT, NULL); }
   |    t_ptr_part MULT           { $$ = ex_new_op1(EX_OP_POINT, $1); }
   |    t_ptr_part CONST          { $$ = ex_new_op1(EX_OP_CONST, $1); }
   |    t_ptr_part VOLATILE       { $$ = ex_new_op1(EX_OP_VOLAT, $1); }
   |    t_ptr_part RESTRICT       { $$ = ex_new_op1(EX_OP_RESTR, $1); }
   ;

t_decl_top : t_qual ;

t_qual : t_typeof
   |    CONST t_qual              { $$ = ex_new_op1(EX_OP_CONST, $2); }
   |    VOLATILE t_qual           { $$ = ex_new_op1(EX_OP_VOLAT, $2); }
   |    RESTRICT t_qual           { $$ = ex_new_op1(EX_OP_RESTR, $2); }
   ;

t_typeof : t_tmpl
   |    TYPEOF LPAR e_top RPAR    { $$ = ex_new_op1(EX_OP_TYPEOF, $3);}
   ;

t_tmpl : t_deriv
   |    TYPE t_tmpl_blk           { $$ = ex_new_op2(EX_OP_TEMPL, $1,$2); }
   ;

t_tmpl_blk :
        LANG t_tmpl_list RANG     { $$ = $2; }
   ;

t_tmpl_list : t_tmpl_arg          { $$ = ex_new_op1(EX_OP_TLIST, $1); }
   |    t_tmpl_list COMA t_tmpl_arg  { $$ = ex_add_operand($1, $3); }
   ;

t_tmpl_arg :
        t_top
   |    e_mptr
   ;

t_deriv : t_fixed 
   |    STRUCT NAME t_memb_blk    { $$ = ex_new_op2(EX_OP_STRUCT, $2, $3); }
   |    CLASS NAME t_memb_blk     { $$ = ex_new_op2(EX_OP_CLASS, $2, $3); }
   |    UNION NAME t_memb_blk     { $$ = ex_new_op2(EX_OP_UNION, $2, $3); }
   |    STRUCT t_memb_blk         { $$ = ex_new_op1(EX_OP_ASTRUCT, $2); }
   |    CLASS t_memb_blk          { $$ = ex_new_op1(EX_OP_ACLASS, $2); }
   |    UNION t_memb_blk          { $$ = ex_new_op1(EX_OP_AUNION, $2); }
   |    STRUCT TYPE               { $$ = ex_new_op1(EX_OP_STRUCT, $2); }
   |    STRUCT NAME               { $$ = ex_new_op1(EX_OP_STRUCT, $2); }
   |    CLASS TYPE                { $$ = ex_new_op1(EX_OP_CLASS, $2); }
   |    CLASS NAME                { $$ = ex_new_op1(EX_OP_CLASS, $2); }
   |    UNION TYPE                { $$ = ex_new_op1(EX_OP_UNION, $2); }
   |    UNION NAME                { $$ = ex_new_op1(EX_OP_UNION, $2); }
   |    ENUM TYPE                 { $$ = ex_new_op1(EX_OP_ENUM, $2); }
   |    ENUM NAME                 { $$ = ex_new_op1(EX_OP_ENUM, $2); }
   ;

t_memb_blk : LCRL RCRL            { $$ = ex_new_op0(EX_OP_MEMB); }
   |    LCRL t_memb_list RCRL     { $$ = $2; }
   ;

t_memb_list : td_top              { $$ = ex_new_op1(EX_OP_MEMB, $1); }
   |    t_memb_list td_top        { $$ = ex_add_operand($1, $2); }
   ;

t_fixed :
        TYPE
   |    t_void
   |    t_bool
   |    t_char
   |    t_integ
   |    t_float
   |    t_double
   |    t_cfloat
   |    t_cdoubl
   ; 

t_void : VOID                     { $$ = ex_new_op0(EX_OP_VOID); }
   ;

t_bool : BOOL                     { $$ = ex_new_op0(EX_OP_BOOL); }
   ;

t_char : CHAR                     { $$ = ex_new_op0(EX_OP_CHAR); }
   |    SIGNED t_char             { $$ = ex_new_op1(EX_OP_SIGNED, $2); }
   |    UNSIGNED t_char           { $$ = ex_new_op1(EX_OP_UNSIGN, $2); }
   ;

t_integ : t_integ_base
   |    SIGNED t_integ            { $$ = ex_new_op1(EX_OP_SIGNED, $2); }
   |    UNSIGNED t_integ          { $$ = ex_new_op1(EX_OP_UNSIGN, $2); }
   |    SHORT t_integ             { $$ = ex_new_op1(EX_OP_SHORT, $2); }
   |    LONG t_integ              { $$ = ex_new_op1(EX_OP_LONG, $2); }
   ;

t_integ_base:
        INT                       { $$ = ex_new_op0(EX_OP_INT); }
   |    SIGNED                    { $$ = ex_c99_modified_int(EX_OP_SIGNED); }
   |    UNSIGNED                  { $$ = ex_c99_modified_int(EX_OP_UNSIGN); }
   |    LONG                      { $$ = ex_c99_modified_int(EX_OP_LONG); }
   |    SHORT                     { $$ = ex_c99_modified_int(EX_OP_SHORT); }
   ;

t_float : FLOAT                   { $$ = ex_new_op0(EX_OP_FLOAT); }
   ;

t_double : DOUBLE                 { $$ = ex_new_op0(EX_OP_DOUBL); }
   |    LONG t_double             { $$ = ex_new_op1(EX_OP_LONG, $2); }
   ;

t_cfloat : COMPLEX FLOAT          { $$ = ex_new_op0(EX_OP_CFLOAT); }
   ;

t_cdoubl : COMPLEX                { $$ = ex_new_op0(EX_OP_CDOUBL); }
   |    COMPLEX DOUBLE            { $$ = ex_new_op0(EX_OP_CDOUBL); }
   |    LONG t_cdoubl             { $$ = ex_new_op1(EX_OP_LONG, $2); }
   ;

%%

extern FILE* yyc99in;

#define YYC99_BUF 16384
char yyc99_data_buf[YYC99_BUF];
char* yyc99_data_pos;
char* yyc99_data_last;

int yyc99wrap() {
   return 1;
}

void yyc99error(const char* s) {
   fprintf(stderr, "%s\n", s);
   exit(1);
}

int yyc99input(char* buf, int max_size) {
   int left = yyc99_data_last - yyc99_data_pos;
   left = max_size > left ? left : max_size;

   if (left > 0) {
      memcpy(buf,yyc99_data_pos, left);
      yyc99_data_pos += left; 
   }

   return left; 
}

int main()
{
   fgets(yyc99_data_buf, YYC99_BUF, stdin);
   yyc99_data_pos = yyc99_data_buf;
   yyc99_data_last = yyc99_data_buf;
   yyc99_data_last += strlen(yyc99_data_last);
   yyc99parse();

   return 0;
}

