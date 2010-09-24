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

file: exp_value.h
created on: Mon May  2 13:08:35 PDT 2005
author: James Strother <jims@pathscale.com>

*/

#ifndef _EXP_VALUE_H_
#define _EXP_VALUE_H_

// VALUE_INTEGER is long enough to hold an address
enum ValueType {
    VALUE_NONE, VALUE_INTEGER, VALUE_REAL, VALUE_STRING, VALUE_REG, VALUE_BOOL, VALUE_VECTOR
    
} ;

class Value {
    public:
    Value() : type(VALUE_NONE), integer(0) {}
    Value (int64_t i): type(VALUE_INTEGER), integer(i) {}
    Value (int i): type(VALUE_INTEGER), integer(i) {}
    Value (double r): type(VALUE_REAL), real(r) {}
    Value (float f): type(VALUE_REAL), real(f) {}
    Value (bool b): type(VALUE_BOOL), integer(b) {}
    Value (std::string s): type(VALUE_STRING), str(s) {}
    Value (const char *s): type(VALUE_STRING), str(s) {}
    Value (ValueType t, int64_t v) : type(t), integer(v) {}
    Value (const Value &v) : type(v.type), integer(v.integer), str(v.str), vec(v.vec) {}
    Value (std::vector<Value> &v) : type (VALUE_VECTOR), vec(v) {}
    ValueType type ;
    union {
        int64_t integer ;
        double real ;
    } ;
    std::string str ;
    std::vector<Value> vec ;            // XXX: make this a pointer?

    operator int64_t() {
        return integer ;
    }

    operator double() {
        return real ;
    }

    operator int() {
        return (int)integer ;
    }

    operator unsigned int() {
        return (unsigned int)integer ;
    }

    operator char() {
        return (char)integer ;
    }

    operator signed char() {
        return (signed char)integer ;
    }

    operator unsigned char() {
        return (unsigned char)integer ;
    }

    operator std::string() {
        return str ;
    }

    operator bool() {
        return (bool)integer ;
    }

    operator std::vector<Value>() {
        return vec ;
    }
} ;

#endif
