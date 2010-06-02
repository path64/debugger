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

file: dwf_info.h
created on: Fri Aug 13 11:02:27 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#ifndef _DWF_INFO_H_
#define _DWF_INFO_H_

#include "dbg_elf.h"
#include "dwf_stab.h"
#include "dwf_abbrv.h"
#include "dbg_dwarf.h"
#include "dwf_cunit.h"

class DwInfo {
public:
    DwInfo(ELF * elffile, std::istream&);
    virtual ~DwInfo() ;

    bool is_elf64() { return elffile->is_elf64(); }
    bool is_dwf64() { return false; /* XXX: support this */ }
    bool do_swap() { return ! elffile->is_little_endian(); }

    const char* get_string(Offset pos) {
       return string_table->getString(pos);
    }

    void set_abb_tab(DwAbbrvTab* t) { abb_tab = t; }
    DwAbbrvMap* get_abb_map(Offset p) { return abb_tab->get(p); }
    void dump_abb_tab() { abb_tab->dump(); }

    void add_cunit(DwCUnit* cu) { cu_list.push_back(cu); }
    void dump_dbg_info() { 
       std::list<DwCUnit*>::iterator i;
       for (i=cu_list.begin(); i!=cu_list.end(); i++) {
          (*i)->dump_dbg_info();
       }
    }

   /* I'm going to gradually migrate the existing code to
    * something a little cleaner.  The functions below this
    * comment are considered "unclean". This refers to the
    * interface only, I'm not terribly fond of how the above
    * functions are implemented.
    */
private:
    friend class FDE ;
    friend class CIE ;
    friend class DwCUnit ;
public:
    void read_abbreviations () ;
    void dump () ;
    void show_symbols () ;
    void read_debug_info (PStream *os, bool reporterror) ;
    void read_string_table () ;
    void read_pub_names() ;
    void read_frames () ;
    void read_frames (std::string section_name) ;
    void read_location_table (DwCUnit* cu) ;
    CUVec& get_compilation_units () ;
    bool is_little_endian() { return elffile->is_little_endian() ;}
    DwSTab *get_string_table() { return string_table ; }
    DIE *make_die (DwCUnit *cu, DIE *parent, Abbreviation *abbrev) ;
    Abbreviation *find_cu_abbreviation (Offset offset) ;
    FDEMap fdes ; // map of FDE start address vs FDE
    FDEVec fdevec ;     // vector of FDE* sorted on start address
    Address GOT_address() { return got ; }
    Address get_base() { return elffile->get_base() ; }
    FDE *find_fde (Address addr) ;              // binary search for FDE on address
    virtual DIE *find_symbol (std::string name, bool search_alias = true) { return NULL; }
    virtual DIE *find_struct (std::string name) { return NULL; }

    int get_ver() { return 3; }

    BVector get_loc_expr(DwCUnit*, Address offset, Address pc);

protected:
    ELF * elffile ;
    std::list<DwCUnit*> cu_list;

    CUVec compilation_units ;
    std::istream& stream;

    virtual void register_subprogram (std::string name, DIE *die) ;
    virtual void register_symbol (std::string name, DIE *die) ;
    virtual void register_struct (std::string name, DIE *die) ;

private:
    Address read_address (BStream& stream);

    void disassemble_location (DwCUnit *cu, BVector location) ;
    void read_frame_entry (Section *section, BStream & stream, bool is_eh) ;
    DwSTab * string_table ; // the string table
    LocationListTable* location_table ; // the location table

    CIEMap cies ; // map of offset vs CIE
    typedef std::map<Offset, Abbreviation*> AbbreviationMap ;
    AbbreviationMap compilationunit_abbreviations ; // map of file offset vs Abbrevation

    DwAbbrvTab* abb_tab;

    int opcode_opcounts[256] ;
    Address got ;                       // address of the global offset table for data relative encodings in FDEs
} ;

#endif
