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

file: funclookup.cc
created on: Fri Aug 13 11:07:42 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#include "funclookup.h"
#include "symtab.h"

// given the name of a file, name of a function and offset, return the filename:lineno

AliasManager aliases ;
DirectoryTable dirlist ;

bool find_function (std::string file, std::string func, int offset, std::string &filename, int &lineno) {
    ELF *elf = new ELF (file) ;
    std::istream *s = elf->open() ;
    elf->read_symbol_table (*s, 0) ;
    SymbolTable *symtab = new SymbolTable (NULL, elf, *s, &aliases) ;
    //symtab->list_functions() ;
    return symtab->find_function (func, offset, filename, lineno) ;
}

int main (int argc, char **argv) {
    int ret = 0;
    if (argc != 4) {
        std::cerr << "usage: funclookup file function offset\n" ;
        exit (1) ;
    }
    std::string file = argv[1] ;
    std::string func = argv[2] ;
    int offset = strtol (argv[3], NULL, 0) ;

    try {
        std::string filename ;
        int lineno ;
        if (find_function (file, func, offset, filename, lineno)) {
            std::cout << filename << ":" << lineno << "\n" ;
        } else {
            std::cerr << "*** not found\n" ;
            ret = 1;
        }
    } catch (const char *s) {
        std::cerr << "*** exception: " << s << "\n" ;
        ret = 1;
    } catch (std::string s) {
        std::cerr << "*** exception: " << s << "\n" ;
        ret = 1;
    } catch (...) {
        std::cerr << "*** unknown exception\n" ;
        ret = 1;
    }

    return ret;
}

