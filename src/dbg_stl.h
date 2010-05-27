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

file: dbg_stl.h
created on: Tue Aug 17 11:02:17 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#ifndef dbg_stl_h_included
#define dbg_stl_h_included

#include "dbg_types.h"

class EvalContext ;
class DIE ;
class SymbolTable ;
class Value ;

namespace stl {

bool print_string (EvalContext &context, Address addr, DIE *type) ;
bool print_vector (EvalContext &context, Address addr, DIE *type) ;

bool print_struct (EvalContext &ctx, Address addr, DIE *type) ;

bool subscript_struct (SymbolTable *symtab, EvalContext &ctx, Address addr, DIE *type, int subscript, Value &outval, DIE *&outtype) ;
bool subscript_struct (SymbolTable *symtab, EvalContext &ctx, Address addr, DIE *type, int lo, int hi, Value &outval, DIE *&outtype) ;

}



#endif
