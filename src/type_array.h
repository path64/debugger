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
