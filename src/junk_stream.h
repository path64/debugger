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
    JunkStream(int _fd) : PStream(_fd) {}

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
