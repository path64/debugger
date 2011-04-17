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

file: type_base.cc
created on: Fri Aug 13 11:02:26 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#include <ctype.h>
#include "process.h"
#include "arch.h"
#include <sys/stat.h>
#include "utils.h"

#include "dbg_stl.h"
#include "dwf_names.h"
#include "dwf_cunit.h"


// given an attribute value that should contain a pointer to a DIE, make it that
// it does.  The DIE may or may not be loaded already.  If it is, the ptr
// will contain the actual DIE pointer.  If it isn't loaded, the ptr will contain
// the offset into the dwarf_info section that contains the die.  This offset should
// be in the range of a 'lazy DIE' (one that has not been loaded yet).  That lazy
// DIE should be loaded.  The act of loading it will cause the attribute's value to
// be fixed up with the DIE pointer.

static DIE *
dereference_die_reference(DwCUnit * cu, Attribute * attr)
{
   //printf ("dereferencing DIE reference\n") ;
   AttributeValue & ptr = attr->value;
   if (ptr.type == AV_DIE) {
      if ((DIE *) ptr == NULL) {
	 printf("DIE *attribute has not been fixed up\n");
      }
      return ptr;
   }
   if (ptr.type != AV_INTEGER) {	// this must be an offset
      printf("DIE *attribute has an illegal type");
      throw Exception("bad DIE reference");
   }
   Offset offset = ptr;
   DIE *lazy = cu->find_lazy_die(offset);
   if (lazy == NULL) {
      return NULL;
      //printf("Cannot find lazy DIE at offset %llx\n",
      //	     (unsigned long long) offset);
      //throw Exception("bad DIE reference");
   }
   lazy->check_loaded();	// load it if necessary
   if (attr->value.type != AV_DIE) {
      DIE *v = cu->get_die(attr, offset);
      if (v == NULL) {
	 printf
	     ("Lazy DIE did not set attribute value (ptr=%llx)\n",
	      (unsigned long long) ptr.integer);
	 throw Exception("bad DIE reference");
      } else {
	 return v;
      }
   }
   return attr->value;
}

// a DIE is being referenced.  If it exists, return a pointer to it.  If it doesn't
// return the offset to it (a fixup will have been added in get_die)

static AttributeValue
get_die_reference(DwCUnit * cu, Attribute * attr, Offset offset)
{
   DIE *die = cu->get_die(attr, offset);
   if (die == NULL) {
      return offset;
   }
   return die;
}

DIE::DIE(DwCUnit * cu, DIE * parent, Abbreviation * abbrev)
:  cu(cu),
parent(parent),
abbrev(abbrev),
id(0), tag(0), children_offset(0), more_info(NULL), definition(NULL)
{
}

DIE::DIE(DwCUnit * cu, DIE * parent, int tag)
:  cu(cu),
parent(parent),
abbrev(NULL),
id(0), tag(tag), children_offset(0), more_info(NULL), definition(NULL)
{
}

DIE::~DIE()
{
   // don't delete the children as these are done by the DwCUnit
   for (AttributeMap::iterator i = attributes.begin();
	i != attributes.end(); ++i) {
      delete i->second;
   }
}

int
DIE::get_tag()
{
   if (abbrev == NULL) {
      return tag;
   } else {
      return abbrev->getTag();
   }
}

void
DIE::doindent(EvalContext & ctx, int indent)
{
   for (int i = 0; i < indent; i++) {
      ctx.os.print(" ");
   }
}

void
DIE::doindent(int indent)
{
   for (int i = 0; i < indent; i++) {
      std::cout << " ";
   }
}

bool DIE::is_case_blind()
{
   int lang = cu->get_language();
   return lang == DW_LANG_Fortran77 || lang == DW_LANG_Fortran90 || lang == DW_LANG_Fortran95;
}

// this is like reading a normal Attribute, except we need to know what the offset is
Offset
DIE::read_sibling(DwCUnit * cu, AttributeAbbreviation * attrabbrev,
		      Attribute * attr, BStream & stream)
{
   int form = attrabbrev->getForm();
   Offset offset;
   switch (form) {
   case DW_FORM_ref1:{
	 offset = stream.read1u();
	 attr->value = get_die_reference(cu, attr, offset);
	 break;
      }
   case DW_FORM_ref2:{
	 offset = stream.read2u();
	 attr->value = get_die_reference(cu, attr, offset);
	 break;
      }
   case DW_FORM_ref4:{
	 offset = stream.read4u();
	 attr->value = get_die_reference(cu, attr, offset);
	 break;
      }
   case DW_FORM_ref8:{
	 offset = stream.read8u();
	 attr->value = get_die_reference(cu, attr, offset);
	 break;
      }
   default:
      throw Exception("unexpected form for sibling attribute");
   }
   return offset;
}

// I want to be able to skip structures as they speed up C++ loading a lot
// but if I do this there is no way to load the subprograms inside them

bool DIE::is_skippable()
{
   // fortran can have nested subprograms and we need to be able to see them in order
   // to set breakpoints.  So for fortran, we load all the dies
   // for example.  If MAIN__ has a subroutine in it called AN.in.EXP1.  If we
   // type:
   // pathdb> b an.in.exp1
   // this will be looked up in the elf and the address obtained.  But if we had
   // skipped the loading of the children of MAIN__ then the address will
   // point to somwehere in MAIN__ because we haven't registered the DIE
   // for the AN.in.EXP1 subprogram
   // 
   // XXX: if this turns out to be slow for a large fortran program then
   // I'll have to revisit this decision
   int language = cu->get_language();
   if (language == DW_LANG_Fortran77 || language == DW_LANG_Fortran90 || language == DW_LANG_Fortran95) {
      return false;
   }
   switch (get_tag()) {
   case DW_TAG_subprogram:
   case DW_TAG_structure_type:	// XXX: want to do these
   case DW_TAG_class_type:
   case DW_TAG_union_type:
      return true;
   }
   return false;
}

void
 DIE::read(DwCUnit * cu, BStream & stream, bool readall)
{
   id = stream.offset();
   // read the attribute values
   int nattrs = abbrev->getNumAttributes();
   bool sibling_present = false;
   Offset sibling = 0;
   for (int i = 0; i < nattrs; i++) {;
      AttributeAbbreviation * attrabbrev = abbrev->getAttribute(i);
      Attribute * attr;
      
      if (attrabbrev->getTag() == DW_AT_specification ||
          attrabbrev->getTag() == DW_AT_abstract_origin) {
         attr = new FixupAttribute(attrabbrev->getForm(), this);
      } else {
         attr = new Attribute (attrabbrev->getForm());
      }

      int attrtag = attrabbrev->getTag();
      if (attrtag == DW_AT_sibling) {
	 sibling_present = true;
	 sibling = read_sibling(cu, attrabbrev, attr, stream) + cu->start_offset;
      } else if (attrtag == DW_AT_specification
		 || attrtag == DW_AT_abstract_origin) {
	 attr->value = attrabbrev->read(cu, attr, stream);
	 // if the referenced DIE hasn't been read yet, a fixup is added for it.
	 if (attr->value.type == AV_DIE) {
	    DIE * d = attr->value;
	    if (d != NULL) {
	       d->set_more_info(this);
	    }
	 }
      } else {
	 attr->value = attrabbrev->read(cu, attr, stream);
      }
      //std::cout << "attribute value (type:" << attr->value.type << ", value: " << attr->value << ")\n" ;
      attributes[attrtag] = attr;
   }

   // read the children
   if (abbrev->has_children()) {
      // when we read this for the first time, we want to skip as much information as
      // possible.  For a subprogram, we will want to skip the children (if we can).  These
      // can be read later when they are needed

      if (!readall && sibling_present && is_skippable()) {
	 children_offset = stream.offset();
	 cu->add_lazy_die(this, children_offset - cu->start_offset,
			  sibling - cu->start_offset);

	 //printf ("%x skipping to sibling at offset %x\n", id, sibling) ;
	 stream.seek(sibling);	// move to the sibling
	 return;
      }

      for (;;) {
	 int offset = stream.offset();
	 int64_t childabbrevnum = stream.read_uleb();
	 Abbreviation * childabbrev = cu->getAbbreviation(childabbrevnum);
	 if (childabbrev->getTag() == 0) {
	    break;
	 }
	 DIE * child = cu->make_die(cu, this, childabbrev);
	 if (child != NULL) {
	    child->read(cu, stream);
	    cu->add_die(offset - cu->start_offset, child);
	    children.push_back(child);
	 }
      }
   }
}

void
 DIE::check_loaded()
{
   if (children_offset > 0) {
      BStream & stream = cu->get_stream();
      read(stream);
      children_offset = 0;
   }
}


// this is called when we actually want to read the children.  It will only be called
// when the children have been skipped initially.  The stream needs to be seeked to
// children_offset and the children read.

void
 DIE::read(BStream & stream)
{
   //std::cout << "reading children for die " << id << "\n" ;
   stream.seek(children_offset);

   for (;;) {
      int
	  offset = stream.offset();
      int64_t
	  childabbrevnum = stream.read_uleb();
      Abbreviation *
	  childabbrev = cu->getAbbreviation(childabbrevnum);
      if (childabbrev->getTag() == 0) {
	 break;
      }
      DIE *
	  child = cu->make_die(cu, this, childabbrev);
      if (child != NULL) {
	 child->read(cu, stream, true);	// read the whole tree
	 cu->add_die(offset - cu->start_offset, child);
	 children.push_back(child);
      }
   }
}

void
 DIE::dumpAttributes(DwCUnit * cu, int indent)
{
   /*
    * XXX: implement me 
    */
}

void
 DIE::dumpChildren(int indent)
{
   /*
    * XXX: implement me 
    */
}

void
 DIE::addChild(DIE * child)
{
   children.push_back(child);
}

Attribute *
DIE::find_attribute(int tag)
{
   AttributeMap::iterator attr = attributes.find(tag);
   if (attr == attributes.end()) {
      return NULL;
   }
   return attr->second;
}

AttributeValue no_value;

AttributeValue & DIE::getAttribute(int tag, bool fullsearch, bool stophere)
{
   Attribute * attr = find_attribute(tag);
   if (attr == NULL) {

      // if the attribute is not found, look for it using the DW_AT_specification or
      // DW_AT_abstract_origin

      if (fullsearch && !stophere) {
	 Attribute * more = find_attribute(DW_AT_abstract_origin);
	 if (more == NULL) {
	    more = find_attribute(DW_AT_specification);
	 }

	 if (more != NULL) {
	    //printf ("dereferencing die %llx\n", id) ;
	    DIE * origin = dereference_die_reference(cu, more);
	    if (origin != NULL) {
	       return origin->getAttribute(tag, fullsearch);
	    } else {
               return no_value;
            }
	 }
      }
      // if there is more info, look there
      if (fullsearch && more_info != NULL) {
	 return more_info->getAttribute(tag, fullsearch, true);
      } else {
	 return no_value;
      }

   }
   // if the attribute references another die, make sure it is loaded
   switch (attr->form) {
   case DW_FORM_ref1:
   case DW_FORM_ref2:
   case DW_FORM_ref4:
   case DW_FORM_ref8:
      dereference_die_reference(cu, attr);
      break;
   }
   return attr->value;
}

void
DIE::addAttribute(int tag, const AttributeValue & value)
{
   Attribute* attr;

   if (tag == DW_AT_specification ||
       tag == DW_AT_abstract_origin) {
       attr = new FixupAttribute(0, this);
   } else {
       attr = new Attribute (0);
   }

   attributes[tag] = attr;
   attr->value = value;
}

bool DIE::is_printable()
{
   AttributeValue
       av = getAttribute(DW_AT_name);

   if (is_artificial())
      return false;

   switch (get_tag()) {
   /* no anonymous data members */
   case DW_TAG_member:
      return (av.type != AV_NONE);
   case DW_TAG_variable:
      return (av.type != AV_NONE);

   /* anonymous types are okay */
   case DW_TAG_structure_type:
   case DW_TAG_class_type:
   case DW_TAG_union_type:
   case DW_TAG_typedef:
   case DW_TAG_subprogram:
      return true;

   /* no printing others  */
   default: return false;
   }
}

std::string DIE::get_name()
{
   // the linkage name overrides the simple name
   AttributeValue & name = getAttribute(DW_AT_MIPS_linkage_name);
   if (name.type != AV_NONE) {
      return name.str;
   }
   AttributeValue & name1 = getAttribute(DW_AT_name);
   if (name1.type != AV_NONE) {
      return name1.str;
   }

   return "<unknown>";
   //throw Exception ("Unnamed top level die ") ;
}

std::vector <DIE*>&DIE::getChildren()
{
   return children;
}


Value DIE::evaluate(EvalContext & context)
{
   return Value(0);
}

Value DIE::evaluate(EvalContext & context, Value & base)
{
   return Value(0);
}

void DIE::print_value(EvalContext & context, Value & value, int indent)
{
}

void DIE::set_value(EvalContext & context, Value & addr, Value & value)
{
   throw Exception("Can't set the value of this type.");
}

void DIE::find_symbol(std::string name, Address pc,
		  std::vector < DIE * >&result, DIE * caller)
{
}

// only called if the variable is local (is_local_var() returns true)
int DIE::get_local_offset()
{
   AttributeValue locatt = getAttribute(DW_AT_location);
   if (locatt.type != AV_BLOCK) {
      throw Exception("variable is not local");
   }
   BStream
   stream(locatt, cu->do_swap());
   stream.read1u();
   return stream.read_sleb();
}


// mapping function to select only the first DIE from a set
DIE *
DIE::find_symbol(std::string name, Address pc)
{
   std::vector < DIE * >r;
   find_symbol(name, pc, r);
   if (r.size() == 0) {
      return NULL;
   }
   return r[0];
}


DIE *
DIE::find_scope(std::string name)
{
   return NULL;
}

// compare if two DIEs are equivalent.  This is used mainly for type comparison operations

bool DIE::compare(EvalContext & context, DIE * die, int flags)
{
   if (this == die) {
      return true;		// short cut for common case
   }
   if (die == NULL) {
      return false;
   }
   if (get_tag() == die->get_tag()) {
      DIE *
	  mysubtype = get_type();
      DIE *
	  hersubtype = die->get_type();
      if (mysubtype == NULL && hersubtype == NULL) {
	 return true;
      }
      if (mysubtype == NULL) {
	 return false;		// hersubtype will be non-NULL because of the test above
      }
      return mysubtype->compare(context, hersubtype, flags);
   }
   return false;
}

void
 DIE::print_name(EvalContext & ctx, DIE * die)
{
   AttributeValue & name = die->getAttribute(DW_AT_name);
   if (name.type != AV_NONE) {
      ctx.os.print("%s", Utils::simplify_type(name.str).c_str());
   }
}

void
 DIE::print_name_or_id(EvalContext & ctx, DIE * die)
{
   AttributeValue & name = die->getAttribute(DW_AT_name);
   if (name.type != AV_NONE) {
      ctx.os.print("%s", Utils::simplify_type(name.str).c_str());
   } else if (!ctx.show_contents) {
      ctx.os.print("{...}");
   }
}

void
 DIE::print_space(EvalContext & ctx, DIE * subtype)
{
   switch (subtype->get_tag()) {
   case DW_TAG_pointer_type:
   case DW_TAG_reference_type:
   case DW_TAG_subroutine_type:;
   case DW_TAG_array_type:
   case DW_TAG_const_type:
   case DW_TAG_volatile_type:
      ctx.os.print(" ");
      break;
   default:
      if (subtype->getAttribute(DW_AT_name).type != AV_NONE) {
	 ctx.os.print(" ");
      }
   }
}

void
 DIE::print(EvalContext & ctx, int indent, int level)
{
   /*
    * XXX:  Dave put this here, so I'm keeping it, but
    * I don't really know what it's supposed to be doing 
    */
   doindent(ctx, indent);
}

void
 DIE::dump(int indent)
{
   DwTagId tag = (DwTagId) get_tag();
   doindent(indent);
   std::cout << globl_dwf_names.get(tag);
   dumpAttributes(cu, indent + 2);
   dumpChildren(indent);
}

void
 DIE::print_type(TypeStack & stack, int i, EvalContext & ctx, int indent)
{
   if (i < 0) {
      return;
   }
   DIE *
       type = stack[i];
   switch (type->get_tag()) {
   case DW_TAG_const_type:
      ctx.os.print("const");
      if (i > 0) {
	 print_space(ctx, stack[i - 1]);
      }
      print_type(stack, i - 1, ctx, indent);
      break;
   case DW_TAG_volatile_type:
      ctx.os.print("volatile");
      if (i > 0) {
	 print_space(ctx, stack[i - 1]);
      }
      print_type(stack, i - 1, ctx, indent);
      break;
   case DW_TAG_pointer_type:
      if (i == (int) stack.size() - 1) {
	 doindent(ctx, indent);
	 ctx.os.print("void ");
      }
      ctx.os.print("*");
      if (i > 0) {
	 if (stack[i - 1]->get_tag() != DW_TAG_pointer_type) {
	    print_space(ctx, stack[i - 1]);
	 }
      }
      print_type(stack, i - 1, ctx, indent);
      break;
   case DW_TAG_reference_type:
      ctx.os.print("&");
      print_type(stack, i - 1, ctx, indent);
      break;
   case DW_TAG_subroutine_type:{
	 if (i == (int) stack.size() - 1) {
	    doindent(ctx, indent);
	    ctx.os.print("void ");
	 }
	 if (i > 0) {
	    int
		subtag = stack[i - 1]->get_tag();
	    if (subtag == DW_TAG_pointer_type
		|| subtag == DW_TAG_reference_type) {
	       ctx.os.print("(");
	       print_type(stack, i - 1, ctx, indent);
	       ctx.os.print(")");
	    } else {
	       print_type(stack, i - 1, ctx, indent);
	    }
	 }
	 // print prototype
	 ctx.os.print("(");
	 bool
	     comma = false;
	 for (uint i = 0; i < type->getChildren().size(); i++) {
	    DIE *
		para = type->getChildren()[i];
	    if (comma) {
	       ctx.os.print(", ");
	    }
	    print_declaration(ctx, para, 0);
	    comma = true;
	 }
	 ctx.os.print(")");
	 break;
      }
   case DW_TAG_array_type:{
	 if (i > 0) {
	    int
		subtag = stack[i - 1]->get_tag();
	    if (subtag == DW_TAG_pointer_type
		|| subtag == DW_TAG_reference_type) {
	       ctx.os.print("(");
	       print_type(stack, i - 1, ctx, indent);
	       ctx.os.print(")");
	    } else {
	       print_type(stack, i - 1, ctx, indent);
	    }
	 }
	 for (uint i = 0; i < type->getChildren().size(); i++) {
	    DIE *
		subrange = type->getChildren()[i];
	    AttributeValue & upperbound =
		subrange->getAttribute(DW_AT_upper_bound);
	    if (upperbound.type != AV_NONE) {
	       ctx.os.print("[%d]", ((int) upperbound + 1));
	    } else {
	       ctx.os.print("[]");
	    }
	 }
	 break;
      }
   case DW_TAG_base_type:
      doindent(ctx, indent);
      print_name(ctx, type);
      if (i > 0) {
	 print_space(ctx, stack[i - 1]);
      }
      print_type(stack, i - 1, ctx, indent);
      break;
   case DW_TAG_structure_type:
      doindent(ctx, indent);
      if ((ctx.language & 0xff) == DW_LANG_C
	  || (ctx.language & 0xff) == DW_LANG_C89) {
	 ctx.os.print("struct ");
      } else if ((ctx.language & 0xff) == DW_LANG_C_plus_plus) {	// print nothing for C++
      }
      print_name_or_id(ctx, type);
      if (i > 0) {
	 print_space(ctx, stack[i - 1]);
      }
      print_type(stack, i - 1, ctx, indent);
      break;
   case DW_TAG_union_type:
      doindent(ctx, indent);
      ctx.os.print("union ");
      print_name_or_id(ctx, type);
      if (i > 0) {
	 print_space(ctx, stack[i - 1]);
      }
      print_type(stack, i - 1, ctx, indent);
      break;
   case DW_TAG_enumeration_type:
      doindent(ctx, indent);
      if ((ctx.language & 0xff) == DW_LANG_C
	  || (ctx.language & 0xff) == DW_LANG_C89) {
	 ctx.os.print("enum ");
      } else if ((ctx.language & 0xff) == DW_LANG_C_plus_plus) {	// print nothing for C++
      }
      print_name_or_id(ctx, type);
      if (i > 0) {
	 print_space(ctx, stack[i - 1]);
      }
      print_type(stack, i - 1, ctx, indent);
      break;
   case DW_TAG_typedef:
      if (i != 0) {
	 doindent(ctx, indent);
	 print_name(ctx, type);
	 ctx.os.print(" ");
      }
      print_type(stack, i - 1, ctx, indent);
      if (i == 0) {		// only print name if bottom of stack
	 print_name(ctx, type);
      }
      break;
   default:			// a variable
      print_type(stack, i - 1, ctx, indent);
      print_name(ctx, type);
      break;
   }
}

void
 DIE::build_stack(TypeStack & stack, DIE * d)
{
   DIE *
       die_volatile = NULL;
   DIE *
       die_const = NULL;

   /*
    * traverse like linked list 
    */
   while (d != NULL) {

      /*
       * catch const wrapping qualifiers 
       */
      if (d->get_tag() == DW_TAG_const_type) {
	 die_const = d;
	 goto next_iteration;
      }

      /*
       * catch volatile wrapping qualifiers 
       */
      if (d->get_tag() == DW_TAG_volatile_type) {
	 die_volatile = d;
	 goto next_iteration;
      }

      /*
       * must be a "real" type, push to stack 
       */
      if (d->get_tag() != DW_TAG_pointer_type) {
	 stack.push_back(d);
      }

      /*
       * push any extant qualifiers 
       */
      if (die_volatile != NULL) {
	 stack.push_back(die_volatile);
	 die_volatile = NULL;
      }
      if (die_const != NULL) {
	 stack.push_back(die_const);
	 die_const = NULL;
      }

      /*
       * pointers must be flip-flopped 
       */
      if (d->get_tag() == DW_TAG_pointer_type) {
	 stack.push_back(d);
      }

    next_iteration:
      /*
       * follow element to next in the list 
       */
      AttributeValue av = d->getAttribute(DW_AT_type, false);
      if (av.type != AV_NONE) {
	 d = av.die;
      } else
	 break;

   }
}

void
 DIE::print_declaration(EvalContext & ctx, DIE * decl, int indent)
{
   // build a stack of the type dies
   TypeStack
       stack;
   build_stack(stack, decl);
   print_type(stack, stack.size() - 1, ctx, indent);
}

DIE *
DIE::get_type()
{
   AttributeValue & type = getAttribute(DW_AT_type);
   DIE *t = NULL;

   if (type.type != AV_NONE) {
      t = type.die;
      if (t == NULL) return t;

      if (t->get_tag() == DW_TAG_typedef) {
         return t->get_type();
      }
   }
   return t;
}

int
DIE::get_size()
{
   return getAttribute(DW_AT_byte_size).integer;
}

int
DIE::get_real_size(EvalContext & ctx)
{
   return getAttribute(DW_AT_byte_size).integer;
}

bool DIE::is_pointer()
{
   return false;
}

bool DIE::is_array()
{
   return false;
}

bool DIE::is_function()
{
   return false;
}

bool DIE::is_scalar()
{
   return false;
}

bool DIE::is_real()
{
   return false;
}

bool DIE::is_integral()
{
   return false;
}

bool DIE::is_address()
{
   return false;
}

bool DIE::is_struct()
{
   return false;
}

bool DIE::is_string()
{
   return false;
}

bool DIE::is_complex()
{
   return false;
}

bool DIE::is_artificial()
{
   AttributeValue
       art = getAttribute(DW_AT_artificial, false);
   if (art.type == AV_NONE || (art.type == AV_INTEGER && (int) art == 0)) {
      return false;
   }
   return true;
}

void
 DIE::complete_symbol(std::string name, Address pc,
		      std::vector < std::string > &result)
{
   check_loaded();

   // first try the lexical blocks
   for (uint i = 0; i < children.size(); i++) {
      DIE * child = children[i];
      int childtag = child->get_tag();
      if (childtag == DW_TAG_lexical_block) {
	 child->complete_symbol(name, pc, result);
      }
   }


   for (uint i = 0; i < children.size(); i++) {
      DIE *
	  child = children[i];
      int
	  childtag = child->get_tag();
      switch (childtag) {
      case DW_TAG_variable:
      case DW_TAG_member:
      case DW_TAG_subprogram:
      case DW_TAG_structure_type:
      case DW_TAG_class_type:
      case DW_TAG_union_type:
      case DW_TAG_enumerator:
      case DW_TAG_typedef:{
	    std::string childname = child->get_name();
	    if (strncmp(name.c_str(), childname.c_str(), name.size()) == 0) {
	       result.push_back(childname.substr(name.size()));
	    }
	    break;
	 }
      case DW_TAG_enumeration_type:
	 child->complete_symbol(name, pc, result);
	 break;
      }
   }

   if (parent != NULL && get_tag() != DW_TAG_lexical_block
       && get_tag() != DW_TAG_enumeration_type) {
      parent->complete_symbol(name, pc, result);
   }

}
