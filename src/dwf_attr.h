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

file: type_attrib.h
created on: Fri Aug 13 11:02:27 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#ifndef _TYPE_ATTRIB_H_
#define _TYPE_ATTRIB_H_

#include "expr.h"
#include "bstream.h"

class DIE;
class DwCUnit;
class Abbreviation;

typedef std::map<int, Abbreviation*> AbbreviationMap ;

enum AVType {
    AV_NONE,
    AV_STRING,
    AV_DIE,
    AV_INTEGER,
    AV_BLOCK,
    AV_ADDRESS
} ;

struct AttributeValue {
    AttributeValue (DIE *die) : type (AV_DIE), die(die) { }
    AttributeValue (BVector block) : type (AV_BLOCK), block(block) {}
    AttributeValue (std::string s) : type (AV_STRING), str(s) { }
    AttributeValue (int64_t i):type (AV_INTEGER), integer(i) {}
    AttributeValue (int i):type (AV_INTEGER), integer(i) {}
    AttributeValue(): type(AV_NONE), integer(0) {}

    Value toValue (DIE* die);

    operator std::string() {
       return str ;
    }

    operator int64_t() {
       return integer ;
    }

    operator DIE *() {
       return die ;
    }

    operator BVector () {
       return block ;
    }

    operator int() {
       return (int)integer ;
    }


    AVType type ;
    union {
        int64_t integer ;
        DIE *die ;
        Address addr ;
    } ;
    BVector block ;
    std::string str ;           // can't be in union
} ;

class Attribute {
public:
    Attribute(int form) ;
    virtual ~Attribute() ;
    int form ;
    AttributeValue value ;

    void set_value (DIE *die) { value.type = AV_DIE ; value.die = die ; }
    void set_value (BVector block) { value.type = AV_BLOCK ; value.block = block ; }
    void set_value (AVType type, int64_t integer) { value.type = type ; value.integer = integer ; }
    void set_value (std::string &s) { value.type = AV_STRING ; value.str = s ; }

    virtual void fixup (DIE *die) ;
} ;

class AttributeAbbreviation {
public:
    AttributeAbbreviation(int tag, int form) ;
    ~AttributeAbbreviation() ;
    void print() ;
    int getTag () ;
    int getForm () ;
    AttributeValue read (DwCUnit *cu, Attribute *attr, BStream & stream) ;
protected:
private:
    int tag ;
    int form ;
} ;

class Abbreviation {
public:
    Abbreviation(int num, int tag, bool haschildren) ;
    ~Abbreviation() ;
    void read (BStream & bstream) ;
    void addAbbreviation (int n, Abbreviation *abb) ;
    Abbreviation *getAbbreviation (int n) ;
    void print () ;
    bool has_children () ;
    int getNum () ;
    int getTag () ;
    int getNumAttributes () ;
    AttributeAbbreviation *getAttribute (int i) ;
protected:
private:
    int num ;
    int tag ;
    bool haschildren ;
    std::vector<AttributeAbbreviation *> attributes ;
    AbbreviationMap table ; // map of abbreviation code vs Abbreviation
} ;



std::ostream &operator << (std::ostream &os, AttributeValue &v) ;



class FixupAttribute : public Attribute {
public:
    FixupAttribute (int form, DIE *parent) : Attribute (form),parent(parent) {}
    ~FixupAttribute() {}
    void fixup (DIE *die) ;
private:
    DIE *parent ;
} ;


#endif
