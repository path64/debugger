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
