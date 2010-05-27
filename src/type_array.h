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

file: type_array.h
created on: Fri Aug 13 11:02:27 PDT 2004
author: David Allison <dallison@pathscale.com>
 
*/

#ifndef _TYPE_ARRAY_H_
#define _TYPE_ARRAY_H_

#include "type_base.h"


// XXX: variable dimensions arrays?
struct Dimension {
   Dimension(int lo, int hi):low(lo),
      high(hi) {
   } Dimension(int i):low(0),
      high(i) {
   }
   int low;
   int high;

   int size() {
      return high - low + 1;
   }

   operator  int () {
      return high;
   }

   bool operator==(Dimension & d) {
      return low == d.low && high == d.high;
   }
};

class TypeArray:public DIE {
 public:
   TypeArray(DwCUnit * cu, DIE * parent, Abbreviation * abbrev);
   TypeArray(DwCUnit * cu, DIE * parent, int tag);
   ~TypeArray();

   virtual void print(EvalContext & ctx, int indent, int level = 0);
   bool is_array();
   int get_num_dims();
      std::vector < Dimension > &get_dims(EvalContext & ctx);
   Address get_index(EvalContext & context, int dim, Address currentaddr,
		     int indexval);
   DIE *get_display_type(int indexcount);
   int get_size();
   void print_value(EvalContext & context, Value & value, int indent = 0);
   Value evaluate(EvalContext & context);
      std::vector < Dimension > dims;
   int get_real_size(EvalContext & ctx);
   bool compare(EvalContext & context, DIE * die, int flags);	// compare 
								// two
								// type
								// dies
   void set_value(EvalContext & context, Value & addr, Value & value);
 protected:
 private:
      bool dimsok;
   void print_dim_c(EvalContext & context, TypeArray * array, Address addr,
		    int dim, int repmin, int &maxelem, bool pretty,
		    int indent, int index, bool indexvalid, bool docomma =
		    false);
   void print_dim_fortran(EvalContext & context, TypeArray * array,
			  Address addr, int dim, int repmin, int &maxelem,
			  bool pretty, int indent, int *indices,
			  bool indexvalid, bool docomma = false);
   void print_char_array(EvalContext & context, TypeArray * array,
			 Address addr, int dim, int repmin);
};

#endif
