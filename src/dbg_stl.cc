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

file: dbg_stl.cc
created on: Tue Aug 17 11:02:17 PDT 2004
author: David Allison <dallison@pathscale.com>

*/



//
// this file implements the Standard Template Library for C++ debugging
// facilities.  Basically it provides printing of strings, vectors etc.
// in a readable fashion

#include "dbg_stl.h"
#include "dbg_dwarf.h"
#include "expr.h"
#include "process.h"
#include "utils.h"
#include "arch.h"
#include "symtab.h"

namespace stl {

bool print_string (EvalContext &context, Address addr, DIE *type) {
    // find the _M_dataplus
    std::vector<DIE*> &children = type->getChildren() ;
    bool found = false ;
    for (unsigned int i = 0 ; i < children.size() ; i++) {
        DIE *child = children[i] ;
        if (child->get_tag() == DW_TAG_member) {
            std::string childname = child->getAttribute (DW_AT_name) ;
            if (childname == "_M_dataplus") {
                found = true ;
                break ;
            }
        }
    }
    if (!found) {
        return false ;
    }
    if (!context.process->test_address (addr)) {
        context.os.print ("<Bad Address 0x%llx>", addr) ;
        return true ;
    }
    Address saddr = context.process->readptr(addr) ;
    if (saddr == 0) {
        context.os.print ("<no data>") ;
        return true ;
    }
    if (!context.process->test_address (saddr)) {
        context.os.print ("<Bad Address 0x%llx>", saddr) ;
        return true ;
    }
    std::string s = context.process->read_string (saddr) ;
    context.os.print ("\"") ;
    Utils::print_string (context, s);
    context.os.print ("\"") ;
    return true ;
}

bool subscript_string (SymbolTable *symtab, EvalContext &context, Address addr, DIE *type, int sub, Value &outval, DIE *&outtype) {
    // find the _M_dataplus
    std::vector<DIE*> &children = type->getChildren() ;
    bool found = false ;
    for (unsigned int i = 0 ; i < children.size() ; i++) {
        DIE *child = children[i] ;
        if (child->get_tag() == DW_TAG_member) {
            std::string childname = child->getAttribute (DW_AT_name) ;
            if (childname == "_M_dataplus") {
                found = true ;
                break ;
            }
        }
    }
    if (!found) {
        return false ;
    }
    Address saddr = context.process->readptr(addr) ;
    outval = context.process->read (saddr + sub, 1) ;
    outtype = symtab->new_scalar_type ("char", DW_ATE_signed_char, 1) ;
    return true ;
}

// there appear to be a couple of different vector definitions.  The simplest one is
// used by g++.  PathCC seems to use a more complex one.  It might be just a libstdc++ issue.

static void print_vector_contents (EvalContext &context, Address addr, DIE *value_type) {
    bool comma = false ;
    try {
        Address start = context.process->readptr (addr) ;
        Address finish = context.process->readptr (addr + context.process->get_arch()->ptrsize()) ;
        Address end = context.process->readptr (addr + context.process->get_arch()->ptrsize()*2) ;
        int size = value_type->get_real_size (context) ;
        if (end == 0 || finish < start) {
            context.os.print ("{}") ;
        } else {
            context.os.print ("{") ;
            addr = start ;
            while (addr != finish) {
                if (comma) {
                    context.os.print (", ") ;
                }
                comma = true ;
                if (value_type->is_struct() || value_type->is_array()) {
                    Value v(addr) ;
                    value_type->print_value (context, v, 0) ;
                } else {
                    Value v = context.process->read (addr, size) ;
                    value_type->print_value (context, v, 0) ;
                }
                addr += size ;
            }
            context.os.print ("}") ;
        }
    } catch (Exception e) {
        context.os.print ("<%s>", e.get().c_str()) ;
    }
}

 
DIE *find_base (DIE *die, std::string prefix) {
    die->check_loaded() ;
    std::vector<DIE*> &children = die->getChildren() ;
    for (unsigned int i = 0 ; i < children.size() ; i++) {
        DIE *child = children[i] ;
        if (child->get_tag() == DW_TAG_inheritance) {
            DIE *childtype = child->get_type() ;
            childtype->check_loaded() ;
            std::string childname = childtype->getAttribute (DW_AT_name) ;
            if (strncmp (childname.c_str(), prefix.c_str(), prefix.size()) == 0) {
               return childtype ;
            }
        }
    }
    return NULL ;
}

DIE *find_member (DIE *die, std::string name) {
    die->check_loaded() ;
    std::vector<DIE*> &children = die->getChildren() ;
    for (unsigned int i = 0 ; i < children.size() ; i++) {
        DIE *child = children[i] ;
        if (child->get_tag() == DW_TAG_member) {
            std::string childname = child->getAttribute (DW_AT_name) ;
            if (childname == name) {
               return child ;
            }
        }
    }
    return NULL ;
}

DIE *find_vector_type (DIE *type) {
    std::vector<DIE*> &children = type->getChildren() ;
    
    DIE *value_type = NULL ;

    // see if the there is a base class of the name _Vector_base<
    // if it exists, it will have a base class whose name is _Vector_alloc_base.  This
    // has a member called _M_start whose type is a pointer to the value type
    DIE *base = find_base (type, "_Vector_base") ;
    if (base != NULL) {
        DIE *alloc_base = find_base (base, "_Vector_alloc_base") ;
        if (alloc_base != NULL) {
            DIE *start = find_member (alloc_base, "_M_start") ;
            if (start != NULL) {
                if (start->get_type()->is_pointer()) {
                    value_type = start->get_type()->get_type() ;
                }
            }
        }
    }

    if (value_type != NULL) {
        return value_type ;
    }

    // find the value_type typedef (g++ mode)
    for (unsigned int i = 0 ; i < children.size() ; i++) {
        DIE *child = children[i] ;
        if (child->get_tag() == DW_TAG_typedef) {
            std::string childname = child->getAttribute (DW_AT_name) ;
            if (childname == "value_type") {
                value_type = child ;
                break ;
            }
        }
    }

    if (value_type == NULL) {
        return NULL ;
    }
                                                                                                                                                        
    value_type->check_loaded() ;
    value_type = value_type->get_type() ;               // dereference the typedef
    return value_type ;
}


bool print_vector (EvalContext &context, Address addr, DIE *type) {
    DIE *value_type = find_vector_type (type) ;
    if (value_type != NULL) {
        print_vector_contents (context, addr, value_type) ;
        return true ;
    }
    return false ;
}

bool subscript_vector (SymbolTable *symtab, EvalContext &context, Address addr, DIE *type, int sub, Value &outval, DIE *&outtype) {
    DIE *value_type = find_vector_type (type) ;
    if (value_type != NULL) {
        Address start = context.process->readptr (addr) ;
        //Address finish = context.process->readptr (addr + context.process->get_arch()->ptrsize()) ;
        //Address end = context.process->readptr (addr + context.process->get_arch()->ptrsize()*2) ;
        int size = value_type->get_real_size (context) ;
        start += sub * size ;
        if (value_type->is_struct() || value_type->is_array()) {
            outval = start ;
        } else {
            outval = context.process->read (start, value_type->get_size()) ;
        }
        outtype = value_type ;
        return true ;
    }
    return false ;
}

static void print_map_node (EvalContext &context, Address addr, DIE *key_type, DIE *mapped_type, bool &comma) {
    Process *proc = context.process ;
    int ptrsize = proc->get_arch()->ptrsize() ;
    Address left = proc->readptr (addr+ptrsize*2) ;
    Address right = proc->readptr (addr+ptrsize*3) ;
    //printf ("0x%llx: left: 0x%llx, right: 0x%llx\n", addr,left, right) ;
    int keysize = key_type->get_real_size (context) ;
    //int mappedsize = mapped_type->get_real_size (context) ;

    if (comma) {
        context.os.print (", ") ;
    }

    comma = true ;
    Value keyvalue = addr + ptrsize*4 ;
    if (!key_type->is_struct() && !key_type->is_array()) {
        keyvalue = proc->read (keyvalue.integer, key_type->get_size()) ;
    }
    //printf ("key address: 0x%llx\n", keyvalue.integer) ;
    key_type->print_value (context, keyvalue) ;

    context.os.print (" = ") ;
    Value mappedvalue = addr + ptrsize * 4 + keysize ;
    //printf ("mapped address: 0x%llx\n", mappedvalue.integer) ;

    // align if necessary
    if (mapped_type->is_struct() || mapped_type->is_pointer()) {
        mappedvalue.integer = (mappedvalue.integer + (ptrsize-1)) & ~(ptrsize-1) ;
    }

    if (!mapped_type->is_struct() && !mapped_type->is_array()) {
        mappedvalue = proc->read (mappedvalue.integer, mapped_type->get_size()) ;
    }
    mapped_type->print_value (context, mappedvalue) ;
    if (left != 0) {
        print_map_node (context, left, key_type, mapped_type, comma) ;
    }
    if (right != 0 && right != left) {
        print_map_node (context, right, key_type, mapped_type, comma) ;
    }
}

static void print_map_contents (EvalContext &context, Address addr, DIE *key_type, DIE *mapped_type) {
    try {
        int ptrsize = context.process->get_arch()->ptrsize() ;
        Address header = context.process->readptr (addr) ;
        int node_count = context.process->read (addr+ptrsize, 4) ;

        //printf ("header: 0x%llx, node count: %d\n", header, node_count) ;
        bool save = context.show_contents ;
        context.show_contents = false ;
        context.os.print ("{") ;
        context.show_contents = save ;

        if (node_count > 0) {
            bool comma = false ;
            Address root = context.process->readptr (header+ptrsize) ;
            if (root != 0) {
                print_map_node (context, root, key_type, mapped_type, comma) ;
            }
        }
        context.os.print ("}") ;
    } catch (Exception e) {
        context.os.print ("<%s>", e.get().c_str()) ;
    }
}

void find_map_types (DIE *type, DIE *&key_type, DIE *&mapped_type) {
    std::vector<DIE*> &children = type->getChildren() ;

    for (unsigned int i = 0 ; i < children.size() ; i++) {
        DIE *child = children[i] ;
        if (child->get_tag() == DW_TAG_typedef) {
            std::string childname = child->getAttribute (DW_AT_name) ;
            if (childname == "key_type") {
                key_type = child ;
                if (mapped_type != NULL) {
                    break ;
                }
            } else if (childname == "mapped_type") {
                mapped_type = child ;
                if (key_type != NULL) {
                    break ;
                }
            }
        }
    }

    if (key_type != NULL && mapped_type != NULL) {
        key_type->check_loaded() ;
        mapped_type->check_loaded() ;
        key_type = key_type->get_type() ;               // dereference the typedef
        mapped_type = mapped_type->get_type() ;               // dereference the typedef
        return ;
    }

// pathCC generates:
//   member _M_t is a struct _Rb_tree
//        inherited from _Rb_tree_base
//              inherited from _Rb_tree_alloc_base
//                 containing _M_header of type pointer to struct _Rb_tree_node
//                      with member _M_value_field (pair with first and second)

    DIE *t = find_member (type, "_M_t") ;
    if (t != NULL) {
        DIE *base = find_base (t->get_type(), "_Rb_tree_base") ;
        if (base != NULL) {
            DIE *alloc_base = find_base (base, "_Rb_tree_alloc_base") ;
            if (alloc_base != NULL) {
                DIE *header = find_member (alloc_base, "_M_header") ;
                if (header != NULL) {
                    if (header->get_type()->is_pointer()) {
                        DIE *node = header->get_type()->get_type() ;
                        DIE *value = find_member (node, "_M_value_field") ;
                        if (value != NULL) {
                            value = value->get_type() ;
                            DIE *first = find_member(value, "first") ;
                            DIE *second = find_member(value, "second") ;
                            if (first != NULL && second != NULL) {
                                key_type = first->get_type() ;
                                mapped_type = second->get_type() ;
                                key_type->check_loaded() ;
                                mapped_type->check_loaded() ;
                            }
                        }
                    }
                }
            }
        }
    }
}


// like vector, pathCC generates different debug information for maps from gcc.

bool print_map (EvalContext &context, Address addr, DIE *type) {
    DIE *key_type = NULL ;
    DIE *mapped_type = NULL ;
    find_map_types (type, key_type, mapped_type) ;
    if (key_type == NULL || mapped_type == NULL) {
        return false ;
    }

    print_map_contents (context, addr, key_type, mapped_type) ;
    return true ;
}

// the address is the first element of the list

static void print_list_contents (EvalContext &context, Address addr, DIE *value_type) {
    bool comma = false ;
    Address sentinel = addr ;
    addr = context.process->readptr (addr) ;            // read next
    int size = value_type->get_real_size (context) ;

    int ptrsize = context.process->get_arch()->ptrsize() ;
    context.os.print ("{") ;

    while (addr != sentinel) {
        if (comma) {
            context.os.print (", ") ;
        }
        comma = true ;
        Address dataaddr = addr + ptrsize * 2 ;
        if (value_type->is_struct() || value_type->is_array()) {
            Value v(dataaddr) ;
            value_type->print_value (context, v, 0) ;
        } else {
            Value v = context.process->read (dataaddr, size) ;
            value_type->print_value (context, v, 0) ;
        }
        addr = context.process->readptr (addr) ;            // read next ptr
    }
    
    context.os.print ("}") ;
}

 
DIE *find_list_type (DIE *type) {
    std::vector<DIE*> &children = type->getChildren() ;
    
    DIE *value_type = NULL ;

    // find the value_type typedef (g++ mode)
    for (unsigned int i = 0 ; i < children.size() ; i++) {
        DIE *child = children[i] ;
        if (child->get_tag() == DW_TAG_typedef) {
            std::string childname = child->getAttribute (DW_AT_name) ;
            if (childname == "value_type") {
                value_type = child ;
                break ;
            }
        }
    }

    if (value_type != NULL) {
        value_type->check_loaded() ;
        value_type = value_type->get_type() ;               // dereference the typedef
        return value_type ;
    }

    // see if the there is a base class of the name _List_base<
    // if it exists, it will have a base class whose name is _List_alloc_base.  This
    // has a member called _M_node whose type is a pointer to _List_node which
    // has a member called _M_data whose type is the value type
    // XXX: this is wrong.  There is actually no way to get the value_type from
    // the output from pathCC.
    DIE *base = find_base (type, "_List_base") ;
    if (base == NULL) return NULL ;

    DIE *alloc_base = find_base (base, "_List_alloc_base") ;
    if (alloc_base == NULL)  return NULL ;

    DIE *start = find_member (alloc_base, "_M_node") ;
    if (start == NULL)  return NULL ;

    if (!start->get_type()->is_pointer()) return NULL;

    DIE *data = find_member (start->get_type()->get_type(), "_M_data") ;
    if (data == NULL)  return NULL ;

    return data->get_type() ;
}


bool print_list (EvalContext &context, Address addr, DIE *type) {
    DIE *value_type = find_list_type (type) ;
    if (value_type != NULL) {
        print_list_contents (context, context.process->readptr(addr), value_type) ;
        return true ;
    }
    return false ;
}


Utils::RegularExpression *string_re  ;
Utils::RegularExpression *vector_re  ;
Utils::RegularExpression *map_re  ;
Utils::RegularExpression *list_re  ;

void init_res() {
    if (string_re == NULL) {
        string_re = new Utils::RegularExpression ("string|basic_string<char,std::char_traits<char>,std::allocator<char> >") ;
    }
    if (vector_re == NULL) {
        vector_re = new Utils::RegularExpression ("std::vector<|vector<") ;
    }
    if (map_re == NULL) {
        map_re = new Utils::RegularExpression ("std::map<|map<") ;
    }

    if (list_re == NULL) {
        list_re = new Utils::RegularExpression ("std::list<|list<") ;
    }


}


bool print_struct (EvalContext &context, Address addr, DIE *type) {
    init_res() ;

    if (context.process->get_int_opt(PRM_P_CSTR)) {
        std::string name = type->get_name() ;
        if (string_re->match (name).size() != 0) {
            if (print_string (context, addr, type)) {
                return true ;
            }
        }
    }

    if (context.process->get_int_opt(PRM_P_CVEC) &&
        vector_re->match (type->get_name()).size() != 0) {
        if (print_vector (context, addr, type)) {
            return true ;
        }
    }

    if (context.process->get_int_opt(PRM_P_CMAP) &&
        map_re->match (type->get_name()).size() != 0) {
        if (print_map (context, addr, type)) {
            return true ;
        }
    }
    if (context.process->get_int_opt(PRM_P_CLIST) &&
        list_re->match (type->get_name()).size() != 0) {
        if (print_list (context, addr, type)) {
            return true ;
        }
    }
    return false ;
}

bool subscript_struct (SymbolTable *symtab, EvalContext &context,
Address addr, DIE *type, int subscript, Value &outval, DIE *&outtype) {
    init_res() ;
    std::string name = type->get_name() ;
    if (string_re->match (name).size() != 0) {
        if (subscript_string (symtab, context, addr, type, subscript, outval, outtype)) {
            return true ;
        }
    }
                                                                                                                                                
    if (vector_re->match (type->get_name()).size() != 0) {
        if (subscript_vector (symtab, context, addr, type, subscript, outval, outtype)) {
            return true ;
        }
    }

    // XXX: this is harder - need to search the map
//    if (map_re->match (type->get_name()).size() != 0) {
        //if (subscript_map (symtab, context, addr, type, subscript, outval, outtype)) {
            //return true ;
        //}
    //}
    return false ;

}

bool subscript_struct (SymbolTable *symtab, EvalContext &context, Address addr, DIE *type, int lo, int hi, DIE *&outtype) {
    return false ;
}

}

