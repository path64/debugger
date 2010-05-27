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

file: dbg_except.cc
created on: Fri Aug 13 11:07:36 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#include "dbg_except.h"

Exception::Exception (const char *format,...) throw() {
    va_list arg ;
    va_start (arg, format) ;
    char buf[1024] ;
    vsprintf (buf, format, arg) ;
    va_end(arg) ;
    error = buf ;
}

Exception::Exception (const char *format, va_list ap) throw() {
    char buf[1024] ;
    vsprintf (buf, format, ap) ;
    error = buf ;
}

Exception::~Exception() throw() {
}


void Exception::report (std::ostream &os) throw() {
    os << error << "\n" ;
    os.flush() ;
}

PendingException::PendingException (const char *format,...) throw() {
    va_list arg ;
    va_start (arg, format) ;
    char buf[1024] ;
    vsprintf (buf, format, arg) ;
    va_end(arg) ;
    error = buf ;
}

PendingException::PendingException (const char *format, va_list ap) throw() {
    char buf[1024] ;
    vsprintf (buf, format, ap) ;
    error = buf ;
}

PendingException::~PendingException() throw() {
}


