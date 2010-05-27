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
