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

file: gen_loc.cc
created on: Mon May  2 13:08:35 PDT 2005
author: James Strother <jims@pathscale.com>

*/

#include "gen_loc.h"
#include "symtab.h"

std::string FunctionLocation::get_name() {
        return symbol->name ;
}

bool FunctionLocation::at_first_line(Address addr) {
        return symtab->at_first_line(this, addr);
}

void FunctionLocation::print() {
    printf ("0x%llx %-20s\n",
	    (unsigned long long) address,
	    symbol->name.c_str()) ;
}


void Location::show_line(PStream &os, bool emacs_mode) {
    if (file == NULL) return;

    if (emacs_mode) {
        os.print("\032\032%s:%d:1:beg:0x%x\n",
         get_filename().c_str(),line,address);
    } else {
        file->open(*dirlist) ;
        file->show_line (line, os, false, 0) ;
    }
}

DIE* Location::get_subp_die() {
    if (func != NULL && func->symbol != NULL) {
       return func->symbol->die;
    }
    return NULL;
}
