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

file: err_nice.h
created on: Thu Jan 13 17:34:58 PST 2005
author: James Strother <jims@pathscale.com>

*/

#ifndef _ERR_NICE_H_
#define _ERR_NICE_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#ifdef DEBUG_FATAL
static inline
void err_fatal(const char* fmt, ...)
{
   va_list ap;
   va_start(ap, fmt);
   printf("FATAL ERROR: ");
   vprintf(fmt, ap);
   putchar('\n');
   va_end(ap);
   abort(); 
}
#else
static inline
void err_fatal(const char* fmt, ...)
{
   ;
}
#endif

static inline
void err_warn(const char* fmt, ...)
{
   va_list ap;
   va_start(ap, fmt);
   printf("Warning: ");
   vprintf(fmt, ap);
   putchar('\n');
   va_end(ap);
}

#ifdef DEBUG_NOTE
static inline
void err_note(const char* fmt, ...)
{
   va_list ap;
   va_start(ap, fmt);
   printf("Remark: ");
   vprintf(fmt, ap);
   putchar('\n');
   va_end(ap); 
}
#else
static inline
void err_note(const char* fmt, ...)
{
   ;
}
#endif

#endif
