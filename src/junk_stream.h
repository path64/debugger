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

file: junk_stream.h
created on: Fri Aug 13 11:02:27 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#ifndef _JUNK_STREAM_H_
#define _JUNK_STREAM_H_

#include "dbg_types.h"
#include "pstream.h"

#include <stdarg.h>
#include <stdio.h>
#include <sys/time.h>

class EvalContext;
class Format;
class Value;

class JunkStream : PStream {
public:
    JunkStream(int fd) : PStream(fd) {}

    void print (const Value &v);
    void print (EvalContext &ctx, int64_t i, int size, bool issigned);
    void print (EvalContext &ctx, int i);
    void print (EvalContext &ctx, double d);
    void print (EvalContext &ctx, unsigned int i);
    void print (EvalContext &ctx, unsigned long i);
    void print (EvalContext &ctx, Address i);
    void print_address (EvalContext &ctx, Address i);
    void print (EvalContext &ctx, void *p);
    void print (EvalContext &ctx, signed char ch);
    void print (EvalContext &ctx, unsigned char ch);
    void print (EvalContext &ctx, bool v);

private:
    void print_char(EvalContext& ctx, int ch);
    void disassemble (EvalContext &ctx, Address addr);

    const char *to_format_string(Format & fmt, bool is64bit);
    void print_binary(int64_t i, int size);
};

#endif
