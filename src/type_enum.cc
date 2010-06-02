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

file: type_enum.cc
created on: Fri Aug 13 11:02:27 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#include "utils.h"
#include "type_enum.h"
#include "process.h"
#include "junk_stream.h"

TypeEnum::TypeEnum (DwCUnit * cu, DIE * parent, Abbreviation * abbrev)
: DIE (cu, parent, abbrev)
{
}

TypeEnum::~TypeEnum ()
{
}

void
TypeEnum::print (EvalContext & ctx, int indent, int level)
{
   doindent (ctx, indent);
   ctx.os.print ("enum ");
   print_name_or_id (ctx, this);
   if (ctx.show_contents)
     {
	ctx.os.print (" {\n");
	bool comma = false;
	for (uint i = 0; i < children.size (); i++)
	  {
	     DIE *child = children[i];
	     if (child->get_tag () == DW_TAG_enumerator)
	       {
		  if (comma)
		    {
		       ctx.os.print (", ");
		    }
		  print_name (ctx, child);
		  AttributeValue & value =
		     child->getAttribute (DW_AT_const_value);
		  if (value.type != AV_NONE)
		    {
		       ctx.os.print (" = %d", (int) value);
		    }
		  comma = true;
	       }
	  }
	ctx.os.print ("\n");
	doindent (ctx, indent);
	ctx.os.print ("}");
     }
}

void
TypeEnum::print_value (EvalContext & context, Value & value, int indent)
{
   int cval = (int) value;
   if (context.fmt.code == 'n' || context.fmt.code == 's')
     {				// only for native and string
	for (uint i = 0; i < children.size (); i++)
	  {
	     DIE *child = children[i];
	     if (child->get_tag () == DW_TAG_enumerator)
	       {
		  int childvalue = child->getAttribute (DW_AT_const_value);
		  if (childvalue == cval)
		    {
		       context.os.print ("%s", child->get_name().c_str ());
		       return;
		    }
	       }
	  }
     }

   { /* XXX: replace me */
      JunkStream* js;
      js = reinterpret_cast<JunkStream*>(&context.os);
      js->print (context, cval);
   }
}

void
TypeEnum::set_value (EvalContext & context, Value & addr, Value & value)
{
   context.process->set_value(addr, value,get_size ());
}

void
TypeEnum::find_symbol (std::string name, Address pc,
		       std::vector < DIE * >&result, DIE * caller)
{
   check_loaded ();
   bool caseblind = is_case_blind ();

   for (uint i = 0; i < children.size (); i++)
     {
	DIE *child = children[i];
	int childtag = child->get_tag ();
	//println ("looking at child tag " + childtag)
	if (childtag == DW_TAG_enumerator)
	  {
	     std::string childname = child->getAttribute (DW_AT_name).str;
	     if (caseblind && Utils::toUpper (childname) == Utils::toUpper (name))
	       {
		  result.push_back (child);
	       }
	     else if (childname == name)
	       {
		  result.push_back (child);
	       }
	  }
     }

}
