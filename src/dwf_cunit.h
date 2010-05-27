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

file: dwf_cunit.h
created on: Fri Aug 13 11:02:27 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#ifndef _DWF_CUNIT_H_
#define _DWF_CUNIT_H_

#include "dbg_types.h"
#include "dwf_stab.h"
#include "dwf_locs.h"
#include "dwf_entry.h"
#include "type_base.h"
#include "file_info.h"

class LazyDIE;
class Process;
class LineInfo;
class DwInfo;

#include <vector>

typedef std::vector<LineInfo *> LineMatrix;

class DwCUnit {
public:
    DwCUnit(DwInfo *dwarf) ;
    ~DwCUnit() ;

    DwEntry* get_die(Offset off){ return NULL; /* XXX: needs to be impl. */}
    bool do_swap() { return ! is_little_endian(); }

    void set_sec_length(Offset p) { sec_length=p; }
    Offset get_sec_length() { return sec_length; }

    void set_sec_offset(Offset p) { start_offset=p; }
    Offset get_sec_offset() { return start_offset; }

    void set_dwf_ver(int n) { dwarf_version=n; }
    int get_dwf_ver() { return dwarf_version; }

    void set_ptr_size(int n) { addr_size=n; }
    int get_ptr_size() { return addr_size; }

    void set_abb_offset(Offset p) { abb_offset=p; }
    Offset get_abb_offset() { return abb_offset; }

    void set_abb_map(DwAbbrvMap* m) { abb_map=m; }
    DwAbbrv* get_abb_num(Offset p) {
       DwAbbrvMap::iterator i;
       i = abb_map->find(p);
       if (i == abb_map->end()) {
          return NULL;
       }
       return i->second;
    }

    void set_dbg_info(DwEntry* e) { dbg_info=e; }
    DwEntry* get_dbg_info() { return dbg_info; }
    void dump_dbg_info(void);


 
   /* all function after this point are "dirty" */
public:
    void add_die (Offset offset, DIE * die) ;
    Address get_base_pc();
    int getAddrSize () ;
    Abbreviation * getAbbreviation (int num) ;
    DIE * get_die (Attribute * attr, Offset off) ;
    void read (BStream &stream) ;
    void dump () ;
    int get_language () ;
    void show_symbols () ;
    void build_line_matrix (BVector data) ;
    void show_line_matrix () ;
    void info (PStream &os) ;
    DIEMap & get_symbols () ;
    DIEMap & get_subprograms () ;
    LineMatrix &get_line_matrix () ;
    DirectoryTable & get_directory_table () ;
    FileTable & get_file_table () ;
    DIE * new_scalar_type (std::string name, int encoding, int size = 0) ;
    DIE * new_pointer_type (DIE * to) ;
    DIE * new_const_type (DIE * to) ;
    DIE * new_volatile_type (DIE * to) ;
    DIE * new_array_type (DIE * to) ;
    DIE * new_subrange_type (int lb, int ub) ;
    DIE * new_subrange_type () ;
    DIE * new_string_type(int len) ;
    DIE * new_subroutine_type() ;
    DIE * new_formal_parameter(DIE *type) ;
    DIE * find_symbol (std::string name) ;
    DIE * find_struct (std::string name) ;

    void delete_temp_dies() ;                // delete all temporary dies
    void keep_temp_dies(std::vector<DIE*> &buffer) ;                // keep all temporary dies and copy them to buffer
    void keep_temp_die (DIE *die) ;
    Address get_base() ;

    Offset start_offset ;
    Offset sec_length;
    Offset abb_offset;

    bool is_little_endian() ;
    DwSTab *get_string_table() ;
    DIE *make_die (DwCUnit *cu, DIE *parent, Abbreviation *abbrev) ;
    Address read_address (BStream &stream) ;
    void disassemble_location (DwCUnit *cu, BVector location) ;
    DwLocExpr evaluate_location (DwCUnit *cu, Address frame_base,
         AttributeValue& attr, Process* process);
    DwLocExpr evaluate_location (DwCUnit *cu, Address frame_base,
         AttributeValue& attr, Process* process, Address top_addr);
    BStream &get_stream() { return mainstream ; }

    AttributeValue get_die_reference(Attribute*attr, Offset offset) {
       DIE *die = get_die (attr, offset) ;
       if (die == NULL) {
          return offset ;
       }
       return die ;
    }

    DwInfo* get_dwinfo() {return dwarf;}

    void add_lazy_die (DIE *die, Offset s, Offset e) ;
    DIE *find_lazy_die (Offset offset) ;
    DIE *get_cu_die() { return cu_die ; }

    bool is_64bit();
protected:

private:
    DwInfo *dwarf ;
    BStream mainstream ;
    int debug_info_length ;
    int dwarf_version ;
    int abbrev_offset ;
    int addr_size ;

    DwAbbrvMap* abb_map; /* XXX: replaces abbrev */
    Abbreviation * abbrev ;

    DwEntry* dbg_info; /* XXX: replaces cu_die */
    DIE *cu_die ; // compile_unit die

    // remove these, they are not used
    DIEMap symbols ; // top level map of symbol name vs DIE
    DIEMap subprograms ; // map of name vs DIE for subprograms (functions)
    DIEMap structs ;    // map of name vs DIE for structs and union tags

    DirectoryTable directory_table ; // vector of strings
    FileTable file_table ; // vector of Files
    std::vector<int> opcode_lengths ;
    LineMatrix line_matrix ;

    OffsetMap dies ; // map of offset vs DIE

    typedef std::vector<Attribute*> AttrVec ;
    //typedef __gnu_cxx::hash_map<Offset, AttrVec> FixupMap ;           // problem on 32 bit
    typedef std::map<Offset, AttrVec> FixupMap ;                

    FixupMap fixups ; // map of offset vs vector of Attribute

    void addFixup (Attribute * attr, Offset offset) ;
    void dofixup (Offset offset, DIE * die) ;

    std::list<LazyDIE*> lazy_dies ;              // dies that haven't been loaded yet

    void register_subprogram (std::string name, DIE *die) ;
    void register_symbol (std::string name, DIE *die) ;

    std::string comp_dir ;                      // compilation directory

    DIE * add_temp_die (DIE *die) ;
    std::list<DIE*> temp_dies ;
} ;

#endif
