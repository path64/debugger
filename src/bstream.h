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

file: bstream.h
created on: Fri Aug 13 11:02:26 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#ifndef _BSTREAM_H_
#define _BSTREAM_H_

#include <stdint.h>
#include <vector>
#include <iostream>
#include <cstring>
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
