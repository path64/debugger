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

file: dwf_names.cc
created on: Fri Jan 28 21:46:21 PST 2005
author: James Strother <jims@pathscale.com>

*/

#include "dwf_names.h"

/* just make one global instance */
DwNames globl_dwf_names;

DwNames::DwNames() {
   StringLUT* lut;

#define MAKE_MAP(TYPE)                     \
   lut = globl_##TYPE##_lut;               \
   while (lut->name != NULL) {             \
      TYPE##_map[lut->id] = lut->name;     \
      ++lut;                               \
   }                                       \

   MAKE_MAP(DwTagId)
   MAKE_MAP(DwAttrId)
   MAKE_MAP(DwFormId)
   MAKE_MAP(DwOpcodeId)
   MAKE_MAP(DwEncodeId)
   MAKE_MAP(DwAccessId)
   MAKE_MAP(DwVisiId)
   MAKE_MAP(DwVirtId)
   MAKE_MAP(DwLangId)
   MAKE_MAP(DwSLineId)
   MAKE_MAP(DwELineId)
   MAKE_MAP(DwFrameId)
   MAKE_MAP(DwEHFrameId)
#undef MAKE_MAP
}

