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

file: dwf_names.h
created on: Fri Jan 28 21:46:21 PST 2005
author: James Strother <jims@pathscale.com>

*/

#ifndef _DWF_NAMES_H_
#define _DWF_NAMES_H_

#include "dwf_spec.h"
#include <map>
#include <stdlib.h>

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

