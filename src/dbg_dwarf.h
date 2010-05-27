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

file: dbg_dwarf.h
created on: Fri Aug 13 11:02:27 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#ifndef dbg_dwarf_h_included
#define dbg_dwarf_h_included

class DIE ;
class Abbreviation ;
class LineInfo ;
class File ;
class DwCUnit ;
class DwInfo ;
class CIE ;
class FDE ;
#include <vector>
#include <map>
#include <list>


#include "dwf_spec.h"
#include "dbg_elf.h"
#include "bstream.h"
#include "expr.h"
#include "pstream.h"

#include <fstream>
#include "type_base.h"
#include "dwf_locs.h"


typedef std::vector<DwCUnit *> CUVec ;
typedef std::map<Offset, CIE*> CIEMap ;
typedef std::map<Address, FDE*> FDEMap ;
typedef std::vector<FDE*> FDEVec ;              // sorted on start address



// a DIE that has not been loaded yet
// This is used when a pointer to another die that hasn't been loaded yet
// needs to be resolved.  If the offset is within the range of the
// start..end then the die's children are loaded

struct LazyDIE {
    LazyDIE (DIE *d, Offset s, Offset e) :die(d), start(s), end(e) {}
    DIE *die ;
    Offset start ;              // offset of first child
    Offset end ;                // offset of sibling
} ;


// classes

class Attribute ;


class LocationListEntry {
public:
    Address low ;
    Address high ;
    BVector expr ;
} ;

class LocationList {
public:
   LocationList () ;
   ~LocationList () ;
   void clear (void) ;
   void push_back (LocationListEntry& entry) ;
   BVector getexpr (DwCUnit* cu, Address pc) ;
protected:
private:
   std::list<LocationListEntry> list;
} ;

class LocationListTable {
public:
   LocationListTable(DwCUnit* cu, BVector data) ;
   ~LocationListTable() ;
   LocationList& getlist (Offset offset) ;
   BVector getexpr (DwCUnit* cu, Offset offset, Address pc) ;
protected:
private:
   std::map<int,LocationList> table ;
} ;


typedef DIE Entry_point;
typedef DIE Imported_declaration;
typedef DIE Label;
typedef DIE Subroutine_type;
typedef DIE Unspecified_parameters;
typedef DIE Variant;
typedef DIE Common_inclusion;
typedef DIE Inlined_subroutine;
typedef DIE Module;
typedef DIE Ptr_to_member_type;
typedef DIE Set_type;
typedef DIE Subrange_type;
typedef DIE With_stmt;
typedef DIE Access_declaration;
typedef DIE Catch_block;
typedef DIE Constant;
typedef DIE File_type;
typedef DIE Friend;
typedef DIE Packed_type;
typedef DIE Thrown_type;
typedef DIE Try_block;
typedef DIE Variant_part;
typedef DIE Lo_user;
typedef DIE MIPS_loop;
typedef DIE Hi_user;
typedef DIE Format_label;
typedef DIE Function_template;
typedef DIE Class_template;


class Formal_parameter: public DIE  {
public:
    Formal_parameter(DwCUnit *cu, DIE *parent, Abbreviation *abbrev) ;
    Formal_parameter(DwCUnit *cu, DIE *parent, int tag) ;
    ~Formal_parameter() ;
    virtual void print (EvalContext &ctx, int indent, int level=0) ;
    Value evaluate (EvalContext & context) ;
    bool is_local_var()  ;
protected:
} ;

class Lexical_block: public DIE  {
public:
    Lexical_block(DwCUnit *cu, DIE *parent, Abbreviation *abbrev) ;
    ~Lexical_block() ;
    virtual void print (EvalContext &ctx, int indent, int level=0) ;
    void find_symbol (std::string name, Address pc, std::vector<DIE*> &result, DIE *caller = NULL) ;
    void get_local_variables (std::vector<DIE*> &vec) ;
protected:
    DIEMap symbols ;
    bool symbolsok ;
    std::vector<DIE*> lexical_blocks ;
    std::vector<DIE*> enumerations ;
} ;

class Member: public DIE  {
public:
    Member(DwCUnit *cu, DIE *parent, Abbreviation *abbrev) ;
    ~Member() ;
    virtual void print (EvalContext &ctx, int indent, int level=0) ;
    Value evaluate (EvalContext & context, Value &base) ;
    Value evaluate (EvalContext & context) ;
protected:
} ;

class Reference_type: public DIE  {
public:
    Reference_type(DwCUnit *cu, DIE *parent, Abbreviation *abbrev) ;
    ~Reference_type() ;
    virtual void print (EvalContext &ctx, int indent, int level=0) ;
    bool is_pointer () ;
    bool is_scalar () ;
    bool is_real () ;
    bool is_integral () ;
    bool is_address() ;
    bool is_struct_deref() ;
    int get_size_immed() ;
    int get_size() ;
    int get_real_size(EvalContext &ctx) ;
    void print_value (EvalContext &context, Value &value, int indent=0) ;
    void set_value (EvalContext &context, Value &addr, Value &value) ;
    bool compare (EvalContext &context, DIE *die, int flags) ;                // references are ignored for the purposes of type comparison
protected:
} ;

class Compile_unit: public DIE  {
public:
    Compile_unit(DwCUnit *cu, DIE *parent, Abbreviation *abbrev) ;
    ~Compile_unit() ;
    virtual void print (EvalContext &ctx, int indent, int level=0) ;
    void find_symbol (std::string name, Address pc, std::vector<DIE*> &result, DIE *caller = NULL) ;
    DIE * find_scope (std::string name) ;
protected:
    MultiDIEMap symbols ;
    bool symbolsok ;
    std::vector<DIE*> enumerations ;
} ;

class String_type: public DIE  {
public:
    String_type(DwCUnit *cu, DIE *parent, Abbreviation *abbrev) ;
    String_type(DwCUnit *cu, DIE *parent, int tag) ;
    ~String_type() ;
    virtual void print (EvalContext &ctx, int indent, int level=0) ;
    void print_value (EvalContext &context, Value &value, int indent=0) ;
    virtual bool is_string () { return true ; }
    int get_length (EvalContext &ctx) ;
    int get_real_size (EvalContext &ctx) { return get_length (ctx) ; }
protected:
} ;

class Common_block: public DIE  {
public:
    Common_block(DwCUnit *cu, DIE *parent, Abbreviation *abbrev) ;
    ~Common_block() ;
    virtual void print (EvalContext &ctx, int indent, int level=0) ;
    void find_symbol (std::string name, Address pc, std::vector<DIE*> &result, DIE *caller = NULL) ;
    void print_value (EvalContext &context, Value &value, int indent=0) ;
protected:
private:
    DIEMap symbols ;
    bool symbolsok ;
} ;


class Inheritance: public DIE  {
public:
    Inheritance(DwCUnit *cu, DIE *parent, Abbreviation *abbrev) ;
    ~Inheritance() ;
    virtual bool is_inheritance() { return true; }
    virtual void print (EvalContext &ctx, int indent, int level=0) ;
    Value evaluate (EvalContext & context, Value &base) ;
    Value evaluate (EvalContext & context) ;
protected:
} ;

class Base_type: public DIE  {
public:
    Base_type(DwCUnit *cu, DIE *parent, Abbreviation *abbrev) ;
    Base_type(DwCUnit *cu, DIE *parent, int tag) ;
    ~Base_type() ;
    virtual void print (EvalContext &ctx, int indent, int level=0) ;
    bool is_scalar () ;
    bool is_real () ;
    bool is_integral () ;
    bool is_complex() ;
    bool is_boolean() ;
    bool is_address() ;
    bool is_signed() ;
    bool is_char() ;
    bool is_uchar();
    bool is_schar();
    void print_value (EvalContext &context, Value &value, int indent=0) ;
    void set_value (EvalContext &context, Value &addr, Value &value) ;
    bool compare (EvalContext &context, DIE *die, int flags) ;                // compare two type dies
protected:
} ;


class Enumerator: public DIE  {
public:
    Enumerator(DwCUnit *cu, DIE *parent, Abbreviation *abbrev) ;
    ~Enumerator() ;
    virtual void print (EvalContext &ctx, int indent, int level=0) ;
    DIE *get_type() { return parent ; }
    Value evaluate (EvalContext &context) ;
protected:
} ;

class Namelist: public DIE  {
public:
    Namelist(DwCUnit *cu, DIE *parent, Abbreviation *abbrev) ;
    ~Namelist() ;
    virtual void print (EvalContext &ctx, int indent, int level=0) ;
protected:
} ;

class Namelist_item: public DIE  {
public:
    Namelist_item(DwCUnit *cu, DIE *parent, Abbreviation *abbrev) ;
    ~Namelist_item() ;
    virtual void print (EvalContext &ctx, int indent, int level=0) ;
protected:
} ;

class Subprogram: public DIE  {
public:
    Subprogram(DwCUnit *cu, DIE *parent, Abbreviation *abbrev) ;
    ~Subprogram() ;
    virtual void print (EvalContext &ctx, int indent, int level=0) ;
    Address get_frame_base (Process *process) ;
    int get_language() ;
    void find_symbol (std::string name, Address pc, std::vector<DIE*> &result, DIE *caller = NULL) ;
    DIE * find_scope (std::string name) ;
    void get_formal_parameters (std::vector<DIE*> &vec) ;
    void get_local_variables (std::vector<DIE*> &vec) ;
    Value evaluate (EvalContext &context) ;
    Value evaluate (EvalContext & context, Value &base) ;
    void print_value (EvalContext &context, Value &value, int indent=0) ;
    bool is_function () { return true ; }
    bool is_static();
    bool is_variadic () ;
    Address get_address(EvalContext &ctx, Address top=0) ;
    DwVirtId get_virtuality() ;
    Address get_virtual_table (EvalContext &ctx, Address thisptr) ;
    bool is_member_function() ;
protected:
private:
    DIEMap symbols ;
    bool symbolsok ;
    std::vector<DIE*> common_blocks ;
    std::vector<DIE*> lexical_blocks ;
    std::vector<DIE*> enumerations ;
} ;

class Template_type_param: public DIE  {
public:
    Template_type_param(DwCUnit *cu, DIE *parent, Abbreviation *abbrev) ;
    ~Template_type_param() ;
    virtual void print (EvalContext &ctx, int indent, int level=0) ;
protected:
} ;

class Template_value_param: public DIE  {
public:
    Template_value_param(DwCUnit *cu, DIE *parent, Abbreviation *abbrev) ;
    ~Template_value_param() ;
    virtual void print (EvalContext &ctx, int indent, int level=0) ;
protected:
} ;

class Variable: public DIE  {
public:
    Variable(DwCUnit *cu, DIE *parent, Abbreviation *abbrev) ;
    ~Variable() ;
    virtual void print (EvalContext &ctx, int indent, int level=0) ;
    Value evaluate (EvalContext &context) ;
    bool is_local_var()  ;
protected:
} ;


class LineInfo {
public:
    LineInfo(DwCUnit *cu, Address address, int file, int lineno, int column, bool is_stmt, bool basic_block) ;
    ~LineInfo() ;
    void print () ;
    DwCUnit * cu ;
    Address address ;
    int file ;
    int lineno ;
    int column ;
    bool is_stmt ;
    bool basic_block ;
} ;


#endif
