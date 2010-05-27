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

file: type_qual.h
created on: Fri Aug 13 11:02:27 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#ifndef _TYPE_QUAL_H_

#include "type_base.h"

class TypeQualifier : public DIE {
public:
   TypeQualifier(DwCUnit* cu, DIE* parent, Abbreviation* abbrev);
   TypeQualifier(DwCUnit* cu, DIE* parent, int tag);
   ~TypeQualifier();

   bool is_pointer();
   bool is_struct();
   bool is_scalar();
   bool is_real();
   bool is_integral();
   bool is_address();
   bool is_signed();
   bool is_array();
   bool is_char();
   bool is_uchar();
   bool is_schar();

   int get_size();
   int get_real_size(EvalContext & ctx);

   virtual void print(EvalContext & ctx, int indent, int level = 0);
   void print_value(EvalContext & context, Value & value, int indent = 0);
   void set_value(EvalContext & context, Value & addr, Value & value);
   bool compare(EvalContext & context, DIE * die, int flags); 
};

class TypeConst : public TypeQualifier {
public:
   TypeConst(DwCUnit* cu, DIE* parent, Abbreviation* abbrev);
   TypeConst(DwCUnit* cu, DIE* parent, int tag);
   ~TypeConst();
};

class TypeVolatile : public TypeQualifier {
public:
   TypeVolatile(DwCUnit* cu, DIE* parent, Abbreviation* abbrev);
   TypeVolatile(DwCUnit* cu, DIE* parent, int tag);
   ~TypeVolatile();
};

class TypeTypedef : public TypeQualifier {
public:
   TypeTypedef(DwCUnit* cu, DIE* parent, Abbreviation* abbrev);
   TypeTypedef(DwCUnit* cu, DIE* parent, int tag);
   ~TypeTypedef();
};

#endif
