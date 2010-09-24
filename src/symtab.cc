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

file: symtab.cc
created on: Fri Aug 13 11:07:47 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#include "symtab.h"
#include "process.h"
#include "dwf_cunit.h"
#include "arch.h"
#include "map_range.h"

#include <set>
#include <algorithm>

// this is in libiberty.a but there appears to be no header for it
extern "C" const char *cplus_demangle (const char *name, int options) ;

AliasManager::AliasManager() {
}

AliasManager::~AliasManager() {
}


std::string demangle(const std::string &func)
{
    const char *demangled = cplus_demangle(func.c_str(), 0);
    return demangled ? demangled : func;
}


void AliasManager::list_aliases() {
    for (AliasMap::iterator i = aliases.begin() ; i != aliases.end() ; i++) {
        printf ("%s: %s\n", i->first, i->second) ;
    }
}

void AliasManager::add_alias (const char *name, const char *alias) {
    aliases[name] = alias ;
}

const char *AliasManager::find_alias (const char *alias) {
    AliasMap::iterator a = aliases.find (alias) ;
    if (a == aliases.end()) {
        return NULL ;
    }
    return a->second ;
}

void AliasManager::complete (std::string name, std::vector<std::string> &result) {
    for (AliasMap::iterator i = aliases.begin() ; i != aliases.end() ; i++) {
        if (strncmp (i->first, name.c_str(), name.size()) == 0) {
            result.push_back (std::string (i->first).substr (name.size())) ;
        }
    }

}

Symbol::Symbol (std::string name, DIE * die)
    : name(name),
    die(die) {
}

Symbol::~Symbol() {
}

class Compare_linematrix {
public:
    bool operator() (LineInfo *l1, LineInfo *l2) {
        return l1->address < l2->address ;
    }
} ;

class Compare_line {
public:
    bool operator() (LineInfo *l1, LineInfo *l2) {
        return l1->lineno < l2->lineno ;
    }
} ;

class Compare_address {
public:
    bool operator() (Address a1, Address a2) {
        return a1 < a2 ;
    }
} ;

// given a full C++ name, extract the base name.
// for example foo::bar::baz(char *) == "baz"

static std::string get_cpp_basename (const char *fullname, int &nlevels) {
    const char *p = fullname ;
    // first find an open paren
    while (*p != 0 && *p != '(') {
        p++ ;
    }
    // is appears that Fedora Core 2 doesn't put the () in.  Let's allow it.
    if (p != fullname) p-- ; // char before ( or eos
    // now scan back for ':' or the start of the string
    int numbra = 0 ;                    // number of brackets
    const char *s = p ;
    while (s != fullname) {
        while (s != fullname) {
            if (*s == '>') {                    // skip :: inside <>
                numbra++ ;
            } else if (*s == '<') {
                numbra-- ;
            } else if (*s == ':') {
                if (numbra == 0) {
                    break ;
                }
            }
            s-- ;
        }
        if (*s == ':') {
            if (--nlevels == 0) {
                return std::string (s+1, p-s) ;
            } else {
                s -= 2 ;                // move back to before ::
            }
            
        } 
    }

    nlevels-- ;
    return std::string (s, p-s+1) ;
}

SymbolTable::SymbolTable (Architecture *arch, ELF * elf, std::istream & stream, AliasManager *aliases, DirectoryTable &dirlist, PStream *os, bool reporterror)
    : DwInfo(elf, stream),
      arch(arch),
      debugger_cu(NULL),
      aliases(aliases),
      dirlist(dirlist)
{
    read(os, reporterror) ;
}

SymbolTable::~SymbolTable() {
    Map_Range<Address,FunctionLocation*>::iterator i;
    for (i=funcmap.begin(); i!=funcmap.end(); ++i) {
       delete i->val;
    }

    delete debugger_cu ;
}

void SymbolTable::do_cxx_alias() {
  /* Add aliases to C++ demangled names to the symbol table.
   * This includes basenames, for example, "a::b::c" adds
   * "c", ""b::c", as well as "a::b::c"
   */

   Map_Range<Address,ELFSymbol*> &symmap = elffile->get_symbols();
   Map_Range<Address,ELFSymbol*>::iterator i;

   for (i=symmap.begin(); i!=symmap.end(); ++i) {
      ELFSymbol *sym = i->val;

      const char *name = sym->get_c_name() ;
      const char *demangled = cplus_demangle (name, 0) ;
      if (demangled != NULL) {
         aliases->add_alias (name, demangled) ;
         aliases->add_alias (demangled, name) ;
         int level = 1 ;
         for (;;) {
            int n = level ;
            std::string x = get_cpp_basename (demangled, n) ;
            if (n != 0) {
                break ;
            }
            cpp_basenames.insert (BaseMap::value_type(x, demangled)) ;
            level++ ;
         }
      }
   }
}

void SymbolTable::do_f90_alias() {
   Map_Range<Address,ELFSymbol*> &symmap = elffile->get_symbols();
   Map_Range<Address,ELFSymbol*>::iterator i;

   for (i=symmap.begin(); i!=symmap.end(); ++i) {
      ELFSymbol* sym = i->val;

      const char *name = sym->get_c_name() ;
      const char *ch = name + strlen (name) - 1; 
      char *demangled = NULL ;
      if (*ch == '_') {
          if (ch[-1] == '_') {
             ch-- ;
             const char *s = name ;
             bool contains_uscore = false ;
             while (s != ch) {
                if (*s == '_') {
                   contains_uscore = true ;
                   break ;
                }
                s++ ;
             }
             if (contains_uscore) {
                demangled = strndup (name, ch-name) ;
             } else {
                demangled = strndup (name, ch-name+1) ;
             }
         } else {                    // single underscore
            demangled = strndup (name, ch-name) ;
         }
      }
      if (demangled != NULL) {
         aliases->add_alias (name, demangled) ;
         aliases->add_alias (demangled, name) ;
      }
   }
}

void SymbolTable::read(PStream *os, bool reporterror) {
    try {
        read_string_table() ;
        read_abbreviations() ;
        read_debug_info(os, reporterror) ;
        read_frames() ;
        debugger_cu = new DwCUnit(this) ;
        bool cpp_found = false ;                // do we have a C++ compilation unit?
        bool fortran_found = false ;            // same for fortran

        for (uint i = 0 ; i < compilation_units.size() ; i++) {
            DwCUnit *cu = compilation_units[i] ;
            //DIEMap &symbols = cu->get_symbols() ;
            FileTable &filetable = cu->get_file_table() ;

            LineMatrix &lines = cu->get_line_matrix() ;
            int lastfile = -1 ;
            LineInfoVec *lastlines = NULL ;
            for (uint i = 0 ; i < lines.size() ; i++) {
                lineaddresses.push_back (lines[i]) ;            // add to main line map (indexed on address)

                LineInfoVec *linevec = NULL ;
                if (lines[i]->file == lastfile) {
                    linevec = lastlines ;
                } else {
                    std::string &filename = filetable[lines[i]->file]->basename ;                // filename for this line
                    FileMap::iterator fi = files.find (filename) ;                              // exitsts in map?
                    if (fi == files.end()) {                                            // no, add new entry
                        linevec = new LineInfoVec() ;
                        files[filename] = linevec ;
                    } else {
                        linevec = fi->second ;
                    }
                }
                linevec->push_back (lines[i]) ;                 // add line to line vec for file
                // cache for next time
                lastfile = lines[i]->file ;
                lastlines = linevec ;
            }

            if (cu->get_language() == DW_LANG_C_plus_plus) {
                cpp_found = true ;
            } else if (cu->get_language() == DW_LANG_Fortran77 || cu->get_language() == DW_LANG_Fortran90) {
                fortran_found = true ;
            }
        }

        std::sort (lineaddresses.begin(), lineaddresses.end(), Compare_linematrix()) ;

        // now sort all the line vectors in the file map (XXX: this is probably already sorted)
        for (FileMap::iterator fi = files.begin() ; fi != files.end() ; fi++) {
            LineInfoVec *vec = fi->second ;
            std::sort (vec->begin(), vec->end(), Compare_line()) ;
        }

        if (cpp_found) {
	  do_cxx_alias();
        }
        if (fortran_found) {
          do_f90_alias();
        }
    } catch (Exception &e) {
	e.report(std::cerr);
        throw e;
    }
}

// because of lazy loading of the dwarf info, some subprograms may not
// be available until they are referenced (inside a class, for example).  This
// virtual function is called whenever a new subprogram is added to the dwarf objects.

void SymbolTable::register_subprogram (std::string name, DIE *die) {
    Symbol *func = new Symbol (name, die) ;
    functions[name] = func ;
    Address lowpc = (Address)func->die->getAttribute (DW_AT_low_pc) + get_base() ;
    Address highpc = (Address)func->die->getAttribute (DW_AT_high_pc) + get_base() ;

    highpc -= 1 ; /* standard says address of first location after valid range */

    FunctionLocation* x = new FunctionLocation(this, lowpc, func, lowpc, highpc);
    funcmap.raw(lowpc, highpc, x);
}

void SymbolTable::register_symbol (std::string name, DIE *die) {
    variables[name] = new Symbol(name, die) ;             // all symbols, not just variables
}

void SymbolTable::register_struct (std::string name, DIE *die) {
    structs[name] = new Symbol(name, die) ;          
}

FunctionLocation* SymbolTable::find_function_by_address(Address addr) {
    FunctionLocation* x;
    if ( funcmap.get(addr,&x) ) {
       return NULL;
    }

    return x;
}

std::string SymbolTable::find_alias (std::string name) {
    const char *alias = aliases->find_alias (name.c_str()) ;
    if (alias == NULL) {
       return name ;
    }
    return alias ;
}


Location SymbolTable::find_address(Address addr, bool guess) {
    int start = 0 ;
    int end = lineaddresses.size() - 1 ;

    // if no DWARF info, look up the location in ELF.
    if (end == -1) {
        std::string symname = "" ;
        int offset = 0 ;
        elffile->find_symbol_at_address (addr, symname, offset) ;

        Location loc;
        loc.set_addr(addr);
        loc.set_dirlist(&dirlist);
        loc.set_symname(symname);
        loc.set_offset(offset);
        return loc;
    }

    while (start <= end) {
        int mid = (end + start) / 2 ;
        if (addr == lineaddresses[mid]->address) {
            LineInfo * info = lineaddresses[mid] ;
            FileTable & files = info->cu->get_file_table() ;
            FunctionLocation * func = find_function_by_address (addr) ;
            int offset = 0 ;
            if (func == NULL) {
                std::string symname = "" ;
                elffile->find_symbol_at_address (addr, symname, offset) ;

                Location loc;
                loc.set_lineinfo(info);
                loc.set_symtab(this);
                loc.set_addr(addr);
                loc.set_file(files[info->file]);
                loc.set_dirlist(&dirlist);
                loc.set_line(info->lineno);
                loc.set_symname(symname);
                loc.set_offset(offset);
                return loc;
            }
            int language = func->symbol->die->get_language() ;
            std::string funcname = func->get_name() ;
            bool infunc = addr >= func->get_start_address() && addr <= func->get_end_address() ;
            if (!infunc) {
                std::string symname = "" ;
                elffile->find_symbol_at_address (addr, symname, offset) ;
                if (language == DW_LANG_C_plus_plus) {
                   symname = find_alias (symname) ;
                }
                Location loc;
                loc.set_lineinfo(info);
                loc.set_symtab(this);
                loc.set_addr(addr);
                loc.set_funcloc(func);
                loc.set_file(files[info->file]);
                loc.set_dirlist(&dirlist);
                loc.set_line(info->lineno);
                loc.set_symname(symname);
                loc.set_offset(offset);
                return loc;
            } else {
                offset = addr - func->get_start_address() ;
            }
            if (language == DW_LANG_C_plus_plus) {
                funcname = find_alias (funcname) ;
            }
            Location loc;
            loc.set_lineinfo(info);
            loc.set_symtab(this);
            loc.set_addr(addr);
            loc.set_funcloc(func);
            loc.set_file(files[info->file]);
            loc.set_line(info->lineno);
            loc.set_dirlist(&dirlist);
            loc.set_symname(funcname);
            loc.set_offset(offset);
            return loc;
        }
        if (addr < lineaddresses[mid]->address) {
            end = mid - 1 ;
        } else {
            start = mid + 1 ;
        }
    }
    if (end < 0) {
        end = 0 ;
    }
    LineInfo *info = lineaddresses[end] ;
    FileTable & files = info->cu->get_file_table() ;
    FunctionLocation * func = find_function_by_address (addr) ;
    int offset = 0 ;
    if (func == NULL) {
        std::string symname = "" ;
        elffile->find_symbol_at_address (addr, symname, offset) ;

        Location loc;
        loc.set_lineinfo(info);
        loc.set_funcloc(func);
        loc.set_addr(addr);
        loc.set_dirlist(&dirlist);
        loc.set_symtab(this);
        loc.set_file(files[info->file]);
        loc.set_symname(find_alias(symname));
        loc.set_offset(offset);
        return loc;
    }
    std::string funcname = func->get_name() ;
    bool infunc = addr >= func->get_start_address() && addr <= func->get_end_address() ;
    int language = func->symbol->die->get_language() ;
    if (!infunc) {
        std::string symname = "" ;
        elffile->find_symbol_at_address (addr, symname, offset) ;
        if (language == DW_LANG_C_plus_plus) {
            symname = find_alias (symname) ;
        }
        Location loc;
        loc.set_lineinfo(info);
        loc.set_addr(addr);
        loc.set_dirlist(&dirlist);
        loc.set_symtab(this);
        loc.set_funcloc(func);
        loc.set_file(files[info->file]);
        loc.set_symname(symname);
        loc.set_offset(offset);
        return loc;
    } else {
        offset = addr - func->get_start_address() ;
    }
    if (language == DW_LANG_C_plus_plus) {
        funcname = find_alias (funcname) ;
    }

    Location loc;
    loc.set_lineinfo(info);
    loc.set_symtab(this);
    loc.set_addr(addr);
    loc.set_file(files[info->file]);
    loc.set_dirlist(&dirlist);
    loc.set_line(guess ? info->lineno : -1);
    loc.set_symname(funcname);
    loc.set_funcloc(func);
    loc.set_offset(offset);
    return loc;
}

LineInfo* SymbolTable::get_line_info(Address address) {
    int start = 0 ;
    int end = lineaddresses.size() - 1 ;

    while (start <= end) {
        int mid = (end + start) / 2 ;
        if (address == lineaddresses[mid]->address) {
            return lineaddresses[mid] ;
        }
        if (address < lineaddresses[mid]->address) {
            end = mid - 1 ;
        } else {
            start = mid + 1 ;
        }
    }
    return NULL ;
}

void SymbolTable::list_functions(EvalContext &context) {
    for (uint i = 0 ; i < compilation_units.size() ; i++) {
        DwCUnit *cu = compilation_units[i] ;
        std::string filename = cu->get_cu_die()->getAttribute (DW_AT_name) ;
        context.os.print ("File %s:\n", filename.c_str()) ;
        context.language = cu->get_language() ;
        DIEMap &funcs = cu->get_subprograms() ;
        for (DIEMap::iterator f = funcs.begin() ; f != funcs.end() ; f++) {
            f->second->check_loaded() ;
            f->second->print (context, 0, 0) ;  
            if (context.language == DW_LANG_Fortran77 || context.language == DW_LANG_Fortran90) {
            } else {
                context.os.print (";\n") ;
            }
        }
    }
  
}

void SymbolTable::list_variables(EvalContext &context) {
    for (uint i = 0 ; i < compilation_units.size() ; i++) {
        DwCUnit *cu = compilation_units[i] ;
        std::string filename = cu->get_cu_die()->getAttribute (DW_AT_name) ;
        context.os.print ("File %s:\n", filename.c_str()) ;
        context.language = cu->get_language() ;
        DIEMap &syms = cu->get_symbols() ;
        for (DIEMap::iterator f = syms.begin() ; f != syms.end() ; f++) {
            if (f->second->get_tag() == DW_TAG_variable) {
                f->second->print (context, 0, 0) ;
                if (context.language == DW_LANG_Fortran77 || context.language == DW_LANG_Fortran90) {
                } else {
                    context.os.print (";\n") ;
                }
            }
        }
    }
}

void SymbolTable::delete_temp_dies() {
    debugger_cu->delete_temp_dies() ;
}

void SymbolTable::keep_temp_dies(std::vector<DIE*> &buffer) {
    debugger_cu->keep_temp_dies(buffer) ;
}

void SymbolTable::keep_temp_die(DIE *die) {
    debugger_cu->keep_temp_die(die) ;
}

DIE * SymbolTable::new_int() {
    return new_scalar_type ("int", DW_ATE_signed, 4) ;
}

DIE * SymbolTable::new_boolean() {
    return new_scalar_type ("boolean", DW_ATE_boolean, 1) ;
}

DIE * SymbolTable::new_scalar_type(std::string name, int encoding, int size) {
    return debugger_cu->new_scalar_type (name, encoding, size) ;
}

DIE *SymbolTable::new_pointer_type(DIE * to) {
    return debugger_cu->new_pointer_type (to) ;
}

DIE *SymbolTable::new_const_type(DIE * to) {
    return debugger_cu->new_const_type (to) ;
}

DIE *SymbolTable::new_volatile_type(DIE * to) {
    return debugger_cu->new_volatile_type (to) ;
}

DIE *SymbolTable::new_array_type(DIE * to) {
    return debugger_cu->new_array_type (to) ;
}

DIE *SymbolTable::new_subrange_type(int lb, int ub) {
    return debugger_cu->new_subrange_type (lb, ub) ;
}

DIE *SymbolTable::new_subrange_type() {
    return debugger_cu->new_subrange_type () ;
}

DIE *SymbolTable::new_string_type(int len) {
    return debugger_cu->new_string_type (len) ;
}

DIE *SymbolTable::new_subroutine_type() {
    return debugger_cu->new_subroutine_type () ;
}

DIE *SymbolTable::new_formal_parameter(DIE *type) {
    return debugger_cu->new_formal_parameter (type) ;
}

DIE * SymbolTable::find_symbol(std::string name, bool search_alias) {
    SymbolMap::iterator s = functions.find (name) ;
    if (s != functions.end()) {
        return s->second->die ;
    }
    s = variables.find(name) ;
    if (s != variables.end()) {
        return s->second->die ;
    }
    s = structs.find(name) ;
    if (s != structs.end()) {
        return s->second->die ;
    }
    if (search_alias) {
        std::string alias = find_alias (name) ;
        return find_symbol (alias, false) ;
    }
    return NULL ;
}

// find a scope by looking at all the compilation units
DIE *SymbolTable::find_scope (std::string name) {
    for (uint i = 0 ; i < compilation_units.size() ; i++) {
        DwCUnit *cu = compilation_units[i] ;
        DIE *die = cu->get_cu_die() ;
        DIE *sym = die->find_scope (name) ;
        if (sym != NULL) {
            return sym ;
        }
    }
    return NULL ;
}


DIE * SymbolTable::find_struct(std::string name) {
    SymbolMap::iterator s = structs.find (name) ;
    if (s != structs.end()) {
        return s->second->die ;
    }
    return NULL ;
}

// skip the preamble for a function at the given address
Address SymbolTable::skip_preamble (Address addr) {
    int start = 0 ;
    int end = lineaddresses.size() - 1 ;
    int foundindex = -1 ;

    FunctionLocation *func = find_function_by_address (addr) ;

    while (start <= end) {
        int mid = (end + start) / 2 ;
        if (addr == lineaddresses[mid]->address) {
            foundindex = mid ;
            break ;
        }
        if (addr < lineaddresses[mid]->address) {
            end = mid - 1 ;
        } else {
            start = mid + 1 ;
        }
    }
    if (foundindex == -1) {             // not found?
        return 0 ;
    }
    int size = lineaddresses.size() ;
    while (foundindex < size) {
        if (lineaddresses[foundindex]->address != addr) {
            break ;
        }
        foundindex++ ;
    }
    if (foundindex < size) {
        Address newaddr = lineaddresses[foundindex]->address ;
        FunctionLocation *newfunc = find_function_by_address (newaddr) ;                // must be in same function
        if (newfunc != func) {
            return 0 ;
        }
        return lineaddresses[foundindex]->address ;
    }
    return 0 ;
}

// we don't want to print an address if the pc is at the first line of a function.  The first
// line is actually the second line held in the line table.
bool SymbolTable::at_first_line (FunctionLocation *func, Address addr) {
    int start = 0 ;
    int end = lineaddresses.size() - 1 ;
    int foundindex = -1 ;
    Address saddr = func->get_start_address() ;                 // start address of function

    while (start <= end) {
        int mid = (end + start) / 2 ;
        if (saddr == lineaddresses[mid]->address) {
            foundindex = mid ;
            break ;
        }
        if (saddr < lineaddresses[mid]->address) {
            end = mid - 1 ;
        } else {
            start = mid + 1 ;
        }
    }
    if (foundindex == -1) {             // not found?
        return false ;
    }

    // now find the next line
    int size = lineaddresses.size() ;
    while (foundindex < size) {
        if (lineaddresses[foundindex]->address != saddr) {
            break ;
        }
        foundindex++ ;
    }

    if (foundindex < size) {
        Address newaddr = lineaddresses[foundindex]->address ;
        FunctionLocation *newfunc = find_function_by_address (newaddr) ;                // must be in same function
        if (newfunc != func) {
            return false ;
        }
        return lineaddresses[foundindex]->address == addr ;
    }
    return 0 ;
}

// is the address at the start of a function?
bool SymbolTable::is_at_function_start (Address addr) {
    int start = 0 ;
    int end = function_start_addresses.size() - 1 ;

    while (start <= end) {
        int mid = (end + start) / 2 ;
        if (addr == function_start_addresses[mid]) {
            return true ;
        }
        if (addr < function_start_addresses[mid]) {
            end = mid - 1 ;
        } else {
            start = mid + 1 ;
        }
    }
    return false ;
}

bool SymbolTable::find_function (std::string name, int offset, std::string &filename, int &lineno) {
    SymbolMap::iterator i = functions.find (name) ;
    if (i == functions.end()) {
        //printf ("function not found\n") ;
        return false ;
    }
    DIE *func = i->second->die ;
    Address lowpc = (Address)func->getAttribute (DW_AT_low_pc) + get_base() ;         // get start address for function
    Address addr = lowpc + offset ;
    Location loc = find_address (addr, true) ;
    if (loc.get_file() != NULL) {
        char buf[1024] ;
        DirectoryTable &dirtab = loc.get_lineinfo()->cu->get_directory_table() ;
        std::string dirname = dirtab[loc.get_file()->dir] ;
        if (dirname != "") {
           dirname += '/' ;
        }
        lineno = loc.get_line() ;
        snprintf (buf, sizeof(buf), "%s%s", dirname.c_str(), loc.get_file()->name.c_str()) ;
        filename = buf ;
        return true ;
    } else {
        return false ;
    }
}

// given a file and line, return the address at the start of the line
// look up the mapping of filename vs vector of LineInfo and then
// use a binary search to find the address held in the LineInfo
// object

Address SymbolTable::find_line (std::string filename, int lineno) {
    std::string::size_type slash = filename.rfind ('/') ;
    if (slash != std::string::npos) {
        filename = filename.substr (slash+1) ;
    }
    FileMap::iterator fi = files.find (filename) ;
    if (fi == files.end()) {
       return 0 ;
    }
    int start = 0 ;
    LineInfoVec &lines = *fi->second ;
    int end = lines.size() - 1 ;

    while (start <= end) {
        int mid = (end + start) / 2 ;
        if (lineno == lines[mid]->lineno) {
            return lines[mid]->address ;
        }
        if (lineno < lines[mid]->lineno) {
            end = mid - 1 ;
        } else {
            start = mid + 1 ;
        }
    }
    return 0 ;
}

void SymbolTable::enumerate_functions (std::string name, std::vector<std::string> &results) {
    std::pair<BaseMap::iterator,BaseMap::iterator> p = cpp_basenames.equal_range (name) ;
    while (p.first != p.second) {
        results.push_back ((*p.first).second) ;
        p.first++ ;
    }

    // there might be regular function with the same name.  If so, add it
    SymbolMap::iterator i = functions.find (name) ;
    if (i != functions.end()) {
        results.push_back (name) ;
    }

    std::vector<std::string>::iterator end = std::unique (results.begin(), results.end()) ;
    results.erase (end, results.end()) ;

    // XXX: what about static functions in files
}

DwCUnit *SymbolTable::find_compilation_unit (std::string name) {
    for (uint i = 0 ; i < compilation_units.size() ; i++) {
        DwCUnit *cu = compilation_units[i] ;
        DIE *die = cu->get_cu_die() ;
        std::string cu_name = die->get_name() ;
        std::string::size_type slash = cu_name.rfind ('/') ;
        if (slash != std::string::npos) {
            cu_name = cu_name.substr (slash+1) ;
        }
        if (cu_name == name) {
            return cu ;
        }
    }
    return NULL ;
}


void SymbolTable::list_source_files (PStream &os, uint width) {
    uint x = 0 ;
    bool comma = false ;
    std::set<std::string> done ;
    for (uint i = 0 ; i < compilation_units.size() ; i++) {
        DwCUnit *cu = compilation_units[i] ;
        FileTable &files = cu->get_file_table() ;
        for (uint j = 1 ; j < files.size() ; j++) {
            std::string name = files[j]->name ;
            if (done.count(name) == 0) {
                if (comma) os.print (", ") ;
                if ((x + name.size() + 2) >= width) {
                    os.print ("\n") ;
                    x = 0 ;
                }
                os.print ("%s", name.c_str()) ;
                x += name.size() + 2 ;
                comma = true ;
                done.insert (name) ;
            }
        }
    }
    os.print ("\n") ;
}

// complete the name of a function.  The name comes from the aliases table and
// the cpp_basenames table
void SymbolTable::complete_function (std::string name, std::vector<std::string> &result) {

    // look in aliases
    aliases->complete (name, result) ;

    // look in C++ basenames
    for (BaseMap::iterator i = cpp_basenames.begin() ; i != cpp_basenames.end() ; i++) {
        if (strncmp (i->first.c_str(), name.c_str(), name.size()) == 0) {
            result.push_back (i->first.substr (name.size())) ;
        }
    }

    // look in the function map
    for (SymbolMap::iterator i = functions.begin() ; i != functions.end() ; i++) {
        if (strncmp (i->first.c_str(), name.c_str(), name.size()) == 0) {
            result.push_back (i->first.substr (name.size())) ;
        }
    }

}

File *SymbolTable::find_file (std::string name) {
    for (uint i = 0 ; i < compilation_units.size() ; i++) {
        DwCUnit *cu = compilation_units[i] ;
        FileTable &files = cu->get_file_table() ;
        for (uint j = 1 ; j < files.size() ; j++) {
            if (files[j]->basename == name) {
                return files[j] ;
            }
            if (files[j]->name == name) {
                return files[j] ;
            }
        }
    }
    return NULL ;
}

