/*

 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at 
 * http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at src/CDDL.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END

 * Copyright (c) 2004-2005 PathScale, Inc.  All rights reserved.
 * Use is subject to license terms.

file: dwf_locs.cc
created on: Wed Feb  2 19:47:08 PST 2005
author: James Strother <jims@pathscale.com>

*/

#include "dbg_dwarf.h"
#include "dwf_locs.h"
#include "dwf_info.h"
#include "dwf_cunit.h"
#include "dwf_spec.h"
#include "process.h"

Value DwLocExpr::getAddress() {
   if (data.size() != 1) {
      throw Exception("Address unavailable for this variable");
   }

   DwLocPiece P = data.front();

   switch (P.type) {
   case LOC_REG:   return Value(VALUE_REG, P.value);
   case LOC_ADDR:  return Value(VALUE_INTEGER, P.value);
   }

   throw Exception("invalid location type");
}

Address DwLocExpr::getValue(Process *proc,
   int size, bool sign_extend) {

    Address val = 0;
    unsigned pos = 0;  

    std::list<DwLocPiece>::iterator i;
    for (i=data.begin(); i!=data.end(); i++) {
       Address mask,seg=0;
       int rsize;

       DwLocPiece P = *i;

       /* zero means take all */
       if (P.size == 0) {
          rsize = size - pos;
       } else {
          rsize = P.size;
       } 

       /* should never be true */
       if (rsize < 0) break;

       /* read value in from process */
       switch (P.type) {
       case LOC_REG: 
          seg = proc->get_reg(P.value); break;
       case LOC_ADDR:
          seg = proc->read(P.value, rsize);
          break;
       }

       /* address-sized is trivial */
       if (rsize == sizeof(Address)) {
          val = seg; break;
       }

       /* mask of high bits */
       mask = (Address) 1L;
       mask <<= rsize * 8;
       mask = - (mask);

       /* combine results */      
       seg &=  ~mask;
       seg <<= pos * 8;
       val |= seg; 

       /* next position */
       pos += rsize;
    }

    /* use shift for sign extend */
    if (sign_extend) {
       int bits = size * 8;
       val <<= 64 - bits;
       val >>= 64 - bits;
    }

    return val;
}

void
DwLocExpr::add(DwLocType type,
  Address val, unsigned size) {

   DwLocPiece P (type, val, size);
   data.push_back(P);
}

DwLocEval::DwLocEval(DwCUnit *_cu, Process *_process,
  Address _frame_base, AttributeValue &_attr)
: stack(100), sp(0), top_is_reg(false), cu(_cu),
   frame_base(_frame_base), attr(_attr), process(_process)
{
}

void
DwLocEval::push(Address addr) {
   stack[sp] = addr;
   sp++;
}

DwLocExpr
DwLocEval::execute()
{
   DwInfo* dwarf = cu->get_dwinfo();
   BVector expr;

   /* if it's a location list, we need to
      find the actual location first */
   /* XXX: check for location list should go elsewhere */
   if (attr.type == AV_INTEGER) {
      Address pc = 0;
      pc = process->get_current_frame()->get_pc();
      expr = dwarf->get_loc_expr(cu, attr.addr, pc);
   } else {
      expr = (BVector) attr;
   }

   BStream stream(expr, dwarf->do_swap());

   int op = stream.read1u();
   eval_op(stream, op);

   while (!stream.eof()) {
      op = stream.read1u();
      eval_op(stream, op);
   }

   if (needs_push) {
      if (top_is_reg) {
         result.add(LOC_REG, stack[sp-1], 0);
      } else {
         result.add(LOC_ADDR, stack[sp-1], 0);
      }
   }

   return result;
}

int
DwLocEval::get_lit_num (int op) {
#define DEF_CASE(NUM)           \
   case DW_OP_lit##NUM: return NUM;

   switch (op) {
   DEF_CASE(0)
   DEF_CASE(1)
   DEF_CASE(2)
   DEF_CASE(3)
   DEF_CASE(4)
   DEF_CASE(5)
   DEF_CASE(6)
   DEF_CASE(7)
   DEF_CASE(8)
   DEF_CASE(9)
   DEF_CASE(10)
   DEF_CASE(11)
   DEF_CASE(12)
   DEF_CASE(13)
   DEF_CASE(14)
   DEF_CASE(15)
   DEF_CASE(16)
   DEF_CASE(17)
   DEF_CASE(18)
   DEF_CASE(19)
   DEF_CASE(20)
   DEF_CASE(21)
   DEF_CASE(22)
   DEF_CASE(23)
   DEF_CASE(24)
   DEF_CASE(25)
   DEF_CASE(26)
   DEF_CASE(27)
   DEF_CASE(28)
   DEF_CASE(29)
   DEF_CASE(30)
   DEF_CASE(31)
   }
   return -1;

#undef DEF_CASE
}

int
DwLocEval::get_reg_num (int op) {
#define DEF_CASE(NUM)           \
   case DW_OP_reg##NUM: return NUM;

   switch (op) {
   DEF_CASE(0)
   DEF_CASE(1)
   DEF_CASE(2)
   DEF_CASE(3)
   DEF_CASE(4)
   DEF_CASE(5)
   DEF_CASE(6)
   DEF_CASE(7)
   DEF_CASE(8)
   DEF_CASE(9)
   DEF_CASE(10)
   DEF_CASE(11)
   DEF_CASE(12)
   DEF_CASE(13)
   DEF_CASE(14)
   DEF_CASE(15)
   DEF_CASE(16)
   DEF_CASE(17)
   DEF_CASE(18)
   DEF_CASE(19)
   DEF_CASE(20)
   DEF_CASE(21)
   DEF_CASE(22)
   DEF_CASE(23)
   DEF_CASE(24)
   DEF_CASE(25)
   DEF_CASE(26)
   DEF_CASE(27)
   DEF_CASE(28)
   DEF_CASE(29)
   DEF_CASE(30)
   }

   return -1;
#undef DEF_CASE
}

int
DwLocEval::get_breg_num(int op) {
#define DEF_CASE(NUM)           \
   case DW_OP_breg##NUM: return NUM;

   switch (op) {
   DEF_CASE(0)
   DEF_CASE(1)
   DEF_CASE(2)
   DEF_CASE(3)
   DEF_CASE(4)
   DEF_CASE(5)
   DEF_CASE(6)
   DEF_CASE(7)
   DEF_CASE(8)
   DEF_CASE(9)
   DEF_CASE(10)
   DEF_CASE(11)
   DEF_CASE(12)
   DEF_CASE(13)
   DEF_CASE(14)
   DEF_CASE(15)
   DEF_CASE(16)
   DEF_CASE(17)
   DEF_CASE(18)
   DEF_CASE(19)
   DEF_CASE(20)
   DEF_CASE(21)
   DEF_CASE(22)
   DEF_CASE(23)
   DEF_CASE(24)
   DEF_CASE(25)
   DEF_CASE(26)
   DEF_CASE(27)
   DEF_CASE(28)
   DEF_CASE(29)
   DEF_CASE(30)
   }

   return -1;
#undef DEF_CASE
}

void
DwLocEval::eval_op(BStream& stream, int op) {
   int num;

   /* set default state */
   needs_push = true;

   /* special case register ops */
   num = get_reg_num(op);
   if (num != -1) {
      stack[sp++] = num;
      top_is_reg = true;
      return;
   }

   if (op == DW_OP_regx) { 
      num = stream.read_uleb();
      stack[sp++] = num;
      top_is_reg = true;
      return;
   } 

   /* special case literal operations */
   num = get_lit_num(op);
   if (num != -1) {
      stack[sp++] = num;
      return;
   }

   /* special case breg operations */
   num = get_breg_num(op);
   if (num != -1) {
      Address offset = stream.read_sleb();
      Address rval = process->get_reg(num);
      stack[sp++] = rval + offset;
      return;
   }

   /* catch vender extensions */
   if (op >= DW_OP_lo_user &&
       op <= DW_OP_hi_user) {
      throw Exception();
   }

   switch (op) {

/*  Push values onto the stack from stream
 **************************************************
 */
#define READ_OP(TAG,CMD)              \
   case DW_OP_##TAG:                  \
      stack[sp++] = stream.CMD();     \
      break;

   READ_OP(const1u, read1u)
   READ_OP(const2u, read2u)
   READ_OP(const4u, read4u)
   READ_OP(const8u, read8u)

   READ_OP(const1s, read1s)
   READ_OP(const2s, read2s)
   READ_OP(const4s, read4s)
   READ_OP(const8s, read8s)

   READ_OP(constu,  read_uleb)
   READ_OP(consts,  read_sleb)
#undef READ_OP 


/*  Modify top stack value with midfix op
 **************************************************
 */
#define MIDFIX_OP(TAG,CMD)              \
   case DW_OP_##TAG: {                  \
      if (sp < 2) throw Exception();    \
      Address v1 = stack[sp - 2];       \
      Address v2 = stack[sp - 1];       \
      stack[sp - 2] = v1 CMD v2;        \
      sp--;                             \
      break;                            \
   }

   MIDFIX_OP(div,   /)
   MIDFIX_OP(mul,   *)
   MIDFIX_OP(mod,   %)
   MIDFIX_OP(plus,  +)
   MIDFIX_OP(minus, -)
   MIDFIX_OP(and,   &)
   MIDFIX_OP(or,    |)
   MIDFIX_OP(xor,   ^)
   MIDFIX_OP(shl,   <<)
   MIDFIX_OP(shra,  >>)
#undef MIDFIX_OP


/*  Modify top stack value with unary op
 **************************************************
 */
#define UNARY_OP(TAG,CMD)                \
   case DW_OP_##TAG: {                   \
      if (sp < 1) throw Exception();     \
      Address v1 = stack[sp - 1];        \
      stack[sp - 1] = CMD v1;            \
      break;                             \
   }

   UNARY_OP(neg, -)
   UNARY_OP(not, ~)
#undef UNARY_OP


/*  Modify top stack value with comparator
 **************************************************
 */
#define COMP_OP(TAG,CMD)                 \
   case DW_OP_##TAG: {                   \
      if (sp < 2) throw Exception();     \
      Address v1 = stack[sp - 2];        \
      Address v2 = stack[sp - 1];        \
      stack[sp - 2] = v1 CMD v2;         \
      sp--;                              \
      break;                             \
   }

   COMP_OP(eq, ==)
   COMP_OP(ne, !=)
   COMP_OP(gt, >)
   COMP_OP(lt, <)
   COMP_OP(ge, >=)
   COMP_OP(le, <=)
#undef COMP_OP


/*  Modify stack through rearrangement
 **************************************************
 */
   case DW_OP_dup:
      if (sp < 1) throw Exception();
      stack[sp] = stack[sp - 1];
      sp++;
      break;
   case DW_OP_over:
      if (sp < 2) throw Exception();
      stack[sp] = stack[sp - 2];
      sp++;
      break;
   case DW_OP_drop:
      if (sp < 1) throw Exception();
      sp--;
      break;
   case DW_OP_swap:{
      if (sp < 2) throw Exception();
      Address tmp = stack[sp - 2];
      stack[sp - 2] = stack[sp - 1];
      stack[sp - 1] = tmp;
      break;
   }
   case DW_OP_rot:{
      if (sp < 3) throw Exception();
      Address tmp = stack[sp - 3];
      stack[sp - 3] = stack[sp - 2];
      stack[sp - 2] = stack[sp - 1];
      stack[sp - 1] = tmp;
      break;
   }
   case DW_OP_pick:{
      int v = sp - stream.read1u() - 1;
      if (v < 0) throw Exception();
      stack[sp] = stack[v];
      sp++;
      break;
   }


/*  Special cases of reading to stack
 **************************************************
 */
   case DW_OP_addr:{
      Address v = cu->read_address(stream);
      stack[sp++] = v;
      break;
   }
   case DW_OP_nop: {
      break;
   }


/*  Special cases of arithmetic operations
 **************************************************
 */
   case DW_OP_abs:{
      if (sp < 1) throw Exception();
      Address v = stack[sp - 1];
      if (v < 0) v = -v;
      stack[sp - 1] = v;
      break;
   }
   case DW_OP_shr:{
      if (sp < 2) throw Exception();
      Address v1 = stack[sp - 2];
      Address v2 = stack[sp - 1];
      stack[sp - 2] = (uint64_t) v1 >> v2;
      sp--;
      break;
   }
   case DW_OP_plus_uconst:{
      if (sp < 1) throw Exception();
      Address v1 = stack[sp - 1];
      Address v2 = stream.read_uleb();
      stack[sp - 1] = v1 + v2;
      break;
   }

/*  Special cases of branching operations
 **************************************************
 */
   case DW_OP_bra:{
      if (sp < 1) throw Exception();
      int offset = stream.read2s();
      Address v = stack[sp];
      if (v != 0) {
         stream.seek(offset, BSTREAM_CUR);
      }
      sp--;
      break;
   }
   case DW_OP_skip:{
      int offset = stream.read2s();
      stream.seek(offset, BSTREAM_CUR);
      break;
   }


/*  Special cases of reading registers
 **************************************************
 */
   case DW_OP_fbreg:{
      Address offset = stream.read_sleb();
      stack[sp++] = frame_base + offset;
      break;
   }
   case DW_OP_bregx:{
      int rnum = stream.read_uleb();
      Address rval = process->get_reg(rnum);
      Address offset = stream.read_sleb();
      stack[sp++] = rval + offset;
      break;
   }


/*  Special cases of dereferencing ops
 **************************************************
 */
   case DW_OP_deref: {
      if (sp < 1) throw Exception();
      Address addr = stack[sp - 1];
      if (addr != 0) {
         Address val = process->read(addr, cu->getAddrSize());
         stack[sp - 1] = val;
      }
      break;
   }
   case DW_OP_deref_size: {
      if (sp < 1) throw Exception();
      int size = stream.read1u();
      Address addr = stack[sp - 1];
      if (addr != 0) {
         Address val = process->read(addr, size); 
         stack[sp - 1] = val;
      }
      break;
    }
   case DW_OP_xderef: {
      if (sp < 2) throw Exception();
      Address addr = stack[sp - 1];
      Address asi = stack[sp - 2];
      (void) asi; /* unused var */
      if (addr != 0) {
         stack[sp - 2] = process->read(addr, cu->getAddrSize());
      } else {
         stack[sp - 2] = addr;
      }
      sp--;
      break;
   }
   case DW_OP_xderef_size: {
      if (sp < 2) throw Exception();
      int size = stream.read1u();
      Address addr = stack[sp - 1];
      Address asi = stack[sp - 2];
      (void) asi; /* unused var */
      if (addr != 0) {
         stack[sp - 2] = process->read(addr, size);
      } else {
         stack[sp - 2] = addr;
      }
      sp--;
      break;
   }
  case DW_OP_piece: {
     if (sp < 1) throw Exception();
     Address v1 = stack[sp - 1];
     Address v2 = stream.read_uleb();
     DwLocType type;

     if (top_is_reg) {
        type = LOC_REG;
     } else {
        type = LOC_ADDR;
     }

     needs_push = false;
     result.add(type,v1,v2);
     break;
  }

/*  If you got this far your're not a register
 **************************************************
 */
  top_is_reg = false;

  } /* end of switch */
}
