/***************************************************
 * This file contains routines for lowering simple *
 * syntatic constructs into low-level operations   *
 ***************************************************/

#include "expr_c99.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <stdio.h>

/*
  The following function un-escapes a C-style
  character, returns the number of characters
  which were processed.
*/
int
ex_c99_unescape_char(const char* s, char* c) {

   /* plain character */
   if (s[0] != '\\') {
      *c = s[0];
      return 1;
   }

   /* no terminus after escape */
   assert(s[1] != '\0');

   /* check for octal */
   if (isdigit(s[1])) {
      char buf[4], *eptr;
      long k;

      strncpy(buf,s+1,3);
      buf[3] = '\0';
      k = strtoul(buf,&eptr,8);

      if (k > 255) {
         /* XXX: do warning */
      }

      *c = k;
      return eptr-buf+1;
   }

   /* check for hexadecimal */
   if (s[1] == 'x') {
      char buf[3], *eptr;
      long k;

      strncpy(buf,s+2,2);
      buf[2] = '\0';
      k = strtoul(buf,&eptr,16);

      if (k > 255) {
         /* XXX: do warning */
      }

      *c = k;
      return eptr-buf+2;
   }

   /* check for escaped seq */
   switch (s[1]) {
   case '\"': *c = '\"'; break;
   case '\\': *c = '\\'; break;
   case 'a':  *c = '\a'; break;
   case 'b':  *c = '\b'; break;
   case 'f':  *c = '\f'; break;
   case 'n':  *c = '\n'; break;
   case 'r':  *c = '\r'; break;
   case 't':  *c = '\t'; break;
   case 'v':  *c = '\v'; break;

   default: *c = s[1];
      /* XXX: do warning */
   }

   return 2;
}

/*
  The following function un-escapes a C-style
  string literal, returning an immed node.
*/
ex_node_t*
ex_c99_new_str(const char* s) {
   char a[strlen(s)+1];
   unsigned i=0, j=0;

   while (s[i] != '\0') {
      unsigned k;

      k = ex_c99_unescape_char(s+i, a+j);
      i += k; j++;
   }

   a[j++] = '\0';
   return ex_new_str(a, j);
}

/*
  The following function un-escapes a C-style
  string literal, returning an immed node.
*/
ex_node_t*
ex_c99_new_char(const char* s) {
   int k, len;
   char c;

   len = strlen(s);
   k = ex_c99_unescape_char(s,&c);
   /* XXX: do warning (k != len) */

   return ex_new_char(c);
}


/*
  The following function constructs a modified
  integer.  Such as unsigned, signed, long, ...
*/
ex_node_t*
ex_c99_modified_int(int modifier) {
   ex_node_t* ret;

   ret = ex_new_op0(EX_OP_INT);
   return ex_new_op1(modifier, ret);
}


/*
  The following function converts a sizeof
  call to an expression, to a sizeof a type
*/
ex_node_t*
ex_c99_sizeof_expr(ex_node_t* a) {
   ex_node_t* ret;
   ret = ex_new_op1(EX_OP_TYPEOF, a);
   return ex_new_op1(EX_OP_SIZEOF, ret);
}


/*
  The following function constructs a 
  pointer to an anonymous function
*/
ex_node_t*
ex_c99_func_pointer(
   ex_node_t* rtp,
   ex_node_t* ptr,
   ex_node_t* arg) {
   ex_node_t* ret;
   ex_node_t* shw;

   shw = ex_new_op2(EX_OP_FUNC, rtp,arg);
   ret = ex_root_tree(shw,ptr);
   return ret;
}


/*
  The following function converts a list of half-
  baked declr nodes into complete declarations.
 */
ex_node_t*
ex_c99_declare_list(
   ex_node_t* low,
   ex_node_t* high) {
   ex_node_t* ret;
   ex_node_t* opd;
   ex_node_t* dup;

   /* shortcut when no glue exists */
   if (EX_ND_TYPE(high) != EX_TP_OPER ||
       EX_OP_TYPE(high) != EX_OP_GLUE) {
      return ex_root_tree(low, high);
   }

   /* need to make a declaration list */
   ret = ex_new_op0(EX_OP_DLIST);

   while (EX_ND_TYPE(high) == EX_TP_OPER &&
          EX_OP_TYPE(high) == EX_OP_GLUE) {
      ex_node_t* left;

      /* duplicate and root new decl */
      left = EX_KID0(high);
      opd = EX_KID1(high);

      dup = ex_dupl_tree(low);
      EX_KID0(opd) = ex_root_tree(dup, EX_KID0(opd));

      /* add the decl to list */
      ret = ex_add_operand(ret, opd);

      /* remove the glue node */
      ex_free_node(high);

      high = left;
   }

   /* add leftmost non-glue node */
   high = ex_root_tree(low, high);
   ret = ex_add_operand(ret, high);

   return ret;
}

/* The following function reduces the messy little
   DLIST nodes which are left behind by declarations
*/
ex_node_t*
ex_c99_reduce_dlist(ex_node_t* a) {
   ex_node_t *dup;
   int i, j;

   /* recurse through all children */
   EX_TRANS_KIDS(a, a, ex_c99_reduce_dlist);

   /* check if I own a DLIST node */
   for (i=0; i < EX_NKID(a); i++) {
      ex_node_t* kid = EX_KIDX(a,i);

      if (EX_ND_TYPE(kid) == EX_TP_OPER &&
          EX_OP_TYPE(kid) == EX_OP_DLIST) {
          goto found_dlist;
      }
   }

   /* no DLIST */
   return a;

   /* found a DLIST node, make dup */
found_dlist:
   dup = ex_new_op0(EX_OP_TYPE(a));

   /* traverse all my children */
   for (i=0; i < EX_NKID(a); i++) {
      ex_node_t* kid = EX_KIDX(a,i);

      /* everything but DLIST just copy */
      if (EX_ND_TYPE(kid) != EX_TP_OPER ||
          EX_OP_TYPE(kid) != EX_OP_DLIST) {
          dup = ex_add_operand(dup, kid);
         continue;
      }

      /* reverse order gets real order */
      for (j=EX_NKID(kid)-1; j>=0; j--) {
         ex_node_t* decl = EX_KIDX(kid,j);
         dup = ex_add_operand(dup, decl);
      }

      /* delete the DLIST node */
      ex_free_node(kid);
   }

   ex_free_node(a);
   return dup;
}

/* The following function takes a pointer wrapper, an
   array wrapper, and a name to make a full decl node.
*/
ex_node_t*
ex_c99_decl_ptr_ary(
   ex_node_t* ptr,
   ex_node_t* nam,
   ex_node_t* ary) {
   ex_node_t* ret;

   ret = ex_new_op2(EX_OP_DECLR, ary, nam);
   ex_root_tree(ptr, ary);
   return ret;
}

/* The following function checks for illegal constructs
   which are parsed for sole purpose of delivering a
   more intelligent message later on.
*/
void
ex_c99_chk_syntax(ex_node_t* a) {

   /* recurse through all children */
   EX_FOR_KIDS(a, ex_c99_chk_syntax);

   /* check for bitfields in declarations */
   if (EX_ND_TYPE(a) == EX_TP_OPER &&
       EX_OP_TYPE(a) == EX_OP_BLOCK) {
      unsigned int i;

      for (i=0; i < EX_NKID(a); i++) {
         ex_node_t* kid = EX_KIDX(a,i);

         if (EX_ND_TYPE(kid) == EX_TP_OPER &&
             EX_OP_TYPE(kid) == EX_OP_BITFD) {
            /* XXX: need to use a messaging system */
            printf("error: bitfields may not be used "
                   "in variable declarations\n");
            exit(1);
         }
      } 
   }

   /* check for initalizers in member decl */
   if (EX_ND_TYPE(a) == EX_TP_OPER &&
       EX_OP_TYPE(a) == EX_OP_MEMB) {
      unsigned int i;

      for (i=0; i < EX_NKID(a); i++) {
         ex_node_t* kid = EX_KIDX(a,i);

         if (EX_ND_TYPE(kid) == EX_TP_OPER &&
             EX_OP_TYPE(kid) == EX_OP_INITL) {
            /* XXX: need to use a messaging system */
            printf("error: initializers may not be "
                   "used in member declarations\n");
            exit(1);
         }
      }
   }
}

