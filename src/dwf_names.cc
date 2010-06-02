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

