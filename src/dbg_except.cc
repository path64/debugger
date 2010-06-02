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


