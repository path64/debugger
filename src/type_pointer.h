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

file: type_pointer.h
created on: Fri Aug 13 11:02:27 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#ifndef _TYPE_POINTER_H_
#define _TYPE_POINTER_H_

#include "type_base.h"

class TypePointer : public DIE  {
public:
    TypePointer (DwCUnit *cu, DIE *parent, Abbreviation *abbrev);
    TypePointer (DwCUnit *cu, DIE *parent, int tag);
    ~TypePointer ();
    virtual void print (EvalContext &ctx, int indent, int level=0);
    bool is_pointer ();
    int get_real_size(EvalContext& ctx);
    void print_value (EvalContext &context, Value &value, int indent=0);
    void set_value (EvalContext &context, Value &addr, Value &value);
protected:
} ;

#endif
