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

file: dbg_elf.h
created on: Fri Aug 13 11:02:27 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#ifndef dbg_elf_h_included
#define dbg_elf_h_included

#include <elf.h>
#include "dbg_types.h"

#include "bstream.h"
#include "pstream.h"
#include "map_range.h"

// classes
class ELF ;

class ProgramSegment {
public:
    ProgramSegment(ELF *elf, Address baseaddr) ;
    ~ProgramSegment() ; 
    void read (std::istream & stream) ;
    void *map (int fd) ;
    int32_t get_type() { return type ; }
    Address get_start() { return vaddr ; }
    Address get_end() { return vaddr+filesz-1; }
    int64_t get_size() { return memsz ; }
    BVector get_contents (std::istream & stream) ;
    
protected:
private:
    ELF *elf ;
    int32_t type ;
    int32_t flags ;
    Offset offset ;
    Address vaddr ;
    Address paddr ;
    int64_t filesz ;
    int64_t memsz ;
    int64_t align ;
    Address baseaddr ;
} ;

class ELFSymbol ;

typedef std::map<std::string, ELFSymbol *> ELFSymbolMap ;

class Section {
public:
    Section(ELF *elf, int index) ;
    ~Section() ; 
    std::string get_name () ;
    Address get_addr () ;
    int get_index () ;
    int read (std::istream & stream) ;
    void print () ;
    int get_offset () ;
    int get_size () ;
    void set_name (std::istream & stream, Section * nametable) ;
    std::string read_string (std::istream & stream, int stroffset) ;
    BVector get_contents (std::istream & stream) ;
protected:
private:
    ELF *elf ;
    int index ; 
    std::string name ; 
    int32_t nameindex ; 
    int32_t type ; 
    int64_t flags ; 
    Address addr ; 
    Offset offset ; 
    int64_t size ; 
    int32_t link ; 
    int32_t info ; 
    int64_t addralign ; 
    int64_t entsize ; 
} ;

class ELFSymbol {
public:
    ELFSymbol(ELF *elf) ;
    ~ELFSymbol() ; 
    std::string get_name () ;
    const char *get_c_name () { return name.c_str() ; }
    Address get_value () ;
    Section* get_section () ;
    int read (std::istream & stream, Address baseaddr) ;
    void set_name (std::istream & stream, Section * nametable) ;
    int get_size () ;
    void print () ;
    Address value ; 
    bool operator < (ELFSymbol &sym) { return value < sym.value ; }
    byte get_info() { return info ; }
    byte get_other() { return other ; }
    int get_section_index() { return shndx ; }
protected:
private:
    ELF *elf ;
    std::string name ; 
    int32_t nameindex ; 
    int64_t size ; 
    byte info ; 
    byte other ; 
    int16_t shndx ; 
    Section* section ; 
} ;

class ELF {
    friend class ELFSymbol ;
    friend class Section ;
    friend class ProgramSegment ;

public:
    ELF(std::string name, Offset mainoffset=0) ;
    ~ELF() ; 
    bool is_elf64 () ;
    bool is_little_endian () ;
    std::istream *open (Address baseaddress=0) ;
    BVector get_section (std::istream & stream, std::string name) ;
    Section *find_section (std::string name) ;
    Section *find_section_by_index (int index) ;
    void read_symbol_table (std::istream & stream, Address baseaddr) ;
    Address find_symbol (std::string name, bool caseblind = false) ;
    Section *find_symbol_section (std::string name, bool caseblind = false) ;
    void find_symbol_at_address (Address addr, std::string &name, int &offset) ;
    Section *find_section_at_addr (Address addr) ;
    void list_symbols (PStream &os) ;
    void list_functions (PStream &os) ;
    void list_variables (PStream &os) ;
    int get_num_segments() { return segments.size() ; }
    ProgramSegment *get_segment (int i) { return segments[i]; }
    Address get_base() { return base ; }                        // base address


    Map_Range<Address,ELFSymbol*>& get_symbols() {
       return symmap;
    }

    ProgramSegment *find_segment (Address addr) ;
    std::string get_name() { return name ; }
protected:
private:
    void read_symtab (std::istream &stream, Section *symtab, Address baseaddr, Section *strtab) ;
    byte read_byte (std::istream & stream) ;
    int64_t read_word8 (std::istream & stream) ;
    int32_t read_word4 (std::istream & stream) ;
    int16_t read_word2 (std::istream & stream) ;
    Address read_address (std::istream & stream) ;
    Offset read_offset (std::istream & stream) ;
    int64_t read_xword (std::istream & stream) ;
    void read_header (std::istream & stream, Address baseaddr) ;
    void print_header() ;
    std::string name ; 
    Offset mainoffset ; 
    byte ident[4] ; 
    byte fileclass ; 
    byte dataencoding ; 
    byte elfversion ; 
    byte abi ; 
    byte abiversion ; 
    int16_t type ; 
    int16_t machine ; 
    int32_t version ; 
    Address entry ; 
    Offset phoff ; 
    Offset shoff ; 
    int32_t flags ; 
    int16_t ehsize ; 
    int16_t phentsize ; 
    int16_t phnum ; 
    int16_t shentsize ; 
    int16_t shnum ; 
    int16_t shstrndx ; 
    std::vector<Section *> sections ; 
    std::vector<ProgramSegment *> segments ; 

    Map_Range<Address,ELFSymbol*> symmap;

    ELFSymbolMap symbols ; // map of name vs ELFSymbol
    ELFSymbolMap cbsymbols ; // map of name vs ELFSymbol (case blind)
    bool caseblind_ok ;
    void make_cb_symbol_table() ;
    Address base ;
} ;


#endif
