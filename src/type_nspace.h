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

#ifndef _TYPE_NSPACE_H_
#define _TYPE_NSPACE_H_

#include "type_base.h"

class EntryNSpace : public DIE {
public:
   EntryNSpace(DwCUnit *cu, DIE *parent, Abbreviation *abbrev);
   ~EntryNSpace();

   virtual void print (EvalContext& ctx, int indent, int level=0);
   virtual void find_symbol (std::string name, Address pc,
       std::vector<DIE*>& result, DIE* caller);
protected:
};

#endif

