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

file: bstream.cc
created on: Fri Aug 13 11:07:33 PDT 2004
author: David Allison <dallison@pathscale.com>
 
*/

#include <stdint.h>
#include "bstream.h"
#include "err_nice.h"

void
BStream::seek(int _offset, BStreamSeek whence)
{
   switch (whence) {
   case BSTREAM_CUR:
      pos += _offset;
      break;
   case BSTREAM_SET:
      pos = _offset;
      break;
   case BSTREAM_END:
      pos = vec.length() - _offset;
      break;
   default:
      err_fatal("invalid stream whence");
   }
}

int64_t
BStream::read_uleb()
{
   int64_t ret = 0;
   int shift = 0;
   byte b;
   do {
      int64_t x;

      b = read1u();
      x = b & 0x7f;
      x <<= shift;
      ret |= x;
      shift += 7;

   } while(b & 0x80);
   return ret;
}

int64_t
BStream::read_sleb()
{
   unsigned sz = 64;
   unsigned shift = 0;
   int64_t ret = 0;
   byte b;

   do {
      int64_t x;

      b = read1u();
      x = b & 0x7f;
      x <<= shift;
      ret |= x;
      shift += 7;

   } while(b & 0x80);

   /* do sign extension */
   if ( shift<sz && (b&0x40) ) {
      ret |= - ((int64_t)1 << shift);
   }

   return ret;
}


int32_t
BStream::read2u()
{
   uint16_t w = 0;
   if (! swap) {
      for (int i = 0; i < 2; i++) {
	 w |= (uint16_t)read1u() << (i * 8);
      }
   } else {
      for (int i = 0; i < 2; i++) {
	 w = (w << 8) | read1u();
      }
   }
   return w;
}

int32_t
BStream::read4u()
{
   uint32_t w = 0;
   if (! swap) {
      for (int i = 0; i < 4; i++) {
	 w |= (uint32_t)read1u() << (i * 8);
      }
   } else {
      for (int i = 0; i < 4; i++) {
	 w = (w << 8) | read1u();
      }
   }
   return w;
}

int64_t
BStream::read8u()
{
   int64_t w = 0;
   if (! swap) {
      for (int i = 0; i < 8; i++) {
	 w |= (int64_t)read1u() << (i * 8);
      }
   } else {
      for (int i = 0; i < 8; i++) {
	 w = (w << 8) | read1u();
      }
   }
   return w;
}

int32_t
BStream::read2s()
{
   int16_t w = 0;
   if (! swap) {
      for (int i = 0; i < 2; i++) {
	 w |= (int16_t)read1u() << (i * 8);
      }
   } else {
      for (int i = 0; i < 2; i++) {
	 w = (w << 8) | read1u();
      }
   }
   return w; /* sign extended */
}

int32_t
BStream::read4s()
{
   int32_t w = 0;
   if (! swap) {
      for (int i = 0; i < 4; i++) {
	 w |= (int32_t)read1u() << (i * 8);
      }
   } else {
      for (int i = 0; i < 4; i++) {
	 w = (w << 8) | read1u();
      }
   }
   return w;
}

int64_t
BStream::read8s()
{
   int64_t w = 0;
   if (! swap) {
      for (int i = 0; i < 8; i++) {
	 w |= (int64_t)read1u() << (i * 8);
      }
   } else {
      for (int i = 0; i < 8; i++) {
	 w = (w << 8) | read1u();
      }
   }
   return w;
}

