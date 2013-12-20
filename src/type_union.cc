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

file: type_union.cc
created on: Fri Aug 13 11:02:27 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#include "type_union.h"
#include "process.h"

TypeUnion::TypeUnion (DwCUnit *_cu, DIE *_parent, Abbreviation *_abbrev)
    : TypeStruct(_cu, _parent, _abbrev), locations_added(false) {

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
