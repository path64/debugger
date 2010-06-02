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

file: type_nspace.h
created on: Wed Jan 26 14:42:47 PST 2005
author: James Strother <jims@pathscale.com>

*/

#include "type_nspace.h"

EntryNSpace::EntryNSpace(DwCUnit *cu, DIE *parent, Abbreviation *abbrev)
    : DIE(cu, parent, abbrev) {
}

EntryNSpace::~EntryNSpace() {
}

void EntryNSpace ::print(EvalContext& ctx, int indent, int level) {
   doindent (ctx, indent);
}

void EntryNSpace::find_symbol(std::string name, Address pc,
   std::vector<DIE*>& result, DIE* caller) {

   check_loaded();
   for (unsigned i=0; i<children.size(); i++) {
       DIE* child = children[i];
       int childtag = child->get_tag();

       if (childtag == DW_TAG_member ||     /* XXX: replace this with more general function */
           childtag == DW_TAG_subprogram || /*  this is a general problem thru-out pathdb */
           childtag == DW_TAG_variable) {

           AttributeValue av = child->getAttribute(DW_AT_name).str;
           if (av.type != AV_STRING || av.str != name) continue;

           result.push_back(child);
       }
   }
}

