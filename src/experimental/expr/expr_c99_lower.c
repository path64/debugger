/****************************************************
 * This file contains routines for lowering C99 ops *
 * into the mid-level representation.               *
 ***************************************************/

#include "expr_c99_lower.h"
#include "expr_low.h"

/* The following function lowers 'A++' and 'A--'
   expression forms.
*/
ex_node_t*
ex_c99_lower_inc(ex_node_t* a, ex_arena_t* w) {

   /* lower pre increment/decrement */
   if (EX_OP_TYPE(a) == EX_OP_INC_PRE ||
       EX_OP_TYPE(a) == EX_OP_DEC_PRE) {
      ex_node_t *m;

      ex_assert(EX_NKID(a) == 1);

      if (EX_OP_TYPE(a) == EX_OP_INC_PRE) {
         EX_SET_OTYPE(a, EX_OP_MOV_ADD);
      } else {
         EX_SET_OTYPE(a, EX_OP_MOV_SUB);
      }

      m = ex_new_int(1, 0);
      a = ex_add_operand(a, m);

      return a;
   }

   /* lower post increment/decrement */
   if (EX_OP_TYPE(a) == EX_OP_INC_POST ||
       EX_OP_TYPE(a) == EX_OP_DEC_POST) {
      int tid = ex_arena_get_tmp(w);
      ex_node_t *m,*n,*p, *comma;

      ex_assert(EX_NKID(a) == 1);

      m = ex_new_stid(EX_KID0(a), tid);
      comma = ex_new_op1(EX_OP_COMMA,m);

      m = ex_new_ldid(tid);
      m = ex_new_op1(EX_OP_REF_ADDR,m);

      n = ex_new_ldid(tid);
      p = ex_new_int(1,0);

      if (EX_OP_TYPE(a) == EX_OP_INC_POST) {
         n = ex_new_op2(EX_OP_ART_ADD,n,p);
      } else {
         n = ex_new_op2(EX_OP_ART_SUB,n,p);
      }

      m = ex_new_op2(EX_LO_STORE,m,n);
      comma = ex_add_operand(comma, m);

      m = ex_new_ldid(tid);
      comma = ex_add_operand(comma, m);

      ex_free_node(a);
      return comma;
   }

   return a;
}

/* The following function lowers A+=B exprs.
*/
ex_node_t*
ex_c99_lower_movx(ex_node_t* a, ex_arena_t* w) {
   ex_node_t *m,*n,*comma;
   int op_ident, nw_ident;
   int t1, t2;

   op_ident = EX_OP_TYPE(a);

   /* special case a straight move op */
   if (op_ident == EX_OP_MOV_EQ) {
      t1 = ex_arena_get_tmp(w);

      assert(EX_NKID(a) == 2);
      m = ex_new_stid(EX_KID1(a),t1);
      comma = ex_new_op1(EX_OP_COMMA,m);

      m = ex_new_op1(EX_OP_REF_ADDR, EX_KID0(a));
      n = ex_new_ldid(t1);
      m = ex_new_op2(EX_LO_STORE, m, n);
      comma = ex_add_operand(comma, m);

      m = ex_new_ldid(t1);
      m = ex_new_op1(EX_LO_STRIP, m);
      comma = ex_add_operand(comma, m);

      return comma;
   }

   /* find equivalent art operation */
   switch (op_ident) {
   case EX_OP_MOV_SHL: nw_ident = EX_OP_BIT_SHL; break;
   case EX_OP_MOV_SHR: nw_ident = EX_OP_BIT_SHR; break;
   case EX_OP_MOV_AND: nw_ident = EX_OP_BIT_AND; break;
   case EX_OP_MOV_OR:  nw_ident = EX_OP_BIT_OR; break;
   case EX_OP_MOV_XOR: nw_ident = EX_OP_BIT_XOR; break;
   case EX_OP_MOV_ADD: nw_ident = EX_OP_ART_ADD; break;
   case EX_OP_MOV_SUB: nw_ident = EX_OP_ART_SUB; break;
   case EX_OP_MOV_MUL: nw_ident = EX_OP_ART_MUL; break;
   case EX_OP_MOV_DIV: nw_ident = EX_OP_ART_DIV; break;
   case EX_OP_MOV_MOD: nw_ident = EX_OP_ART_MOD; break;
   default: nw_ident = EX_LO_NULL;
   }

   /* doesn't match the a+=b form */
   if (nw_ident == EX_LO_NULL) {
      return a;
   }

   ex_assert(EX_NKID(a) == 2);

   t1 = ex_arena_get_tmp(w);
   t2 = ex_arena_get_tmp(w);

   m = EX_KID0(a);
   m = ex_new_stid(m, t1); 
   comma = ex_new_op1(EX_OP_COMMA, m);

   n = ex_new_ldid(t1);
   n = ex_new_op2(nw_ident,n,EX_KID1(a));
   n = ex_new_stid(n, t2);
   comma = ex_add_operand(comma, n);

   m = ex_new_ldid(t1);
   m = ex_new_op1(EX_OP_REF_ADDR, m);
   n = ex_new_ldid(t2);
   n = ex_new_op2(EX_LO_STORE, m, n);
   comma = ex_add_operand(comma, n);

   m = ex_new_ldid(t2);
   comma = ex_add_operand(comma, m);
  
   return comma; 
}

/* The following function helps in lowering array
 * operations by recursively traversing the tree.
 */
ex_node_t*
ex_c99_lower_array_r(
   ex_node_t** top,
   int* count,
   ex_node_t* a,
   int t1)
{
   ex_node_t *m;

   /* check for array of arrays */
   if (EX_ND_TYPE(a) == EX_TP_OPER &&
       EX_OP_TYPE(a) == EX_OP_REF_ARRY) {
      ex_node_t *n, *r; 

      /* recurse through array children */
      m = EX_KID0(a);
      m = ex_c99_lower_array_r(top, count, m, t1);

      /* special case returning base */
      if (m == 0) {
         *count = 0;
         m = ex_new_ldid(t1);
         m = ex_new_op1(EX_OP_REF_ADDR,m);
      }

      /* construct an sized array access */
      n = ex_new_ldid(t1);
      r = ex_new_int((*count)++, 0);
      n = ex_new_op2(EX_OP_REF_SPAN,n,r);

      EX_KID0(a) = n;
      EX_SET_OTYPE(a, EX_OP_ART_MUL);
      m = ex_new_op2(EX_OP_ART_ADD,m,a); 

      return m;
   }

   /* base expr gets here */
   m = ex_new_stid(a, t1);
   *top = ex_add_operand(*top, m);

   return 0;
}

/* The following function lower array operations */
ex_node_t*
ex_c99_lower_array(ex_node_t* a, ex_arena_t* w) {
   ex_node_t *m, *n;
   int t1, count;

#if 1
   /* XXX: need to check if real array */

   if (EX_OP_TYPE(a) != EX_OP_REF_ARRY)
      return a;

   /* access to contigous array */
   m = ex_new_op0(EX_OP_COMMA);

   /* recursive build up size */
   t1 = ex_arena_get_tmp(w);
   n = ex_c99_lower_array_r(&m,&count,a,t1);

   /* add final to end of comma */
   n = ex_new_op1(EX_OP_REF_STAR, n);
   m = ex_add_operand(m, n);
 
   return m;
#else
   return a;
#endif
}


/* The following function lower pointer operations */
ex_node_t*
ex_c99_lower_ptr(ex_node_t* a, ex_arena_t* w) {
   ex_node_t *m;

   if (EX_OP_TYPE(a) != EX_OP_REF_ARRY)
      return a;

   /* array syntax to a pointer */
   m = ex_new_op1(EX_OP_REF_STAR, a);
   EX_SET_OTYPE(a, EX_OP_ART_ADD);
   return m;
}

/* The following function invokes all of the
   lowering stages described above.
*/
ex_node_t*
ex_c99_lower(ex_node_t* a, ex_arena_t* w) {
   /* flatly return immediate values */
   if (EX_ND_TYPE(a) != EX_TP_OPER) {
      return a;
   }

   /* XXX: lower overloaded operators */

   /* lower post/pre-increment operators */
   a = ex_c99_lower_inc(a, w);

   /* lower assignment operators */
   a = ex_c99_lower_movx(a, w);

   /* lower array operators */
   a = ex_c99_lower_array(a, w);

   /* lower pointer operators */
   a = ex_c99_lower_ptr(a, w);

   /* recurse through children */
   EX_TRANS_KIDS1(a, a, ex_c99_lower, w);

   return a;
}

