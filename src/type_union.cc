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

file: type_union.cc
created on: Fri Aug 13 11:02:27 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#include "type_union.h"
#include "process.h"

TypeUnion::TypeUnion (DwCUnit *cu, DIE *parent, Abbreviation *abbrev)
    : TypeStruct(cu, parent, abbrev), locations_added(false) {

    // a structure is always a reference to the structure.  Sizeof will have to
    // get the real size of it

}

TypeUnion::~TypeUnion() {
}

void TypeUnion::print(EvalContext &ctx, int indent, int level) {
        check_loaded() ;
        doindent (ctx, indent) ;
        ctx.os.print ("union ") ;
        print_name_or_id (ctx, this) ;
        if (ctx.show_contents) {
            ctx.os.print (" {\n") ;
            for (uint i = 0 ; i < children.size(); i++) {
                DIE *child = children[i] ;
                child->print (ctx, indent+4, level+1) ;
                ctx.os.print (";\n") ;
            }
            doindent (ctx, indent) ;
            ctx.os.print ("}") ;
        }
}

// union members don't have a data_member_location attribute because they will all
// be zero.  The easiest way to deal with this is just to add one to each member

void TypeUnion::add_locations() {
    if (locations_added) {
        return ;
    }
    check_loaded() ;

    /* XXX: this will leak 2 bytes, which is fine because I don't like
     the way this works, and so eventually it will be tossed */

    byte* addr = (byte*)malloc(2);
    addr[0] = DW_OP_plus_uconst ;
    addr[1] = 0 ;
    BVector loc (addr, 2);

    for (uint i = 0 ; i < children.size(); i++) {
        DIE *child = children[i] ;
        if (child->get_tag() == DW_TAG_member) {
            child->addAttribute (DW_AT_data_member_location, loc) ;
        }
    } 
    locations_added = true ;
}


void TypeUnion::print_value(EvalContext &context, Value &value, int indent) {
        if (!context.process->is_active()) {
            throw Exception("A running process is required for this operation") ;
        }
        check_loaded() ;

        if (!context.process->get_int_opt(PRM_P_UNION)) {
            context.os.print ("{<members omitted>}") ;
            return ;
        }
        add_locations(); 

        TypeStruct::print_value (context, value, indent) ;
}
