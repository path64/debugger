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

file: type_struct.h
created on: Fri Aug 13 11:02:27 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#ifndef _TYPE_STRUCT_H_
#define _TYPE_STRUCT_H_

#include "type_base.h"

class TypeStruct : public DIE  {
public:
    TypeStruct(DwCUnit *cu, DIE *parent, Abbreviation *abbrev) ;
    ~TypeStruct() ;

    virtual void print (EvalContext &ctx, int indent, int level=0) ;
    bool is_struct () ;
    int get_size () ;
    DIE *find_member (std::string &name) ;
    DIE *find_member (DIE *member) ;
    void find_member (std::string &name, std::vector<DIE*> &result)  ;
    void print_value (EvalContext &context, Value &value, int indent=0) ;
    void find_symbol (std::string name, Address pc, std::vector<DIE*> &result, DIE *caller = NULL) ;
    DIE * find_scope (std::string name) ;
    void find_operator (std::string name, std::vector<DIE*> &result) ;
    void find_bases(std::vector<DIE*> &result) ;
    Address get_virtual_table (EvalContext &ctx, Address thisptr) ;
    void set_value (EvalContext &context, Value &addr, Value &value) ;
    DIE *get_dynamic_type(EvalContext &ctx, Address thisptr) ;
    bool compare (EvalContext &context, DIE *die, int flags) ;                // compare two type dies
protected:
    MultiDIEMap symbols ;
    bool symbolsok ;
    std::vector<DIE*> enumerations ;
private:
    void print_c (EvalContext& ctx, int indent, int level=0);
    void print_cxx_access(EvalContext& ct, int indent, int level, DIE* child, int& cur_access);
    void print_cxx (EvalContext& ctx, int indent, int level=0);
    void print_fort (EvalContext& ctx, int indent, int level=0);
} ;

#endif
