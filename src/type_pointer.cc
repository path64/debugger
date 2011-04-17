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

file: type_pointer.cc
created on: Fri Aug 13 11:02:27 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#include "utils.h"
#include "process.h"
#include "dwf_cunit.h"
#include "junk_stream.h"

#include "type_pointer.h"

TypePointer::TypePointer(DwCUnit * cu, DIE * parent, Abbreviation * abbrev)
:  DIE(cu, parent, abbrev)
{
}

TypePointer::TypePointer(DwCUnit * cu, DIE * parent, int tag)
: DIE(cu, parent, tag)
{
}

TypePointer::~TypePointer()
{
}

void
TypePointer::print(EvalContext & ctx, int indent, int level)
{
   doindent(ctx, indent);
   switch (ctx.language & 0xff) {
   case DW_LANG_C89:
   case DW_LANG_C:
   case DW_LANG_C_plus_plus:
      print_declaration(ctx, this, indent);
      break;
   case DW_LANG_Fortran77:
   case DW_LANG_Fortran90:
   case DW_LANG_Fortran95:
      get_type()->print(ctx, indent, level + 1);
      ctx.os.print(", pointer");
      break;
   default:
      ctx.os.print("pointer to ");
      get_type()->print(ctx, indent, level + 1);
      break;
   }
}

bool
TypePointer::is_pointer()
{
   return true;
}

void
TypePointer::set_value(EvalContext & context, Value & addr, Value & value)
{
   context.process->set_value(addr, value, get_size());
}

void
TypePointer::print_value(EvalContext & context, Value & value, int indent)
{
   DIE *subtype = get_type();
   // ignore const and volatile
   while (subtype != NULL && (subtype->get_tag() == DW_TAG_const_type ||
			      subtype->get_tag() == DW_TAG_volatile_type)) {
      subtype = subtype->get_type();
   }

   switch (context.language & 0xff) {
   case DW_LANG_C89:
   case DW_LANG_C:
   case DW_LANG_C_plus_plus:
      if (value == 0) {
	 context.os.print("NULL");
      } else {
         /* XXX: replace me */
         JunkStream* js = reinterpret_cast<JunkStream*>(&context.os);
	 js->print_address(context, (int64_t) value);
      }
      if (context.process->is_active() && subtype != NULL && value != 0 &&
	  context.process->test_address(value.integer)) {
	 if (subtype->is_char()) {
	    std::string s = context.process->read_string(value);
	    context.os.print(" \"");
	    Utils::print_string(context, s);
	    context.os.print("\"");
	 }
      }
      break;
   case DW_LANG_Ada83:
      throw Exception("Ada is not a supported language");
      break;
   case DW_LANG_Cobol74:
   case DW_LANG_Cobol85:
      throw Exception("COBOL is not a supported language");

   case DW_LANG_Fortran77:
   case DW_LANG_Fortran95:
   case DW_LANG_Fortran90:{	// we want to print both the address
				// pointed to and the contents of it
	 // value is the address pointed to
	 Address addr = value;
	 if (addr == 0) {
	    context.os.print("null");
	    break;
	 }
	 if (context.process->get_int_opt(PRM_P_ADDR)) {
            /* XXX: replace me */
            JunkStream* js = reinterpret_cast<JunkStream*>(&context.os);
	    js->print_address(context, addr);
	 }
	 if (addr == 0) {
	    context.os.print(" => null");
	 } else if (context.show_reference && context.process->is_active()) {
	    context.os.print(" => ");
	    if (!context.process->test_address(value.integer)) {
	       context.os.print("<Bad address>");
	    } else {
	       DIE *subtype = get_type();	// type pointed to
	       Value v;
	       if (subtype->is_struct() || subtype->is_array()
		   || subtype->is_complex()) {
		  v = addr;
	       } else {
		  try {
		     v = context.process->read(addr, subtype->get_size());
		  }
		  catch(Exception e) {
		     context.os.print("<%s>", e.get().c_str());
		     break;
		  }
	       }
	       subtype->print_value(context, v);
	    }
	 }
	 break;
      }

   case DW_LANG_Pascal83:
      throw Exception("Pascal is not a supported language");
   case DW_LANG_Modula2:
      throw Exception("Modula2 is not a supported language");
   }
}

int
TypePointer::get_real_size(EvalContext & ctx)
{
   return cu->is_64bit()? 8 : 4;
}

