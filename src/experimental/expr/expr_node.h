
#ifndef _EXPR_NODE_H_
#define _EXPR_NODE_H_
#include <stdint.h>
#include <assert.h>

/*******************************************************
 * The following types are used to store the values of
 * literals from the source (strings, ints, floats).
 *******************************************************/
typedef
struct _ex_im_str_t {
   char* mem;
   int64_t len;
} ex_im_str_t;


/*******************************************************
 * The following type is the fundamental unit for any  *
 * expression.  It is used for both variable and type  *
 * expression trees and operations.                    *
 *******************************************************/

#define EX_MAX_CHILD 64
#define EX_STATIC_SIZE 3

typedef
struct _ex_node_t {
   int _class;
   int _sym_id;

   struct _ex_node_t** opdv;

   union {
      long long    crd;
      long double  flt;
      char chr;

      struct _ex_im_str_t str;
      struct _ex_node_t* opds[EX_STATIC_SIZE];
   } data;

} ex_node_t;


/*******************************************************
 * The following enumeration contains constants used   *
 * for the type field of expression nodes.             *
 *******************************************************/

#define   EX_NP_MASK    0x0000ffff  /* number of kids */
#define   EX_TP_MASK    0x000f0000  /* expr node type */
#define   EX_IP_MASK    0xfff00000  /* immed/oper type */

#define   EX_TP_IMMD    0x00000000  /* immediate type */
#define   EX_TP_OPER    0x00000001  /* operator type */


/*******************************************************
 * The following macros provide access to the class of *
 * an expression node.  They do NOT return lvalues.    *
 *******************************************************/

#define EX_INIT_OPDS(X)   ( (X)->opdv = (X)->data.opds )
#define EX_SET_NKID(X,Y)  ( ((X)->_class &= ~EX_NP_MASK), \
                            ((X)->_class |= ((Y) & 0xffff) << 0 ) )
#define EX_SET_TYPE(X,Y)  ( ((X)->_class &= ~EX_TP_MASK), \
                            ((X)->_class |= ((Y) & 0x000f) << 16) )
#define EX_SET_ITYPE(X,Y) ( ((X)->_class &= ~EX_IP_MASK), \
                            ((X)->_class |= ((Y) & 0x0fff) << 24) )
#define EX_SET_OTYPE(X,Y) ( ((X)->_class &= ~EX_IP_MASK), \
                            ((X)->_class |= ((Y) & 0x0fff) << 24) )

#define EX_NKID(X)        ( ((X)->_class & EX_NP_MASK) >> 0  )
#define EX_ND_TYPE(X)     ( ((X)->_class & EX_TP_MASK) >> 16 )
#define EX_IM_TYPE(X)     ( ((X)->_class & EX_IP_MASK) >> 24 )
#define EX_OP_TYPE(X)     ( ((X)->_class & EX_IP_MASK) >> 24 )

#define EX_SYMID(X)       ( (X)->_sym_id )

/*******************************************************
 * The following macros provide access to the values   *
 * stored by an expression. They do return lvalues.    *
 *******************************************************/

#define EX_S_KIDX(X,Y) ((X)->data.opds[(Y)])
#define EX_KIDX(X,Y)   ((X)->opdv[(Y)])
#define EX_KID0(X)     ((X)->opdv[0])
#define EX_KID1(X)     ((X)->opdv[1])
#define EX_KID2(X)     ((X)->opdv[2])
#define EX_KIDS(X)     ((X)->opdv)

#define EX_INTEG(X)   ((X)->data.crd)
#define EX_FLOAT(X)   ((X)->data.flt)
#define EX_CHAR(X)    ((X)->data.chr)
#define EX_STRING(X)  (&(X)->data.str)
#define EX_CSTRING(X) (EX_STRING(X)->mem)


/*******************************************************
 * The following macros call a function on every node  *
 * of an expression tree.                              *
 *******************************************************/
#define EX_TRANS_KIDS(X,Y,Z)                     \
   do {                                          \
      int __i = EX_NKID(X) - 1;                  \
      for (; __i >= 0 ; __i--) {                 \
         EX_KIDX(X,__i) =                        \
           (Z)(EX_KIDX(Y,__i));                  \
      }                                          \
   } while(0)

#define EX_TRANS_KIDS1(X,Y,Z,P)                  \
   do {                                          \
      int __i = EX_NKID(X) - 1;                  \
      for (; __i >= 0 ; __i--) {                 \
         EX_KIDX(X,__i) =                        \
           (Z)(EX_KIDX(Y,__i),(P));              \
      }                                          \
   } while(0)

#define EX_TRANS_KIDS2(X,Y,Z,P,Q)                \
   do {                                          \
      int __i = EX_NKID(X) - 1;                  \
      for (; __i >= 0 ; __i--) {                 \
         EX_KIDX(X,__i) =                        \
           (Z)(EX_KIDX(Y,__i),(P),(Q));          \
      }                                          \
   } while(0)

#define EX_TRANS_KIDS3(X,Y,Z,P,Q,T)              \
   do {                                          \
      int __i = EX_NKID(X) - 1;                  \
      for (; __i >= 0 ; __i--) {                 \
         EX_KIDX(X,__i) =                        \
           (Z)(EX_KIDX(Y,__i),(P),(Q),(T));      \
      }                                          \
   } while(0)

#define EX_FOR_KIDS(X,Y)                         \
   do {                                          \
      int __i,__j = EX_NKID(X);                  \
      for (__i = 0; __i < __j; __i++) {          \
         (Y)(EX_KIDX(X,__i));                    \
      }                                          \
   } while(0)

#define EX_FOR_KIDS1(X,Y,Z)                      \
   do {                                          \
      int __i,__j = EX_NKID(X);                  \
      for (__i = 0; __i < __j ; __i++) {         \
         (Y)(EX_KIDX(X,__i),(Z));                \
      }                                          \
   } while(0)

#define EX_FOR_KIDS2(X,Y,Z,Q)                    \
   do {                                          \
      int __i,__j = EX_NKID(X);                  \
      for (__i = 0; __i < __j ; __i++) {         \
         (Y)(EX_KIDX(X,__i),(Z),(Q));            \
      }                                          \
   } while(0)

#define EX_FOR_KIDS3(X,Y,Z,Q,T)                  \
   do {                                          \
      int __i,__j = EX_NKID(X);                  \
      for (__i = 0; __i < __j ; __i++) {         \
         (Y)(EX_KIDX(X,__i),(Z),(Q),(T));        \
      }                                          \
   } while(0)

#define EX_RFOR_KIDS(X,Y)                        \
   do {                                          \
      int __i = EX_NKID(X) - 1;                  \
      for (; __i >= 0; __i--) {                  \
         (Y)(EX_KIDX(X,__i));                    \
      }                                          \
   } while(0)

#define EX_RFOR_KIDS1(X,Y,Z)                     \
   do {                                          \
      int __i = EX_NKID(X) - 1;                  \
      for (; __i >= 0; __i--) {                  \
         (Y)(EX_KIDX(X,__i),(Z));                \
      }                                          \
   } while(0)

#define EX_RFOR_KIDS2(X,Y,Z,Q)                   \
   do {                                          \
      int __i = EX_NKID(X) - 1;                  \
      for (; __i >= 0; __i--) {                  \
         (Y)(EX_KIDX(X,__i),(Z),(Q));            \
      }                                          \
   } while(0)

#define EX_RFOR_KIDS3(X,Y,Z,Q,T)                 \
   do {                                          \
      int __i = EX_NKID(X) - 1;                  \
      for (; __i >= 0; __i--) {                  \
         (Y)(EX_KIDX(X,__i),(Z),(Q),(T));        \
      }                                          \
   } while(0)

/*******************************************************
 * The following operations are not language-specific. *
 * However, different languages may interpret them in  *
 * different ways. Before the tree can be evaluated it *
 * has to be lowered for each language.                *
 *******************************************************/
enum {

/* The following operations are built up in order to   *
 * put together complete expressions.                  *
 *******************************************************/
   EX_OP_LOOKUP,
   EX_OP_LIST,
   EX_OP_CALL,
   EX_OP_PARAM,
   EX_OP_NAMED,
   EX_OP_COMMA,
   EX_OP_SIZEOF,
   EX_OP_SELECT,
   EX_OP_KST_DATA,
   EX_OP_KST_CMPX,
   EX_OP_STR_CAT,
   EX_OP_CVT_EXP,
   EX_OP_CVT_DYN,
   EX_OP_CVT_STC,
   EX_OP_CVT_ITP,
   EX_OP_CVT_CST,
   EX_OP_CVT_KND,
   EX_OP_CVT_BIT,
   EX_OP_MOV_EQ,
   EX_OP_MOV_SHL,
   EX_OP_MOV_SHR,
   EX_OP_MOV_AND,
   EX_OP_MOV_OR,
   EX_OP_MOV_XOR,
   EX_OP_MOV_ADD,
   EX_OP_MOV_SUB,
   EX_OP_MOV_MUL,
   EX_OP_MOV_DIV,
   EX_OP_MOV_MOD,
   EX_OP_MOV_PTR,
   EX_OP_LOG_OR,
   EX_OP_LOG_NOT,
   EX_OP_LOG_AND,
   EX_OP_DEF_UNRY,
   EX_OP_DEF_BNRY,
   EX_OP_BIT_NEG,
   EX_OP_BIT_SHL,
   EX_OP_BIT_SHR,
   EX_OP_BIT_OR,
   EX_OP_BIT_XOR,
   EX_OP_BIT_AND,
   EX_OP_CMP_NE,
   EX_OP_CMP_EQ,
   EX_OP_CMP_GT,
   EX_OP_CMP_LT,
   EX_OP_CMP_GE,
   EX_OP_CMP_LE,
   EX_OP_ART_ADD,
   EX_OP_ART_SUB,
   EX_OP_ART_MUL,
   EX_OP_ART_DIV,
   EX_OP_ART_MOD,
   EX_OP_ART_POW,
   EX_OP_SGN_NEG,
   EX_OP_SGN_POS,
   EX_OP_INC_PRE,
   EX_OP_INC_POST,
   EX_OP_DEC_PRE,
   EX_OP_DEC_POST,
   EX_OP_REF_STAR,
   EX_OP_REF_ADDR,
   EX_OP_REF_MEMB,
   EX_OP_REF_MCPP,
   EX_OP_REF_MPTR,
   EX_OP_REF_PCPP,
   EX_OP_REF_ARRY,
   EX_OP_REF_RNGE,
   EX_OP_REF_SPAN,

/* The following operations silly little operations    *
 * that are used to build up expressions but removed   *
 * in a quick post-processing phase.                   *
 *******************************************************/
   EX_OP_GLUE,
   EX_OP_DLIST,

/* The following operations are for putting together   *
 * declarations as well as statement expressions       *
 *******************************************************/
   EX_OP_SEXPR,
   EX_OP_DECLR,
   EX_OP_INITL,
   EX_OP_STMT,
   EX_OP_BLOCK,

/* The following operations are used for building up   *
 * type expressions.  These are used mostly for casts. *
 *******************************************************/
   EX_OP_TYPEOF,
   EX_OP_KINDOF,
   EX_OP_FUNC,
   EX_OP_VARARG,
   EX_OP_VARRAY,
   EX_OP_ARRAY,
   EX_OP_POINT,
   EX_OP_CONST,
   EX_OP_VOLAT,
   EX_OP_RESTR,
   EX_OP_TEMPL,
   EX_OP_TLIST,
   EX_OP_ASTRUCT,
   EX_OP_STRUCT,
   EX_OP_ACLASS,
   EX_OP_CLASS,
   EX_OP_AUNION,
   EX_OP_UNION,
   EX_OP_ENUM,
   EX_OP_MEMB,
   EX_OP_BITFD,
   EX_OP_UNSIGN,
   EX_OP_SIGNED,
   EX_OP_SHORT,
   EX_OP_LONG,

/* The following node types represent base types which *
 * are then modified by the type operators above.      *
 *******************************************************/
   EX_OP_VOID,
   EX_OP_BOOL,
   EX_OP_CHAR,
   EX_OP_INT,
   EX_OP_FLOAT,
   EX_OP_DOUBL,
   EX_OP_CFLOAT,
   EX_OP_CDOUBL,

/* The following are used to define any literal values *
 * that occur within an expression.                    *
 *******************************************************/
   EX_IM_BOOL,
   EX_IM_CHAR,
   EX_IM_INT,
   EX_IM_LONG,
   EX_IM_LLONG,
   EX_IM_UINT,
   EX_IM_ULONG,
   EX_IM_ULLONG,
   EX_IM_FLOAT,
   EX_IM_DOUBL,
   EX_IM_LDOUBL,
   EX_IM_STRING,

/* The following operations low-level operations that  *
 * are produced after language-level lowering.
 *******************************************************/
   EX_LO_NULL,
   EX_LO_LDID,     /* load from temporary id */
   EX_LO_STID,     /* store to temporary id */   
   EX_LO_STRIP,    /* strip away the location */
   EX_LO_LOAD,     /* load from process memory */
   EX_LO_STORE     /* store to process memory */
};

#define ex_assert(X) (assert(X))

void ex_dump(ex_node_t*, int level);

ex_node_t* ex_new_str(const char*, int);
ex_node_t* ex_new_bool(int);
ex_node_t* ex_new_char(char);

ex_node_t* ex_parse_float(const char*);
ex_node_t* ex_parse_doubl(const char*);
ex_node_t* ex_parse_ldoubl(const char*);

ex_node_t* ex_new_float(float);
ex_node_t* ex_new_doubl(double);
ex_node_t* ex_new_ldoubl(long double);

ex_node_t* ex_parse_int(const char*,int,int);
ex_node_t* ex_parse_long(const char*,int,int);
ex_node_t* ex_parse_llong(const char*,int,int);

ex_node_t* ex_new_int(int, int);
ex_node_t* ex_new_long(long, int);
ex_node_t* ex_new_llong(long long, int);

ex_node_t* ex_new_op0(int);
ex_node_t* ex_new_op1(int,ex_node_t*);
ex_node_t* ex_new_op2(int,ex_node_t*,ex_node_t*);
ex_node_t* ex_new_op3(int,ex_node_t*,ex_node_t*,ex_node_t*);

ex_node_t* ex_del_operand(ex_node_t*, int);
ex_node_t* ex_add_operand(ex_node_t*,ex_node_t*);
ex_node_t* ex_root_tree(ex_node_t*,ex_node_t*);
ex_node_t* ex_dupl_tree(ex_node_t*);

void ex_free_node(ex_node_t*);
void ex_free_tree(ex_node_t*);

#endif
