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

