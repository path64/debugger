
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "expr_node.h"

void
ex_dump_char(char c, int w_single) {
   const char* s = NULL;

   if (w_single && c == '\'') {
     s = "\\\'";
   }

   switch (c) {
   case '\"': s = "\\\""; break;
   case '\\': s = "\\\\"; break;
   case '\a': s = "\\a"; break;
   case '\b': s = "\\b"; break;
   case '\f': s = "\\f"; break;
   case '\n': s = "\\n"; break;
   case '\r': s = "\\r"; break;
   case '\t': s = "\\t"; break;
   case '\v': s = "\\v"; break;
   }
   if (s != NULL) {
      printf("%s", s);
      return;
   }

   if (isprint(c)) {
      printf("%c", c); 
      return;
   }

   printf("\\%o", (int)c & 0xff);
}

void
ex_dump_str(ex_im_str_t* s) {
   int i;

   for (i=0; i < s->len; i++) {
      ex_dump_char(s->mem[i],0);
   }
}

void
ex_dump_op(ex_node_t* a) {
   int op_ident = EX_OP_TYPE(a);
   const char* s;

   switch(op_ident) {
   case EX_OP_LOOKUP:    s = "LOOKUP"; break;
   case EX_OP_LIST:      s = "LIST"; break;
   case EX_OP_CALL:      s = "CALL"; break;
   case EX_OP_PARAM:     s = "PARAM"; break;
   case EX_OP_NAMED:     s = "NAMED"; break;
   case EX_OP_COMMA:     s = "COMMA"; break;
   case EX_OP_SIZEOF:    s = "SIZEOF"; break;
   case EX_OP_SELECT:    s = "SELECT"; break;
   case EX_OP_KST_DATA:  s = "KST_DATA"; break;
   case EX_OP_KST_CMPX:  s = "KST_CMPX"; break;
   case EX_OP_STR_CAT:   s = "STR_CAT"; break;
   case EX_OP_CVT_EXP:   s = "CVT_EXP"; break;
   case EX_OP_CVT_DYN:   s = "CVT_DYN"; break;
   case EX_OP_CVT_STC:   s = "CVT_STC"; break;
   case EX_OP_CVT_ITP:   s = "CVT_ITP"; break;
   case EX_OP_CVT_CST:   s = "CVT_CST"; break;
   case EX_OP_CVT_KND:   s = "CVT_KND"; break;
   case EX_OP_CVT_BIT:   s = "CVT_BIT"; break;
   case EX_OP_MOV_EQ:    s = "MOV_EQ";  break;
   case EX_OP_MOV_SHL:   s = "MOV_SHL"; break;
   case EX_OP_MOV_SHR:   s = "MOV_SHR"; break;
   case EX_OP_MOV_AND:   s = "MOV_AND"; break;
   case EX_OP_MOV_OR:    s = "MOV_OR";  break;
   case EX_OP_MOV_XOR:   s = "MOV_XOR"; break;
   case EX_OP_MOV_ADD:   s = "MOV_ADD"; break;
   case EX_OP_MOV_SUB:   s = "MOV_SUB"; break;
   case EX_OP_MOV_MUL:   s = "MOV_MUL"; break;
   case EX_OP_MOV_DIV:   s = "MOV_DIV"; break;
   case EX_OP_MOV_MOD:   s = "MOV_MOD"; break;
   case EX_OP_MOV_PTR:   s = "MOV_PTR"; break;
   case EX_OP_LOG_OR:    s = "LOG_OR";  break;
   case EX_OP_LOG_NOT:   s = "LOG_NOT"; break;
   case EX_OP_LOG_AND:   s = "LOG_AND"; break;
   case EX_OP_DEF_UNRY:  s = "DEF_UNRY"; break;
   case EX_OP_DEF_BNRY:  s = "DEF_BNRY"; break;
   case EX_OP_BIT_NEG:   s = "BIT_NEG"; break;
   case EX_OP_BIT_SHL:   s = "BIT_SHL"; break;
   case EX_OP_BIT_SHR:   s = "BIT_SHR"; break;
   case EX_OP_BIT_OR:    s = "BIT_OR";  break;
   case EX_OP_BIT_XOR:   s = "BIT_XOR"; break;
   case EX_OP_BIT_AND:   s = "BIT_AND"; break;
   case EX_OP_CMP_NE:    s = "CMP_NE";  break;
   case EX_OP_CMP_EQ:    s = "CMP_EQ";  break;
   case EX_OP_CMP_GT:    s = "CMP_GT";  break;
   case EX_OP_CMP_LT:    s = "CMP_LT";  break;
   case EX_OP_CMP_GE:    s = "CMP_GE";  break;
   case EX_OP_CMP_LE:    s = "CMP_LE";  break;
   case EX_OP_ART_ADD:   s = "ART_ADD"; break;
   case EX_OP_ART_SUB:   s = "ART_SUB"; break;
   case EX_OP_ART_MUL:   s = "ART_MUL"; break;
   case EX_OP_ART_DIV:   s = "ART_DIV"; break;
   case EX_OP_ART_MOD:   s = "ART_MOD"; break;
   case EX_OP_ART_POW:   s = "ART_POW"; break;
   case EX_OP_SGN_NEG:   s = "SGN_NEG"; break;
   case EX_OP_SGN_POS:   s = "SGN_POS"; break;
   case EX_OP_INC_PRE:   s = "INC_PRE"; break;
   case EX_OP_INC_POST:  s = "INC_POST"; break;
   case EX_OP_DEC_PRE:   s = "DEC_PRE"; break;
   case EX_OP_DEC_POST:  s = "DEC_POST"; break;
   case EX_OP_REF_STAR:  s = "REF_STAR"; break;
   case EX_OP_REF_ADDR:  s = "REF_ADDR"; break;
   case EX_OP_REF_MEMB:  s = "REF_MEMB"; break;
   case EX_OP_REF_MCPP:  s = "REF_MCPP"; break;
   case EX_OP_REF_MPTR:  s = "REF_MPTR"; break;
   case EX_OP_REF_PCPP:  s = "REF_PCPP"; break;
   case EX_OP_REF_ARRY:  s = "REF_ARRY"; break;
   case EX_OP_REF_RNGE:  s = "REF_RNGE"; break;
   case EX_OP_REF_SPAN:  s = "REF_SPAN"; break;

   case EX_OP_GLUE:      s = "GLUE"; break;
   case EX_OP_DLIST:     s = "DLIST"; break;

   case EX_OP_SEXPR:     s = "SEXPR"; break;
   case EX_OP_DECLR:     s = "DECLR"; break;
   case EX_OP_INITL:     s = "INITL"; break;
   case EX_OP_STMT:      s = "STMT"; break;
   case EX_OP_BLOCK:     s = "BLOCK"; break;

   case EX_OP_TYPEOF:    s = "TYP_TYPEOF"; break;
   case EX_OP_KINDOF:    s = "TYP_KINDOF"; break;
   case EX_OP_FUNC:      s = "TYP_FUNC"; break;
   case EX_OP_VARARG:    s = "TYP_VARARG"; break;
   case EX_OP_VARRAY:    s = "TYP_VARRAY"; break;
   case EX_OP_ARRAY:     s = "TYP_ARRAY"; break;
   case EX_OP_POINT:     s = "TYP_POINT"; break;
   case EX_OP_CONST:     s = "TYP_CONST"; break;
   case EX_OP_VOLAT:     s = "TYP_VOLAT"; break;
   case EX_OP_RESTR:     s = "TYP_RESTR"; break;
   case EX_OP_TEMPL:     s = "TYP_TEMPL"; break;
   case EX_OP_TLIST:     s = "TYP_TLIST"; break;
   case EX_OP_ASTRUCT:   s = "TYP_ASTRUCT"; break;
   case EX_OP_STRUCT:    s = "TYP_STRUCT"; break;
   case EX_OP_ACLASS:    s = "TYP_ACLASS"; break;
   case EX_OP_CLASS:     s = "TYP_CLASS"; break;
   case EX_OP_AUNION:    s = "TYP_AUNION"; break;
   case EX_OP_UNION:     s = "TYP_UNION"; break;
   case EX_OP_ENUM:      s = "TYP_ENUM"; break;
   case EX_OP_MEMB:      s = "TYP_MEMB"; break;
   case EX_OP_BITFD:     s = "TYP_BITFD"; break;
   case EX_OP_UNSIGN:    s = "TYP_UNSIGN"; break;
   case EX_OP_SIGNED:    s = "TYP_SIGNED"; break;
   case EX_OP_SHORT:     s = "TYP_SHORT"; break;
   case EX_OP_LONG:      s = "TYP_LONG"; break;

   case EX_OP_VOID:      s = "TYP_VOID"; break;
   case EX_OP_BOOL:      s = "TYP_BOOL"; break;
   case EX_OP_INT:       s = "TYP_INT"; break;
   case EX_OP_CHAR:      s = "TYP_CHAR"; break;
   case EX_OP_FLOAT:     s = "TYP_FLOAT"; break;
   case EX_OP_DOUBL:     s = "TYP_DOUBL"; break;
   case EX_OP_CFLOAT:    s = "TYP_CFLOAT"; break;
   case EX_OP_CDOUBL:    s = "TYP_CDOUBL"; break;

   case EX_LO_NULL:      s = "LO_NULL"; break;
   case EX_LO_LDID:      s = "LO_LDID"; break;
   case EX_LO_STID:      s = "LO_STID"; break;
   case EX_LO_STRIP:     s = "LO_STRIP"; break;
   case EX_LO_LOAD:      s = "LO_LOAD"; break;
   case EX_LO_STORE:     s = "LO_STORE"; break;

   default: s = "<invalid operator>";
   }

   printf("%s", s);
}

void
ex_dump_immd(ex_node_t* a) {
   int op_ident = EX_IM_TYPE(a);

   switch(op_ident) {
   case EX_IM_STRING:
      printf("STRCONST \"");
      ex_dump_str(EX_STRING(a));
      printf("\" ");
      break;
   case EX_IM_BOOL:
      printf("LOGCONST ");
      if (EX_INTEG(a)) {
         printf("true");
      } else {
         printf("false");
      } break;
   case EX_IM_CHAR:
      printf("CHRCONST \'");
      ex_dump_char(EX_CHAR(a),1);
      printf("\' ");
      break;
   case EX_IM_INT:
      printf("INTCONST %d ", (int)EX_INTEG(a));
      break;
   case EX_IM_LONG:
      printf("LONGCONST %ld ", (long)EX_INTEG(a));
      break;
   case EX_IM_LLONG:
      printf("LLONGCONST %lld ", (long long)EX_INTEG(a));
      break;
   case EX_IM_UINT:
      printf("UINTCONST %u ", (unsigned)EX_INTEG(a));
      break;
   case EX_IM_ULONG:
      printf("ULONGCONST %lu ", (unsigned long)EX_INTEG(a));
      break;
   case EX_IM_ULLONG:
      printf("ULLONGCONST %llu ", (unsigned long long)EX_INTEG(a));
      break;
   case EX_IM_FLOAT:
      printf("FLOATCONST %.8g ",(double)EX_FLOAT(a));
      break;
   case EX_IM_DOUBL:
      printf("DBLCONST %.16g ", (double)EX_FLOAT(a));
      break;
   case EX_IM_LDOUBL:
      printf("LDBLCONST %.20Lg ", (long double)EX_FLOAT(a));
      break;
   }
}

void
ex_dump(ex_node_t* a, int level) {

   /* drop and run for NULL */
   if (a == NULL) { 
      {int i; for (i=0; i<level; i++) {
         putchar(' ');
      }}
      printf("(nil)\n");
      return;
   }

   /* special case the load nodes */
   if (EX_ND_TYPE(a) == EX_TP_OPER && 
       EX_OP_TYPE(a) == EX_OP_LOOKUP &&
       EX_NKID(a) == 1) {
       ex_node_t* kid = EX_KID0(a);

       if (EX_ND_TYPE(kid) == EX_TP_IMMD &&
           EX_OP_TYPE(kid) == EX_IM_STRING) {
          {int i; for (i=0; i<level; i++) {
             putchar(' ');
          }}

          ex_dump_op(a);
          printf(" \"");
          ex_dump_str(EX_STRING(kid));
          printf("\"\n");
          return;
       }
   }

   /* recurse through child nodes */
   EX_FOR_KIDS1(a, ex_dump, level+1);
   {int i; for (i=0; i<level; i++) {
      putchar(' ');
   }}

   /* special case the ldid/stid nodes */
   if ( EX_ND_TYPE(a) == EX_TP_OPER  &&
        ( (EX_OP_TYPE(a) == EX_LO_LDID && EX_NKID(a) == 0) ||
          (EX_OP_TYPE(a) == EX_LO_STID && EX_NKID(a) == 1)
        )
      )
   {
      ex_dump_op(a);
      printf(" <%d>\n", EX_SYMID(a));
      return;
   }

   /* refer to type specific print */
   if (EX_ND_TYPE(a) == EX_TP_IMMD) {
      ex_dump_immd(a);
   }
   else if (EX_ND_TYPE(a) == EX_TP_OPER) {
      ex_dump_op(a);
   }
   else {
      printf("<invalid op-type>\n");
   }

   printf("\n");
}

ex_node_t*
ex_new_str(const char* s, int len) {
   ex_node_t* ret;
   ex_im_str_t* str;

   ret = malloc(sizeof(ex_node_t));
   EX_SET_TYPE(ret, EX_TP_IMMD);
   EX_SET_ITYPE(ret, EX_IM_STRING);

   str = EX_STRING(ret);
   str->mem = malloc(len+1);
   memcpy(str->mem, s, len);
   str->mem[len] = '\0';
   str->len = len;

   return ret;
}

ex_node_t*
ex_new_bool(int s) {
   ex_node_t* ret;

   ret = malloc(sizeof(ex_node_t));
   EX_SET_TYPE(ret, EX_TP_IMMD);
   EX_SET_ITYPE(ret, EX_IM_BOOL);
   EX_INTEG(ret) = s;

   return ret;
}

ex_node_t*
ex_new_char(char c) {
   ex_node_t* ret;

   ret = malloc(sizeof(ex_node_t));
   EX_SET_TYPE(ret, EX_TP_IMMD);
   EX_SET_ITYPE(ret, EX_IM_CHAR);
   EX_INTEG(ret) = (long)c;

   return ret;
}
ex_node_t*
ex_parse_float(const char* s) {
   char* endptr;
   return ex_new_float(strtof(s,&endptr));
   /* XXX: do error-checking */
}

ex_node_t*
ex_new_float(float value) {
   ex_node_t* ret;

   ret = malloc(sizeof(ex_node_t));
   EX_SET_TYPE(ret, EX_TP_IMMD);
   EX_SET_ITYPE(ret, EX_IM_FLOAT);
   EX_FLOAT(ret) = value;

   return ret;
}

ex_node_t*
ex_parse_doubl(const char* s) {
   char* endptr;
   return ex_new_doubl(strtod(s,&endptr));
   /* XXX: do error-checking */
}

ex_node_t*
ex_new_doubl(double value) {
   ex_node_t* ret;

   ret = malloc(sizeof(ex_node_t));
   EX_SET_TYPE(ret, EX_TP_IMMD);
   EX_SET_ITYPE(ret, EX_IM_DOUBL);
   EX_FLOAT(ret) = value;

   return ret;
}

ex_node_t*
ex_parse_ldoubl(const char* s) {
   char* endptr;
   return ex_new_ldoubl(strtold(s,&endptr));
   /* XXX: do error-checking */
}

ex_node_t*
ex_new_ldoubl(long double value) {
   ex_node_t* ret;

   ret = malloc(sizeof(ex_node_t));
   EX_SET_TYPE(ret, EX_TP_IMMD);
   EX_SET_ITYPE(ret, EX_IM_LDOUBL);
   EX_FLOAT(ret) = value;

   return ret;
}

ex_node_t*
ex_parse_int(const char* s, int base, int sign) {
   char* endptr;

   if (sign) {
     return ex_new_int(strtol(s, &endptr, base), sign);
   } else {
     return ex_new_int(strtoul(s, &endptr, base), sign);
   }
   /* XXX: do error checking */
}

ex_node_t*
ex_new_int(int value, int sign) {
   ex_node_t* ret;

   ret = malloc(sizeof(ex_node_t));

   if (sign) {
     EX_SET_TYPE(ret, EX_TP_IMMD);
     EX_SET_ITYPE(ret, EX_IM_INT);
     EX_INTEG(ret) = value;
   } else {
     EX_SET_TYPE(ret, EX_TP_IMMD);
     EX_SET_ITYPE(ret, EX_IM_UINT);
     EX_INTEG(ret) = value;
   }

   return ret;
}

ex_node_t*
ex_parse_long(const char* s, int base, int sign) {
   char* endptr;

   if (sign) {
      return ex_new_long(strtol(s, &endptr, base), sign);
   } else {
      return ex_new_long(strtoul(s, &endptr, base), sign);
   }

   /* XXX: do error checking */
}

ex_node_t*
ex_new_long(long value, int sign) {
   ex_node_t* ret;

   ret = malloc(sizeof(ex_node_t));

   if (sign) {
      EX_SET_TYPE(ret, EX_TP_IMMD);
      EX_SET_ITYPE(ret, EX_IM_LONG);
      EX_INTEG(ret) = value;
   } else {
      EX_SET_TYPE(ret, EX_TP_IMMD);
      EX_SET_ITYPE(ret, EX_IM_ULONG);
      EX_INTEG(ret) = value;
   }

   return ret;
}

ex_node_t*
ex_parse_llong(const char* s, int base, int sign) {
   char* endptr;

   if (sign) {
      return ex_new_llong(strtoll(s, &endptr, base), sign);
   } else {
      return ex_new_llong(strtoull(s, &endptr, base), sign);
   }
 
   /* XXX: do error checking */
}

ex_node_t*
ex_new_llong(long long value, int sign) {
   ex_node_t* ret;

   ret = malloc(sizeof(ex_node_t));

   if (sign) {
      EX_SET_TYPE(ret, EX_TP_IMMD);
      EX_SET_ITYPE(ret, EX_IM_LLONG);
      EX_INTEG(ret) = value;
   } else {
      EX_SET_TYPE(ret, EX_TP_IMMD);
      EX_SET_ITYPE(ret, EX_IM_ULLONG);
      EX_INTEG(ret) = value;
   }

   return ret;
}

ex_node_t*
ex_new_op0(int id) {
   ex_node_t* ret;

   ret = malloc(sizeof(ex_node_t));
   EX_INIT_OPDS(ret);
   EX_SET_TYPE(ret, EX_TP_OPER);
   EX_SET_OTYPE(ret, id);
   EX_SET_NKID(ret, 0);
   return ret;
}

ex_node_t*
ex_new_op1(int id,
   ex_node_t* arg1) {
   ex_node_t* ret;

   ret = malloc(sizeof(ex_node_t));
   EX_INIT_OPDS(ret);
   EX_SET_TYPE(ret, EX_TP_OPER);
   EX_SET_OTYPE(ret, id);
   EX_SET_NKID(ret, 1);

   EX_KID0(ret) = arg1;
   return ret;
}

ex_node_t*
ex_new_op2(int id,
   ex_node_t* arg1,
   ex_node_t* arg2) {
   ex_node_t* ret;

   ret = malloc(sizeof(ex_node_t));
   EX_INIT_OPDS(ret);
   EX_SET_TYPE(ret, EX_TP_OPER);
   EX_SET_OTYPE(ret, id);
   EX_SET_NKID(ret, 2);

   EX_KID0(ret) = arg1;
   EX_KID1(ret) = arg2;
   return ret;
}

ex_node_t*
ex_new_op3(int id,
   ex_node_t* arg1,
   ex_node_t* arg2,
   ex_node_t* arg3) {
   ex_node_t* ret;

   ret = malloc(sizeof(ex_node_t));
   EX_INIT_OPDS(ret);
   EX_SET_TYPE(ret, EX_TP_OPER);
   EX_SET_OTYPE(ret, id);
   EX_SET_NKID(ret, 3);

   EX_KID0(ret) = arg1;
   EX_KID1(ret) = arg2;
   EX_KID2(ret) = arg3;
   return ret;
}

/* The following function takes a hollowed-out modifier
 * node and places a base type down at the bottom
 */
ex_node_t* ex_root_tree(
   ex_node_t* low,
   ex_node_t* high) {
   ex_node_t* ret;
   ex_node_t* opd;

   if (high == NULL) {
      return low;
   }
   ret = high;

   if (EX_NKID(high) == 0) {
      return NULL;
   }
   opd = EX_KID0(high);

   while (opd != NULL) {
      high = opd;

      if (EX_NKID(high) == 0) {
         return NULL;
      }
      opd = EX_KID0(high);
   }

   if (EX_NKID(high) == 0) {
      return NULL;
   }
   EX_KID0(high) = low;

   return ret;
}

/* The following function removes and operand from
   a node.  It *DOESN'T* free the node itself.
*/
ex_node_t*
ex_del_operand(ex_node_t* a, int id) {
   unsigned int i;

   /* shift following nodes upward */
   for (i = id; i < EX_NKID(a)-1; i++) {
      EX_KIDX(a,i) = EX_KIDX(a,i+1);
   }

   /* check if we now fit in static */
   if (EX_NKID(a) == EX_STATIC_SIZE+1) {
      /* move data to static array */
      for (i=0; i < EX_NKID(a); i++) {
         EX_S_KIDX(a,i) = EX_KIDX(a,i);
      }

      /* free aray storage */
      free(EX_KIDS(a));
      EX_INIT_OPDS(a); 
   }

   /* reset the number of kids */
   i = EX_NKID(a) - 1;
   EX_SET_NKID(a, i);

   return a;
}

/* The following function adds an operand to a node.
*/
ex_node_t*
ex_add_operand(ex_node_t* a, ex_node_t* b) {
   unsigned int i;

   if (EX_NKID(a) == EX_STATIC_SIZE) {

      EX_KIDS(a) = (ex_node_t**)malloc(
        EX_MAX_CHILD * sizeof(ex_node_t*));

      for (i=0; i<EX_STATIC_SIZE; i++) {
         EX_KIDX(a,i) = EX_S_KIDX(a,i);
      }
   }

   i = EX_NKID(a);
   EX_KIDX(a,i) = b;
   EX_SET_NKID(a, i+1);

   return a;
}

/* Duplicate a tree of expression nodes.  This
 * is necessary to avoid redundant free calls.
 */
static void
ex_dupl_str(ex_im_str_t* a, ex_im_str_t* b) {
   a->mem = (char*)malloc(b->len);
   memcpy(a->mem, b->mem, b->len);
   a->len = b->len;
}

ex_node_t*
ex_dupl_tree(ex_node_t* a) {
   ex_node_t* r;

   r = malloc(sizeof(ex_node_t));
   memcpy(r, a, sizeof(ex_node_t));

   if (EX_ND_TYPE(a) == EX_TP_IMMD &&
       EX_IM_TYPE(a) == EX_IM_STRING) {
      ex_dupl_str(EX_STRING(r),EX_STRING(a));
   }

   /* recurse through child nodes */
   EX_TRANS_KIDS(r, a, ex_dupl_tree);

   return r;
}

/* Free a single expression node
*/
void ex_free_node(ex_node_t* a) {
   if (EX_ND_TYPE(a) == EX_TP_IMMD &&
       EX_IM_TYPE(a) == EX_IM_STRING) {
      free(EX_STRING(a)->mem);
   }

   if (EX_NKID(a) > EX_STATIC_SIZE) {
      free(EX_KIDS(a));
   }

   free(a);
}

/* Free a tree of expression nodes.
 */
void ex_free_tree(ex_node_t* a) {
   /* recurse through child nodes */
   EX_FOR_KIDS(a, ex_free_tree);

   /* magnificent self-liberation */
   ex_free_node(a);
}

