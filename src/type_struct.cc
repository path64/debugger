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

file: type_struct.h
created on: Fri Aug 13 11:02:27 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#include "dwf_cunit.h"
#include "type_struct.h"
#include "arch.h"
#include "utils.h"
#include "process.h"
#include "dbg_stl.h"

TypeStruct::TypeStruct(DwCUnit * cu, DIE * parent, Abbreviation * abbrev)
:  DIE(cu, parent, abbrev), symbolsok(false)
{
}

TypeStruct::~TypeStruct()
{
}

void
TypeStruct::print_c(EvalContext & ctx, int indent, int level)
{
   /* with C, men are men */
   /* and structs are structs */
   ctx.os.print("struct ");
   print_name_or_id(ctx, this);

   /* don't always need guts */
   if (!ctx.show_contents)
      return;

   /* do recursive calls for children */
   ctx.os.print(" {\n");
   for (unsigned i = 0; i < children.size(); i++) {
      DIE *child = children[i];
      child->print(ctx, indent + 4, level + 1);
      ctx.os.print(";\n");
   }

   /* close off and print */
   doindent(ctx, indent);
   ctx.os.print("}");
}

/* XXX: I don't like the number of arguments this has, but it will be
 * fixed as soon as indent is moved inside EvalContext */
void
TypeStruct::print_cxx_access(EvalContext & ctx, int indent, int level,
				DIE * child, int &cur_access)
{

   /* get the new access level */
   AttributeValue av = child->getAttribute(DW_AT_accessibility);
   int chd_access = av.integer;

   /* set default for access */
   if (av.type == AV_NONE) {
      chd_access = DW_ACCESS_public;
   }

   /* only print differences */
   if (chd_access == cur_access) {
      return;
   }

   /* indent and print */
   doindent(ctx, indent + 2);
   switch (chd_access) {
   case DW_ACCESS_public:
      ctx.os.print("public:\n");
      break;
   case DW_ACCESS_protected:
      ctx.os.print("protected:\n");
      break;
   case DW_ACCESS_private:
      ctx.os.print("private:\n");
      break;
   }

   /* reset current */
   cur_access = chd_access;
}

void
TypeStruct::print_cxx(EvalContext & ctx, int indent, int level)
{
   /* without contents just do name */
   if (!ctx.show_contents) {
      print_name_or_id(ctx, this);
      return;
   }

   /* otherwise print full name */
   ctx.os.print("class ");
   print_name_or_id(ctx, this);

   /* first print any derivations */
   bool first_elem = true;
   for (uint i = 0; i < children.size(); i++) {
      DIE *child = children[i];
      if (child->is_inheritance()) {
	 /* print a separator */
	 if (first_elem) {
	    ctx.os.print(" : ");
	    first_elem = false;
	 } else {
	    ctx.os.print(", ");
	 }

	 /* print deriving class */
	 child->print(ctx, indent, level);
      }
   }

   /* open structure tag */
   ctx.os.print(" {\n");

   /* recursively print children */
   int cur_access = 0;
   for (uint i = 0; i < children.size(); i++) {
      DIE *child = children[i];

      /* some children just fall out */
      if (!child->is_printable())
	 continue;

      /* print public, private etc */
      print_cxx_access(ctx, indent, level, child, cur_access);

      /* print the name of child */
      child->print(ctx, indent + 4, level + 1);
      ctx.os.print(";\n");
   }

   /* close off structure */
   doindent(ctx, indent);
   ctx.os.print("}");
}

void
TypeStruct::print_fort(EvalContext & ctx, int indent, int level)
{
   /* always print `type' */
   ctx.os.print("type ");

   /* respect show_contents with nesting */
   if (!ctx.show_contents || level > 0) {
      ctx.os.print("(");
      print_name_or_id(ctx, this);
      ctx.os.print(") ");
      return;
   }

   /* print out my type name */
   print_name_or_id(ctx, this);
   ctx.os.print("\n");

   /* recursively print all children */
   for (uint i = 0; i < children.size(); i++) {
      DIE *child = children[i];
      if (child->get_tag() == DW_TAG_member) {
	 child->print(ctx, indent + 4, level + 1);
	 ctx.os.print("\n");
      }
   }

   /* close off type */
   doindent(ctx, indent);
   ctx.os.print("end type ");
   print_name_or_id(ctx, this);
}

void
TypeStruct::print(EvalContext & ctx, int indent, int level)
{
   check_loaded();
   doindent(ctx, indent);
   switch (ctx.language & 0xff) {
   case DW_LANG_C89:
   case DW_LANG_C:
      print_c(ctx, indent, level);
      break;
   case DW_LANG_C_plus_plus:
      print_cxx(ctx, indent, level);
      break;
   case DW_LANG_Fortran77:
   case DW_LANG_Fortran90:
      print_fort(ctx, indent, level);
      break;
   default:
      print_c(ctx, indent, level);
   }
}

bool
TypeStruct::compare(EvalContext & context, DIE * die, int flags)
{
   /* must point to same DIE in memory */
   return this == die;
}

bool
TypeStruct::is_struct()
{
   return true;
}

int
TypeStruct::get_size()
{
   return cu->getAddrSize();
}

// anonymous unions are problematic.  The only way to deal with them is to
// tell the expression handler that the member is inside one.  This is
// done
// with an out-of-band exception
DIE *
TypeStruct::find_member(std::string & name)
{
   check_loaded();
   bool caseblind = is_case_blind();

   for (uint i = 0; i < children.size(); i++) {
      DIE *child = children[i];
      if (child->get_tag() == DW_TAG_member
	  || child->get_tag() == DW_TAG_subprogram) {
	 std::string childname = child->getAttribute(DW_AT_name);
	 if (caseblind
	     && Utils::toUpper(childname) == Utils::toUpper(name)) {
	    return child;
	 } else if (childname == name) {
	    return child;
	 }
	 if (childname == "") {	// possible anonymous union
	    if (child->get_type()->get_tag() == DW_TAG_union_type) {
	       DIE *sym = child->get_type()->find_member(name);
	       if (sym != NULL) {
		  throw child;
	       }
	    }
	 }
      }
   }

   std::vector < DIE * >bases;
   find_bases(bases);

   // if the name contains a :: then the first part of (before the ::) is
   // the name of a base class.  We need to extract this and find the base
   // class with that name.  The reference parameter 'name' is then
   // modified
   // to be the suffix
   std::string::size_type coloncolon = name.find("::");
   if (coloncolon != std::string::npos) {
      std::string basename = name.substr(0, coloncolon);
      std::string name1 = name.substr(coloncolon + 2);	// modify name
      if (basename == get_name()) {
	 name = name1;
	 return this->find_member(name);
      }
      for (uint i = 0; i < bases.size(); i++) {
	 if (bases[i]->get_type()->get_name() == basename) {
	    DIE *member = bases[i]->get_type()->find_member(name1);
	    if (member != NULL) {
	       name = name1;
	       throw bases[i];	// if member is in the base tell the
				// expression handler
	    } else {
	       return NULL;	// member not in base
	    }
	 }
      }
   }
   // if this is a derived type, look in base type
   // like anonymous unions, base inheritance needs to be able to locate
   // the base
   // address.  This is inside the DW_TAG_inheritance DIE.  We have to
   // tell the
   // expression handler that we have passed through a base class, so we
   // throw
   // an out-of-band exception that is caught by the expression parser.
   for (uint i = 0; i < bases.size(); i++) {
      DIE *member = bases[i]->get_type()->find_member(name);
      if (member != NULL) {
	 throw bases[i];	// if member is in the base tell the
				// expression handler
      }
   }
   return NULL;
}

// find a member given it's DIE.  This wouldn't be necessary if it wasn't
// for the use of virtual base classes and anonymous unions where we
// need to calculate the address of the object
DIE *
TypeStruct::find_member(DIE * die)
{
   check_loaded();
   for (uint i = 0; i < children.size(); i++) {
      DIE *child = children[i];
      if (die == child) {
	 return die;
      }
      std::string childname = child->getAttribute(DW_AT_name);
      if (childname == "") {	// possible anonymous union
	 if (child->get_type()->get_tag() == DW_TAG_union_type) {
	    DIE *sym = child->get_type()->find_member(die);
	    if (sym != NULL) {
	       throw child;
	    }
	 }
      }
   }
   // if this is a derived type, look in base type
   // like anonymous unions, base inheritance needs to be able to locate
   // the base
   // address.  This is inside the DW_TAG_inheritance DIE.  We have to
   // tell the
   // expression handler that we have passed through a base class, so we
   // throw
   // an out-of-band exception that is caught by the expression parser.
   std::vector < DIE * >bases;
   find_bases(bases);
   for (uint i = 0; i < bases.size(); i++) {
      DIE *member = bases[i]->get_type()->find_member(die);
      if (member != NULL) {
	 throw bases[i];	// if member is in the base tell the
				// expression handler
      }
   }
   return NULL;
}

// find all the members with the given name.  This is used for function
// overloads
// this will not be used in fortran so no caseblind search is necessary
void
TypeStruct::find_member(std::string & name,
			   std::vector < DIE * >&result)
{
   check_loaded();
   for (uint i = 0; i < children.size(); i++) {
      DIE *child = children[i];
      if (child->get_tag() == DW_TAG_member
	  || child->get_tag() == DW_TAG_subprogram) {
	 std::string childname = child->getAttribute(DW_AT_name);
	 if (childname == name) {
	    result.push_back(child);
	 } else if (childname == "") {	// possible anonymous union
	    if (child->get_type()->get_tag() == DW_TAG_union_type) {
	       child->get_type()->find_member(name, result);
	    }
	 }
      }
   }
   if (result.size() != 0) {
      return;
   }
   // if this is a derived type, look in base type
   std::vector < DIE * >bases;
   find_bases(bases);

   // if the name contains a :: then the first part of (before the ::) is
   // the name of a base class.  We need to extract this and find the base
   // class with that name.  The reference parameter 'name' is then
   // modified
   // to be the suffix
   std::string::size_type coloncolon = name.find("::");
   if (coloncolon != std::string::npos) {
      std::string basename = name.substr(0, coloncolon);
      std::string name1 = name.substr(coloncolon + 2);	// modify name
      if (basename == get_name()) {
	 name = name1;
	 return this->find_member(name, result);
      }
      for (uint i = 0; i < bases.size(); i++) {
	 if (bases[i]->get_type()->get_name() == basename) {
	    bases[i]->get_type()->find_member(name1, result);
	 }
	 if (result.size() != 0) {
	    name = name1;
	    return;
	 }
      }
   } else {
      for (uint i = 0; i < bases.size(); i++) {
	 bases[i]->get_type()->find_member(name, result);
      }
   }
}

DIE *
TypeStruct::find_scope(std::string name)
{
   std::vector < DIE * >matches;
   check_loaded();
   bool caseblind = is_case_blind();
   // first find all matches
   for (uint i = 0; i < children.size(); i++) {
      DIE *child = children[i];
      int childtag = child->get_tag();
      // println ("looking at child tag " + childtag)
      if (childtag == DW_TAG_subprogram
	  || childtag == DW_TAG_structure_type
	  || childtag == DW_TAG_union_type || childtag == DW_TAG_typedef) {
	 std::string childname = child->getAttribute(DW_AT_name).str;
	 if (caseblind
	     && Utils::toUpper(childname) == Utils::toUpper(name)) {
	    matches.push_back(child);
	 } else if (childname == name) {
	    matches.push_back(child);
	 }
      }
   }

   // there may be more than one die with the same name.  For example, a
   // class name
   // and a constructor within that class.  We prefer class names over
   // subprograms
   // because that's probably what the user wanted

   if (matches.size() == 0) {
      return parent->find_scope(name);
   }

   if (matches.size() == 1) {
      return matches[0];
   }
   // first look for a struct/union
   for (uint i = 0; i < matches.size(); i++) {
      if (matches[i]->is_struct()) {
	 return matches[i];
      }
   }

   return matches[0];		// just return the first match
}

// look for symbol in structure.  This will be used in C++ for class
// scope.  
// XXX: do we need to do caseblind for fortran? Will this be called from
// fortran?
void
TypeStruct::find_symbol(std::string name, Address pc,
			   std::vector < DIE * >&result, DIE * caller)
{
   check_loaded();
   if (!symbolsok) {
      for (uint i = 0; i < children.size(); i++) {
	 DIE *child = children[i];
	 int childtag = child->get_tag();
	 // println ("looking at child tag " + child.get_tag())
	 if (childtag == DW_TAG_member || childtag == DW_TAG_subprogram) {
	    std::string childname = child->getAttribute(DW_AT_name).str;
	    symbols.insert(MultiDIEMap::value_type(childname, child));
	 } else if (childtag == DW_TAG_enumeration_type) {
	    enumerations.push_back(child);
	 }
      }
      symbolsok = true;
   }

   std::pair < MultiDIEMap::iterator, MultiDIEMap::iterator > sym =
       symbols.equal_range(name);
   while (sym.first != sym.second) {
      result.push_back((*sym.first).second);
      sym.first++;
   }
   if (!result.empty()) {
      return;
   }
   // search the enumerations
   for (uint i = 0; i < enumerations.size(); i++) {
      DIE *die = enumerations[i];
      die->find_symbol(name, pc, result, this);
      if (!result.empty()) {
	 return;
      }
   }

   // if this is a derived type, look in base type
   std::vector < DIE * >bases;
   find_bases(bases);
   for (uint i = 0; i < bases.size(); i++) {
      bases[i]->get_type()->find_symbol(name, pc, result, this);
      if (!result.empty()) {
	 return;
      }
   }

   // not a child, look in the parent
   parent->find_symbol(name, pc, result, this);
}

// there can be multiple operators with the same name in a class.  Return
// all that
// match the name

void
TypeStruct::find_operator(std::string name,
			     std::vector < DIE * >&result)
{
   for (uint i = 0; i < children.size(); i++) {
      DIE *child = children[i];
      int childtag = child->get_tag();
      // println ("looking at child tag " + child.get_tag())
      if (childtag == DW_TAG_subprogram) {
	 std::string childname = child->getAttribute(DW_AT_name).str;
	 if (childname == name) {
	    result.push_back(child);
	 }
      }
   }

   // if this is a derived type, look in base type
   // XXX: what if the operator is virtual and overridden?
   std::vector < DIE * >bases;
   find_bases(bases);
   for (uint i = 0; i < bases.size(); i++) {
      bases[i]->get_type()->find_operator(name, result);
   }
}

// return the DW_TAG_inheritance DIEs
void
TypeStruct::find_bases(std::vector < DIE * >&result)
{
   for (uint i = 0; i < children.size(); i++) {
      DIE *child = children[i];
      if (child->get_tag() == DW_TAG_inheritance) {
	 result.push_back(child);
      }
   }
}

// this is like Value::set() in expr.cc except it can copy memory.  Also,
// the
// meaning of VALUE_INTEGER is different in that it represents an address
// in 
// memory, not an integral value
void
TypeStruct::set_value(EvalContext & context, Value & addr,
			 Value & value)
{
   if (addr.type == VALUE_REG) {
      // destination is a register
      if (value.type == VALUE_REG) {
	 Address contents = context.process->get_reg(value.integer);
	 context.process->set_reg(addr.integer, contents);
      } else {
	 Address contents =
	     context.process->read(value.integer, get_real_size(context));
	 context.process->set_reg(addr.integer, contents);
      }
   } else {
      Address dest = addr.integer;
      if (value.type == VALUE_REG) {
	 // dest is in memory, source is in a register
	 Address contents = context.process->get_reg(value.integer);
	 context.process->write(dest, contents, get_real_size(context));
      } else {
	 Address src = value.integer;
	 std::string tmp =
	     context.process->read_string(src, get_real_size(context));
	 context.process->write_string(dest, tmp);
      }
   }
}


// if the structure is small and in a register then we need to push it
// onto the runtime stack of the process.  This is because there is no way
// to extract bits of a register in the routines that expect things to be
// in memory.
// XXX: let's try to fix that

void
TypeStruct::print_value(EvalContext & context, Value & value,
			   int indent)
{
   if (!context.process->is_active()) {
      throw Exception("A running process is required for this operation");
   }
   Architecture *arch = context.process->get_arch();
   check_loaded();

   // save the contents of the struct to memory if it is in a register.
   Address oldsp = context.process->get_reg("sp");
   Address sp = oldsp;
   
   /* this means that value.integer is a register number */
   if (value.type == VALUE_REG) {
      Address v = context.process->get_reg(value.integer);
      sp = arch->stack_space(context.process, get_real_size(context));
      context.process->write(sp, v, get_real_size(context));
      value.type = VALUE_INTEGER;
      value.integer = sp;
   }
   EvalContext childcontext = context;
   bool pretty = context.process->get_int_opt(PRM_P_PRETTY) && context.pretty;

   switch (context.language & 0xff) {
   case DW_LANG_C89:
   case DW_LANG_C:{
	 context.os.print("{");
	 if (pretty) {
	    context.os.print("\n");
	    indent += 2;
	 }
	 bool comma = false;
	 for (uint i = 0; i < children.size(); i++) {
	    DIE *child = children[i];
	    if (child->get_tag() == DW_TAG_member) {
	       if (comma) {
		  if (pretty) {
		     context.os.print("\n");
		  } else {
		     context.os.print(", ");
		  }
	       }
	       if (pretty) {
		  doindent(context, indent);
	       }
	       std::string childname = child->getAttribute(DW_AT_name).str;
	       if (childname != "") {
		  context.os.print("%s = ", childname.c_str());
	       }
	       Value v = child->evaluate(childcontext,value);
	       if (child->get_type()->is_real()) {
		  if (child->get_type()->get_real_size(context) == 4) {
		     v.real = (double) (*(float *) &v.real);	// convert 
								// to
								// double
		  }
	       }
	       child->get_type()->print_value(context, v, indent + 2);
	       comma = true;
	    }
	 }
	 if (pretty) {
	    context.os.print("\n");
	    indent -= 2;
	    doindent(context, indent);
	 }
	 context.os.print("}");
	 break;
      }
   case DW_LANG_Ada83:
      throw Exception("Ada is not a supported language");
   case DW_LANG_C_plus_plus:{
	 if (stl::print_struct(context, value, this)) {
	    return;
	 }

	 context.os.print("{");
	 if (pretty) {
	    context.os.print("\n");
	    indent += 2;
	 }
	 bool comma = false;
	 // we don't want references defererenced inside a structure print
	 bool old = context.show_reference;
	 context.show_reference = false;
	 for (uint i = 0; i < children.size(); i++) {
	    DIE *child = children[i];
	    if (child->get_tag() == DW_TAG_member
		|| child->get_tag() == DW_TAG_inheritance) {
	       if (comma) {
		  if (pretty) {
		     context.os.print("\n");
		  } else {
		     context.os.print(", ");
		  }
	       }
	       if (pretty) {
		  doindent(context, indent);
	       }
	       std::string childname;
	       // for a base type, the name is in the struct, not the
	       // member
	       if (child->get_tag() == DW_TAG_inheritance) {
		  childname =
		      child->get_type()->getAttribute(DW_AT_name).str;
	       } else {
		  childname = child->getAttribute(DW_AT_name).str;
	       }
	       if (childname != "") {
		  context.os.print("%s = ", childname.c_str());
	       }
	       Value v = child->evaluate(childcontext,value);
	       if (child->get_type()->is_real()) {
		  if (child->get_type()->get_real_size(context) == 4) {
		     v.real = (double) (*(float *) &v.real);	// convert 
								// to
								// double
		  }
	       }
	       child->get_type()->print_value(context, v, indent + 2);
	       comma = true;
	    }
	 }
	 context.show_reference = old;
	 if (pretty) {
	    context.os.print("\n");
	    indent -= 2;
	    doindent(context, indent);
	 }
	 context.os.print("}");
	 break;
      }
   case DW_LANG_Cobol74:
   case DW_LANG_Cobol85:
      throw Exception("COBOL is not a supported language");
   case DW_LANG_Fortran77:
   case DW_LANG_Fortran90:{	// fortran types print like C ones as
				// there is no better alternative
	 context.os.print("{");
	 if (pretty) {
	    context.os.print("\n");
	    indent += 2;
	 }
	 bool comma = false;
	 for (uint i = 0; i < children.size(); i++) {
	    DIE *child = children[i];
	    if (child->get_tag() == DW_TAG_member) {
	       if (comma) {
		  if (pretty) {
		     context.os.print("\n");
		  } else {
		     context.os.print(", ");
		  }
	       }
	       if (pretty) {
		  doindent(context, indent);
	       }
	       std::string childname = child->getAttribute(DW_AT_name).str;
	       context.os.print("%s = ", childname.c_str());
	       Value v = child->evaluate(childcontext,value);
	       if (child->get_type()->is_real()) {
		  if (child->get_type()->get_real_size(context) == 4) {
		     v.real = (double) (*(float *) &v.real);	// convert 
								// to
								// double
		  }
	       }
	       child->get_type()->print_value(context, v, indent + 2);
	       comma = true;
	    }
	 }
	 if (pretty) {
	    context.os.print("\n");
	    indent -= 2;
	    doindent(context, indent);
	 }
	 context.os.print("}");
	 break;
      }
   case DW_LANG_Pascal83:
      throw Exception("Pascal is not a supported language");
   case DW_LANG_Modula2:
      throw Exception("Modula2 is not a supported language");
   }

   // restore stack pointer
   if (sp != oldsp) {
      context.process->set_reg("sp", oldsp);
   }
}

// get the address of the virtual table for a structure.  The information
// is all
// encoded in attributes of the DIE - including the location of the base
// classes.

Address
TypeStruct::get_virtual_table(EvalContext & context, Address thisptr)
{
   if (thisptr == 0) {
      return 0;
   }
   std::vector < DIE * >bases;

   for (uint i = 0; i < children.size(); i++) {
      DIE *child = children[i];
      int childtag = child->get_tag();
      if (childtag == DW_TAG_inheritance) {
	 bases.push_back(child->get_type());
      } else if (childtag == DW_TAG_member) {
	 std::string childname = child->get_name();
	 if (childname.size() >= 6
	     && strncmp(childname.c_str(), "_vptr.", 6) == 0) {
	    AttributeValue loc =
		child->getAttribute(DW_AT_data_member_location);
	    if (loc.type == AV_NONE) {
	       throw
		   Exception
		   ("Unable to get the address of the virtual table");
	    }

	    DwLocExpr addr = cu->evaluate_location(cu, context.fb,
                loc, context.process, thisptr);

	    Value v = addr.getValue(context.process, child->get_type()->get_size());
	    return v;
	 }
      }
   }

   for (uint i = 0; i < bases.size(); i++) {
      DIE *base = bases[i];
      Address savedthis = thisptr;
      // need to work out the 'this' pointer for the base class
      AttributeValue loc = base->getAttribute(DW_AT_data_member_location);
      if (loc.type != AV_NONE) {	// no data_member_location
					// attribute means offset 0

	 DwLocExpr addr = cu->evaluate_location(cu,
            context.fb, loc, context.process, thisptr);

	 thisptr = addr.getAddress();
      }
      Address addr = bases[i]->get_virtual_table(context, thisptr);
      if (addr != 0) {
	 return addr;
      }
      thisptr = savedthis;
   }

   return 0;
}


DIE *
TypeStruct::get_dynamic_type(EvalContext & context, Address thisptr)
{
   if (!context.process->get_int_opt(PRM_P_OBJECT)) {
      return this;
   }
   Address vtbl = get_virtual_table(context, thisptr);
   if (vtbl == 0) {
      return this;
   }
   Location loc = context.process->lookup_address(vtbl);
   std::string name;
   if (loc.get_symname()!= "") {
      name = loc.get_symname();
      std::string::size_type space = name.rfind(' ');
      if (space != std::string::npos) {
	 name = name.substr(space + 1);
	 DIE *type = cu->find_struct(name);
	 if (type != NULL) {
	    return type;
	 }
      }
   }
   return this;
}
