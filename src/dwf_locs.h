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

file: dwf_locs.h
created on: Fri Aug 13 11:02:27 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#ifndef _DWF_LOCS_H_
#define _DWF_LOCS_H_

#include <list>
#include "dwf_attr.h"

class DwInfo;

enum DwLocType {
   LOC_REG,
   LOC_ADDR
};

struct DwLocPiece {
   DwLocType type;
   Address value;
   unsigned size;

   DwLocPiece(DwLocType _type, Address _value, unsigned _size)
     : type(_type), value(_value), size(_size) {} 
};

class DwLocExpr {
public:
    DwLocExpr () {};
    ~DwLocExpr () {};

    Value getAddress();
    Address getValue(Process* process, int size, bool sign_extend=false);
    void add(DwLocType, Address, unsigned);

private:
    std::list<DwLocPiece> data;
};


class DwLocEval {
public:

   /* XXX: should have generic struct to package this info */
   DwLocEval(DwCUnit*, Process*,Address,AttributeValue&);

   DwLocExpr execute();  
   void push(Address);

private:
   int get_lit_num(int op);
   int get_reg_num(int op);
   int get_breg_num(int op);

   void eval_op(BStream&, int);

   LocationStack stack;
   DwLocExpr result;
   unsigned int sp;
   bool top_is_reg;
   bool needs_push;

   /* XXX: reduce following list*/
   DwCUnit* cu;
   Address frame_base;
   AttributeValue& attr;
   Process* process;

};



#endif
