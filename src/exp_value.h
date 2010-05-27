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

struct Value {
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
