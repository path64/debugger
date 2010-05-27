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

file: type_base.h
created on: Fri Aug 13 11:02:26 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#ifndef _TYPE_BASE_H_
#define _TYPE_BASE_H_

#include "dwf_attr.h"

class DwCUnit;

class DIE {
 public:

   DIE(DwCUnit * cu, DIE * parent, Abbreviation * abbrev); /* PORT: not needed */
   DIE(DwCUnit * cu, DIE * parent, int tag); /* PORT: not needed */
    virtual ~ DIE(); /* PORT: not needed */
   int get_tag(); /* PORT: impl. */
   DIE *get_parent() {return parent;} /* PORT: impl. */
   virtual bool is_pointer(); /* PORT: impl. */
   virtual bool is_array(); /* PORT: impl. */
   virtual bool is_function(); /* PORT: impl. */
   virtual bool is_scalar(); /* PORT: impl. */
   virtual bool is_real(); /* PORT: impl. */
   virtual bool is_integral(); /* PORT: impl. */
   virtual bool is_address(); /* PORT: impl. */
   virtual bool is_boolean() { return false; } /* PORT: impl. */
   virtual bool is_char() { return false; } /* PORT: impl. */
   virtual bool is_uchar() { return false; } /* PORT: impl. */
   virtual bool is_schar() { return false; } /* PORT: impl. */
   virtual bool is_signed() { return false; } /* PORT: impl. */
   virtual bool is_local_var() { return false; } /* PORT: impl.*/
   virtual bool is_member_function() { return false; } /* PORT: impl. */
   void check_loaded(); /* PORT: not needed */
   void read(DwCUnit * cu, BStream & stream, bool readall = false); /* PORT: not needed */
   void read(BStream & stream); /* PORT: not needed */
   DwCUnit *get_cunit() { return cu; } /* PORT: impl. */
   virtual int get_language() { return 0; } /* PORT: impl. */
   virtual bool is_inheritance() { return false; } /* PORT: impl. */
   std::string get_name(); /* PORT: impl. */
   virtual DIE *find_member(std::string & name) { return NULL; } /* PORT: impl. */
   virtual DIE *find_member(DIE * member) { return NULL; } /* PORT: not needed */
   virtual void find_member(std::string & name, std::vector < DIE * >&result) { } /* PORT: not needed */
   virtual bool is_string(); /* PORT: impl. */
   virtual bool is_struct(); /* PORT: impl. */
   virtual bool is_complex(); /* PORT: impl. */




   virtual void print(EvalContext & ctx, int indent, int level = 0);
   virtual void dump(int indent = 0);


   void addChild(DIE * child);

   AttributeValue & getAttribute(int tag, bool fullsearch =true, bool stophere = false);
   void addAttribute(int tag, const AttributeValue & value);

   bool is_printable();
   bool is_virtual() {
      int virt = getAttribute(DW_AT_virtuality).integer;
      return (virt == DW_VIRTUALITY_virtual);
   }

   std::vector < DIE * >&getChildren();
   void print_space(EvalContext & ctx, DIE * sub);
   void print_declaration(EvalContext & ctx, DIE * decl, int indent);
   virtual int get_size();	// size of dereferenced type
   virtual int get_size_immed() {
      return get_size();
   }				// size of immediate type
   virtual int get_real_size(EvalContext & ctx);
 

   virtual bool is_struct_deref() {
      return is_struct();
   }				// is the dereferenced type a struct
   virtual bool is_artificial();

   virtual int get_local_offset();
   virtual Value evaluate(EvalContext & context);
   virtual Value evaluate(EvalContext & context, Value & base);
   virtual void print_value(EvalContext & context, Value & value,
			    int indent = 0);
   virtual void set_value(EvalContext & context, Value & addr, Value & value);
   virtual DIE *get_type();

   virtual void find_symbol(std::string name, Address pc, std::vector<DIE*>& result, DIE* caller=NULL);
   DIE *find_symbol(std::string name, Address pc);

   virtual DIE *find_scope(std::string name);
   virtual void find_operator(std::string name,
			      std::vector < DIE * >&result) {
   }
   virtual Address get_frame_base(Process * process) { return 0; }

   virtual void complete_symbol(std::string name, Address pc,
				std::vector < std::string > &result);
   virtual Address get_virtual_table(EvalContext & ctx, Address thisptr) { return 0; }
   bool is_case_blind();

   void set_more_info(DIE * d) { more_info = d; }
   DIE *get_more_info() { return more_info; }
   DIE *get_definition() { return definition; }
   void set_definition(DIE * def) { definition = def; }

   virtual bool compare(EvalContext & context, DIE * die, int flags);	// compare two type dies

 protected:
   Offset read_sibling(DwCUnit * cu, AttributeAbbreviation * attrabbrev,
		       Attribute * attr, BStream & stream); /*PORT: not needed */
   bool is_skippable();/* PORT: not needed */

   typedef std::vector <DIE*>TypeStack; 
   void build_stack(TypeStack & stack, DIE * d);
   void print_type(TypeStack & stack, int i, EvalContext & ctx,
		   int indent);
   void print_name(EvalContext & ctx, DIE * die);
   void print_name_or_id(EvalContext & ctx, DIE * die);
   void doindent(int indent);
   void doindent(EvalContext & ctx, int indent);
   void dumpAttributes(DwCUnit * cu, int indent = 0);
   void dumpChildren(int indent);
   typedef std::map < int, Attribute * >AttributeMap;
   Attribute *find_attribute(int tag);

   AttributeMap attributes;	// map of Attr vs Attribute
   std::vector < DIE * >children;
   DwCUnit *cu;
   DIE *parent;
   Abbreviation *abbrev;
   int id;
   int tag;
   Offset children_offset;	// offset to children
   DIE *more_info;		// another DIE that provides additional info (DW_AT_specification)
   DIE *definition;		// definition DIE if this is a declaration
};

typedef std::map < std::string, DIE * > DIEMap;
typedef std::multimap < std::string, DIE * > MultiDIEMap;
typedef std::map < Offset, DIE * > OffsetMap;

#endif
