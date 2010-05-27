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
