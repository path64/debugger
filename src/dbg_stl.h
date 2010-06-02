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
