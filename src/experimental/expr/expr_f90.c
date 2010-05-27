/***************************************************
 * This file contains routines for lowering simple *
 * syntatic constructs into low-level operations   *
 ***************************************************/

#include "expr_f90.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

/* The following function removes excess whitespace
   from a raw input string.  It's a bit involved so
   just hope to hell that it continues to work.
*/
static int
ex_f90_next(const char* s) {
   while (*s == ' ') {
      s++;
   }
   return *s;
}

void
ex_f90_trim_input(char* s) {
   unsigned int i=0, j=0;

   while (s[j] != '\0') {
      /* protect double quote strings */
      if (s[j] == '"') {
         s[i] = s[j]; i++; j++;
         while (s[j] != '\0') {
            if (s[j] == '"') {
               s[i] = s[j];
               i++; j++;

               if (s[j] != '"') {
                  if (ex_f90_next(s+j)=='"') {
                     printf("syntax error\n");
                     /*XXX: do real error */
                     exit(1);
                  }
                  break;
               }
            }
            s[i] = s[j];
            i++; j++;
         }
         continue;
      }

      /* protect single quote strings */
      if (s[j] == '\'') {
         s[i] = s[j]; i++; j++;
         while (s[j] != '\0') {
            if (s[j] == '\'') {
               s[i] = s[j];
               i++; j++;

               if (s[j] != '\'') {
                  if (ex_f90_next(s+j)=='\'') {
                     printf("syntax error\n");
                     /*XXX: do real error */
                     exit(1);
                  }
                  break;
               }
            }
            s[i] = s[j];
            i++; j++;
         }
         continue;
      }

      /* scan through whitespace */
      if ( s[j] == ' ' ) {
         j++; continue;
      }
  
      /* default does literal */   
      s[i] = s[j];
      i++; j++;
   }

   s[i] = '\0';
}


/* The following function makes a string, either
   single or double quoted depending on the second
   argument
*/
ex_node_t*
ex_f90_new_str(const char* s, int is_single) {
   char cpy[strlen(s)+1];
   unsigned int i,j;

   i = 0; j = 0;
   if (is_single) {
      while (s[i] != '\0') {
         if (s[i] == '\'') {
            cpy[j] = '\'';
            i++;
         } else {
            cpy[j] = s[i];
         }
         i++; j++;
      }
   } else { 
      while (s[i] != '\0') {
         if (s[i] == '"') {
            cpy[j] = '"';
            i++;
         } else {
            cpy[j] = s[i];
         }
         i++; j++;
      }
   } 

   cpy[j] = '\0';
   return ex_new_str(cpy, j);
}

/* The following function takes a fortran-style
   floating point constant and returns a node
*/
ex_node_t*
ex_f90_new_float(const char* s, int go_big) {
   char cpy[strlen(s)+1];
   unsigned is_doubl = 0;
   unsigned int i = 0;

   while (s[i] != '\0') {
      if (s[i] == 'd' || s[i] == 'D') {
         is_doubl = 1;
         cpy[i] = 'e';
      } else {
         cpy[i] = s[i];
      }
      i++;
   }
   cpy[i] = '\0';

   if (go_big) {
      return ex_parse_ldoubl(cpy);
   } else if (is_doubl) {
      return ex_parse_doubl(cpy);
   }

   return ex_parse_float(cpy);
}

/* The following function takes a fortran-style
   BOZ integer constant and returns a node
*/
ex_node_t*
ex_f90_new_integ(const char* s, int base, int sgn) {
   char cpy[strlen(s)+1];
   unsigned int i, j;

   i = 1; j = 0;
   while (s[i] != '\'' && s[i] != '"') {
      i++; 
   }

   i++;
   while (s[i] != '\'' && s[i] != '"') {
       cpy[j] = s[i];
       i++; j++;
   } 
   cpy[j] = '\0';

   return ex_parse_long(cpy, base, sgn);
}

/* The following function makes a named cast node
*/
ex_node_t*
ex_f90_kind_cast(ex_node_t* typ, ex_node_t* lit) {
   ex_node_t* ret;

   ret = ex_new_op1(EX_OP_KINDOF, typ);
   ret = ex_new_op2(EX_OP_CVT_KND, ret, lit);
   return ret;
}

/* The following function puts together a range
*/
ex_node_t*
ex_f90_do_range(
   ex_node_t* lo,
   ex_node_t* hi,
   ex_node_t* inc) {

   /* make void for one sided ops */
   if (hi == NULL) {
      hi = ex_new_op0(EX_OP_VOID); 
   }
   if (lo == NULL) {
      lo = ex_new_op0(EX_OP_VOID); 
   }

   /* put together the range node */
   if (inc == NULL) {
      return ex_new_op2(EX_OP_REF_RNGE,lo,hi);
   } else {
      return ex_new_op3(EX_OP_REF_RNGE,lo,hi,inc);
   }
}

/* The following function checks for illegal constructs
   which are parsed for sole purpose of delivering a
   more intelligent message later on.
*/
void
ex_f90_chk_syntax(ex_node_t* a) {

   /* recurse through all children */
   EX_FOR_KIDS(a, ex_f90_chk_syntax);

   /* check for A**-B type expressions */
   if (EX_ND_TYPE(a) == EX_TP_OPER &&
       EX_OP_TYPE(a) == EX_OP_ART_POW) {
      ex_node_t* kid = EX_KID1(a);

      if (EX_ND_TYPE(kid) == EX_TP_OPER && (
          EX_OP_TYPE(kid) == EX_OP_SGN_NEG ||
          EX_OP_TYPE(kid) == EX_OP_SGN_POS)) {
         /* XXX: replace with messaging sytem */
         printf("error: expressions of the type A**-B "
                "are not standard compliant\n");
         printf("error: this form is not consistently "
                "evaluated across compilers\n");
         exit(1);
      }
   }

   /* check for untyped kind suffices */
   if (EX_ND_TYPE(a) == EX_TP_OPER &&
       EX_OP_TYPE(a) == EX_OP_CVT_KND) {
      ex_node_t* kid0 = EX_KID0(a);
      ex_node_t* kid1 = EX_KID1(a);

      /* check for untyped kind argument */
      if (EX_ND_TYPE(kid0) == EX_TP_OPER &&
          EX_OP_TYPE(kid0) == EX_OP_KST_DATA) {
         /* XXX: replace with messaging system */
         printf("error: kind parameter must be "
                "integer or constant variable\n");
         exit(1);
      }

      /* check for untyped constant value */
      if (EX_ND_TYPE(kid1) == EX_TP_OPER &&
          EX_OP_TYPE(kid1) == EX_OP_KST_DATA) {
         /* XXX: replace with messaging system */
         printf("error: kind parameter not allowed "
                "with untyped constant values\n");
         exit(1);
      }
   }
}

