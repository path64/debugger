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

file: dwf_names.h
created on: Fri Jan 28 21:46:21 PST 2005
author: James Strother <jims@pathscale.com>

*/

#ifndef _DWF_NAMES_H_
#define _DWF_NAMES_H_

#include "dwf_spec.h"
#include <map>

class DwNames {
private:
   typedef std::map<int,const char*> smap;

   /* declare private to prevent usage */
   const char* get(int A) { return NULL; }

public:
   DwNames();

#define MAKE_GET(TYPE)                     \
   const char* get(TYPE id) {              \
      smap::iterator i;                    \
      i = TYPE##_map.find(id);             \
      if (i == TYPE##_map.end()) {        \
         return NULL;                      \
      }                                    \
      return i->second;                    \
   }

   MAKE_GET(DwTagId)
   MAKE_GET(DwAttrId)
   MAKE_GET(DwFormId)
   MAKE_GET(DwOpcodeId)
   MAKE_GET(DwEncodeId)
   MAKE_GET(DwAccessId)
   MAKE_GET(DwVisiId)
   MAKE_GET(DwVirtId)
   MAKE_GET(DwLangId)
   MAKE_GET(DwSLineId)
   MAKE_GET(DwELineId)
   MAKE_GET(DwFrameId)
   MAKE_GET(DwEHFrameId)
#undef MAKE_GET

private:
#define MAKE_DATA(TYPE)                    \
   std::map<int,const char*> TYPE##_map;

   MAKE_DATA(DwTagId)
   MAKE_DATA(DwAttrId)
   MAKE_DATA(DwFormId)
   MAKE_DATA(DwOpcodeId)
   MAKE_DATA(DwEncodeId)
   MAKE_DATA(DwAccessId)
   MAKE_DATA(DwVisiId)
   MAKE_DATA(DwVirtId)
   MAKE_DATA(DwLangId)
   MAKE_DATA(DwSLineId)
   MAKE_DATA(DwELineId)
   MAKE_DATA(DwFrameId)
   MAKE_DATA(DwEHFrameId)
#undef MAKE_DATA
};

/* define a global instance */
extern DwNames globl_dwf_names;

#endif

