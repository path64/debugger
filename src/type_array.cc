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

file: type_array.cc
created on: Fri Aug 13 11:07:34 PDT 2004
author: David Allison <dallison@pathscale.com>
 
*/

#include "utils.h"
#include "process.h"
#include "type_array.h"
#include "dwf_cunit.h"

TypeArray::TypeArray(DwCUnit * cu, DIE * parent,
		     Abbreviation * abbrev)
:  DIE(cu, parent, abbrev), dimsok(false)
{
}

TypeArray::TypeArray(DwCUnit * cu, DIE * parent, int tag)
    :
DIE(cu, parent, tag),
dimsok(false)
{
}

TypeArray::~TypeArray()
{
}

void
TypeArray::print(EvalContext & ctx, int indent, int level)
{
   doindent(ctx, indent);
   switch (ctx.language & 0xff) {
   case DW_LANG_C:
   case DW_LANG_C89:
   case DW_LANG_C_plus_plus:
      print_declaration(ctx, this, 0);
      break;
   case DW_LANG_Fortran77:{
	 get_type()->print(ctx, 0, level + 1);
	 ctx.os.print(" (");
	 get_dims(ctx);
	 bool comma = false;
	 for (uint i = 0; i < dims.size(); i++) {
	    if (comma) {
	       ctx.os.print(", ");
	    }
	    comma = true;
	    Dimension & dim = dims[i];
	    if (dim.low != 1) {
	       if (dim.high < dim.low) {
		  ctx.os.print(":");
	       } else {
		  ctx.os.print("%d:%d", dim.low, dim.high);
	       }
	    } else {
	       ctx.os.print("%d", dim.high);
	    }
	 }
	 ctx.os.print(")");
	 break;
      }
   case DW_LANG_Fortran90:{
	 get_type()->print(ctx, 0, level + 1);
	 ctx.os.print(", dimension(");
	 get_dims(ctx);
	 bool comma = false;
	 for (uint i = 0; i < dims.size(); i++) {
	    if (comma) {
	       ctx.os.print(", ");
	    }
	    comma = true;
	    Dimension & dim = dims[i];
	    if (dim.low != 1) {
	       if (dim.high < dim.low) {
		  ctx.os.print(":");
	       } else {
		  ctx.os.print("%d:%d", dim.low, dim.high);
	       }
	    } else {
	       ctx.os.print("%d", dim.high);
	    }
	 }
	 ctx.os.print(")");
	 break;
      }
   }
}

void
TypeArray::set_value(EvalContext & context, Value & addr, Value & value)
{
   get_dims(context);
   if (dims.size() != 1) {
      throw Exception("Can't set the value of a multidimensional array");
   }
   DIE *type = get_type();
   if (value.type == VALUE_VECTOR) {
      if ((int) value.vec.size() != dims[0].size()) {
	 throw
	     Exception
	     ("Incorrect number of elements for setting the array value");
      }
      int high = dims[0].high;
      int j = 0;
      bool save = context.addressonly;
      context.addressonly = true;
      for (int i = dims[0].low; i <= high; i++, j++) {
	 Value elemaddr = get_index(context, 0, addr, i);
	 type->set_value(context, elemaddr, value.vec[j]);
      }
      context.addressonly = save;
   } else {
      int high = dims[0].high;
      int j = 0;
      bool save = context.addressonly;
      context.addressonly = true;
      for (int i = dims[0].low; i <= high; i++, j++) {
	 Value elemaddr = get_index(context, 0, addr, i);
	 type->set_value(context, elemaddr, value);
      }
      context.addressonly = save;
   }
}

bool
TypeArray::compare(EvalContext & context, DIE * die, int flags)
{
   if (!DIE::compare(context, die, flags)) {
      return false;
   }
   TypeArray *herarray = dynamic_cast < TypeArray * >(die);
   if (herarray == NULL) {
      return false;
   }
   get_dims(context);
   herarray->get_dims(context);
   if (dims.size() != herarray->dims.size()) {
      return false;
   }
   for (uint i = 0; i < dims.size(); i++) {
      if (dims[i] != herarray->dims[i]) {
	 return false;
      }
   }
   return true;
}

bool
TypeArray::is_array()
{
   return true;
}

Value
TypeArray::evaluate(EvalContext & context)
{
   return Value(0);
}

int
TypeArray::get_num_dims()
{
   int n = 0;
   for (uint i = 0; i < children.size(); i++) {
      DIE *child = children[i];
      if (child->get_tag() == DW_TAG_subrange_type) {
	 n++;
      }
   }
   return n;
}

std::vector < Dimension > &TypeArray::get_dims(EvalContext & ctx)
{
   if (dimsok) {
      return dims;
   }
   bool
       variabledims = false;
   dims.clear();		// clear dims from last time
   bool
       save = ctx.addressonly;
   ctx.addressonly = false;	// we don't want the address, we want the
				// value
   for (uint i = 0; i < children.size(); i++) {
      DIE *
	  child = children[i];
      if (child->get_tag() == DW_TAG_subrange_type) {
	 int
	     ub = 0;
	 int
	     lb = 0;
	 AttributeValue & lowerbound =
	     child->getAttribute(DW_AT_lower_bound);
	 if (lowerbound.type != AV_NONE) {	// lower bound present?
	    if (lowerbound.type == AV_DIE) {
	       Value
		   v = ((DIE *) lowerbound)->evaluate(ctx);
	       lb = v;
	       DIE *
		   countdie = child->getAttribute(DW_AT_count);
	       Value
		   count = countdie->evaluate(ctx);
	       ub = lb + (int) count - 1;
	       variabledims = true;
	    } else {
	       lb = (int) lowerbound;
	    }
	 }
	 AttributeValue & upperbound =
	     child->getAttribute(DW_AT_upper_bound);
	 if (upperbound.type != AV_NONE) {	// upper bound present?
	    if (upperbound.type == AV_DIE) {
	       Value
		   v = ((DIE *) upperbound)->evaluate(ctx);
	       ub = v;
	       variabledims = true;
	    } else {
	       ub = (int) upperbound;
	    }
	 }
	 dims.push_back(Dimension(lb, ub));
      }
   }
   if (!variabledims) {		// only keep dimensions if they were
				// constant
      dimsok = true;
   }
   ctx.addressonly = save;
   return dims;
}

int
TypeArray::get_real_size(EvalContext & ctx)
{
   get_dims(ctx);
   int size = get_type()->get_real_size(ctx);
   for (uint i = 0; i < dims.size(); i++) {
      size *= dims[i].size();
   }
   return size;
}

    // get the address or value of an element of an array
Address
TypeArray::get_index(EvalContext & context, int dim, Address currentaddr,
		     int indexval)
{
   DIE *type = get_type();
   int phi = type->get_size();
   if (context.language == DW_LANG_Fortran77
       || context.language == DW_LANG_Fortran90) {
      for (int i = 0; i < dim; i++) {
	 phi *= dims[i].size();
      }
   } else {
      for (int i = dims.size() - 1; i > dim; i--) {
	 phi *= dims[i].size();
      }
   }
   Address addr;
   if (context.language == DW_LANG_Fortran77
       || context.language == DW_LANG_Fortran90) {
      addr = currentaddr + ((indexval - dims[dim].low) * phi);	// subtract 
								// the
								// lower
								// bound
   } else {
      addr = currentaddr + (indexval * phi);
   }
   if (dim == (int) dims.size() - 1) {	// last dimension?
      if (context.addressonly || type->is_complex()) {
	 return addr;
      } else if (type->is_scalar() || type->is_pointer()) {
	 return context.process->read(addr, type->get_size());	// get
								// value
								// of
								// variable
      } else if (type->is_struct()) {
	 return addr;
      } else if (type->is_array()) {
	 return addr;
      } else if (type->is_string()) {
	 return addr;
      } else {
	 throw Exception("Unknown type for array element");
      }
   } else {
      return addr;
   }
}

    // get the type to display given an indexcount.  If the indexcount is
    // the same
    // as the number of dimensions then the type is the element type.
    // Otherwise
    // we need to invent a new DIE for the TypeArray with the appropriate
    // subrange
    // children (yuk)

    // an array is always a reference to the structure.  Sizeof will have
    // to
    // get the real size of it


DIE *
TypeArray::get_display_type(int indexcount)
{
   int ndims = 0;
   // this might be called before the dims have been set up.  Since we
   // just need the
   // number of dimensions, lets count them
   if (dims.size() == 0) {
      for (uint i = 0; i < children.size(); i++) {
	 DIE *child = children[i];
	 if (child->get_tag() == DW_TAG_subrange_type) {
	    ndims++;
	 }
      }
   } else {
      ndims = dims.size();
   }

   if (indexcount == ndims) {
      return get_type();
   }
   DIE *newdie = new TypeArray(cu, parent, DW_TAG_array_type);
   newdie->addAttribute(DW_AT_type, get_type());

   uint childnum = indexcount;
   while (childnum < children.size()) {
      newdie->addChild(children[childnum]);
      childnum++;
   }
   return newdie;
}

int
TypeArray::get_size()
{
   return cu->getAddrSize();
}

void
TypeArray::print_char_array(EvalContext & context, TypeArray * array,
			    Address addr, int dim, int repmin)
{
   std::string s =
       context.process->read_string(addr, array->dims[dim].size());
   context.os.print(" \"");
   Utils::print_string(context, s);
   context.os.print("\"");
}

void
TypeArray::print_dim_c(EvalContext & context, TypeArray * array,
		       Address addr, int dim, int repmin, int &maxelem,
		       bool pretty, int indent, int index, bool indexvalid,
		       bool docomma)
{
   if (!pretty && docomma) {
      context.os.print(", ");
   } else if (pretty && indexvalid) {
      context.os.print("[%d] = ", index);
   }

   /*
    * catch adimensional (empty?) arrays 
    */
   if (dim > (int) array->dims.size() - 1) {
      context.os.print("{}");
      return;
   }

   DIE *type = array->get_type();
   // if last dimension and the array is a character array
   if (dim == (int) array->dims.size() - 1 && type->is_char()) {
      print_char_array(context, array, addr, dim, repmin);
      return;
   }

   context.os.print("{");
   // calculate the address of the dimension
   int phi = type->get_size();
   for (int i = array->dims.size() - 1; i > dim; i--) {
      phi *= array->dims[i].size();
   }
   Value last_value;
   bool lv_valid = false;
   int nrepeats = 0;

   // for each element in the dimension
   bool comma = false;
   for (int i = 0; i < array->dims[dim].size(); i++) {
      Address dimaddr = addr + i * phi;
      if (dim == (int) array->dims.size() - 1) {	// last dimension?
	 Value val;
	 if (type->is_struct() || type->is_complex() || type->is_array()
	     || type->is_string()) {
	    val = dimaddr;
	 } else {
	    try {
	       val = context.process->read(dimaddr, type->get_size());	// get 
									// value 
									// of 
									// variable
	    }
	    catch(Exception e) {
	       context.os.print("<%s>}", e.get().c_str());
	       return;
	    }
	    if (type->is_real()) {
	       if (type->get_size() == 4) {
		  val.real = (double) (*(float *) &val.real);
	       }
	       val.type = VALUE_REAL;
	    }
	 }
	 // if value is repeat, inc count and don't print
	 if (lv_valid && val == last_value) {
	    nrepeats++;
	 } else {
	    // value is different from previous, flush accumulated repeats
	    if (nrepeats > 0) {
	       if (repmin > 0 && nrepeats >= repmin) {
		  context.os.print(" <repeats %d times>", nrepeats + 1);
		  nrepeats = 0;
		  comma = true;
	       } else {
		  while (nrepeats-- > 0) {
		     if (comma) {
			context.os.print(", ");
		     }
		     type->print_value(context, last_value);
		     comma = true;
		  }
		  nrepeats = 0;
	       }
	    }
	    if (maxelem-- <= 0) {	// limit print to max number of
					// elements
	       context.os.print("...");
	       break;
	    }
	    // now print the value itself
	    if (comma) {
	       context.os.print(", ");
	    }
	    type->print_value(context, val);
	    comma = true;
	 }
	 last_value = val;
	 lv_valid = true;
      } else {
	 // want each row on a line of its own
	 if (pretty) {
	    context.os.print("\n");
	    doindent(context, indent + 4);
	 }

	 print_dim_c(context, array, dimaddr, dim + 1, repmin, maxelem,
		     pretty, indent + 4, i, true, comma);
	 comma = true;
      }
   }
   // print all repeated values
   if (nrepeats > 0) {
      if (repmin > 0 && nrepeats >= repmin) {
	 context.os.print(" <repeats %d times>", nrepeats + 1);
	 nrepeats = 0;
	 comma = true;
      } else {
	 while (nrepeats-- > 0) {
	    if (comma) {
	       context.os.print(", ");
	    }
	    type->print_value(context, last_value);
	    comma = true;
	 }
      }
   }

   if (pretty && dim < int (array->dims.size() - 1)) {
      context.os.print("\n");
      doindent(context, indent);
   }
   context.os.print("}");
}


void
TypeArray::print_dim_fortran(EvalContext & context, TypeArray * array,
			     Address addr, int dim, int repmin,
			     int &maxelem, bool pretty, int indent,
			     int *indices, bool indexvalid, bool docomma)
{
   if (!pretty && docomma) {
      context.os.print(", ");
   } else if (pretty && indexvalid) {
      context.os.print("(");
      bool cma = false;
      for (int i = 0; i < dim; i++) {
	 if (cma) {
	    context.os.print(",");
	 }
	 context.os.print("%d", indices[i]);
	 cma = true;
      }
      context.os.print(") = ");
   }

   DIE *type = array->get_type();
   // if last dimension and the array is a character array
   if (dim == (int) array->dims.size() - 1 && type->is_char()) {
      print_char_array(context, array, addr, dim, repmin);
      return;
   }

   if (maxelem <= 0) {		// limit print to max number of elements
      return;
   }

   context.os.print("(");
   // calculate the address of the dimension
   // note that fortran arrays are different than C arrays.  Fortran
   // allocates earlier dimensions
   // in contiguous memory (C does the later ones)
   int phi = type->get_size();
   for (int i = 0; i < dim; i++) {
      phi *= array->dims[i].size();
   }
   Value last_value;
   bool lv_valid = false;
   int nrepeats = 0;

   // for each element in the dimension
   bool comma = false;
   for (int i = array->dims[dim].low; i <= array->dims[dim].high; i++) {
      indices[dim] = i;
      Address dimaddr = addr + (i - array->dims[dim].low) * phi;
      if (maxelem-- <= 0) {	// limit print to max number of elements
	 context.os.print("...");
	 break;
      }
      if (dim == (int) array->dims.size() - 1) {	// last dimension?
	 Value val;
	 if (type->is_struct() || type->is_complex() || type->is_array()
	     || type->is_string()) {
	    val = dimaddr;
	 } else {
	    try {
	       val = context.process->read(dimaddr, type->get_size());	// get 
									// value 
									// of 
									// variable
	    }
	    catch(Exception e) {
	       context.os.print("<%s>)", e.get().c_str());
	       return;
	    }
	    if (type->is_real()) {
	       if (type->get_size() == 4) {
		  val.real = (double) (*(float *) &val.real);	// convert 
								// to
								// double
	       }
	       val.type = VALUE_REAL;
	    }
	 }
	 if (lv_valid && val == last_value) {	// if value is repeat, inc 
						// count and don't print
	    nrepeats++;
	 } else {
	    // value is different from previous, flush accumulated repeats
	    if (nrepeats > 0) {
	       if (repmin > 0 && nrepeats >= repmin) {
		  context.os.print(" <repeats %d times>", nrepeats + 1);
		  nrepeats = 0;
		  comma = true;
	       } else {
		  while (nrepeats-- > 0) {
		     if (comma) {
			context.os.print(", ");
		     }
		     type->print_value(context, last_value);
		     comma = true;
		  }
		  nrepeats = 0;
	       }
	    }
	    // now print the value itself
	    if (comma) {
	       context.os.print(", ");
	    }
	    type->print_value(context, val);
	    comma = true;
	 }
	 last_value = val;
	 lv_valid = true;
      } else {
	 // want each row on a line of its own
	 if (pretty) {
	    context.os.print("\n");
	    doindent(context, indent + 4);
	 }

	 print_dim_fortran(context, array, dimaddr, dim + 1, repmin,
			   maxelem, pretty, indent + 4, indices, true,
			   comma);
      }
      comma = true;
   }
   // print all repeated values
   if (nrepeats > 0) {
      if (repmin > 0 && nrepeats >= repmin) {
	 context.os.print(" <repeats %d times>", nrepeats + 1);
	 nrepeats = 0;
	 comma = true;
      } else {
	 while (nrepeats-- > 0) {
	    if (comma) {
	       context.os.print(", ");
	    }
	    type->print_value(context, last_value);
	    comma = true;
	 }
      }
   }

   if (pretty && dim < int (array->dims.size() - 1)) {
      context.os.print("\n");
      doindent(context, indent);
   }
   context.os.print(")");
}


void
TypeArray::print_value(EvalContext & ctx, Value & value, int indent)
{
   EvalContext context = ctx;

   if (!context.process->is_active()) {
      throw Exception("A running process is required for this operation");
   }

   if (value.integer == 0) {
      context.os.print("<omitted>");
      return;
   }

   bool pretty = context.pretty
       && context.process->get_int_opt(PRM_P_ARRAY);
   int repmin = context.process->get_int_opt(PRM_P_REPS);
   int maxelem = context.process->get_int_opt(PRM_P_ELEM);
   if (context.truncate_aggregates) {
      switch (context.language & 0xff) {
      case DW_LANG_Fortran77:
      case DW_LANG_Fortran90:
	 maxelem = 5;
	 break;
      default:
	 maxelem = 5;
      }
   }

   switch (context.language & 0xff) {
   case DW_LANG_C_plus_plus:
   case DW_LANG_C89:
   case DW_LANG_C:{
	 bool comma = false;
	 get_dims(context);
	 DIE *type = get_type();	// type of the array
	 (void) type;
	 print_dim_c(context, this, value, 0, repmin, maxelem, pretty,
		     indent, 0, false, comma);
	 break;
      }
   case DW_LANG_Ada83:
      throw Exception("Ada is not a supported language");
      break;
   case DW_LANG_Cobol74:
   case DW_LANG_Cobol85:
      throw Exception("COBOL is not a supported language");
   case DW_LANG_Fortran77:
   case DW_LANG_Fortran90:{
	 bool comma = false;
	 get_dims(context);
         context.truncate_aggregates = true;
	 int *indices = new int[dims.size()];
	 DIE *type = get_type();	// type of the array
	 (void) type;
	 try {
	    print_dim_fortran(context, this, value, 0, repmin, maxelem,
			      pretty, indent, indices, false, comma);
	    delete indices;
	 } catch(...) {
	    delete indices;
	    throw;
	 }
	 break;
      }
   case DW_LANG_Pascal83:
      throw Exception("Pascal is not a supported language");
   case DW_LANG_Modula2:
      throw Exception("Modula2 is not a supported language");
   }
}
