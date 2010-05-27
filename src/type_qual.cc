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

file: type_qual.cc
created on: Fri Aug 13 11:02:27 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#include "dwf_attr.h"
#include "type_qual.h"

TypeQualifier::TypeQualifier(DwCUnit* cu, DIE* parent, Abbreviation * abbrev)
 : DIE(cu, parent, abbrev) {
}

TypeQualifier::TypeQualifier(DwCUnit* cu, DIE* parent, int tag)
 : DIE(cu, parent, tag) {
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


TypeConst::TypeConst(DwCUnit* cu, DIE* parent, Abbreviation* abbrev)
 : TypeQualifier(cu, parent, abbrev) {
}

TypeConst::TypeConst(DwCUnit * cu, DIE * parent, int tag)
 : TypeQualifier (cu, parent, tag) {
}

TypeConst::~TypeConst() {
}

TypeVolatile::TypeVolatile(DwCUnit* cu, DIE* parent, Abbreviation * abbrev)
 : TypeQualifier(cu, parent, abbrev) {
}

TypeVolatile::TypeVolatile(DwCUnit* cu, DIE* parent, int tag)
 : TypeQualifier (cu, parent, tag) {
}

TypeVolatile::~TypeVolatile() {
}

TypeTypedef::TypeTypedef(DwCUnit* cu, DIE* parent, Abbreviation * abbrev)
 : TypeQualifier(cu, parent, abbrev) {
}

TypeTypedef::TypeTypedef(DwCUnit* cu, DIE* parent, int tag)
 : TypeQualifier (cu, parent, tag) {
}

TypeTypedef::~TypeTypedef() {
}


