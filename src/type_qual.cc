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

file: type_qual.cc
created on: Fri Aug 13 11:02:27 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#include "dwf_attr.h"
#include "type_qual.h"

TypeQualifier::TypeQualifier(DwCUnit *_cu, DIE *_parent, Abbreviation *_abbrev)
 : DIE(_cu, _parent, _abbrev) {
}

TypeQualifier::TypeQualifier(DwCUnit *_cu, DIE *_parent, int _tag)
 : DIE(_cu, _parent, _tag) {
}

TypeQualifier::~TypeQualifier() {
}


bool
TypeQualifier::is_pointer() {
   return get_type()->is_pointer();
}

bool
TypeQualifier::is_char() {
   return get_type()->is_char();
}

bool
TypeQualifier::is_uchar() {
   return get_type()->is_uchar();
}

bool
TypeQualifier::is_schar() {
   return get_type()->is_schar();
}

bool
TypeQualifier::is_array() {
   return get_type()->is_array();
}

bool
TypeQualifier::is_scalar() {
   return get_type()->is_scalar();
}

bool
TypeQualifier::is_real() {
   return get_type()->is_real();
}

bool
TypeQualifier::is_integral() {
   return get_type()->is_integral();
}

bool
TypeQualifier::is_address() {
   return get_type()->is_address();
}

bool
TypeQualifier::is_signed() {
   return get_type()->is_signed();
}

bool
TypeQualifier::is_struct() {
   return get_type()->is_struct();
}

int
TypeQualifier::get_size() {
   return get_type()->get_size();
}

int
TypeQualifier::get_real_size(EvalContext& ctx) {
   return get_type()->get_real_size(ctx);
}

void
TypeQualifier::set_value(EvalContext& ctx, Value& addr, Value& value) {
   get_type()->set_value(ctx, addr, value);
}

void
TypeQualifier::print(EvalContext& ctx, int indent, int level) {
   doindent(ctx, indent);
   print_declaration(ctx, this, 0);
}

void
TypeQualifier::print_value(EvalContext& ctx, Value& value, int indent) {
   get_type()->print_value(ctx, value);
}

bool
TypeQualifier::compare(EvalContext& ctx, DIE * die, int flags)
{
   while (die != NULL && (
       die->get_tag() == DW_TAG_const_type ||
       die->get_tag() == DW_TAG_volatile_type ||
       die->get_tag() == DW_TAG_typedef)) {
      die = die->get_type();
   }
   return get_type()->compare(ctx, die, flags);
}


TypeConst::TypeConst(DwCUnit *_cu, DIE *_parent, Abbreviation *_abbrev)
 : TypeQualifier(_cu, _parent, _abbrev) {
}

TypeConst::TypeConst(DwCUnit *_cu, DIE *_parent, int _tag)
 : TypeQualifier (_cu, _parent, _tag) {
}

TypeConst::~TypeConst() {
}

TypeVolatile::TypeVolatile(DwCUnit *_cu, DIE *_parent, Abbreviation *_abbrev)
 : TypeQualifier(_cu, _parent, _abbrev) {
}

TypeVolatile::TypeVolatile(DwCUnit *_cu, DIE *_parent, int _tag)
 : TypeQualifier (_cu, _parent, _tag) {
}

TypeVolatile::~TypeVolatile() {
}

TypeTypedef::TypeTypedef(DwCUnit *_cu, DIE *_parent, Abbreviation *_abbrev)
 : TypeQualifier(_cu, _parent, _abbrev) {
}

TypeTypedef::TypeTypedef(DwCUnit *_cu, DIE *_parent, int _tag)
 : TypeQualifier (_cu, _parent, _tag) {
}

TypeTypedef::~TypeTypedef() {
}


