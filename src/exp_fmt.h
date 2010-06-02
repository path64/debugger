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
