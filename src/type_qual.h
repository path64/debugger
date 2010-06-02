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
