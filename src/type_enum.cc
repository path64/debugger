/*

   Copyright (c) 2004-2005 PathScale, Inc.  All rights reserved.
   PathDB is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation version 3

   PathDB is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with PathDB; see the file COPYING.  If not, write to the Free
   Software Foundation, 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

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
