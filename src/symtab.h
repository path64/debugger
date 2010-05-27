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

file: symtab.h
created on: Fri Aug 13 11:02:33 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#ifndef symtab_h_included
#define symtab_h_included

#include "dbg_dwarf.h"
#include "dwf_info.h"
#include "file_info.h"
#include "pstream.h"
#include "gen_loc.h"

// imported classes
class Process ;
class Frame ;
class Architecture ;
class EvalContext ;

//
// alias object (holds aliases of symbol names)
//
struct cstring_less {
    bool operator() (const char *s1, const char *s2) {
        return strcmp (s1,s2) < 0 ;
    }
} ;

class AliasManager {
public:
    AliasManager() ;
    ~AliasManager() ;
    void lock() ;
    void unlock() ;
    void add_alias (const char *name, const char *alias) ;
    const char *find_alias(const char *alias) ;
    void list_aliases() ;
    void complete (std::string name, std::vector<std::string> &result) ;
private:
    //typedef __gnu_cxx::hash_map<const char *, const char *, cstring_equal_to> AliasMap ;
    typedef std::map<const char *, const char *, cstring_less> AliasMap ;
    AliasMap aliases ;
} ;

class SymbolTable ;

class Symbol {
public:
    Symbol(std::string name, DIE * die) ;
    ~Symbol() ; 
    std::string name ; 
    DIE * die ; 
} ;




// the symbol table provides a number of indexes to allow quick searches
// of the data.
// XXX: describe the indexes here

class SymbolTable: public DwInfo  {
public:
    SymbolTable(Architecture *arch, ELF * elf, std::istream & stream, AliasManager *aliases,
      DirectoryTable &dirlist, PStream *os = NULL, bool reporterror = false) ;
    ~SymbolTable() ; 
    Location  find_address (Address addr, bool guess) ;
    LineInfo * get_line_info (Address address) ;
    void list_functions (EvalContext &context) ;
    void list_variables (EvalContext &context) ;
    void list_source_files (PStream &os, uint width) ;
    void delete_temp_dies() ;
    void keep_temp_dies (std::vector<DIE*> &buffer) ;
    void keep_temp_die (DIE *die) ;
    DIE * new_int() ;
    DIE * new_boolean() ;
   
    DIE * new_scalar_type (std::string name, int encoding, int size = 0) ;
    DIE * new_pointer_type (DIE * to) ;
    DIE * new_const_type (DIE * to) ;
    DIE * new_volatile_type (DIE * to) ;
    DIE * new_array_type (DIE * to) ;
    DIE * new_subrange_type (int lb, int ub) ;
    DIE * new_subrange_type () ;
    DIE * new_string_type (int len) ;
    DIE * new_subroutine_type () ;
    DIE * new_formal_parameter (DIE *type) ;
    DIE * find_symbol (std::string name, bool search_alias=true) ;
    DIE * find_scope (std::string name) ;
    DIE * find_struct (std::string name) ;
    Address skip_preamble (Address addr) ;
    bool at_first_line (FunctionLocation *func, Address addr) ;
    bool is_at_function_start (Address addr) ;          // is the address at the start of a function?
    bool find_function (std::string name, int offset, std::string &filename, int &lineno) ;          // find filename:lineno for function+offset
    Address find_line (std::string filename, int line) ;
    std::string find_alias (std::string name) ;
    void enumerate_functions (std::string name, std::vector<std::string> &result) ;
    DwCUnit *find_compilation_unit (std::string name) ;
    void complete_function (std::string name, std::vector<std::string> &result) ;
    Architecture *arch ;

    File *find_file (std::string name) ;
protected:
private:
    void read(PStream *os, bool reporterror) ;
    void wait() ;                       // wait for symbol table to become ready

    FunctionLocation * find_function_by_address (Address addr) ;

    typedef std::map<std::string, Symbol *> SymbolMap ;
    typedef std::vector<LineInfo*> LineInfoVec ;

    // for looking up file:line we need a mapping of filename vs array of lines, sorted by linenum
    typedef std::map<std::string, LineInfoVec*> FileMap ;

    SymbolMap functions ; 
    SymbolMap variables ; 
    SymbolMap structs ; 
    FileMap files ;

    LineInfoVec lineaddresses ; // vector of LineInfo, sorted on address

    Map_Range<Address,FunctionLocation*> funcmap;

    std::vector<Address> function_start_addresses ;                     // add function start addresses
    DwCUnit * debugger_cu ; 
    AliasManager *aliases ;

    void do_cxx_alias();
    void do_f90_alias();

    void register_subprogram (std::string name, DIE *die) ;
    void register_symbol (std::string name, DIE *die) ;
    void register_struct (std::string name, DIE *die) ;

    typedef std::multimap<std::string, const char*> BaseMap ;
    BaseMap cpp_basenames ;
public:
    DirectoryTable &dirlist ;

} ;


extern std::string demangle(const std::string &func);


#endif
