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
