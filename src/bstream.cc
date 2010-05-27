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

file: bstream.cc
created on: Fri Aug 13 11:07:33 PDT 2004
author: David Allison <dallison@pathscale.com>
 
*/

#include <stdint.h>
#include "bstream.h"
#include "err_nice.h"

void
BStream::seek(int offset, BStreamSeek whence)
{
   switch (whence) {
   case BSTREAM_CUR:
      pos += offset;
      break;
   case BSTREAM_SET:
      pos = offset;
      break;
   case BSTREAM_END:
      pos = vec.length() - offset;
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

