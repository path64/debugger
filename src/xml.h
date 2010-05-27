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

file: xml.h
created on: Fri Aug 13 11:02:36 PDT 2004
author: David Allison <dallison@pathscale.com>

*/


#ifndef _xml_h_included
#define _xml_h_included

#include <iostream>
#include <string>
#include <vector>
#include <ctype.h>
#include "dbg_except.h"

namespace XML {

class Attribute {
public:
    Attribute (const std::string &nm, const std::string &val) : name(nm), value(val) {}
    void print (std::ostream &out) {
        out << ' ' << name << "=\"" << value << '"' ;
    }

     const std::string name ;
     const std::string value ;
} ;

class Element {
public:
    Element (std::string nm) : name (nm), isempty(false) {}
    ~Element() {
        for (unsigned int i = 0 ; i < attributes.size() ; i++) {
            delete attributes[i] ;
        }
        for (unsigned int i = 0 ; i < children.size() ; i++) {
            delete children[i] ;
        }
    }

    void dump (std::ostream &out, int indent = 0) {
        for (int i = 0 ; i < indent ; i++) {
            out << ' ' ;
        }
        out << '<' <<  name ;
        for (unsigned int i = 0 ; i < attributes.size() ; i++) {
            attributes[i]->print (out) ;
        }
        if (isempty) {
            out << "/>\n" ;
            return ;
        }
        out << ">\n" ;
        if (body != "") {
            for (int i = 0 ; i < indent + 4; i++) {
                out << ' ' ;
            }
            out << body ;
        }
        for (unsigned int i = 0 ; i < children.size() ; i++) {
            children[i]->dump (out, indent + 4) ;
        }
        for (int i = 0 ; i < indent ; i++) {
            out << ' ' ;
        }
        out << "</" << name <<  ">\n" ;
    }

    void print (std::ostream &out, int indent = 0) {
        if (name.size() > 4 && name.substr(0, 4) == "XML.") {               // internal element
            out << body ;
                                                                                                                 
            for (unsigned int i = 0 ; i < children.size() ; i++) {
                children[i]->print (out, indent + 4) ;
            }
        } else {
            for (int i = 0 ; i < indent ; i++) {
                out << ' ' ;
            }
            out << '<' << name ;
            for (unsigned int i = 0 ; i < attributes.size() ; i++) {
                attributes[i]->print (out) ;
            }
            if (isempty) {
                out << "/>\n" ;
                return ;
            }
            out << ">\n" ;
            if (body != "") {
                for (int i = 0 ; i < indent + 4; i++) {
                    out << ' ' ;
                }
                out << body ;
            }
            for (unsigned int i = 0 ; i < children.size() ; i++) {
                children[i]->print (out, indent + 4) ;
            }
            out << '\n' ;
            for (int i = 0 ; i < indent ; i++) {
                out << ' ' ;
            }
            out << "</" << name <<  ">\n" ;
        }
    }
                                                                                                                 
    std::string &getBody() {
        return body ;
    }
                                                                                                                 
    void setBody (std::string &b) {
       body = b ;
    }
                                                                                                                 
    void setEmpty () {
        isempty = true ;
    }

    const std::string &getAttribute (std::string &n) {
        for (unsigned int i = 0 ; i < attributes.size() ; i++) {
            if (attributes[i]->name == n) {
                return attributes[i]->value ;
            }
        }
        throw (std::string ("No such attribute: ") + n).c_str() ;
    }
                                                                                                                 
    const std::string &getAttribute (const char *n) {
        for (unsigned int i = 0 ; i < attributes.size() ; i++) {
            if (attributes[i]->name == n) {
                return attributes[i]->value ;
            }
        }
        throw (std::string ("No such attribute: ") + n).c_str() ;
    }
                                                                                                                 
    bool attributePresent (std::string &n) {
        for (unsigned int i = 0 ; i < attributes.size() ; i++) {
            if (attributes[i]->name == n) {
                return true ;
            }
        }
        return false ;
    }
                                                                                                                 
    void addBody (std::string &s) {
        bool empty = true ;
        for (unsigned int i = 0 ; i < s.size() ; i++) {
            if (!isspace (s[i])) {
                empty = false ;
                break ;
            }
        }
        if (empty) {
            return ;
        }
        Element *bodyelement = new Element("XML.text") ;
        bodyelement->setBody (s) ;
        children.push_back (bodyelement) ;
    }
                                                                                                                 
    void addAttribute (const std::string &n, const std::string &v) {
        attributes.push_back (new Attribute (n,v)) ;
    }
                                                                                                                 
    void addChild (Element *elem) {
        children.push_back (elem) ;
    }
                                                                                                                 
    // find an element with the given name
    Element *find (std::string &nm) {
       if (nm == name) {
           return this ;
       }
       for (unsigned int i = 0 ; i < children.size() ; i++) {
           Element *r = children[i]->find (nm) ;
           if (r != NULL) {
               return r ;
           }
       }
       return NULL ;
    }

    Element *find (const char *nm) {
       if (name == nm) {
           return this ;
       }
       for (unsigned int i = 0 ; i < children.size() ; i++) {
           Element *r = children[i]->find (nm) ;
           if (r != NULL) {
               return r ;
           }
       }
       return NULL ;
    }

    std::vector<Element*> &getChildren() { return children ; }

    Attribute *findAttribute (std::string &n) {
        for (unsigned int i = 0 ; i < attributes.size() ; i++) {
            if (attributes[i]->name == n) {
                return attributes[i] ;
            }
        }
        return NULL ;
    }
                                                                                                                 
    Attribute *findAttribute (const char *n) {
        for (unsigned int i = 0 ; i < attributes.size() ; i++) {
            if (attributes[i]->name == n) {
                return attributes[i] ;
            }
        }
        return NULL ;
    }
                                                                                                                 

    std::string name ;
private:
    std::vector<Attribute*> attributes ;
    std::string body ;
    std::vector<Element*> children ;
    bool isempty ;

    std::istream *str ;
} ;


class Parser {
public:
    Parser (std::istream &s) : str(s) {}
    bool readToElement (std::string &result) ;
    char skipSpaces (char lastchar = '\0') ;
    void readName (std::string &name, char &lastchar) ;
    std::string readAttributeValue (char &lastchar) ;
private:
   std::istream &str ;
} ;

Element *parseStream (std::istream &str) ;

}

#endif
