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

file: dbg_except.h
created on: Fri Aug 13 11:02:27 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#ifndef dbg_except_h_included
#define dbg_except_h_included

#include <iostream>
#include <stdarg.h>
#include <string>
#include <stdexcept>

class Exception : public std::exception {
public:
    Exception (const char *format, ...) throw() ;
    Exception (const char *format, va_list ap) throw() ;
    Exception() throw () {}
    virtual ~Exception() throw() ;

    void report (std::ostream &os) throw() ;
    std::string get() { return error ; }
    virtual bool pending_ok() { return false ; }
protected:
    std::string error ;
} ;

class PendingException : public Exception {
public:
    PendingException (const char *format, ...) throw() ;
    PendingException (const char *format, va_list ap) throw() ;
    ~PendingException() throw() ;
    bool pending_ok() { return true ; }
} ;


#endif
