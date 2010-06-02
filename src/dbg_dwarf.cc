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

file: dbg_dwarf.cc
created on: Fri Aug 13 11:07:34 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#include "process.h"
#include "dbg_dwarf.h"

#include "dwf_cfa.h"
#include "dwf_names.h"
#include "dwf_locs.h"
#include "dwf_cunit.h"

#include "type_nspace.h"
#include "type_array.h"
#include "type_class.h"
#include "type_enum.h"
#include "type_union.h"
#include "type_pointer.h"

#include "arch.h"
#include "utils.h"
#include "pstream.h"
#include "junk_stream.h"
#include "dbg_stl.h"

#include <ctype.h>
#include <sys/stat.h>

Value AttributeValue::toValue(DIE* type) {
    if (type->is_integral()) {
        return Value(VALUE_INTEGER, integer) ; 
    } else if (type->is_real()) {
        return Value(VALUE_REAL, integer) ;
    } else {
        // XXX: needs to be implemented
        throw Exception("Unable to get value") ;
    }
}



LocationList::LocationList() {
}

LocationList::~LocationList() {
}

void LocationList::push_back (LocationListEntry& entry) {
    list.push_back(entry) ;
}

void LocationList::clear (void) {
    list.clear() ;
}

BVector LocationList::getexpr (DwCUnit* cu, Address pc) {
    std::list<LocationListEntry>::iterator i;

    pc = pc - cu->get_base_pc();

    for (i = list.begin(); i != list.end(); i++) {
       if (pc >= i->low && pc < i->high)
          return i->expr ;
    }

    throw Exception("Unable to get value of variable");
}

LocationListTable::LocationListTable (DwCUnit* cu, BVector vec) {
    BStream stream (vec, cu->do_swap());

    while (!stream.eof()) {
        Offset offset;
        Address base_addr;
        LocationList list;
        LocationListEntry entry;       

        base_addr = 0 ;
        offset = stream.offset() ;

        for (;;) {
           int32_t entry_len;

           entry.low = cu->read_address(stream) ;
           entry.high = cu->read_address(stream) ;
           if (entry.low == 0 && entry.high == 0) {
              break ;  /* end of list marker */
           }
           else if (entry.low == -1) {
              base_addr = entry.high ;
              continue ; /* base address selector */
           }

           entry_len = stream.read2u();
           const byte* addr = stream.address();
           entry.expr = BVector(addr, entry_len);
           stream.seek(entry_len, BSTREAM_CUR); 

           entry.low += base_addr ;
           entry.high += base_addr ;
           list.push_back(entry) ;
        }

        table[offset] = list;
        list.clear();
    } 
}

LocationListTable::~LocationListTable () {

}

LocationList& LocationListTable::getlist (Offset offset) {
    std::map<int,LocationList>::iterator i;

    i = table.find(offset);
    if (i != table.end()) {
       return i->second;
    }

    throw Exception ("Unable to get value of variable");
}

BVector LocationListTable::getexpr (DwCUnit* cu, Offset offset, Address pc) {
    LocationList& loclist = getlist(offset);
    return loclist.getexpr(cu, pc);
}

AttributeAbbreviation::AttributeAbbreviation (int tag, int form)
    : tag(tag), form(form) { }

AttributeAbbreviation::~AttributeAbbreviation() {
}

void AttributeAbbreviation::print() {
       std::cout << "\ttag: "  <<  tag  <<  " form: "  <<  form << '\n' ;
}

int AttributeAbbreviation::getTag() {
       return tag ;
}

int AttributeAbbreviation::getForm() {
       return form ;
}

static BVector readblock (int len, BStream &stream) {
    const byte* addr = stream.address();
    stream.seek(len, BSTREAM_CUR);
    return BVector(addr, len); 
}


AttributeValue AttributeAbbreviation::read(DwCUnit *cu, Attribute * attr, BStream & stream) {
        switch (form) {
        case DW_FORM_addr:
            return cu->read_address (stream) ;

        case DW_FORM_block1: {
            int len = stream.read1u() ;
            return readblock (len, stream) ;
            }

        case DW_FORM_block2: {
            int len = stream.read2u() ;
            return readblock (len, stream) ;
            }

        case DW_FORM_block4: {
            int len = stream.read4u() ;
            return readblock (len, stream) ;
            }

        case DW_FORM_data1:
            return stream.read1u() ;

        case DW_FORM_data2:
            return stream.read2u() ;

        case DW_FORM_data4: {
            int v = stream.read4u() ;
            return v ;
            }
        case DW_FORM_data8:
            return stream.read8u() ;
            break ;
        case DW_FORM_string: {
            std::string s = "" ;
            while (!stream.eof()) {
                char ch = stream.read1u() ;
                if (ch == 0) {
                    break ;
                }
                s += (char)(ch) ;
            }
            return s ;
            break ;
            }
        case DW_FORM_blockv: {
            int len = stream.read_uleb() ;
            return readblock (len, stream) ;
            }
        case DW_FORM_flag:
            return stream.read1u() ;
            break ;
        case DW_FORM_sdata:
            return stream.read_sleb() ;
            break ;
        case DW_FORM_strp: {
            int index = stream.read4u() ;
            //println ("index = " + index)
            std::string s = cu->get_string_table()->getString (index) ;
            return s ;
            break ;
            }
        case DW_FORM_udata:
            return stream.read_uleb() ;
            break ;
        case DW_FORM_ref_addr:
	    std::cerr << "FORM DW_FORM_ref_addr ignored";
	    return cu->read_address (stream) ;
	    //throw Exception ("FORM Not yet implemented") ;
	    break ;
        case DW_FORM_ref1: {
            Offset offset = stream.read1u() ;
            return cu->get_die_reference (attr, offset) ;
            }
        case DW_FORM_ref2: {
            int offset = stream.read2u() ;
            return cu->get_die_reference(attr, offset) ;
            }
        case DW_FORM_ref4: {
            int offset = stream.read4u() ;
            return cu->get_die_reference (attr, offset) ;
            }
        case DW_FORM_ref8: {
            int64_t offset = stream.read8u() ;
            return cu->get_die_reference (attr, offset) ;
            }
        case DW_FORM_ref_udata:
            throw Exception ("FORM Not yet implemented") ;
            break ;
        case DW_FORM_indirect:
            throw Exception ("FORM Not yet implemented") ;
            break ;
       }
	throw Exception("not reached");
}


Abbreviation::Abbreviation (int num, int tag, bool haschildren)
    : num(num),
    tag(tag),
    haschildren(haschildren) {
}

Abbreviation::~Abbreviation() {
    // delete the enclosed abbreviations (only populated for compilation units)
    for (AbbreviationMap::iterator i = table.begin() ; i != table.end() ; i++) {
        delete i->second ;
    }

    // delete the attribute abbreviations
    for (uint i = 0 ; i < attributes.size() ; i++) {
        delete attributes[i] ;
    }
}

void Abbreviation::read(BStream & bstream) {
       // read the attributes
       for (;;) {
           int t = bstream.read_uleb() ;
           int tag = t ;
           t = bstream.read_uleb() ;
           int form =t ;
           if (tag == 0 && form == 0) {
               break ;
           }
           attributes.push_back (new AttributeAbbreviation (tag, form)) ;
       }
}

void Abbreviation::addAbbreviation(int n, Abbreviation * abb) {
       //println ("code: " + num + ": adding abbreviation code " + n)
       table[n] = abb ;
}

Abbreviation *Abbreviation::getAbbreviation(int n) {
       AbbreviationMap::iterator abb = table.find (n) ;
       if (abb == table.end()) {
           throw Exception ("No such abbreviation code: ") ;
       }
       return abb->second ;
}

void Abbreviation::print() {
       std::cout << "num: "  <<  num  <<  " tag: "  <<  tag  <<  "["  <<  (haschildren?"has":"no")  <<  " children]" << '\n' ;
       for (uint attr = 0 ; attr < attributes.size(); attr++) {
           attributes[attr]->print () ;
       }
}

bool Abbreviation::has_children() {
       return haschildren ;
}

int Abbreviation::getNum() {
       return num ;
}

int Abbreviation::getTag() {
      return tag ;
}

int Abbreviation::getNumAttributes() {
       return attributes.size() ;
}

AttributeAbbreviation *Abbreviation::getAttribute(int i) {
       return attributes[i] ;
}

Attribute::Attribute (int form)
    : form(form) {
}

Attribute::~Attribute() {
}


void Attribute::fixup(DIE *die) {
    value = die ;
}

void FixupAttribute::fixup(DIE *die) {
    value = die ;
    die->set_more_info (parent) ;
}



Formal_parameter::Formal_parameter (DwCUnit *cu, DIE *parent, Abbreviation *abbrev)
    : DIE(cu, parent, abbrev) {
}

Formal_parameter::Formal_parameter (DwCUnit *cu, DIE *parent, int tag)
    : DIE(cu, parent, tag) {
}


Formal_parameter::~Formal_parameter() {
}

void Formal_parameter::print(EvalContext &ctx, int indent, int level) {
        doindent (ctx, indent) ;
        switch (ctx.language) {
        case DW_LANG_C:
        case DW_LANG_C89:
        case DW_LANG_C_plus_plus:
            print_declaration (ctx, this, 0) ;
            break ;
        case DW_LANG_Fortran77:
            get_type()->print (ctx, 0, level+1) ;
            ctx.os.print (" %s", get_name().c_str()) ;
            break ;
        case DW_LANG_Fortran90: 
            get_type()->print (ctx, 0, level+1) ;
            ctx.os.print (" :: %s", get_name().c_str()) ;
            break ;
        default:
            ctx.os.print ("parameter %s", get_name().c_str()) ;
            break ;
        }
}

Value Formal_parameter::evaluate(EvalContext &context) {
        AttributeValue &loc = getAttribute (DW_AT_location) ;
        if (loc.type == AV_NONE) {
           throw Exception ("Unable to get value of formal parameter %s"
              " - perhaps it is optimized out", get_name().c_str()) ;
        }
        DwLocExpr addr = cu->evaluate_location (cu, context.fb, loc, context.process) ;

        DIE *type = get_type() ;
        if (type == NULL) throw Exception("<incomplete debug info>");

        if (type->is_struct() || type->is_array() || type->is_string() || type->is_complex()) {
            if (type->get_tag() == DW_TAG_union_type) {
                dynamic_cast<TypeUnion*>(type)->add_locations() ;
            }
            return addr.getAddress();
        }

        return context.addressonly ? addr.getAddress() : (Value)addr.getValue(
          context.process, type->get_size(), type->is_signed()) ;
}

bool Formal_parameter::is_local_var() {
    return true ;
}

Lexical_block::Lexical_block (DwCUnit *cu, DIE *parent, Abbreviation *abbrev)
    : DIE(cu, parent, abbrev), symbolsok(false) {
}

Lexical_block::~Lexical_block() {
}

void Lexical_block::print(EvalContext &ctx, int indent, int level) {
        doindent (ctx, indent) ;
}

void Lexical_block::find_symbol (std::string name, Address pc, std::vector<DIE*> &result, DIE *caller) {
    check_loaded() ;
    bool caseblind = is_case_blind() ;

    if (!symbolsok) {
        for (uint i = 0 ; i < children.size() ; i++) {
            DIE *child = children[i] ;
            int childtag = child->get_tag() ;
            //println ("looking at child tag " + child.get_tag())
            if (childtag == DW_TAG_variable || childtag == DW_TAG_formal_parameter ||
                childtag == DW_TAG_structure_type || childtag == DW_TAG_class_type ||
                childtag == DW_TAG_union_type || childtag == DW_TAG_typedef ||
                childtag == DW_TAG_enumeration_type) {
                std::string childname = child->getAttribute (DW_AT_name).str ;
                if (caseblind) {
                    symbols[Utils::toUpper(childname)] = child ;
                } else {
                    symbols[childname] = child ;
                }
                if (childtag == DW_TAG_enumeration_type) {
                    enumerations.push_back (child) ;
                }
            } else if (childtag == DW_TAG_lexical_block) {
                lexical_blocks.push_back (child) ;
            }
        }
        symbolsok = true ;
    }

    // search the enumerations
    for (uint i = 0 ; i < enumerations.size() ; i++) {
        DIE *die = enumerations[i] ;
        die->find_symbol (name, pc, result, this) ;
        if (!result.empty()) {
            return ;
        }
    }

    // first search the nested lexical blocks for the symbol. 
    for (uint i = 0 ; i < lexical_blocks.size() ; i++) {
        DIE *lex = lexical_blocks[i] ;
        Address low = lex->getAttribute (DW_AT_low_pc) ;
        low += cu->get_base() ;
        Address high = lex->getAttribute (DW_AT_high_pc) ;
        high += cu->get_base() ;
        if (pc == 0 || (pc >= low && pc < high)) {
            lex->find_symbol (name, pc, result, this) ;
            if (!result.empty()) {
                return ;
            }
        }
    }

    DIEMap::iterator symi = symbols.find (caseblind?Utils::toUpper(name):name) ;
    if (symi != symbols.end()) {
        result.push_back (symi->second) ;
    }
}

void Lexical_block::get_local_variables (std::vector<DIE*> &vec) {
    for (uint i = 0 ; i < children.size() ; i++) {
        DIE *child = children[i] ;
        int childtag = child->get_tag() ;
        //println ("looking at child tag " + child.get_tag())
        if (childtag == DW_TAG_variable) {
            vec.push_back (child) ;
        } else if (childtag == DW_TAG_lexical_block) {
            Lexical_block *block = dynamic_cast<Lexical_block*>(child) ;
            block->get_local_variables (vec) ;
        }
    }
}

Member::Member (DwCUnit *cu, DIE *parent, Abbreviation *abbrev)
    : DIE(cu, parent, abbrev) {
}

Member::~Member() {
}

void Member::print(EvalContext &ctx, int indent, int level) {
        doindent (ctx, indent) ;
        switch (ctx.language & 0xff) {
        case DW_LANG_C89:
        case DW_LANG_C:
        case DW_LANG_C_plus_plus:
            print_declaration (ctx, this, 0) ;
            break ;
        case DW_LANG_Fortran77:
            get_type()->print (ctx, 0, level+1) ;
            ctx.os.print (" %s", get_name().c_str()) ;
            break ;
        case DW_LANG_Fortran90: 
            get_type()->print (ctx, 0, level+1) ;
            ctx.os.print (" :: %s", get_name().c_str()) ;
            break ;
        default:
            ctx.os.print ("member: ") ;
            get_type()->print (ctx, 0, level+1) ;
            ctx.os.print (" %s", get_name().c_str()) ;
            break ;
        }
}

Value Member::evaluate(EvalContext &context, Value &parentstruct) {
    AttributeValue loc = getAttribute (DW_AT_data_member_location) ;
    if (loc.type == AV_NONE) {
       throw Exception ("Unable to get value of member %s - perhaps it is optimized out", get_name().c_str()) ;
    }

    DwLocExpr addr = cu->evaluate_location (cu, context.fb, loc, context.process, parentstruct) ;
    DIE * type = get_type() ;
    if (type->is_struct() || type->is_array() || type->is_string() || type->is_complex()) {
        if (type->get_tag() == DW_TAG_union_type) {
            dynamic_cast<TypeUnion*>(type)->add_locations() ;
        }
        return addr.getAddress();
    } else if (context.addressonly) {
        return addr.getAddress() ;
    } else {
        Value v = addr.getValue(context.process, get_type()->get_size(), get_type()->is_signed()) ;
        AttributeValue bs = getAttribute (DW_AT_bit_size) ;             // is this member a bit field?
        if (bs.type == AV_NONE) {
            return v ;                                                  // not bitfield, return whole value
        }
        int bitsize = (int)bs ;
        int bitoffset = getAttribute (DW_AT_bit_offset) ;
        // bit offset is from MSB of word and is inclusive.  For example:
        // struct A {
        //    int a:1 ;         - offset 31
        //    int b:2 ;         - offset 29
        //    int c:1 ;         - offset 28

        int val = v.integer << bitoffset ;                                      // put high order bit in top bit of word
        val >>= 32 - bitsize ;                                                  // shift down to bottom bit, sign extending
        int encoding = get_type()->getAttribute (DW_AT_encoding) ;
        if (encoding == DW_ATE_unsigned) {
            if (bitsize == 32) {
                v.integer = val ;
                v.integer &= 0xffffffffULL ;
            } else {
                int mask = (1 << bitsize) - 1 ;            // mask of appropriate width
                val &= mask ;
                v.integer = val ;
            }
        } else {
            v.integer = val ;
        }
        return v; 
    }
}

Value Member::evaluate(EvalContext &context) {
    throw Exception("cannot evaluate non-static members");
}

Reference_type::Reference_type (DwCUnit *cu, DIE *parent, Abbreviation *abbrev)
    : DIE(cu, parent, abbrev) {
}

Reference_type::~Reference_type() {
}

void Reference_type::print(EvalContext &ctx, int indent, int level) {
        doindent (ctx, indent) ;
        print_declaration (ctx, this, indent) ;
}

// references are ignored for type comparison purposes
bool Reference_type::compare (EvalContext &context, DIE *die, int flags) {
    while (die != NULL && die->get_tag() == DW_TAG_reference_type) {
        die = die->get_type() ;
    }
    return get_type()->compare (context, die, flags) ;
}

int Reference_type::get_size() {
    return get_type()->get_size() ;
}

int Reference_type::get_real_size(EvalContext &ctx) {
    return get_type()->get_real_size(ctx) ;
}

bool Reference_type::is_scalar() {
    return get_type()->is_scalar() ;
}

bool Reference_type::is_pointer() {
    return get_type()->is_pointer() ;
}

bool Reference_type::is_real() {
    return get_type()->is_real() ;
}

bool Reference_type::is_integral() {
    return get_type()->is_integral() ;
}

bool Reference_type::is_address() {
    return get_type()->is_address() ;
}

bool Reference_type::is_struct_deref() {
    return get_type()->is_struct() ;
}

int Reference_type::get_size_immed() {
    return getAttribute (DW_AT_byte_size).integer ;
}

void Reference_type::set_value (EvalContext &context, Value &addr, Value &value) {
    get_type()->set_value (context, addr, value) ;
}

void Reference_type::print_value(EvalContext &context, Value &value, int indent) {
        DIE * subtype = get_type() ;

        switch (context.language & 0xff) {
        case DW_LANG_C89:
        case DW_LANG_C:
        case DW_LANG_C_plus_plus:
            if (context.process->get_int_opt(PRM_P_ADDR)) {
                context.os.print ("@0x%llx", (int64_t)value) ;
            }
            if (context.process->is_active() && context.show_reference && subtype != NULL && value != 0) {
                context.os.print (": ") ;
                if (subtype->is_struct() || subtype->is_array()) {
                    subtype->print_value (context, value) ;
                } else {
                    int size = subtype->get_size() ;
                    Value v = context.process->read (value.integer, size) ;
                    if (subtype->is_signed()) {
                        v.integer <<= 64 - size*8 ;
                        v.integer >>= 64 - size*8 ;
                    }
                    subtype->print_value (context, v) ;
                }
            }
            break ;
        case DW_LANG_Ada83:
            throw Exception ("Ada is not a supported language") ;
           break ;
        case DW_LANG_Cobol74:
        case DW_LANG_Cobol85:
            throw Exception ("COBOL is not a supported language") ;
        case DW_LANG_Fortran77:
        case DW_LANG_Fortran90:
            break ;
        case DW_LANG_Pascal83:
            throw Exception ("Pascal is not a supported language") ;
        case DW_LANG_Modula2:
            throw Exception ("Modula2 is not a supported language") ;
        }
}

Compile_unit::Compile_unit (DwCUnit *cu, DIE *parent, Abbreviation *abbrev)
    : DIE(cu, parent, abbrev) ,
    symbolsok(false) {
}

Compile_unit::~Compile_unit() {
}

void Compile_unit::print(EvalContext &ctx, int indent, int level) {
        doindent (ctx, indent) ;
}

DIE * Compile_unit::find_scope(std::string name) {
    std::vector<DIE *> matches ;
    check_loaded() ;
    bool caseblind = is_case_blind() ;
    // first find all matches
    for (uint i = 0 ; i < children.size(); i++) {
        DIE *child = children[i] ;
        int childtag = child->get_tag() ;
        //println ("looking at child tag " + childtag)
        if (childtag == DW_TAG_subprogram ||
            childtag == DW_TAG_structure_type ||
            childtag == DW_TAG_union_type ||
            childtag == DW_TAG_typedef ||
            childtag == DW_TAG_enumeration_type ||
            childtag == DW_TAG_class_type ||
            childtag == DW_TAG_namespace) {
            std::string childname = child->getAttribute (DW_AT_name).str ;
            if (caseblind && Utils::toUpper (childname) == Utils::toUpper (name)) {
                matches.push_back (child) ;
            } else if (childname == name) {
                matches.push_back (child) ;
            }
        }
    }

    // there may be more than one die with the same name.  For example, a class name
    // and a constructor within that class.  We prefer class names over subprograms
    // because that's probably what the user wanted

    if (matches.size() == 0) {
        return NULL ;
    }

    if (matches.size() == 1) {
        return matches[0] ;
    }

    // first look for a struct/union
    for (uint i = 0 ; i < matches.size() ; i++) {
        if (matches[i]->is_struct()) {
            return matches[i] ;
        }
    }

    return matches[0] ;         // just return the first match
}


void  Compile_unit::find_symbol(std::string name, Address pc, std::vector<DIE*> &result, DIE *caller) {
    check_loaded() ;
    bool caseblind = is_case_blind() ;
    
    if (!symbolsok) {
        for (uint i = 0 ; i < children.size(); i++) {
            DIE *child = children[i] ;
            int childtag = child->get_tag() ;
            //println ("looking at child tag " + childtag)
            if (childtag == DW_TAG_variable || childtag == DW_TAG_subprogram || childtag == DW_TAG_structure_type
                    || childtag == DW_TAG_union_type || childtag == DW_TAG_typedef || childtag == DW_TAG_enumeration_type ||
                  childtag == DW_TAG_class_type) {
                std::string childname = child->getAttribute (DW_AT_name).str ;
                if (caseblind) {
                    symbols.insert (MultiDIEMap::value_type (Utils::toUpper(childname), child)) ;
                } else {
                    symbols.insert (MultiDIEMap::value_type (childname, child)) ;
                }
                if (childtag == DW_TAG_enumeration_type) {
                    enumerations.push_back (child) ;
                }
            }
        }
        symbolsok = true ;
    }
    
    // search the enumerations
    for (uint i = 0 ; i < enumerations.size() ; i++) {
        DIE *die = enumerations[i] ;
        die->find_symbol (name, pc, result, this) ;
        if (!result.empty()) {
            return ;
        }
    }

    std::pair<MultiDIEMap::iterator,MultiDIEMap::iterator> sym = symbols.equal_range(caseblind? Utils::toUpper (name) : name) ;
    while (sym.first != sym.second) {
        result.push_back ((*sym.first).second) ;
        sym.first++ ;
    }
}

String_type::String_type (DwCUnit *cu, DIE *parent, Abbreviation *abbrev)
    : DIE(cu, parent, abbrev) {
}

String_type::String_type (DwCUnit *cu, DIE *parent, int tag)
    : DIE(cu, parent, tag) {
}

String_type::~String_type() {
}

int String_type::get_length(EvalContext &context) {
    AttributeValue bs =  getAttribute (DW_AT_byte_size) ;
    AttributeValue sl = getAttribute (DW_AT_string_length) ;
    if (sl.type == AV_NONE) {
        return bs ;
    }
    int lsize ;                 // number of bytes to load
    if (bs.type == AV_NONE) {
        lsize = context.process->get_arch()->ptrsize() ;
    } else {
        lsize = bs ;
    }
    DwLocExpr addr = cu->evaluate_location (cu, context.fb, sl, context.process);
    return addr.getValue(context.process, lsize, false) ;
}

void String_type::print(EvalContext &ctx, int indent, int level) {
    doindent (ctx, indent) ;
    std::string name =  getAttribute(DW_AT_name).str ;
    switch (ctx.language & 0xff) {
    case DW_LANG_C:
    case DW_LANG_C89:
    case DW_LANG_C_plus_plus:
        ctx.os.print ("char [%d]", get_length(ctx)) ;
        break ;
    default:
        ctx.os.print ("%s", name.c_str()) ;
        break ;
    case DW_LANG_Fortran77:    
        ctx.os.print ("character*%d", get_length(ctx)) ;
        break ;
    case DW_LANG_Fortran90: 
        ctx.os.print ("character(len=%d)", get_length(ctx)) ;
        break ;
    }
}

// value is the address of the string variable
void String_type::print_value(EvalContext &context, Value &value, int indent) {
        if (!context.process->is_active()) {
            throw Exception("A running process is required for this operation") ;
        }
        if (value.type == VALUE_INTEGER && value.integer == 0) {
            context.os.print ("<omitted>") ;
            return ;
        }
        if (value.type == VALUE_STRING) {                       // string literal from debugger?
            context.os.print ("\"") ;
            Utils::print_string (context, value.str);
            context.os.print ("\"") ;
            return ;
        }

        int len = get_length(context) ;
        std::string s = context.process->read_string (value.integer, len) ;

        switch (context.language & 0xff) {
        case DW_LANG_C89:
        case DW_LANG_C: 
        case DW_LANG_C_plus_plus:
            context.os.print ("\"") ;
            Utils::print_string (context, s);
            context.os.print ("\"") ;
            break ;
        case DW_LANG_Ada83:
            throw Exception ("Ada is not a supported language") ;
            break ;
        case DW_LANG_Cobol74:
        case DW_LANG_Cobol85:
            throw Exception ("COBOL is not a supported language") ;
        case DW_LANG_Fortran77:
        case DW_LANG_Fortran90:
            context.os.print("\'") ;
            Utils::print_string (context, s);
            context.os.print ("\'") ;
            break ;
        case DW_LANG_Pascal83:
            throw Exception ("Pascal is not a supported language") ;
        case DW_LANG_Modula2:
            throw Exception ("Modula2 is not a supported language") ;
        }
}

Common_block::Common_block (DwCUnit *cu, DIE *parent, Abbreviation *abbrev)
    : DIE(cu, parent, abbrev), symbolsok(false) {
}

Common_block::~Common_block() {
}

void Common_block::print(EvalContext &ctx, int indent, int level) {
    doindent (ctx, indent) ;
    bool newline = false ;
    ctx.os.print ("common/%s/ ", get_name().c_str()) ;
    if (ctx.show_contents) {
        for (uint i = 0 ; i < children.size() ; i++) {
            DIE *child = children[i] ;
            if (newline) {
                ctx.os.print ("\n") ;
            }
            child->print (ctx, indent+4, 0) ;
            newline = true ;
        }
    }
}

void Common_block::find_symbol (std::string name, Address pc, std::vector<DIE*> &result, DIE *caller) {
    check_loaded() ;
    bool caseblind = is_case_blind() ;

    if (!symbolsok) {
        for (uint i = 0 ; i < children.size() ; i++) {
            DIE *child = children[i] ;
            int childtag = child->get_tag() ;
            //println ("looking at child tag " + child.get_tag())
            if (childtag == DW_TAG_variable || childtag == DW_TAG_formal_parameter || childtag == DW_TAG_structure_type
                    || childtag == DW_TAG_union_type || childtag == DW_TAG_typedef || childtag == DW_TAG_enumeration_type
                    || childtag == DW_TAG_class_type) {
                std::string childname = child->getAttribute (DW_AT_name).str ;
                if (caseblind) {
                    symbols[Utils::toUpper(childname)] = child ;
                } else {
                    symbols[childname] = child ;
                }
            }
        }
        symbolsok = true ;
    }

    DIEMap::iterator symi = symbols.find (caseblind?Utils::toUpper(name):name) ;
    if (symi != symbols.end()) {
        result.push_back (symi->second) ;
        return ;
    }

    // we can't look in the parent because that is the Subprogram that called us
}

void Common_block::print_value(EvalContext &context, Value &value, int indent) {
    context.os.print ("common/%s/\n", get_name().c_str()) ;

    bool newline = false ;
    for (uint i = 0 ; i < children.size(); i++) {
        DIE *child = children[i] ;
        if (newline) {
            context.os.print ("\n") ;
        }
        doindent (context, indent+4) ;
        std::string childname = child->getAttribute (DW_AT_name).str ;
        if (childname != "") {
            context.os.print ("%s = ", childname.c_str()) ;
        }
        Value v = child->evaluate (context) ;
        if (child->get_type()->is_real()) {
            if (child->get_type()->get_real_size(context) == 4) {
                v.real = (double)(*(float*)&v.real) ;           // convert to double
            }
        }
        child->get_type()->print_value (context, v, indent+4) ;
        newline = true ;
    }
}



Inheritance::Inheritance (DwCUnit *cu, DIE *parent, Abbreviation *abbrev)
    : DIE(cu, parent, abbrev) {
}

Inheritance::~Inheritance() {
}

void Inheritance::print(EvalContext &ctx, int indent, int level) {
   int access = getAttribute (DW_AT_accessibility).integer ;

   /* first print protections */
   switch (access) {
      case DW_ACCESS_protected:
         ctx.os.print ("protected ") ;
         break ;
      case DW_ACCESS_private:
         ctx.os.print ("private ") ;
         break ;
      case DW_ACCESS_public:
      default:
         ctx.os.print ("public ") ;
         break ;
   }

   /* watch out for virtuals */
   if (is_virtual()) {
       ctx.os.print ("virtual ") ;
   }

   /* finally just print out name */
   ctx.os.print ("%s", get_type()->get_name().c_str()) ;
   doindent (ctx, indent) ;
}


Value Inheritance::evaluate(EvalContext &context, Value &parentstruct) {
    AttributeValue loc = getAttribute (DW_AT_data_member_location) ;
    if (loc.type == AV_NONE) {
       throw Exception ("Unable to get value of base class") ;
    }

    DwLocExpr addr = cu->evaluate_location (cu, context.fb, loc, context.process, parentstruct);
    return addr.getAddress() ;
}

Value Inheritance::evaluate(EvalContext &context) {
    /* XXX: shouldn't this be deleted */
    AttributeValue loc = getAttribute (DW_AT_data_member_location) ;
    if (loc.type == AV_NONE) {
       throw Exception ("Unable to get value of base class") ;
    }

    DwLocExpr addr = cu->evaluate_location (cu, context.fb, loc, context.process);
    return addr.getAddress() ;
}


Base_type::Base_type (DwCUnit *cu, DIE *parent, Abbreviation *abbrev)
    : DIE(cu, parent, abbrev) {
}

Base_type::Base_type (DwCUnit *cu, DIE *parent, int tag)
    : DIE(cu, parent, tag) {
}

Base_type::~Base_type() {
}

// for FORTRAN we want to print the proper syntax for the type.  For example:
// INTEGER_2 is the name of the type (in the DIE).  This should be printed as follows:
//   f77: integer*2
//   f90: integer(kind=2)

void Base_type::print(EvalContext &ctx, int indent, int level) {
    doindent (ctx, indent) ;
    std::string name =  getAttribute (DW_AT_name) ;
    switch (ctx.language & 0xff) {
    case DW_LANG_C89:
    case DW_LANG_C:
    case DW_LANG_C_plus_plus:
    default:
        ctx.os.print ("%s", name.c_str()) ;
        break ;
    case DW_LANG_Fortran77: {
        int size = getAttribute (DW_AT_byte_size) ;
        int encoding = getAttribute (DW_AT_encoding) ;
        switch (encoding) {
        case DW_ATE_address:
           ctx.os.print ("unknown*%d", size) ;
           break ;
        case DW_ATE_boolean:
           ctx.os.print ("logical*%d", size) ;
           break ;
        case DW_ATE_complex_float: 
           ctx.os.print ("complex*%d", size) ;
           break ;
        case DW_ATE_float:
           ctx.os.print ("real*%d", size) ;
           break ;
        case DW_ATE_signed:                     // regular signed integer
        case DW_ATE_unsigned:                   // unsigned integer
           ctx.os.print ("integer*%d", size) ;
           break ;
        case DW_ATE_signed_char:
        case DW_ATE_unsigned_char:
           ctx.os.print ("character*%d", size) ;
           break ;
        }
        break ;
        }
    case DW_LANG_Fortran90: {
        int size = getAttribute (DW_AT_byte_size) ;
        int encoding = getAttribute (DW_AT_encoding) ;
        switch (encoding) {
        case DW_ATE_address:
           ctx.os.print ("unknown(kind=%d)", size) ;
           break ;
        case DW_ATE_boolean:
           ctx.os.print ("logical(kind=%d)", size) ;
           break ;
        case DW_ATE_complex_float: 
           ctx.os.print ("complex(kind=%d)", size) ;
           break ;
        case DW_ATE_float:
           ctx.os.print ("real(kind=%d)", size) ;
           break ;
        case DW_ATE_signed:                     // regular signed integer
        case DW_ATE_unsigned:                   // unsigned integer
           ctx.os.print ("integer(kind=%d)", size) ;
           break ;
        case DW_ATE_signed_char:
        case DW_ATE_unsigned_char:
           ctx.os.print ("character(len=%d)", size) ;
           break ;
        }
        break ;
        }
    }
}

bool Base_type::compare (EvalContext &context, DIE *die, int flags) {
    if (this == die) {          // most common case probably
        return true ;
    }
    // if not the same DIE, check the encoding and size
    if ((int)getAttribute (DW_AT_encoding) != (int)die->getAttribute (DW_AT_encoding)) {
        return false ;
    }
    if ((int)getAttribute (DW_AT_byte_size) != (int)die->getAttribute (DW_AT_byte_size)) {
        return false ;
    }
    return true ;
}

bool Base_type::is_scalar() {
        return true ;
}

bool Base_type::is_complex() {
    int enc = getAttribute (DW_AT_encoding) ;
    return enc == DW_ATE_complex_float ;
}

bool Base_type::is_real() {
    int enc = getAttribute (DW_AT_encoding) ;
    return enc ==  DW_ATE_complex_float || enc ==  DW_ATE_float ;
}

bool Base_type::is_integral() {
    return !is_real() && !is_address() ;
}

bool Base_type::is_address() {
    return (int)getAttribute (DW_AT_encoding) == DW_ATE_address ;
}

bool Base_type::is_boolean() {
    int enc = getAttribute (DW_AT_encoding) ;
    return enc == DW_ATE_boolean ;
}

bool Base_type::is_signed() {
    int encoding = getAttribute (DW_AT_encoding) ;
    return is_integral() && (encoding == DW_ATE_signed_char || encoding == DW_ATE_signed) ;
}

bool Base_type::is_char() {
    /* check for a direct encoding */
    int enc = getAttribute (DW_AT_encoding);
    if (enc == DW_ATE_signed_char ||
        enc == DW_ATE_unsigned_char)
        return true;

    /* check for encoding by name */
    std::string nam = getAttribute (DW_AT_name);
    if (nam == "char") return true;
  
    /* not caught, isn't a char */
    return false; 
}

bool Base_type::is_uchar() {
    /* check for direct encoding */
    int enc = getAttribute (DW_AT_encoding);
    if (enc == DW_ATE_unsigned_char)
       return true;

    /* check for encoding by name */
    std::string nam = getAttribute (DW_AT_name);
    if (nam == "unsigned char")
       return true;

    /* not caught, not uchar */
    return false; 
}

bool Base_type::is_schar() {
    /* check for direct encoding */
    int enc = getAttribute (DW_AT_encoding);
    if (enc == DW_ATE_signed_char)
       return true;

    /* check for encoding by name */
    std::string nam = getAttribute (DW_AT_name);
    if (nam == "char" || nam == "signed char")
       return true;

    /* not caught, not uchar */
    return false; 
}

void Base_type::set_value (EvalContext &context, Value &addr, Value &value) {
    context.process->set_value(addr, value, get_size()) ;
}

void Base_type::print_value(EvalContext &context, Value &value, int indent) {
   int enc = getAttribute(DW_AT_encoding);

   /* special case complex numbers */
   if (enc == DW_ATE_complex_float) {
      /* value is the address of complex */
      if (!context.process->is_active()) {
          throw Exception("A running process is required for this operation") ;
      }
      int size = getAttribute (DW_AT_byte_size) ;
      Address addr = value;
      double re, im;

      /* XXX: this is still a little ugly */
      if (size == 8) {          // 4 byte components
          int x = context.process->read (addr, 4) ;
          int y = context.process->read (addr+4, 4) ;
          re = (double)(*((float*)&x)) ;
          im = (double)(*((float*)&y)) ;
      } else {
          int64_t x = context.process->read (addr, 8) ;
          int64_t y = context.process->read (addr+8, 8) ;
          re = (*((double*)&x)) ;
          im = (*((double*)&y)) ;
      }
      context.os.print ("(%g,%g)", re, im) ;
      return; 
   }

   /* special case floating point numbers */
   if (enc == DW_ATE_float) {
      EvalContext ctx = context;

      int lang = ctx.process->get_language();
      if (lang == DW_LANG_Fortran77 || lang == DW_LANG_Fortran90 ) {
         if (ctx.truncate_aggregates) {
            ctx.num_sigdigs = 4;
            ctx.trunc_digs = false;
         } else {
            int size = getAttribute(DW_AT_byte_size); 
            ctx.num_sigdigs = size * 2;
            ctx.trunc_digs = false;
         }
      } else {
         int digs = ctx.process->get_int_opt(PRM_P_SDIGS);
         ctx.num_sigdigs = digs;
         ctx.trunc_digs = true;
      }

      /* XXX: replace me */
      JunkStream* js = reinterpret_cast<JunkStream*>(&ctx.os);
      js->print (ctx, (double)value) ;

      return;
   }

   /* special case characters */
   if (is_uchar()) {
      unsigned char ch = value;

      /* XXX: replace me */
      JunkStream* js = reinterpret_cast<JunkStream*>(&context.os);
      js->print(context, ch);

      return;
   }
   if (is_schar()) {
      signed char ch = value;

      /* XXX: replace me */
      JunkStream* js = reinterpret_cast<JunkStream*>(&context.os);
      js->print(context, ch);

      return;
   }

   /* otherwise, use encoding to print */
   switch (enc) {
   case DW_ATE_address:
   {
      /* XXX: replace me */
      JunkStream* js = reinterpret_cast<JunkStream*>(&context.os);
      js->print (context, (Address)value) ;
      return;
   }
   case DW_ATE_boolean:
   {
      /* XXX: replace me */
      JunkStream* js = reinterpret_cast<JunkStream*>(&context.os);
      js->print(context, (bool)value) ;
      return;
   }
   case DW_ATE_signed: 
   {
      /* XXX: replace me */
      JunkStream* js = reinterpret_cast<JunkStream*>(&context.os);
      js->print (context, (int64_t)value, get_size(), true) ;
      return;
   }
   case DW_ATE_signed_char:
   {
      /* XXX: replace me */
      JunkStream* js = reinterpret_cast<JunkStream*>(&context.os);
      js->print (context, (signed char)value) ;
      return;
   }
   case DW_ATE_unsigned:
   {
      /* XXX: replace me */
      JunkStream* js = reinterpret_cast<JunkStream*>(&context.os);
      js->print (context, (int64_t)value, get_size(), false) ; 
      return;
   }
   case DW_ATE_unsigned_char:
   {
      /* XXX: replace me */
      JunkStream* js = reinterpret_cast<JunkStream*>(&context.os);
      js->print (context ,(unsigned char)value) ;
      return;
   }
   }

}

Enumerator::Enumerator (DwCUnit *cu, DIE *parent, Abbreviation *abbrev)
    : DIE(cu, parent, abbrev) {
}

Enumerator::~Enumerator() {
}

void Enumerator::print(EvalContext &ctx, int indent, int level) {
        doindent (ctx, indent) ;
        parent->print (ctx, 0, level) ;
}

Value Enumerator::evaluate(EvalContext &context) {
    return (int)getAttribute (DW_AT_const_value) ;
}


Namelist::Namelist (DwCUnit *cu, DIE *parent, Abbreviation *abbrev)
    : DIE(cu, parent, abbrev) {
}

Namelist::~Namelist() {
}

void Namelist::print(EvalContext &ctx, int indent, int level) {
        doindent (ctx, indent) ;
}

Namelist_item::Namelist_item (DwCUnit *cu, DIE *parent, Abbreviation *abbrev)
    : DIE(cu, parent, abbrev) {
}

Namelist_item::~Namelist_item() {
}

void Namelist_item::print(EvalContext &ctx, int indent, int level) {
        doindent (ctx, indent) ;
}

Subprogram::Subprogram (DwCUnit *cu, DIE *parent, Abbreviation *abbrev)
    : DIE(cu, parent, abbrev), symbolsok(false) {
}

Subprogram::~Subprogram() {
}

bool Subprogram::is_static() {

   /* can't tell static global functions */
   if (parent->get_tag() != DW_TAG_structure_type ||
       parent->get_tag() != DW_TAG_class_type ||
       parent->get_tag() != DW_TAG_union_type) {
      return false;
   }

   /* scan through all children */
   for (unsigned i = 0; i<children.size(); i++) {
      /* get parameter DIE */
      DIE* d = children[i];

      /* skip the non-parameters */
      if (d->get_tag() != DW_TAG_formal_parameter)
         continue;

      /* check for artificiality */
      AttributeValue av = d->getAttribute(DW_AT_artificial);

      /* by default, it's real */
      if (av.type != AV_INTEGER)
         return true;

      /* "this" is artificial */
      return !av.integer;
   }

   /* catch no children */
   return true;
}

void Subprogram::print(EvalContext &ctx, int indent, int level) {
    check_loaded() ;
    doindent (ctx, indent) ;
    switch (ctx.language) {
    case DW_LANG_C:
    case DW_LANG_C89:
    case DW_LANG_C_plus_plus: {
        std::string parentname = "" ;
        if (parent->get_tag() == DW_TAG_structure_type ||
            parent->get_tag() == DW_TAG_class_type) {        // a member function?
            parentname = parent->get_name() ;
        }
        DwVirtId virtuality = get_virtuality() ;

        if (virtuality != DW_VIRTUALITY_none) {
            ctx.os.print ("virtual ") ;
        }

        DIE *returntype = DIE::get_type() ;
        if (returntype == NULL) {
            std::string name = get_name() ;
            if (name[0] != '~' && parentname != name) {          // not construtor or descructor?
                if (is_static()) {
                   ctx.os.print("static ");
                }
                ctx.os.print ("void ") ;
            }
        } else {
            if (is_static()) {
               ctx.os.print("static ");
            }
            print_declaration (ctx, returntype, 0) ;            // print the type of the subprogram
            ctx.os.print (" ") ;                            // XXX: this might not be always correct
        }
        print_name (ctx, this) ;
        ctx.os.print ("(") ;
        bool comma = false ;
        for (uint i = 0 ; i < children.size(); i++) {
            DIE *child = children[i] ;
            if (child->get_tag() == DW_TAG_formal_parameter) {
                if (child->is_artificial()) {
                    // ignore the this parameter for member functions
                } else {
                    if (comma) {
                        ctx.os.print (", ") ;
                    }
                    print_declaration (ctx, child, 0) ;
                    comma = true ;
                }
            }
        }
        ctx.os.print (")") ;
        if (virtuality == DW_VIRTUALITY_pure_virtual) {
            ctx.os.print (" = 0") ;
        }
        break ;
        }
    case DW_LANG_Fortran77:
    case DW_LANG_Fortran90: {
        std::string name = getAttribute (DW_AT_name).str ;
        int len = name.size() -1 ;
        // demangle the fortran name (will not always work!)
        if (name[len] == '_') {
           if (name[len-1] == '_') {
               name = name.substr (0, len-1) ;
           } else {
               name = name.substr (0, len) ;
           }
        }
        // if there is a variable in the subprogram body with the same name as the subprogram then
        // this is a function.  Otherwise it is a subroutine
        DIE *funcvar = NULL ;
        for (uint i = 0 ; i < children.size() ; i++) {
            DIE *child = children[i] ;
            if (child->get_tag() == DW_TAG_variable && Utils::toUpper(child->get_name()) == Utils::toUpper(name)) {
                funcvar = child ;
                break ;
            }
        }
        if (funcvar == NULL) {
            ctx.os.print ("subroutine ") ;
        } else {
            DIE *t = funcvar->get_type() ;
            if (t != NULL) {
                funcvar->get_type()->print (ctx, 0, level) ;
            }
            ctx.os.print (" function ") ;
        }
        print_name (ctx, this) ;
        // first list the parameter names
        ctx.os.print ("(") ;
        bool comma = false ;
        for (uint i = 0 ; i < children.size(); i++) {
            DIE *child = children[i] ;
            if (child->get_tag() == DW_TAG_formal_parameter) {
                if (child->is_artificial()) {
                    // ignore the this parameter for member functions
                } else {
                    if (comma) {
                        ctx.os.print (", ") ;
                    }
                    ctx.os.print ("%s", child->get_name().c_str()) ;
                    comma = true ;
                }
            }
        }
        ctx.os.print (")\n") ;
        // now list the types of the parameters
        for (uint i = 0 ; i < children.size(); i++) {
            DIE *child = children[i] ;
            if (child->get_tag() == DW_TAG_formal_parameter) {
                if (child->is_artificial()) {
                    // ignore the this parameter for member functions
                } else {
                    try {
                        child->print (ctx, indent+4, level) ;
                    } catch (Exception e) {
                        if (ctx.language == DW_LANG_Fortran90) {
                            ctx.os.print ("<unknown> :: %s", child->get_name().c_str()) ;
                        } else {
                            ctx.os.print ("<unknown> %s", child->get_name().c_str()) ;
                        }
                    }
                    ctx.os.print ("\n") ;
                }
            }
        }
        break ;
        }
    default:
        ctx.os.print ("subprogram ") ;
        print_name (ctx, this) ;
        break ;
    }
}

void Subprogram::print_value(EvalContext &context, Value &value, int indent) {
    JunkStream* js = reinterpret_cast<JunkStream*>(&context.os); 
    js->print (context, (Address)value) ;
}

Address Subprogram::get_frame_base(Process *process) {
    AttributeValue fb = getAttribute (DW_AT_frame_base) ;
    if (fb.type == AV_NONE) {
       return process->get_reg(process->arch->frame_base_reg());
    }
    DwLocExpr loc = cu->evaluate_location (cu, -1, fb, process) ;
    return loc.getValue(process, cu->getAddrSize()) ;
}

int Subprogram::get_language (){
    return cu->get_language() ;
}

bool Subprogram::is_member_function() {
    return parent != NULL && (parent->get_tag() == DW_TAG_structure_type ||
           parent->get_tag() == DW_TAG_union_type ||
           parent->get_tag() == DW_TAG_class_type) ;
}

DIE * Subprogram::find_scope(std::string name) {
    std::vector<DIE *> matches ;
    check_loaded() ;
    bool caseblind = is_case_blind() ;
    // first find all matches
    for (uint i = 0 ; i < children.size(); i++) {
        DIE *child = children[i] ;
        int childtag = child->get_tag() ;
        //println ("looking at child tag " + childtag)
        if (childtag == DW_TAG_subprogram || childtag == DW_TAG_structure_type
                || childtag == DW_TAG_union_type || childtag == DW_TAG_typedef ||
              childtag == DW_TAG_class_type) {
            std::string childname = child->getAttribute (DW_AT_name).str ;
            if (caseblind && Utils::toUpper (childname) == Utils::toUpper (name)) {
                matches.push_back (child) ;
            } else if (childname == name) {
                matches.push_back (child) ;
            }
        }
    }

    // there may be more than one die with the same name.  For example, a class name
    // and a constructor within that class.  We prefer class names over subprograms
    // because that's probably what the user wanted

    if (matches.size() == 0) {
        return parent->find_scope (name) ;
    }

    if (matches.size() == 1) {
        return matches[0] ;
    }

    // first look for a struct/union
    for (uint i = 0 ; i < matches.size() ; i++) {
        if (matches[i]->is_struct()) {
            return matches[i] ;
        }
    }

    return matches[0] ;         // just return the first match
}

void Subprogram::find_symbol (std::string name, Address pc, std::vector<DIE*> &result, DIE *caller) {
    check_loaded() ;
    bool caseblind = is_case_blind() ;

    if (!symbolsok) {
        for (uint i = 0 ; i < children.size() ; i++) {
            DIE *child = children[i] ;
            int childtag = child->get_tag() ;
            //println ("looking at child tag " + child.get_tag())
            if (childtag == DW_TAG_variable || childtag == DW_TAG_formal_parameter || childtag == DW_TAG_structure_type
                    || childtag == DW_TAG_union_type || childtag == DW_TAG_typedef || childtag == DW_TAG_enumeration_type ||
                          childtag == DW_TAG_subprogram || childtag == DW_TAG_class_type) {
                std::string childname = child->getAttribute (DW_AT_name).str ;
                if (caseblind) {
                    symbols[Utils::toUpper(childname)] = child ;
                } else {
                    symbols[childname] = child ;
                }
                if (childtag == DW_TAG_enumeration_type) {
                    enumerations.push_back (child) ;
                }
            } else if (childtag == DW_TAG_common_inclusion) {
                DIE *cb = child->getAttribute (DW_AT_common_reference) ;
                common_blocks.push_back (cb) ;
            } else if (childtag == DW_TAG_lexical_block) {
                lexical_blocks.push_back (child) ;
            } else if (childtag == DW_TAG_common_block) {
                // common block names seem to be mangled.  Store both mangled and            
                // demanged forms
                std::string childname = child->getAttribute (DW_AT_name).str ;
                const char *name = childname.c_str() ;
                const char *ch = name + childname.size() - 1;
                char *demangled = NULL ;
                // a name is possibly mangled if it ends in an underscore
                if (*ch == '_') {
                    if (ch[-1] == '_') {                // two underscores?
                        ch-- ;
                        const char *s = name ;
                        bool contains_uscore = false ;
                        while (s != ch) {
                            if (*s == '_') {
                                contains_uscore = true ;
                                break ;
                            }
                            s++ ;
                        }
                        if (contains_uscore) {
                            demangled = strndup (name, ch-name) ;
                        } else {
                            demangled = strndup (name, ch-name+1) ;
                        }
                    } else {                    // single underscore
                        demangled = strndup (name, ch-name) ;
                    }
                }
                symbols[Utils::toUpper(childname)] = child ;            // add raw child name
                if (demangled != NULL) {
                    symbols[Utils::toUpper(demangled)] = child ;            // add demangled name
                    free (demangled) ;
                }
            }

        }
        symbolsok = true ;
    }

    // search the enumerations
    for (uint i = 0 ; i < enumerations.size() ; i++) {
        DIE *die = enumerations[i] ;
        die->find_symbol (name, pc, result, this) ;
        if (!result.empty()) {
            return ;
        }
    }

    // first search the lexical blocks for the symbol.
    for (uint i = 0 ; i < lexical_blocks.size() ; i++) {
        DIE *lex = lexical_blocks[i] ;
        Address low = lex->getAttribute (DW_AT_low_pc) ;
        Address high = lex->getAttribute (DW_AT_high_pc) ;
        low += cu->get_base() ;
        high += cu->get_base() ;
        if (pc >= low && pc < high) {
            lex->find_symbol (name, pc, result, this) ;
            if (!result.empty()) {
                return ;
            }
        }
    }

    DIEMap::iterator symi = symbols.find (caseblind?Utils::toUpper(name):name) ;
    if (symi != symbols.end()) {
        result.push_back (symi->second) ;
        return ;
    }

    bool has_specification = false ;
    if (more_info == NULL) {
        AttributeValue specatt = getAttribute (DW_AT_specification, false) ;
        if (specatt.type == AV_DIE) {
            has_specification = true ;
            DIE *spec = specatt ;
            if (spec != caller) {
                spec->find_symbol (name, pc, result, this) ;
                if (!result.empty()) {
                    return ;
                }
            }
        }
    }

    if (more_info == NULL) {
        AttributeValue specatt = getAttribute (DW_AT_abstract_origin, false) ;
        if (specatt.type == AV_DIE) {
            has_specification = true ;
            DIE *spec = specatt ;
            if (spec != caller) {
                spec->find_symbol (name, pc, result, this) ;
                if (!result.empty()) {
                    return ;
                }
            }
        }
    }

    // look in common blocks
    for (uint i = 0 ; i < common_blocks.size() ; i++) {
        common_blocks[i]->find_symbol (name, pc, result, this) ;
        if (!result.empty()) {
            return ;
        }
    }

    // if there is more infomation in another DIE, look there
    if (more_info != NULL && more_info != caller) {
        more_info->find_symbol (name, pc, result, this) ;
        if (!result.empty()) {
            return ;
        }
    }

    // if the subprogram has a DW_AT_containing_type attribute, look there.  This is for
    // C++ class scope and fortran subroutines
    AttributeValue typeatt = getAttribute (DW_AT_containing_type) ;
    if (typeatt.type == AV_DIE) {
        DIE *t = typeatt ;
        if (t != caller) {
            t->find_symbol (name, pc, result, this) ;
            if (!result.empty()) {
                return ;
            }
        }
    }


    
    // gcc has a bug that puts some symbols in the wrong lexical block.  To work around this,
    // lets search all lexical blocks, ignoring the pc value
    
    for (uint i = 0 ; i < lexical_blocks.size() ; i++) {
        DIE *lex = lexical_blocks[i] ;
        lex->find_symbol (name, 0, result, this) ;
        if (!result.empty()) {
            return ;
        }
    }


    // not a child, look in the parent, but only if there is no DW_AT_specification attribute.  
    // A specification attribute signifies that the DIE is not in the correct location for
    // a parent search
    if (!has_specification) {
        parent->find_symbol (name, pc, result, this) ;
    }
}

void Subprogram::get_formal_parameters (std::vector<DIE*> &vec) {
    if (more_info != NULL) {
        Subprogram *sub = dynamic_cast<Subprogram*>(more_info) ;
        more_info->check_loaded() ;
        if (sub != NULL) {
            sub->get_formal_parameters (vec) ;
            return ;
        }
    }
    for (uint i = 0 ; i < children.size() ; i++) {
        DIE *child = children[i] ;
        int childtag = child->get_tag() ;
        //println ("looking at child tag " + child.get_tag())
        if (childtag == DW_TAG_formal_parameter) {
            vec.push_back (child) ;
        }
    }
}

bool Subprogram::is_variadic() {
    for (uint i = 0 ; i < children.size() ; i++) {
        DIE *child = children[i] ;
        int childtag = child->get_tag() ;
        //println ("looking at child tag " + child.get_tag())
        if (childtag == DW_TAG_unspecified_parameters) {
            return true ;
        }
    }
    return false ;
}

void Subprogram::get_local_variables (std::vector<DIE*> &vec) {
    for (uint i = 0 ; i < children.size() ; i++) {
        DIE *child = children[i] ;
        int childtag = child->get_tag() ;
        //println ("looking at child tag " + child.get_tag())
        if (childtag == DW_TAG_variable) {
            vec.push_back (child) ;
        } else if (childtag == DW_TAG_lexical_block) {
            Lexical_block *block = dynamic_cast<Lexical_block*>(child) ;
            block->get_local_variables (vec) ;
        }
    }
}

Address Subprogram::get_address(EvalContext &context, Address stack_top) {
    AttributeValue v ;

    // if virtual function, check virtual table
    int virtuality = getAttribute (DW_AT_virtuality) ;
    if (virtuality != DW_VIRTUALITY_none) {
        v = getAttribute (DW_AT_vtable_elem_location) ;
        if (v.type == AV_NONE) {
            return 0 ;
        }
        DwLocExpr addr = cu->evaluate_location (cu, context.fb, v, context.process, stack_top);
        return addr.getAddress() ;
    }

    // if has low_pc value, just use that
    v = getAttribute (DW_AT_low_pc) ;
    if (v.type != AV_NONE) {
        return (Address)v + cu->get_base() ;
    }

    // probably member of a class, try linkage name
    v = getAttribute (DW_AT_MIPS_linkage_name) ;
    if (v.type != AV_NONE) {
        Address addr = context.process->lookup_symbol(v.str) ;
        if (addr != 0) {
            return addr + cu->get_base() ;
        }
    }

    // darn, no joy
    return 0;
}

DwVirtId Subprogram::get_virtuality() {
    int v = getAttribute (DW_AT_virtuality) ;
    return (DwVirtId)v ;
}

Value Subprogram::evaluate(EvalContext &context) {
    return get_address(context) ;
}

Value Subprogram::evaluate(EvalContext &context, Value &thisptr) {
    return get_address(context, thisptr) ;
}


Address Subprogram::get_virtual_table (EvalContext &context, Address thisptr) {
    DIE *cls = getAttribute (DW_AT_containing_type) ;
    if (cls == NULL) {
        throw Exception ("Cannot determine class containing this function") ;
    }
    return cls->get_virtual_table (context, thisptr) ;
}

Template_type_param::Template_type_param (DwCUnit *cu, DIE *parent, Abbreviation *abbrev)
    : DIE(cu, parent, abbrev) {
}

Template_type_param::~Template_type_param() {
}

void Template_type_param::print(EvalContext &ctx, int indent, int level) {
        doindent (ctx, indent) ;
}

Template_value_param::Template_value_param (DwCUnit *cu, DIE *parent, Abbreviation *abbrev)
    : DIE(cu, parent, abbrev) {
}

Template_value_param::~Template_value_param() {
}

void Template_value_param::print(EvalContext &ctx, int indent, int level) {
        doindent (ctx, indent) ;
}


Variable::Variable (DwCUnit *cu, DIE *parent, Abbreviation *abbrev)
    : DIE(cu, parent, abbrev) {
}

Variable::~Variable() {
}

void Variable::print(EvalContext &ctx, int indent, int level) {
        doindent (ctx, indent) ;
        switch (ctx.language) {
        case DW_LANG_C:
        case DW_LANG_C89:
        case DW_LANG_C_plus_plus:
            if (parent != NULL &&
                parent->get_tag() == DW_TAG_structure_type ||
                parent->get_tag() == DW_TAG_class_type ||
                parent->get_tag() == DW_TAG_union_type) {
               ctx.os.print("static ");
            }
            print_declaration (ctx, this, 0) ;
            break ;
        case DW_LANG_Fortran77:
            get_type()->print (ctx, 0, level+1) ;
            ctx.os.print (" %s", get_name().c_str()) ;
            break ;
        case DW_LANG_Fortran90: 
            get_type()->print (ctx, 0, level+1) ;
            ctx.os.print (" :: %s", get_name().c_str()) ;
            break ;
        default:
            ctx.os.print ("variable %s", get_name().c_str()) ;
            break ;
        }
}

Value Variable::evaluate(EvalContext &context) {
    DIE* type = get_type() ;

    AttributeValue cval = getAttribute(DW_AT_const_value) ;
    if (cval.type != AV_NONE) {
        // XXX: just ignores addressonly, no idea what it should do
        return cval.toValue(type);
    }

    AttributeValue loc = getAttribute (DW_AT_location) ;
    if (loc.type != AV_NONE) {
        DwLocExpr addr = cu->evaluate_location (cu, context.fb, loc, context.process) ;
        if (type->is_struct() || type->is_array() || type->is_string() || type->is_complex()) {
            if (type->get_tag() == DW_TAG_union_type) {
                dynamic_cast<TypeUnion*>(type)->add_locations() ;
            }
            return addr.getAddress();
        } else {
            if (context.addressonly) {
                return addr.getAddress() ;
            }
            return addr.getValue (context.process,
               type->get_size_immed(), type->is_signed()) ;
        }
    }

    throw Exception ("Unable to get value of variable %s - perhaps it is optimized out", get_name().c_str()) ;
}

// is the variable local?  The only way to tell this is to look at the location and see if
// it is a DW_OP_fbreg.  The variable must also be inside a subprogram

bool Variable::is_local_var() {
    if (parent == NULL) {
       return false ;
    }
    if (parent->get_tag() != DW_TAG_subprogram) {
        return false ;
    }
    AttributeValue locatt = getAttribute (DW_AT_location) ;
    if (locatt.type != AV_BLOCK) {
        return false ;
    }
    BVector loc = locatt ;
    if (loc[0] == DW_OP_fbreg) {
        return true ;
    }
    return false ;
}




LineInfo::LineInfo (DwCUnit *cu, Address address, int file, int lineno, int column, bool is_stmt, bool basic_block)
    : cu(cu),
    address(address),
    file(file),
    lineno(lineno),
    column(column),
    is_stmt(is_stmt),
    basic_block(basic_block) {
}

LineInfo::~LineInfo() {
}

void LineInfo::print() {
   FileTable & files = cu->get_file_table() ;
   printf ("0x%-20llx %-30s %-6d %-6d %d %d\n", (unsigned long long) address,
	   files[file]->name.c_str(), lineno, column, is_stmt, basic_block) ;
}

