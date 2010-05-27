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

file: exp_fmt.h
created on: Fri Aug 13 11:02:30 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#ifndef _EXP_FMT_H_
#define _EXP_FMT_H_

// formatting for output:
// format codes are:
//   'o' octal
//   'x' hex
//   'd' decimal
//   'u' unsigned decimal
//   't' binary
//   'f' floating point
//   'a' address
//   'c' char
//   's' string
//   'i' instruction disassembly
//   'n' native (default for type)

// size is:
//   'b' byte
//   'h' halfword (16 bits)
//   'w' word (32 bits)
//   'g' giant word (64 bits)
//   'n' native (default for type)

// count is the repeat count
// if 'fill' is true then fill to left with zeroes

struct Format { 
    Format (int cnt, char code, char sz) : count (cnt),code(code), size(sz), fill(false) {}
    Format () : count(1), code('n'), size('n'), fill(false) {}
    short count ;
    char code ;
    char size ;
    bool fill ;         // fill with zeroes
} ;


#endif
