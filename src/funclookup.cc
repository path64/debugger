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
    SymbolTable *symtab = new SymbolTable (NULL, elf, *s, &aliases, dirlist) ;
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

