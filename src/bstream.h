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

file: bstream.h
created on: Fri Aug 13 11:02:26 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#ifndef _BSTREAM_H_
#define _BSTREAM_H_

#include <stdint.h>
#include <vector>
#include "dbg_types.h"

class BVector {
public:
   BVector() {
      data = NULL;
      len = 0;
   }
   BVector(const byte* _data, long _len) {
      data = _data;
      len = _len;
   }
   const byte& operator[](long p) const { 
      return data[p];
   }
   unsigned long length() const {
      return len;
   }

private:
   const byte* data;
   unsigned long len;
};

enum BStreamSeek {
   BSTREAM_CUR,
   BSTREAM_SET,
   BSTREAM_END
};


class BStream {
private: /* made private to outlaw */
   BStream();

public:
   /* Constructors and Destructors
    * ******************************************* 
    */
   BStream(BVector _vec, bool _swap)
    : vec(_vec), swap(_swap), pos(0) {
   }

   ~BStream() {
      /* doesn't own array */
   }


   /* Position accessors and modifiers
    * ******************************************* 
    */

   void seek(int offset, BStreamSeek whence = BSTREAM_SET);

   bool eof() {
      return pos >= vec.length();
   }

   int offset() {
      return pos;
   }

   int remaining() {
      return vec.length()-pos;
   }

   const byte* address() {
      return &vec[pos];
   }


   /*  Read Subroutines
    * ******************************************** 
    */

   int read1u() {
      if (pos >= vec.length()) {
	 return 0;
      }
      int x = vec[pos++];
      return x;
   }

   int read1s() {
      if (pos >= vec.length()) {
         return 0;
      }
      int x = vec[pos++];
      if ( x > 127 ) {
         x <<= 24;
         x >>= 24;
      } 
      return x;
   }

   void read(byte* dest, int n) {
      if (pos+n > vec.length()) {
         return;
      }
      memcpy(dest, &vec[pos], n);
      pos += n;
   }

   void read(char* dest, int n) {
      read((byte*)dest, n);
   }

   int32_t read2u();
   int32_t read4u();
   int64_t read8u();
   int32_t read2s();
   int32_t read4s();
   int64_t read8s();

   int64_t read_uleb();
   int64_t read_sleb();


private:
   BVector vec;
   bool swap;
   unsigned long pos;
};


#endif
