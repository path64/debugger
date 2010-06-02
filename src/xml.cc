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

file: xml.cc
created on: Fri Aug 13 11:07:48 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#include "xml.h"
#include <stack>
#include <fstream>

namespace XML {

bool Parser::readToElement (std::string &result) {
    bool wasElement = false ;
    while (!str.eof()) {
        char ch =str.get() ;
        if (str.eof()) {
            break ;
        }
        if (ch == '<') {
            wasElement = true ;
            break ;
        }
        result.push_back (ch) ;
    }
    return wasElement ;
}

char Parser::skipSpaces (char lastchar) {
    if (lastchar != '\0' && !isspace(lastchar)) {
        return lastchar ;
    }
    while (!str.eof()) {
        char ch = str.get() ;
        if (!(isspace(ch) || ch == '\n')) {
            lastchar = ch ;
            break ;
        }
    }
    return lastchar ;
}

void Parser::readName (std::string &name, char &lastchar) {
    char ch = skipSpaces(lastchar) ;
    name.push_back (ch) ;
    while (!str.eof()) {
        ch = str.get() ;
        if (isspace (ch) || ch == '>' || ch == '=' || ch == '/') {
            lastchar = ch ;
            break ;
        }
        name.push_back (ch) ;
    }
    lastchar = skipSpaces(lastchar) ;
}

std::string Parser::readAttributeValue (char &lastchar) {
    std::string value = "" ;
    char ch = skipSpaces() ;
    if (ch == '"') {            // string value
        while (!str.eof()) {
            char ch = str.get() ;
            if (ch == '"') {
                lastchar = str.get() ;
                break ;
            }
            value.push_back (ch) ;
        }
    } else if (ch == '>') {
        throw Exception ("Missing attribute value") ;
    } else {
        value.push_back(ch) ;
        while (!str.eof()) {
            char ch = str.get() ;
            if (isspace(ch) || ch == '>' || ch == '/') {
                lastchar = ch ;
                break ;
            }
            value.push_back (ch) ;
        }
    }
    lastchar = skipSpaces (lastchar) ;
    return value ;
}

Element *parseStream (std::istream &str) {
    Parser parser (str) ;

    std::stack<Element *>stack ;
    stack.push (new Element ("XML.top")) ;               // top of the XML tree

    std::string initial = "" ;
    bool foundElement = parser.readToElement (initial) ;
    stack.top()->addBody (initial) ;            // before first element is part of top
                                                                                                                 
    while (!str.eof()) {
        if (foundElement) {                 // are we looking at an element?
            std::string name = "" ;
            char termchar = '\0' ;            // terminating char of last read
            parser.readName (name, termchar) ;      // read element name
                                                                                                                 
            if (name.size() > 0) {          // any name?
                if (name.size() >= 3 && name.substr (0, 3) == "!--") {
                    while (!str.eof()) {
                       char ch = str.get() ;
                       if (ch == '>') {
                           break ;
                       }
                    }
                } else if (name[0] == '/') {           // end of an element?
                    name = name.substr (1) ;
                    Element *top = stack.top() ;
                    if (top->name == name) {
                        stack.pop() ;
                    } else {
                        throw Exception ("Element is not top of stack") ;
                    }
                } else {                            // start of an element
                    Element *element = new Element (name) ;
                    stack.top()->addChild (element) ;
                                                                                                                 
                    bool empty = false ;
                    while (termchar != '>') {            // collect attributes
                        if (str.eof()) {
                            throw Exception ("Missing element terminator") ;
                        }
                        std::string attrname = "" ;
                        parser.readName (attrname, termchar) ;
                        if (attrname == "/" && termchar == '>') {
                             empty = true ;
                             element->setEmpty() ;
                        } else if (termchar == '=') {
                            std::string attrvalue = parser.readAttributeValue (termchar) ;
                            element->addAttribute (attrname, attrvalue) ;
                        } else {
                            element->addAttribute (attrname, "") ;            // no value
                        }
                    }
                    if (!empty) {
                        stack.push (element) ;
                    }
                }
            }
        }
        std::string body = "" ;
        foundElement = parser.readToElement (body) ;
        if (body != "") {
            stack.top()->addBody (body) ;
        }
    }
    return stack.top() ;
}                                                                                                                           

}

#if 0
main() {
    XML::Element *top = new XML::Element ("top") ;
    top->addAttribute ("value", "hello") ;
    XML::Element *child = new XML::Element ("child") ;
    child->addAttribute ("color", "blue") ;
    top->addChild (child) ;
    top->dump (std::cout) ;
    top->print (std::cout) ;
}
#endif

