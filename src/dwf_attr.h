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
