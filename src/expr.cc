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

file: expr.cc
created on: Fri Aug 13 11:07:39 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#include "expr.h"
#include "type_array.h"
#include "type_struct.h"
#include <ctype.h>
#include "symtab.h"
#include "process.h"
#include <math.h>
#include <stdint.h>
#include "pstream.h"
#include "arch.h"
#include "dbg_stl.h"
#include "cli.h"

static void checktypes (const Value &v1, const Value &v2) {
    if (v1.type == VALUE_INTEGER && v2.type == VALUE_BOOL ||
                v1.type == VALUE_BOOL && v2.type == VALUE_INTEGER) {
        return ;
    }

    if (v1.type != v2.type) {
        throw Exception ("Invalid type combination") ;
    }
}

static bool isint (const Value &v) {
    return v.type == VALUE_INTEGER || v.type == VALUE_BOOL ;
}

// Value object operators
Value operator+ (const Value &v1, const Value &v2) {
    checktypes (v1, v2) ;
    if (isint(v1)) {
        return v1.integer + v2.integer ;
    } else if (v1.type == VALUE_REAL) {
        return v1.real + v2.real ;
    } else if (v1.type == VALUE_VECTOR) {
        throw Exception ("Invalid vector operation") ;
    } else {
        return v1.str + v2.str ;
    }
}

Value operator- (const Value &v1, const Value &v2) {
    checktypes (v1, v2) ;
    if (isint(v1)) {
        return v1.integer - v2.integer ;
    } else if (v1.type == VALUE_REAL) {
        return v1.real - v2.real ;
    } else if (v1.type == VALUE_VECTOR) {
        throw Exception ("Invalid vector operation") ;
    } else {
        throw Exception ("Invalid string operation") ;
    }
}

Value operator* (const Value &v1, const Value &v2) {
    checktypes (v1, v2) ;
    if (isint(v1)) {
        return v1.integer * v2.integer ;
    } else if (v1.type == VALUE_REAL) {
        return v1.real * v2.real ;
    } else if (v1.type == VALUE_VECTOR) {
        throw Exception ("Invalid vector operation") ;
    } else {
        throw Exception ("Invalid string operation") ;
    }
}

Value operator/ (const Value &v1, const Value &v2) {
    checktypes (v1, v2) ;
    if (isint(v1)) {
        return v1.integer / v2.integer ;
    } else if (v1.type == VALUE_REAL) {
        return v1.real / v2.real ;
    } else if (v1.type == VALUE_VECTOR) {
        throw Exception ("Invalid vector operation") ;
    } else {
        throw Exception ("Invalid string operation") ;
    }
}

Value operator% (const Value &v1, const Value &v2) {
    checktypes (v1, v2) ;
    if (isint(v1)) {
        return v1.integer % v2.integer ;
    } else if (v1.type == VALUE_REAL) {
        throw Exception ("Invalid floating point operation") ;
    } else if (v1.type == VALUE_VECTOR) {
        throw Exception ("Invalid vector operation") ;
    } else {
        throw Exception ("Invalid string operation") ;
    }
}

Value operator< (const Value &v1, const Value &v2) {
    checktypes (v1, v2) ;
    if (isint(v1)) {
        return v1.integer < v2.integer ;
    } else if (v1.type == VALUE_REAL) {
        return v1.real < v2.real ;
    } else if (v1.type == VALUE_VECTOR) {
        throw Exception ("Invalid vector operation") ;
    } else {
        return v1.str < v2.str ;
    }
}

Value operator> (const Value &v1, const Value &v2) {
    checktypes (v1, v2) ;
    if (isint(v1)) {
        return v1.integer > v2.integer ;
    } else if (v1.type == VALUE_REAL) {
        return v1.real > v2.real ;
    } else if (v1.type == VALUE_VECTOR) {
        throw Exception ("Invalid vector operation") ;
    } else {
        return v1.str > v2.str ;
    }
}

Value operator<= (const Value &v1, const Value &v2) {
    checktypes (v1, v2) ;
    return !(v1 > v2) ;
}

Value operator>= (const Value &v1, const Value &v2) {
    checktypes (v1, v2) ;
    return !(v1 < v2) ;
}

Value operator== (const Value &v1, const Value &v2) {
    checktypes (v1, v2) ;
    if (v1.type == VALUE_INTEGER) {
        return v1.integer == v2.integer ;
    } else if (v1.type == VALUE_REAL) {
        return v1.real == v2.real ;
    } else if (v1.type == VALUE_BOOL) {
        return v1.integer == v2.integer ;
    } else if (v1.type == VALUE_VECTOR) {
        throw Exception ("Invalid vector operation") ;
    } else {
        return v1.str == v2.str ;
    }
}

bool operator== (const Value &v1, int v) {
    if (v1.type == VALUE_INTEGER) {
        return v1.integer == v ;
    } else if (v1.type == VALUE_REAL) {
        return v1.real == v ;
    } else if (v1.type == VALUE_BOOL) {
        return v1.integer == v ;
    } else if (v1.type == VALUE_VECTOR) {
        throw Exception ("Invalid vector operation") ;
    } else {
        throw Exception ("Invalid string operation") ;
    }
}

bool operator!= (const Value &v1, int v) {
    return !(v1 == v) ;
}

Value operator!= (const Value &v1, const Value &v2) {
    checktypes (v1, v2) ;
    return !(v1 == v2) ;
}

Value operator<< (const Value &v1, const Value &v2) {
    if (!isint(v1) || !isint(v2)) {
        throw Exception ("Invalid shift types") ;
    }
    return v1.integer << v2.integer ;
}

Value operator>> (const Value &v1, const Value &v2) {
    if (!isint(v1) || !isint(v2)) {
        throw Exception ("Invalid shift types") ;
    }
    return v1.integer >> v2.integer ;
}

Value operator^ (const Value &v1, const Value &v2) {
    if (!isint(v1) || !isint(v2)) {
        throw Exception ("Invalid bit operation types") ;
    }
    return v1.integer ^ v2.integer ;
}

Value operator& (const Value &v1, const Value &v2) {
    if (!isint(v1) || !isint(v2)) {
        throw Exception ("Invalid bit operation types") ;
    }
    return v1.integer & v2.integer ;
}

Value operator| (const Value &v1, const Value &v2) {
    if (!isint(v1) || !isint(v2)) {
        throw Exception ("Invalid bit operation types") ;
    }
    return v1.integer | v2.integer ;
}

Value operator- (const Value &v1) {
    if (isint(v1)) {
        return -v1.integer ;
    } else if (v1.type == VALUE_REAL) {
        return -v1.real ;
    } else if (v1.type == VALUE_VECTOR) {
        throw Exception ("Invalid vector operation") ;
    } else {
        throw Exception ("Invalid string operation") ;
    }
}

Value operator~ (const Value &v1) {
    if (isint(v1)) {
        return ~v1.integer ;
    } else if (v1.type == VALUE_REAL) {
        throw Exception ("Invalid floating point operation") ;
    } else if (v1.type == VALUE_VECTOR) {
        throw Exception ("Invalid vector operation") ;
    } else {
        throw Exception ("Invalid string operation") ;
    }
}

Value operator! (const Value &v1) {
    if (isint(v1)) {
        return !v1.integer ;
    } else if (v1.type == VALUE_REAL) {
        return !v1.real ;
    } else if (v1.type == VALUE_VECTOR) {
        throw Exception ("Invalid vector operation") ;
    } else {
        throw Exception ("Invalid string operation") ;
    }
}


std::ostream &operator<< (std::ostream &os, const Value &v) {
    switch (v.type) {
    case VALUE_NONE:
        os << "none" ;
        break ;
    case VALUE_INTEGER:
        os << v.integer ;
        break ;
    case VALUE_REAL:
        os << v.real ;
        break ;
    case VALUE_STRING:
        os << "\"" << v.str << "\"" ;
        break ;
    case VALUE_REG:
        break ;
    case VALUE_BOOL:
        if (v.integer) {
            os << "on" ;
        } else {
            os << "off" ;
        }
        break ;
    default:
        os << "unknown" ;
    }
    return os ;
}

EvalContext::EvalContext (Process *process, Address fb, int language, PStream &os)
    : addressonly(false),
      ignore_expr(false),
      language(language),
      os(os),
      process(process),
      fb(fb),
      show_contents(true),
      show_reference(true),
      truncate_aggregates(false),
      pretty(true),
      trunc_digs(false),
      num_sigdigs(6)
{
}

EvalContext::~EvalContext() {
}

Node::Node (SymbolTable *symtab)
  : type(NULL),
    symtab(symtab)
{
}

Node::~Node() {
}

DIE *Node::get_type() {
        return type ;
}

ConstantNode::ConstantNode(SymbolTable *symtab) 
    : Node(symtab) {

}

ConstantNode::~ConstantNode() {
}



IntConstant::IntConstant (SymbolTable *symtab, int64_t v, int sz)
    : ConstantNode (symtab), v(v), size(sz) {
    switch (size) {
    case 1: type = symtab->new_scalar_type ("bool", DW_ATE_boolean, sz) ; break ;
    case 2: type = symtab->new_scalar_type ("short", DW_ATE_signed, sz) ; break ;
    case 4: type = symtab->new_scalar_type ("int", DW_ATE_signed, sz) ; break ;
    case 8: type = symtab->new_scalar_type ("long", DW_ATE_signed, sz) ; break ;
    default: type = symtab->new_scalar_type ("int", DW_ATE_signed, sz) ; break ;
    }
}

IntConstant::~IntConstant() {
}

Value IntConstant::evaluate(EvalContext &context) {
    return v ;
}

StringConstant::StringConstant (SymbolTable *symtab, std::string v)
    : ConstantNode (symtab), v(v) {
    type = symtab->new_string_type(v.size()+1) ;
}

StringConstant::~StringConstant() {
}

Value StringConstant::evaluate(EvalContext &context) {
    return v ;
}

CharConstant::CharConstant (SymbolTable *symtab, char ch)
    : ConstantNode (symtab), ch(ch) {
    type = symtab->new_scalar_type ("char", DW_ATE_signed_char, 1) ;
}

CharConstant::~CharConstant() {
}

Value CharConstant::evaluate(EvalContext &context) {
    return (int)ch ;
}


RealConstant::RealConstant (SymbolTable *symtab, double r, int size)
    : ConstantNode (symtab), r(r) {
    type = symtab->new_scalar_type ("double", DW_ATE_float, size) ;
}

RealConstant::~RealConstant() {
}

Value RealConstant::evaluate(EvalContext &context) {
    return r ;
}

RegisterExpression::RegisterExpression(SymbolTable *symtab, std::string name)
    : Node(symtab), regname(name)
{
	// FIXME? Floating point registers?
	RegisterType regtype = symtab->arch->type_of_register(name);
	int t;
	switch (regtype)
	{
		case RT_INTEGRAL:
			t = DW_ATE_signed;
			break;
		case RT_ADDRESS:
			t = DW_ATE_address;
			break;
		default:
			assert(false && "Unreachable!");
	}
	type = symtab->new_scalar_type("register", t, symtab->arch->size_of_register(name));
}

RegisterExpression::~RegisterExpression() {}

Value RegisterExpression::evaluate(EvalContext &context) {
    return context.process->get_reg (regname) ;
}

void RegisterExpression::set_value (EvalContext &context, Value &value, DIE *exprtype) {
    context.process->set_reg (regname, value) ;         // XXX: check value is correct type
}


DebuggerVarExpression::DebuggerVarExpression (SymbolTable *symtab, std::string name, DebuggerVar *var) 
    : Node(symtab),
      var(var),
      name(name)
{
    if (var != NULL) {
        type = var->type ;
    } else {
        type = symtab->new_int() ;
    }
    symtab->keep_temp_die (type);
}

DebuggerVarExpression::~DebuggerVarExpression() {
}

Value DebuggerVarExpression::evaluate(EvalContext &context) {
    if (var == NULL) {
       if (name[0] == '$' && isdigit(name[1])) {
           throw Exception ("Undefined value-history variable %s", name.c_str()) ;
       } else if (name == "$bpnum") {
           return context.process->get_cli()->get_last_breakpoint() ;
       } else {
           type = NULL ;                        // void
           return 0 ;
           //throw Exception ("Undefined debugger variable $%s", name.c_str()) ;
       }
    }
    type = var->type ;
    return var->value ;
}

void DebuggerVarExpression::set_value (EvalContext &context, Value &value, DIE *exprtype) {
    symtab->keep_temp_die (exprtype) ;
    type = exprtype ;
    if (var == NULL) {
        var = context.process->add_debugger_variable (name, value, exprtype) ;
    } else {
        var->value = value ;
        var->type = exprtype ;
    }
}


Identifier::Identifier (SymbolTable *symtab, DIE *sym)
    : Node(symtab), sym(sym) {
    if (sym->get_tag() == DW_TAG_subprogram ||
        sym->get_tag() == DW_TAG_structure_type ||
        sym->get_tag() == DW_TAG_union_type ||
        sym->get_tag() == DW_TAG_enumeration_type ||
        sym->get_tag() == DW_TAG_class_type) {
        type = sym ;
    } else {
        type = sym->get_type() ;                    // useful to have a static type for fortran
    }
}

Identifier::~Identifier() {
}

Value Identifier::evaluate(EvalContext &context) {
    sym->check_loaded() ;
    DIE *symtype = sym ;
    Value v;   

    // find the type of the symbol to be evaluated 
    if (sym->get_tag() == DW_TAG_member || sym->get_tag() == DW_TAG_variable || sym->get_tag() == DW_TAG_formal_parameter) {
        symtype = symtype->get_type() ;
    }

    // if reference type make a new EvalContext that doesn't insist on addressonly
    if (symtype->get_tag() == DW_TAG_reference_type) {
        EvalContext dupl = context ;
        dupl.addressonly = false ;
        v = sym->evaluate (dupl) ;
    } else {
        v = sym->evaluate (context) ;
    }

    if (symtype->is_struct_deref()) {
        DIE *t = symtype ;

        while (t->get_tag() != DW_TAG_structure_type &&
               t->get_tag() != DW_TAG_union_type &&
               t->get_tag() != DW_TAG_class_type) {
            t = t->get_type() ;
        }
        type = t ;

        // get the dynamic type of the struct
        type = dynamic_cast<TypeStruct*>(type)->get_dynamic_type (context, v) ;
        return v ;
    }

    // for a subprogram, the DW_AT_type attribute is the return type.  We don't want to
    // dereference this until later.
    if (sym->get_tag() == DW_TAG_subprogram) {
        type = sym ;
    } else {
        type = symtype ;
    }
    if (type == NULL) {
        type = sym ;
    }
    if (context.addressonly) {
        return v ;
    }

    // reals will come back as integers with the correct bit pattern.
    // We need to convert these to doubles

    if (type->is_real()) {
        if (type->get_size() == 4) {                    // float?
            v.real = (double)(*(float*)&v.real) ;
        }
        v.type = VALUE_REAL ;
    }
    return v ;
}

void Identifier::set_value (EvalContext &context, Value &value, DIE *exprtype) {
    bool old = context.addressonly ;
    context.addressonly = true ;
    Value addr = evaluate (context) ;           // get the address
    context.addressonly = old ;

    type->set_value (context, addr, value) ;
}

bool Identifier::is_local() {
    return sym->is_local_var() ;
}

// a set of identifiers
IdentifierSet::IdentifierSet (SymbolTable *symtab, std::vector<DIE*> &syms)
    : Node(symtab), syms(syms) {
    if (syms[0]->get_tag() == DW_TAG_subprogram) {
        type = syms[0] ;
    } else {
        type = syms[0]->get_type() ;                    // useful to have a static type for fortran
    }
}

IdentifierSet::~IdentifierSet() {
}

Value IdentifierSet::evaluate(EvalContext &context) {
    // the syms vector may contain both variables and structure tags.  For the evaluation
    // we are only interested in variables
    std::vector<DIE*> vars ;
    for (uint i = 0 ; i < syms.size() ; i++) {
        DIE *sym = syms[i] ;
        if (sym->get_tag() == DW_TAG_variable || sym->get_tag() == DW_TAG_formal_parameter) {
            vars.push_back (sym) ;
        }
    }
    if (vars.size() > 1) {
        throw Exception ("Ambiguous expression evaluation") ;
    } 
    if (vars.size() == 0) {
        // if we get here, look for structures
        for (uint i = 0 ; i < syms.size() ; i++) {
            DIE *sym = syms[i] ;
            if (sym->get_tag() == DW_TAG_structure_type ||
                sym->get_tag() == DW_TAG_union_type ||
                sym->get_tag() == DW_TAG_class_type) {
                vars.push_back (sym) ;
            }
        }
        if (vars.size() > 1) {
            throw Exception ("Ambiguous expression evaluation") ;
        } 
    }

    if (vars.size() == 0) {
        // if we get here, look for subprograms
        for (uint i = 0 ; i < syms.size() ; i++) {
            DIE *sym = syms[i] ;
            if (sym->get_tag() == DW_TAG_subprogram) {
                vars.push_back (sym) ;
            }
        }
        if (vars.size() > 1) {
            throw Exception ("Ambiguous expression evaluation") ;
        } 
    }

    if (vars.size() == 0) {
        throw Exception ("Can't evaluate this expression type") ;
    }

    if (vars[0]->get_tag() == DW_TAG_member || vars[0]->get_tag() == DW_TAG_formal_parameter) {   
        type = vars[0]->get_type() ;
    } else {
        type = vars[0] ;
    }
    return vars[0]->evaluate (context) ;
    
}

void IdentifierSet::set_value (EvalContext &context, Value &value, DIE *exprtype) {
    // the syms vector may contain both variables and structure tags.  For the evaluation
    // we are only interested in variables
    std::vector<DIE*> vars ;
    for (uint i = 0 ; i < syms.size() ; i++) {
        DIE *sym = syms[i] ;
        if (sym->get_tag() == DW_TAG_variable || sym->get_tag() == DW_TAG_formal_parameter) {
            vars.push_back (sym) ;
        }
    }
    if (vars.size() > 1) {
        throw Exception ("Ambiguous expression evaluation") ;
    } 

    if (vars.size() == 0) {
        // if we get here, look for structures
        for (uint i = 0 ; i < syms.size() ; i++) {
            DIE *sym = syms[i] ;
            if (sym->get_tag() == DW_TAG_structure_type ||
                sym->get_tag() == DW_TAG_union_type ||
                sym->get_tag() == DW_TAG_class_type) {
                vars.push_back (sym) ;
            }
        }
        if (vars.size() > 1) {
            throw Exception ("Ambiguous expression evaluation") ;
        } 
    }

    if (vars.size() == 0) {
        // if we get here, look for subprograms
        for (uint i = 0 ; i < syms.size() ; i++) {
            DIE *sym = syms[i] ;
            if (sym->get_tag() == DW_TAG_subprogram) {
                vars.push_back (sym) ;
            }
        }
        if (vars.size() > 1) {
            throw Exception ("Ambiguous expression evaluation") ;
        } 
    }

    if (vars.size() == 0) {
        throw Exception ("Can't evaluate this expression type") ;
    }
    bool old = context.addressonly ;
    context.addressonly = true ;
    Value addr = vars[0]->evaluate (context) ;           // get the address
    context.addressonly = old ;

    type = exprtype ;
    exprtype->set_value (context, addr, value) ;
}

bool IdentifierSet::is_local() {
    for (uint i = 0 ; i < syms.size() ; i++) {
        if (syms[i]->is_local_var()) {
            return true ;
        }
    }
    return false ;
}

IntrinsicIdentifier::IntrinsicIdentifier (SymbolTable *symtab, std::string name, Intrinsic it)
    : Node(symtab), name(name), it(it) {

}

IntrinsicIdentifier::~IntrinsicIdentifier() {
}

MemberExpression::MemberExpression (SymbolTable *symtab, Node *l, std::string name)
    : Node(symtab), left(l), membername(name) {

	type = left->get_type() ;
	while (type && (type->get_tag() == DW_TAG_const_type || type->get_tag() == DW_TAG_reference_type || type->get_tag() == DW_TAG_formal_parameter)) {
		type = type->get_type() ;
	}
}

MemberExpression::~MemberExpression() {
    delete left ;
}

bool MemberExpression::is_local() {
    return left->is_local() ;
}

int MemberExpression::num_variables() {
    return left->num_variables() ;
}

// for calling functions, we need to get the types of all the subprograms named
// by the member name
void MemberExpression::resolve (EvalContext &context, std::vector<DIE*> &result) {
    Value leftvalue = left->evaluate (context) ;                // address of aggregate
    if (!left->get_type()->is_struct()) {
        throw Exception ("struct/union/class type expected") ;
    }
    type = left->get_type() ;
    // at this point, type points to the Structure or Union DIE

    type->check_loaded() ;
    type->find_member (membername, result) ;
}


void MemberExpression::set_value (EvalContext &context, Value &value, DIE *exprtype) {
    Value leftvalue = left->evaluate (context) ;                // address of aggregate

    type = left->get_type() ;

    // if the value is in a register then we need to put it in memory in order for
    // all the inferior routines to work.   It is copied to the stack and
    // the stack address passed downwards as the address of the struct.  Then is
    // is copied back again
    Address oldsp = context.process->get_reg ("sp") ;
    Address sp = oldsp ;
    bool in_reg = false ;
    int regnum = 0 ;

    if (leftvalue.type == VALUE_REG) {          // in a register?
        in_reg = true ;
        regnum = leftvalue.integer ;
        Address v = context.process->get_reg (leftvalue.integer) ;              // contents
        sp = context.process->get_arch()->stack_space (context.process, type->get_real_size(context)) ;
        context.process->write (sp, v, type->get_real_size(context)) ;
        leftvalue.type = VALUE_INTEGER ;
        leftvalue.integer = sp ;
    }

    if (!left->get_type()->is_struct()) {
        throw Exception ("struct/union/class type expected") ;
    }
    // at this point, type points to the Structure or Union DIE

    type->check_loaded() ;
    DIE *sym = NULL ;
    std::vector<DIE*> member_sequence ;

    std::string mname = membername ;            // find_member may overwrite
    // see the evaluate function for the details of the following code
    for (;;) {
        try {
            sym = type->find_member (mname) ;
            break ;                             // no exception means we found the member directly
        } catch (DIE *die) {                        // anonymous union member or inheritance detected
            member_sequence.push_back (die) ;
            type = die->get_type() ;
            if (die->get_tag() == DW_TAG_member) {
                sym = type->find_member (mname) ;           // should be found in union
                break ;
            }
        }
    }
    if (sym == NULL) {
        throw Exception ("There is no member named %s", membername.c_str()) ;
    }

    // member_sequence contains a set of DIEs that need to be evaluated in sequence
    // to get to the member referenced

    Value current = leftvalue ;               // start with the address of the struct
    for (uint i = 0 ; i < member_sequence.size() ; i++) {
        Value v = member_sequence[i]->evaluate (context, current) ;
        current = v ;
    }
 
    // now current contains the address of the final struct containing sym
    bool old = context.addressonly ;
    context.addressonly = true ;
    Value mem = sym->evaluate (context, current) ;
    context.addressonly = old ;

    // we don't want to dereference functions here
    if (sym->is_function()) {
        type = sym ;
    } else {
        type = sym->get_type() ;
    }

    AttributeValue bs = sym->getAttribute(DW_AT_bit_size) ;             // check for bitfield
    if (bs.type != AV_NONE) {
        int bitsize = bs ;
        if (bitsize == 32) {
            value.integer &= 0xffffffffULL ;
            sym->get_type()->set_value (context, mem, value) ;
        } else {
            int bitoffset = sym->getAttribute (DW_AT_bit_offset) ;
            int oldval = context.process->read (mem, 4) ;               // read the whole word
            int newval = value.integer ;
            newval <<= 32 - bitsize ;
            newval >>= bitoffset ;              // might be sign extended - need to mask

            // make a mask and move to correct position
            unsigned int mask = (1 << bitsize) - 1 ;
            mask <<= 32 - bitsize ;
            mask >>= bitoffset ;
            oldval &= ~mask ;
            newval &= mask ;
            value.integer = newval | oldval ;
            sym->get_type()->set_value (context, mem, value) ;
        }
    }  else {
        sym->get_type()->set_value (context, mem, value) ;
    }

    // if the value was in a register, copy it from the temporary stack location
    // to the register and deallocate the stack storage
    if (in_reg) {
        Address v = context.process->read (sp, type->get_real_size(context)) ;
        context.process->set_reg (regnum, v) ;
        context.process->set_reg ("sp", oldsp) ;
    }
}

Node *MemberExpression::get_this_pointer() {
    if (left->get_opcode() == CONTENTS) {       
        return dynamic_cast<Expression*>(left)->get_left() ;
    }

    return new Expression (symtab, ADDRESS, left) ;
}

Value MemberExpression::evaluate(EvalContext &context) {
    bool save = context.addressonly ;
    context.addressonly = false ;                       // we want the value of the left, not the address
    Value leftvalue = left->evaluate (context) ;                // address of aggregate
    context.addressonly = save ;

    if (!type->is_struct()) {
        throw Exception ("struct/union/class type expected") ;
    }
    // at this point, type points to the Structure or Union DIE

    // copy to memory if it is in a register
    Address oldsp = context.process->get_reg ("sp") ;
    Address sp = oldsp ;

    if (leftvalue.type == VALUE_REG) {          // in a register?
        Address v = context.process->get_reg (leftvalue.integer) ;              // contents
        sp = context.process->get_arch()->stack_space (context.process, type->get_real_size(context)) ;
        context.process->write (sp, v, type->get_real_size(context)) ;
        leftvalue.type = VALUE_INTEGER ;
        leftvalue.integer = sp ;
    }


    type->check_loaded() ;
    DIE *sym = NULL ;
    std::vector<DIE*> member_sequence ;

    //
    // this part deserves some explanation.  When looking for a member of an aggregate
    // we want a DIE *.  This may be located in the struct itself, in an anonymous
    // union within the struct, or in a base class of the struct.  For the case where
    // we need to go through an anonymous union or base class, we need to insert an
    // extra MEMBER expression node on the left because the union or base class will have
    // a location attribute that specifies where it is.  This is achieved by throwing
    // an exception containing the DIE (the DW_TAG_union_type or DW_TAG_inheritance DIE)
    //
    std::string mname = membername ;            // copy because find_member can overwrite

    for (;;) {
        try {
            sym = type->find_member (mname) ;
            break ;                             // no exception means we found the member directly
        } catch (DIE *die) {                        // anonymous union member or inheritance detected
            // the die is  member of the struct with no name for an anonymous union
            // of the DW_TAG_inheritance for a base class
            member_sequence.push_back (die) ;
            // look in the dereferenced DIE thrown for the symbol.  In the case of an
            // anonymous union it will definitely be there, but for a base class it may
            // result in another exception (if the name is in a base of the base).  Come to think
            // if it, the anoynmous union may contain another anonymous union so that might happen
            // there too.
            type = die->get_type() ;
            if (die->get_tag() == DW_TAG_member) {
                sym = type->find_member (mname) ;           // should be found in union
                break ;
            }
        }
    }
    if (sym == NULL) {
        throw Exception ("There is no member named %s", membername.c_str()) ;
    }

    // member_sequence contains a set of DIEs that need to be evaluated in sequence
    // to get to the member referenced

    Value current = leftvalue ;               // start with the address of the struct
    for (uint i = 0 ; i < member_sequence.size() ; i++) {
        Value v = member_sequence[i]->evaluate (context, current) ;
        current = v ;
    }
 
    // we don't want to dereference functions here
    if (sym->is_function()) {
        type = sym ;
    } else {
        type = sym->get_type() ;
    }

    // now current contains the address of the final struct containing sym
    Value mem = sym->evaluate (context, current) ;
    if (sym->is_function()) {
         Subprogram *subprog = dynamic_cast<Subprogram*>(sym) ;
         DwVirtId virt = subprog->get_virtuality() ;
         if (virt != DW_VIRTUALITY_none) {
            Address vtbl = subprog->get_virtual_table (context, current) ;              // address of virtual table
            if (vtbl == 0) {
                throw Exception ("Unable to determine address of virtual table") ;
            }
            int index = mem.integer * symtab->arch->ptrsize() ;                        // index into table 
            mem = context.process->readptr (vtbl + index) ;            // now read the actual address
         }
    }

    if (type->is_real()) {
        if (type->get_real_size(context) == 4) {
           mem.real = (double)(*(float*)&mem.real) ;
        }
        mem.type = VALUE_REAL ;
    }

    // restore stack pointer
    if (sp != oldsp) {
        context.process->set_reg ("sp", oldsp) ;
    }
    return mem ;
}

Value MemberExpression::evaluate(EvalContext &context, DIE *member) {
    bool save = context.addressonly ;
    context.addressonly = false ;                       // we want the value of the left, not the address
    Value leftvalue = left->evaluate (context) ;                // address of aggregate
    context.addressonly = save ;

    if (!left->get_type()->is_struct()) {
        throw Exception ("struct/union/class type expected") ;
    }
    type = left->get_type() ;
    // at this point, type points to the Structure or Union DIE

    // copy to memory if it is in a register
    Address oldsp = context.process->get_reg ("sp") ;
    Address sp = oldsp ;

    if (leftvalue.type == VALUE_REG) {          // in a register?
        Address v = context.process->get_reg (leftvalue.integer) ;              // contents
        sp = context.process->get_arch()->stack_space (context.process, type->get_real_size(context)) ;
        context.process->write (sp, v, type->get_real_size(context)) ;
        leftvalue.type = VALUE_INTEGER ;
        leftvalue.integer = sp ;
    }

    type->check_loaded() ;
    DIE *sym = NULL ;
    std::vector<DIE*> member_sequence ;

    //
    // this part deserves some explanation.  When looking for a member of an aggregate
    // we want a DIE *.  This may be located in the struct itself, in an anonymous
    // union within the struct, or in a base class of the struct.  For the case where
    // we need to go through an anonymous union or base class, we need to insert an
    // extra MEMBER expression node on the left because the union or base class will have
    // a location attribute that specifies where it is.  This is achieved by throwing
    // an exception containing the DIE (the DW_TAG_union_type or DW_TAG_inheritance DIE)
    for (;;) {
        try {
            sym = type->find_member (member) ;
            break ;                             // no exception means we found the member directly
        } catch (DIE *die) {                        // anonymous union member or inheritance detected
            // the die is  member of the struct with no name for an anonymous union
            // of the DW_TAG_inheritance for a base class
            member_sequence.push_back (die) ;
            // look in the dereferenced DIE thrown for the symbol.  In the case of an
            // anonymous union it will definitely be there, but for a base class it may
            // result in another exception (if the name is in a base of the base).  Come to think
            // if it, the anoynmous union may contain another anonymous union so that might happen
            // there too.
            type = die->get_type() ;
            if (die->get_tag() == DW_TAG_member) {
                sym = type->find_member (member) ;           // should be found in union
                break ;
            }
        }
    }
    if (sym == NULL) {
        throw Exception ("There is no member named %s", membername.c_str()) ;
    }

    // member_sequence contains a set of DIEs that need to be evaluated in sequence
    // to get to the member referenced

    Value current = leftvalue ;               // start with the address of the struct
    for (uint i = 0 ; i < member_sequence.size() ; i++) {
        Value v = member_sequence[i]->evaluate (context, current) ;
        current = v ;
    }
 
    // we don't want to dereference functions here
    if (sym->is_function()) {
        type = sym ;
    } else {
        type = sym->get_type() ;
    }

    // now current contains the address of the final struct containing sym
    Value mem = sym->evaluate (context, current) ;
    if (sym->is_function()) {
         Subprogram *subprog = dynamic_cast<Subprogram*>(sym) ;
         DwVirtId virt = subprog->get_virtuality() ;
         if (virt != DW_VIRTUALITY_none) {
            Address vtbl = subprog->get_virtual_table (context, current) ;              // address of virtual table
            if (vtbl == 0) {
                throw Exception ("Unable to determine address of virtual table") ;
            }
            int index = mem.integer * symtab->arch->ptrsize() ;                        // index into table 
            mem = context.process->readptr (vtbl + index) ;            // now read the actual address
         }
    }

    if (type->is_real()) {
        if (type->get_real_size(context) == 4) {
           mem.real = (double)(*(float*)&mem.real) ;
        }
        mem.type = VALUE_REAL ;
    }

    // restore stack pointer
    if (sp != oldsp) {
        context.process->set_reg ("sp", oldsp) ;
    }

    return mem ;
}

Expression::Expression (SymbolTable *symtab, Token opcode, Node *left, Node *right)
    : Node(symtab),
      left(left),
      right(right),
      opcode(opcode)
{

    if (left != NULL) {
        type = left->get_type() ;           // have to do
    }
}

Expression::Expression (const Expression &expr):Node (expr.symtab) {
    opcode = expr.opcode ;
    left = expr.left ;
    right = expr.right ;
    type = expr.type ;
}

Expression::~Expression() {
    delete left ;
    delete right ;
}

int Expression::num_variables() {
    int n = left->num_variables() ;
    if (right != NULL) {
        n += right->num_variables() ;
    }
    return n ;
}

// convert the left(right) to real if necessary.  Sets the type of the expression
// to the result (if it changes)
void Expression::convert(EvalContext &context, DIE *ltype, Value &l, DIE *rtype, Value &r) {
    if (context.addressonly) {
        return ;
    }
    if (rtype == NULL) {
        r.integer = 0 ;
        type = ltype ;
    }
    else if (ltype->is_integral() && rtype->is_real()) {
        l = (double)l.integer ;
        type = rtype ;
    } else if (ltype->is_real() && rtype->is_integral()) {
        r = (double)r.integer ;
        type =  ltype ;         // type is real
    } else {  // both are same type
        type = ltype->get_size_immed() > rtype->get_size_immed() ? ltype : rtype ;
    }
}

// scale pointer arithmetic
// convert the following:
//   pointer + int      => pointer + (int * sizeof (*pointer))          - same for minus
//   int + pointer      => (int * sizeof(*pointer)) + pointer
//   pointer - pointer  -> (pointer - pointer) / sizeof (*pointer)

// returns true if the calculation has been performed (in which case the result is
// written to the last parameter).

// also sets the type of the expression

bool Expression::scale(EvalContext &context, DIE *ltype, Value &l, DIE *rtype, Value &r, Value &result) {
    if (ltype->is_pointer() && rtype->is_integral()) {
        int size = ltype->get_type()->get_real_size(context) ;
        r.integer *= size ;
        type = ltype ;
        return false ;
    } else if (ltype->is_integral() && rtype->is_pointer()) {
        int size = rtype->get_type()->get_real_size(context) ;
        l.integer *= size ;
        type = rtype ;  
        return false ;
    } else if (opcode == MINUS && ltype->is_pointer() && rtype->is_pointer()) {
        Address diff = l.integer - r.integer ;
        int size = rtype->get_type()->get_real_size(context) ;
        type  =symtab->new_int() ;
        result = diff / size ;
        return true ;
    // convert array+n to &array[n]
    } else if (ltype->is_array() && rtype->is_integral()) {
        std::vector<Node *> indices ;
        indices.push_back (new ValueConstant (symtab, r, rtype)) ;
        Node *node = new Expression (symtab, ADDRESS, new ArrayExpression (symtab, new ValueConstant (symtab, l, ltype), indices)) ;
        result = node->evaluate(context) ;
        type = node->get_type() ;
        delete node ;
        return true ;
    // n+array -> &array[n]
    } else if (ltype->is_integral() && rtype->is_array()) {
        std::vector<Node *> indices ;
        indices.push_back (new ValueConstant (symtab, l, ltype)) ;
        Node *node = new Expression (symtab, ADDRESS, new ArrayExpression (symtab, new ValueConstant (symtab, r, rtype), indices)) ;
        result = node->evaluate(context) ;
        type = node->get_type() ;
        delete node ;
        return true ;
    }
    return false ;
}

bool Expression::is_local() {
    if (left != NULL && left->is_local()) {
        return true ;
    }
    if (right != NULL && right->is_local()) {
        return true ;
    }
    return false ;
}

static std::string opname (Token t) {
    std::string s = "operator" ;
    switch (t) {
    case PLUS:
        return s + "+" ;
    case MINUS:
        return s + "-" ;
    case STAR:
        return s + "*" ;
    case SLASH:
        return s + "/" ;
    case MOD:
        return s + "%" ;
    case LSHIFT:
        return s + "<<" ;
    case RSHIFT:
        return s + ">>" ;
    case EQUAL:
        return s + "==" ;
    case NOTEQUAL:
        return s + "!=" ;
    case LESS:
        return s + "<" ;
    case LESSEQ:
        return s + "<=" ;
    case GREATER:
        return s + ">" ;
    case GREATEREQ:
        return s + ">=" ;
    case BITAND:
        return s + "&" ;
    case BITOR:
        return s + "|" ;
    case ONESCOMP:
        return s + "~" ;
    case BITXOR:
        return s + "^" ;
    case NOT:
        return s + "!" ;
    case UMINUS:
        return s + "-" ;
    case UPLUS:
        return s + "+" ;
    case CONTENTS:
        return s + "*" ;
    case PLUSPLUS:
        return s + "++" ;
    case MINUSMINUS:
        return s + "--" ;
    case POSTINC:
        return s + "++" ;
    case POSTDEC:
        return s + "--" ;
    default:
        return "<bad>" ;
    }
}

static bool is_overloadable (Token t) {
    switch (t) {
    case PLUS:
    case MINUS:
    case STAR:
    case SLASH:
    case MOD:
    case LSHIFT:
    case RSHIFT:
    case EQUAL:
    case NOTEQUAL:
    case LESS:
    case LESSEQ:
    case GREATER:
    case GREATEREQ:
    case BITAND:
    case BITOR:
    case ONESCOMP:
    case BITXOR:
    case NOT:
    case UMINUS:
    case UPLUS:
    case CONTENTS:
    case PLUSPLUS:
    case MINUSMINUS:
    case POSTINC:
    case POSTDEC:
       return true ;
    default:
	return false ;
    }
}

// check for an operator overload and make the call if necessary.  If the call is
// made, the function returns true and writes the result to the last parameter.
// also sets the type of the result

// XXX: things left to do with this:
//    1. ++ and -- pre and post differences

bool Expression::check_operator_overload(EvalContext &context, DIE *ltype, Value &l, DIE *rtype, Value &r, Value &result) {
    if (!is_overloadable (opcode)) {
        return false ;
    }
    std::string name = opname (opcode) ;
    if (ltype->is_struct()) {
        std::vector<DIE*> ops ;
        ltype->find_operator (name, ops) ;
        if (ops.size() > 0) {
            std::vector<Node*> args ;
            args.push_back (new ValueConstant (symtab, l, ltype)) ;
            int nargs = 1 ;
            if (right != NULL) {
                args.push_back (new ValueConstant (symtab, r, rtype)) ;
                nargs++ ;
            }  else {
                // XXX: postinc and postdec in here
            }
            
            Node *left = new IdentifierSet (symtab, ops) ;
            CallExpression call (symtab, left, args) ;
            result = call.evaluate (context) ;
            type = call.get_type() ;
            return true ;
        }
    } else if (rtype != NULL && rtype->is_struct()) {
        std::vector<DIE*> ops ;
        rtype->find_operator (name, ops) ;
        if (ops.size() > 0) {
            std::vector<Node*> args ;
            args.push_back (new ValueConstant (symtab, r, rtype)) ;
            args.push_back (new ValueConstant (symtab, l, ltype)) ;
            CallExpression call (symtab, new IdentifierSet (symtab, ops), args) ;     
            result = call.evaluate (context) ;
            type = call.get_type() ;
            return true ;
        }
    }
    return false ;
}

Value Expression::evaluate(EvalContext &context) {
        Value l ;
        Value r ;
        Value result ;

        if (left == NULL) {
           throw Exception("illegal operation");
        }

        if (opcode != LOGAND && opcode != LOGOR &&
            opcode != COLON && opcode != QUESTION &&
            opcode != ADDRESS) {
            l = left->evaluate (context) ;
            if (right != NULL) {
                r = right->evaluate (context) ;
            }
        }

        // check for operator overloads.  If one is present, the check_operator_overload
        // function makes the call and returns the result in the last parameter.  It returns
        // true if this happens.
        if (check_operator_overload (context, left->get_type(), l, right!= NULL?right->get_type():NULL, r, result)) {
            return result ;
        }

        type = left->get_type() ;               // common default
        switch (opcode) {
        case POWER: {
            switch (l.type) {
            case VALUE_INTEGER:
                switch (r.type) {
                case VALUE_INTEGER:
                    return (int64_t)pow (l.integer, r.integer) ;
                case VALUE_REAL:
                    return pow (l.integer, r.real) ;
                case VALUE_STRING:
		default:
                    throw Exception ("Illegal type in ** operator") ;
                }
                break ;
            case VALUE_REAL:
                switch (r.type) {
                case VALUE_INTEGER:
                    return pow (l.real, r.integer) ;
                case VALUE_REAL:
                    return pow (l.real, r.real) ;
                case VALUE_STRING:
		default:
                    throw Exception ("Illegal type in ** operator") ;
                }
                break ;
            case VALUE_STRING:
	    default:
                throw Exception ("Illegal type in ** operator") ;
                break ;
            }
            break ;
            }
        case PLUS:
        case STRCAT: 
            convert (context, left->get_type(), l, right->get_type(), r) ;
            if (scale (context, left->get_type(), l, right->get_type(), r, result)) {   
                return result ;
            }
            return l + r ;
        case MINUS:
            convert (context, left->get_type(), l, right->get_type(), r) ;
            if (scale (context, left->get_type(), l, right->get_type(), r, result)) {   
                return result ;
            }
            return l - r ;
        case STAR:
            convert (context, left->get_type(), l, right->get_type(), r) ;
            return l * r ;
        case SLASH:
            convert (context, left->get_type(), l, right->get_type(), r) ;
            return l/ r;
        case MOD:
            return l% r;
        case BITAND:
            if (!left->get_type()->is_integral() || !right->get_type()->is_integral()) {
                 throw Exception ("Illegal type for & operator") ;
            }
            return l & r ;
        case BITOR:
            if (!left->get_type()->is_integral() || !right->get_type()->is_integral()) {
                 throw Exception ("Illegal type for | operator") ;
            }
            return l | r ;
        case BITXOR:
            if (!left->get_type()->is_integral() || !right->get_type()->is_integral()) {
                 throw Exception ("Illegal type for ^ operator") ;
            }
            return l ^ r ;
        case EQUAL:
            convert (context, left->get_type(), l, right->get_type(), r) ;
            type = symtab->new_boolean() ;
            return l == r ;
        case NOTEQUAL:
            convert (context, left->get_type(), l, right->get_type(), r) ;
            type = symtab->new_boolean() ;
            return l != r ;
        case LESS:
            convert (context, left->get_type(), l, right->get_type(), r) ;
            type = symtab->new_boolean() ;
            return l < r ;
        case LESSEQ:
            convert (context, left->get_type(), l, right->get_type(), r) ;
            type = symtab->new_boolean() ;
            return l <= r ;
        case GREATER:
            convert (context, left->get_type(), l, right->get_type(), r) ;
            type = symtab->new_boolean() ;
            return l > r ;
        case GREATEREQ:
            convert (context, left->get_type(), l, right->get_type(), r) ;
            type = symtab->new_boolean() ;
            return l >= r ;
        case LSHIFT:
            if (!left->get_type()->is_integral() || !right->get_type()->is_integral()) {
                throw Exception ("Illegal type for << operator") ;
            }
            return l << r ;
        case RSHIFT:
            if (!left->get_type()->is_integral() || !right->get_type()->is_integral()) {
                throw Exception ("Illegal type for >> operator") ;
            }
            return l >> r ;
        case LOGAND: {
            type = symtab->new_boolean()  ;
            Value v = left->evaluate(context) ;
            if (v != 0) {
                v =right->evaluate(context) ;
            }
            return v != 0 ;
            }
        case LOGOR: {
            type = symtab->new_boolean()  ;
            Value v = left->evaluate(context) ;
            if (v == 0) {
                v =right->evaluate(context) ;
            }
            return v != 0;
            }
        case PLUSPLUS: {
            Value inc(1) ;
            if (left->get_type()->is_pointer()) {
                inc = left->get_type()->get_type()->get_real_size(context) ;
            }
            l = l + inc ;
            left->set_value (context, l, get_type()) ;
            return l ;
            }
        case MINUSMINUS: {
            Value inc(1) ;
            if (left->get_type()->is_pointer()) {
                inc = left->get_type()->get_type()->get_real_size(context) ;
            }
            l = l - inc ;
            left->set_value (context, l, get_type()) ;
            return l ;
            }
        case POSTINC: {
            Value inc(1) ;
            if (left->get_type()->is_pointer()) {
                inc = left->get_type()->get_type()->get_real_size(context) ;
            }
            Value v1 = l + inc ;
            left->set_value (context, v1, get_type()) ;
            return l ;
            }
        case POSTDEC: {
            Value inc(1) ;
            if (left->get_type()->is_pointer()) {
                inc = left->get_type()->get_type()->get_real_size(context) ;
            }
            Value v1 = l - inc ;
            left->set_value (context, v1, get_type()) ;
            return l ;
            }
        case UMINUS:
            return -l ;
        case UPLUS:
            return l ;
        case ONESCOMP:
            if (!left->get_type()->is_integral()) {
                 throw Exception ("Illegal type for ~ operator") ;
            }
            return ~l ;
        case NOT:
            return !l ;
        case CONTENTS: {
            if (left->get_type()->is_array()) {         // convert *array into array[0]
                std::vector<Node *> indices ;
                indices.push_back (new IntConstant (symtab, 0, 4)) ;
                ArrayExpression array (symtab, new ValueConstant (symtab, l, left->get_type()), indices) ;
                Value v = array.evaluate (context) ;
                type = array.get_type() ;
                return v ;
            }

            if (!left->get_type()->is_pointer()) {
                throw Exception ("Pointer type required for * operator") ;
            }
            // may have const and volatile in the way
            while (type->get_tag() != DW_TAG_pointer_type) {
               type = type->get_type() ;
            }
            type = type->get_type() ;                // dereference the pointer
            type->check_loaded() ;
            if (type->is_struct()) {
                DIE *t = left->get_type()->get_type() ;
                while (t->get_tag() != DW_TAG_structure_type &&
                       t->get_tag() != DW_TAG_union_type &&
                       t->get_tag() != DW_TAG_class_type) {
                    t = t->get_type() ;
                }
                type = t ;

                // get the dynamic type of the struct
                if (context.addressonly) {      // l will contain the address of the pointer, we need to dereference it for get_dynamic_type
                    Address v = context.process->read (l.integer, context.process->get_arch()->ptrsize()) ;
                    type = dynamic_cast<TypeStruct*>(type)->get_dynamic_type (context, v) ;
                } else {
                    type = dynamic_cast<TypeStruct*>(type)->get_dynamic_type (context, l) ;
                }
                return l ;
            }

            if (type->is_array()) {
                return l ;
            }

            Value v =  context.process->read (l.integer, type->get_size()) ;

            // reals will come back as integers with the correct bit pattern.  We need to convert these to
            // doubles
            if (type->is_real()) {
                if (type->get_size() == 4) {                    // float?
                    v.real = (double)(*(float*)&v.real) ;
                }
                v.type = VALUE_REAL ;
            }
            return v ;
            }
        case ADDRESS: {
            bool saved = context.addressonly ;
            context.addressonly = true ;
            Value v = left->evaluate (context) ;
            context.addressonly = saved ;
            type = symtab->new_pointer_type (left->get_type()) ;
            return v ;
            break ;
            }
        case COMMA: {
            if (right != NULL) {
                type = right->get_type() ;
                return r ;
            } else {
                return l ;
            }
            break ;
            }
        case COLON: {
            type = right->get_type() ;
            Value v = ((Expression*)left)->left->evaluate (context) ;
            if (v != 0) {
                return ((Expression *)left)->right->evaluate (context) ;
            } else {
                return right->evaluate (context) ;
            }
            break ;
            }
        case SIZEOF:
            type = symtab->new_int() ;
            return left->get_type()->get_real_size(context) ;
	default:
	    break;
        }
	throw Exception("not reached");
}

void Expression::set_value (EvalContext &context, Value &value, DIE *exprtype) {
    switch (opcode) {
    case CONTENTS: {
        Value addr = left->evaluate(context) ;          // get what the pointer is pointing to
        type = left->get_type() ;
        if (type->is_array()) {
            throw Exception ("Can't set the value of an array.") ;
        }

        type->set_value (context, addr, value) ;                // set the value
        break ;
        }
    default:
        throw Exception ("Can't set the value of this expression.") ;
    }
}


AssignmentExpression::AssignmentExpression (SymbolTable *symtab, Token tok, Node *l, Node *r)
    : Node(symtab), tok(tok),
    left(l),
    right(r) {
    type = left->get_type() ;
}

AssignmentExpression::~AssignmentExpression() {
    delete left ;
    delete right ;
}

int AssignmentExpression::num_variables() {
    int n = left == NULL ? 0 : left->num_variables() ;
    if (right != NULL) {
        n += right->num_variables() ;
    }
    return n ;
}

bool AssignmentExpression::is_local() {
    if (left != NULL && left->is_local()) {
        return true ;
    }
    if (right != NULL && right->is_local()) {
        return true ;
    }
    return false ;
}

Value AssignmentExpression::convert (EvalContext &context, Value &rhs) {
    if (context.addressonly) {
        return rhs ;
    }
    if (!left->is_generic()) {                      // assignment to generic nodes doesn't do any conversion
        if (left->get_type()->is_integral() && right->get_type()->is_real()) {
            rhs.integer = (int64_t)rhs.real ;
            rhs.type = VALUE_INTEGER ;
        } else if (left->get_type()->is_real() && right->get_type()->is_integral()) {
            rhs.real = (double)rhs.integer ;
            rhs.type = VALUE_REAL ;
        }
    }
    return rhs ;
}

Value AssignmentExpression::evaluate(EvalContext &context) {
    Value rhs = right->evaluate(context) ;
    Value lhs ;

    if (tok != ASSIGN) {
        lhs = left->evaluate (context) ;
    } else {
        // need to evaluate the address of the left in order to get the type
        bool save = context.addressonly ;
        context.addressonly = true ;
        lhs = left->evaluate (context) ;
        context.addressonly = save ;
    }
    
    switch (tok) {
    case ASSIGN:
        rhs = convert (context, rhs) ;
        break ;
    case PLUSEQ:
        rhs = lhs + convert (context, rhs) ;
        break ;
    case MINUSEQ:
        rhs = lhs - convert (context, rhs) ;
        break ;
    case STAREQ:
        rhs = lhs * convert (context, rhs) ;
        break ;
    case SLASHEQ:
        rhs = lhs / convert (context, rhs) ;
        break ;
    case PERCENTEQ:
        if (!left->get_type()->is_integral() || !right->get_type()->is_integral()) {
            throw Exception ("Illegal type for %= operator") ;
        }
        rhs = lhs % rhs ;
        break ;
    case ANDEQ:
        if (!left->get_type()->is_integral() || !right->get_type()->is_integral()) {
            throw Exception ("Illegal type for &= operator") ;
        }
        rhs = lhs & rhs ;
        break ;
    case OREQ:
        if (!left->get_type()->is_integral() || !right->get_type()->is_integral()) {
            throw Exception ("Illegal type for |= operator") ;
        }
        rhs = lhs | rhs ;
        break ;
    case XOREQ:
        if (!left->get_type()->is_integral() || !right->get_type()->is_integral()) {
            throw Exception ("Illegal type for ^= operator") ;
        }
        rhs = lhs ^ rhs ;
        break ;
    case LSHIFTEQ:
        if (!left->get_type()->is_integral() || !right->get_type()->is_integral()) {
            throw Exception ("Illegal type for <<= operator") ;
        }
        rhs = lhs << rhs ;
        break ;
    case RSHIFTEQ:
        if (!left->get_type()->is_integral() || !right->get_type()->is_integral()) {
            throw Exception ("Illegal type for >>= operator") ;
        }
        rhs = lhs >> rhs ;
        break ;
    default:
	break; // added by bos for -Wall niceness
    }

    left->set_value (context, rhs, right->get_type()) ;
    type = left->get_type() ;
    return left->evaluate (context) ;
}

CastExpression::CastExpression (SymbolTable *symtab, Node *left, DIE *casttype)
    : Node(symtab),
    left(left),
    casttype(casttype) {
    type = casttype ;
}

bool CastExpression::is_local() {
    if (left != NULL && left->is_local()) {
        return true ;
    }
    return false ;
}

int CastExpression::num_variables() {
    if (left != NULL) {
       return left->num_variables() ;
    }
    return 0; 
}

CastExpression::~CastExpression() {
    delete left ;
}

Value CastExpression::evaluate(EvalContext &context) {
    Value v = left->evaluate (context) ;
 
    type = casttype ;
    if (left->get_type()->is_integral() && type->is_real()) {
        v.real = (double)v.integer ;
        v.type = VALUE_REAL ;
    } else if (left->get_type()->is_real() && type->is_integral()) {
        v.integer = (int64_t)v.real ;
        v.type = VALUE_INTEGER ;
    }
    // we may need to mask integers (e.g. (unsigned char)(~0) is 0xff
    int dest_size = type->get_real_size(context) ;
    int src_size = left->get_type()->get_real_size(context) ;
    if (v.type == VALUE_INTEGER) {
        if (dest_size < src_size) {
            int64_t mask = dest_size == 64 ? 0xffffffffffffffffLL : (1LL << (dest_size*8)) - 1 ;
            v.integer &= mask ;
        }
    }
    // casting a string requires that it be written to memory and the address returned
    // we write it to the current stack.  It will be overwritten when the program continues
    if (v.type == VALUE_STRING) {
        int len = v.str.size() ;
        len = (len + 15) & ~15 ;                        // align to 16 byte boundary
        Address oldsp = context.process->get_reg("sp") ;
        Address sp = context.process->get_arch()->stack_space (context.process, len) ;
        context.process->write_string (sp, v.str) ;
        context.process->set_reg ("sp", oldsp) ;
        v.type = VALUE_INTEGER ;
        v.integer = sp ;
    }
    return v ;
}

/* cast kind of left expresion to that of the value in right.
   this is used for fortran expression of the form "4_8" */
TypeCastExpression::TypeCastExpression (SymbolTable *symtab, Node *left, Node *right)
    : Node(symtab),
    left(left),
    right(right)
{
}

bool TypeCastExpression::is_local() {
    if (left != NULL && left->is_local() &&
        right != NULL && right->is_local()) {
        return true;
    }
    return false;
}

int TypeCastExpression::num_variables() {
    int n = 0;

    if (left != NULL) {
       n += left->num_variables() ;
    }
    if (right != NULL) {
       n += right->num_variables() ;
    }

    return n;
}

TypeCastExpression::~TypeCastExpression() {
    delete left ;
    delete right ;
}

Value TypeCastExpression::evaluate(EvalContext &context) {
    Value vl = left->evaluate (context) ;
    Value vr = right->evaluate (context) ;

    if (!right->get_type()->is_integral()) {
        throw Exception ("Must use integer when doing KIND cast");
    }

    // logical is subset of integral and complex is subset of
    // real, so the ordering of conditions must be preserved
    if (left->get_type()->is_boolean()) {
        if (vr.integer != 1 && vr.integer != 2 &&
            vr.integer != 4 && vr.integer != 8) {
            throw Exception ("Invalid KIND value %d in LOGICAL cast", vr.integer);
        }
        type = symtab->new_scalar_type("LOGICAL", DW_ATE_boolean, vr.integer);
    } else if (left->get_type()->is_integral()) {
        if (vr.integer != 1 && vr.integer != 2 &&
            vr.integer != 4 && vr.integer != 8) {
            throw Exception ("Invalid KIND value %d in INTEGER cast", vr.integer);
        }
        type = symtab->new_scalar_type("INTEGER", DW_ATE_signed, vr.integer);
    } else if (left->get_type()->is_complex()) {
        if (vr.integer != 4 && vr.integer != 8) {
            throw Exception ("Invalid KIND value %d in COMPLEX cast", vr.integer);
        }
        type = symtab->new_scalar_type("COMPLEX", DW_ATE_complex_float, vr.integer);
    } else if (left->get_type()->is_real()) {
        if (vr.integer != 4 && vr.integer != 8) {
            throw Exception ("Invalid KIND value %d in REAL cast", vr.integer);
        }
        type = symtab->new_scalar_type("REAL", DW_ATE_float, vr.integer);
    } else {
        throw Exception ("Invalid type for cast expression");
    }
    return vl;
}

SizeofExpression::SizeofExpression (SymbolTable *symtab, DIE *t) : Node(symtab) {
    type = t ;
}

SizeofExpression::~SizeofExpression() {
}

Value SizeofExpression::evaluate (EvalContext &ctx) {
    // XXX: dynamic type
    return type->get_real_size (ctx) ;
}

VectorExpression::VectorExpression (SymbolTable *symtab, std::vector<Node*> &v)
    : Node(symtab),
    values(v) {

}

VectorExpression::~VectorExpression() {
    for (uint i = 0 ; i < values.size() ; i++) {
        delete values[i] ;
    }

}

int VectorExpression::num_variables() {
    int n = 0 ;
    for (uint i = 0 ; i < values.size() ; i++) {
        n += values[i]->num_variables() ;
    }

    return n ;
}

Value VectorExpression::evaluate(EvalContext &context) {
    std::vector<Value> v ;
    for (uint i = 0 ; i < values.size() ; i++) {
        v.push_back (values[i]->evaluate (context)) ;
    }
    if (values.size() > 0) {
        type = values[0]->get_type() ;              // XXX:check all the types are the same
    } else {
        type = NULL ;
    }
    return v; 
}

ConstructorExpression::ConstructorExpression (SymbolTable *symtab, Node *sn, std::vector<Node*> & v)
    : Node(symtab),
    structnode(sn),
    values(v) {


}

ConstructorExpression::~ConstructorExpression() {
    for (uint i = 0; i < values.size() ; i++) {
        delete values[i] ;
    }
    delete structnode ;
}

int ConstructorExpression::num_variables() {
    int n = 0 ;
    for (uint i = 0 ; i < values.size() ; i++) {
        n += values[i]->num_variables() ;
    }

    return n ;
}

Value ConstructorExpression::evaluate(EvalContext &context) {
    Architecture *arch = context.process->get_arch() ;
    type = structnode->get_type() ;
    Address oldsp = context.process->get_reg ("sp") ;
    Address sp = arch->stack_space (context.process, type->get_real_size(context)) ;            // space for structure
    Value addr = sp ;
    std::vector<DIE *> &children = type->getChildren() ;
    uint val = 0 ;                                       // current value
    bool old = context.addressonly ;
    context.addressonly = true ;

    for (uint i = 0 ; i < children.size() ; i++) {
        DIE *child = children[i] ;
        if (child->get_tag() == DW_TAG_member) {
            if (val == values.size()) {
                throw Exception ("Too many values for type constructor") ;
            }
            Value v = values[val]->evaluate (context) ;                 // value to assign
            Value mem = child->evaluate (context, addr) ;               // get address of child
            child->get_type()->set_value (context, mem, v) ;            // set the value in memory
            val++ ;                                                     // next value
        }
    }
    if (val != values.size()) {
        throw Exception ("Insufficient values for type constructor") ;
    }
    context.addressonly = old ;
    context.process->set_reg ("sp", oldsp) ;
    return addr ;
}


CallExpression::CallExpression (SymbolTable *symtab, Node *l, std::vector<Node*> & a)
    : Node(symtab),
    left(l),
    args(a), is_func(false) {


}

CallExpression::~CallExpression() {
    uint i = left->is_member_expression() ? 1 : 0 ;
    // don't delete args[0] if it points to this (it will be deleted by the MemberExpression)
    for (; i < args.size() ; i++) {
        delete args[i] ;
    }
    delete left ;
}

int CallExpression::num_variables() {
    int n = left->num_variables() ;
    for (uint i = 0 ; i < args.size() ; i++) {
        n += args[i]->num_variables() ;
    }

    return n ;
}

bool CallExpression::is_local() {
    if (left != NULL && left->is_local()) {
        return true ;
    }
    for (uint i = 0 ; i < args.size() ; i++) {
        if (args[i]->is_local()) {
            return true ;
        }
    }
    return false ;
}

// given a set of DIEs corresponding to overloads of a particular function name,
// determine which one to call based on the set of arguments passed to the
// function.  The arguments are in the 'args' vector in the object and they
// have already been evaluated

DIE *CallExpression::resolve_overloads (EvalContext &ctx, bool thisadded, std::vector<DIE*> &overloads) {
    // short cut for only one overload
    if (overloads.size() == 1) {
        return overloads[0] ;
    }

    std::vector<DIE *> matches ;                        // this holds the set of DIEs matched
    bool foundfunc = false ;                            // true if any function exists
    std::string funcname ;

    for (uint i = 0 ; i < overloads.size() ; i++) {
        Subprogram *subprog = dynamic_cast<Subprogram*>(overloads[i]) ;
        if (subprog != NULL) {                  // really a subprogram?
            AttributeValue lowpc = subprog->getAttribute (DW_AT_low_pc) ;
            if (lowpc.type == AV_NONE) {        
                continue ;
            }
            if (!foundfunc) {
                funcname = subprog->getAttribute (DW_AT_name).str ;
            }
            foundfunc = true ;
            std::vector<DIE*> paras ;
            subprog->get_formal_parameters (paras) ;
            int nformal = paras.size() ;
            bool variadic = subprog->is_variadic() ;
            int nactual = args.size() ;
            if (nactual == nformal || (nactual > nformal && variadic)) {
                bool good = true ;
                for (uint arg = 0 ; arg < std::min(args.size(), paras.size()) ; arg++) {
                    if (!paras[arg]->get_type()->compare (ctx, args[arg]->get_type(), 0)) {
                        good = false ;
                        break ;
                    }
                }
                if (good) {
                    matches.push_back (overloads[i]) ;
                }
            }
        }
    }

    if (matches.size() == 0) {
        // XXX: no exact match, try less exact ones
    }

    if (matches.size() == 0) {
        if (foundfunc) {
            ctx.os.print ("No match for call %s(", funcname.c_str()) ;
            bool comma = false ;
            for (uint i = thisadded?1:0 ; i < args.size() ; i++) {
                if (comma) {
                   ctx.os.print (", ") ;
                }
                args[i]->get_type()->print (ctx, 0, 0) ;
                comma = true ;
            }
            ctx.os.print (")\n") ;
            ctx.os.print ("Possible functions are:\n") ;
            for (uint i = 0 ; i < overloads.size() ; i++) {
                Subprogram *subprog = dynamic_cast<Subprogram*>(overloads[i]) ;
                if (subprog != NULL) {                  // really a subprogram?
                    AttributeValue lowpc = subprog->getAttribute (DW_AT_low_pc) ;
                    if (lowpc.type == AV_NONE) {        
                        continue ;
                    }
                    subprog->print (ctx, 4, 0) ;
                    ctx.os.print ("\n") ;
                }
            }
            throw Exception ("Call failed") ;
        } else {
            return NULL ;
        }
    }

    // exactly one match, return it
    if (matches.size() == 1) {
        return matches[0] ;
    }

    // more than one match, ask the user
    ctx.os.print ("Ambiguous function, please choose from the following list\n") ;
    ctx.os.print ("0. cancel\n") ;
    for (uint i = 0 ; i < matches.size() ; i++) {
        ctx.os.print ("%d. ", i+1) ;
        matches[i]->print (ctx, 0, 0) ;
        ctx.os.print ("\n") ;
    }
    int n ;
    for (;;) {
        ctx.os.print ("> ") ;
        ctx.os.flush() ;
        std::string r ;
        std::cin.clear();
        std::getline (std::cin, r) ;
        if (r.size() == 0) {
            continue ;
        }
        n = atoi(r.c_str()) ;
        if (n < 0 || (uint) n > matches.size()) {
            ctx.os.print ("Please enter a number between 0 and %d.\n", matches.size()) ;
            continue ;
        }
        break ;
    }
    if (n == 0) {
        throw Exception ("Cancelled") ;
    }
    return matches[n-1] ;
}

Value CallExpression::evaluate(EvalContext &context) {
    Architecture *arch = symtab->arch ;

    std::vector<DIE*> *overloads = NULL ;
    std::vector<DIE*> members ;

    // add the this argument to the args list if calling a member function
    if (left->is_member_expression()) {             // calling a member
        dynamic_cast<MemberExpression*>(left)->resolve (context, members) ;
        overloads = &members ;
        // if the left is a member expression, we need to add the this pointer to the
        // args.
        Node *thisptr = dynamic_cast<MemberExpression*>(left)->get_this_pointer() ;
        args.push_back (NULL) ;
        for (int i = (int)args.size() - 2 ; i >= 0 ; i--) {
            args[i+1] = args[i] ;
        }
        args[0] = thisptr ;
    } else if (left->is_identifier_set()) {
        overloads = &dynamic_cast<IdentifierSet*>(left)->get_dies() ;
    }

    int numargs = (int)args.size() ;
    std::vector<Value> argvalues ;

    // now, evaluate all the arguments
    for (int i = numargs-1 ; i >= 0 ; i--) {
        Value v = args[i]->evaluate (context) ;
        if (v.type == VALUE_NONE) {
            throw Exception ("Unable to evaluate function arguments") ;
        }
        argvalues.push_back (v) ;
    }

    // this is the address to call
    Address func = 0 ;
    DIE *functype = NULL ;

    // if the left is a struct, check for operator()
    if (overloads != NULL && overloads->size() == 1 && (*overloads)[0] != NULL && (*overloads)[0]->is_struct_deref()) {
        std::vector<DIE*> ops ;
        (*overloads)[0]->find_operator ("operator()", ops) ;
        if (ops.size() > 0) {
            functype = resolve_overloads (context, true, ops) ;
            if (functype != NULL) {
                func = functype->evaluate (context) ;
            } else {
                throw Exception ("Cannot call operator() - perhaps it is optimized out") ;
            }
        } else {
            throw Exception ("Cannot call this struct - no operator() present") ;
        }
    } else {
        // if we have a set of identifiers to call, we need to resolve which one it is
        if (left->is_member_expression() || left->is_identifier_set()) {
            functype = resolve_overloads (context, left->is_member_expression(), *overloads) ;
            if (functype != NULL) {     
                // a member expression must be evaluated as a whole since the function being called
                // might be virtual and we need to find the address of the object (virtual base classes
                // and all that stuff)
                if (left->is_member_expression()) {
                    func = static_cast<MemberExpression*>(left)->evaluate (context, functype) ;
                } else {
                    func = functype->evaluate (context) ;                // not member, just evaluate directly
                }
            }
        } else {
            func = left->evaluate (context) ;
            functype = left->get_type() ;
        }
    }

    if (func == 0) {
       throw Exception ("Cannot call this function - it may have been optimized out or inlined") ;
    }
    

    // now dereference the function type to get the return type
    if (functype->is_function()) {
        type = functype->get_type() ;
        is_func = true ;
    } else {
        type = functype ;
    }


    // save the current process state
    StateHolder *state = context.process->save_and_reset_state();

    Subprogram *subprog = NULL ;

    // allocate space for the call instruction
    std::string buffer ;
    Address target = arch->write_call (context.process, func, buffer) ;
    //context.process->disassemble (target, target+8) ;
    //context.process->dump (target, 16) ;

    Address struct_return = 0 ;
    int arg_adjust = 0 ;
    bool small_struct = false ;
    bool is_struct_return = false ;
    int return_class = 0 ;

    // does the function return a struct?
    if (type != NULL && type->is_struct()) {            // NULL is void
        type->check_loaded() ;
        return_class = arch->classify_struct (context, type) ;
        struct_return = arch->stack_space (context.process, type->get_real_size(context)) ;     // space on stack
        if (arch->is_small_struct(type->get_real_size(context))) {
            small_struct = true ;
        } else {
            arg_adjust = 1 ;
            is_struct_return = true ;
        }
    }

    // copy all the args to the stack

    std::vector<DIE *> paras ;
    bool variadic = false ;
    if (is_func) {
        subprog = dynamic_cast<Subprogram*>(functype) ;
        subprog->check_loaded() ;

        subprog->get_formal_parameters (paras) ;
        variadic = subprog->is_variadic() ;

        if ((int)paras.size() != numargs) {
            if (!variadic || numargs < (int)paras.size()) {
                throw Exception ("Incorrect number of arguments for function call") ;
            }
        }
    }

    // some arguments need to be on the stack.  Here is where we push them.
    for (int i = numargs-1 ; i >= 0 ; i--) {
        Value &v = argvalues[i] ;
        if (i < (int)paras.size() && paras[i]->get_type()->is_real()) {
            // fortran passes everything by reference, so push constants onto the stack
            // all nonconstants have already had their address taken

            if (context.language == DW_LANG_Fortran77 || context.language == DW_LANG_Fortran90 || context.language == DW_LANG_Fortran95) {
                if (args[i]->is_constant()) {
                    int size = paras[i]->get_type()->get_size() ;
                    Address tmp = arch->stack_space (context.process, size) ;
                    context.process->write (tmp, v.integer, size) ;
                    v.integer = tmp ;
                    v.type = VALUE_INTEGER ;
                }
            } else {
                if (v.type != VALUE_REAL) {
                    v.real = (double)v.integer ;
                    v.type = VALUE_REAL ;
                }
            }
        } else {
            if (v.type == VALUE_STRING) {
                Address tmp = arch->stack_space (context.process, v.str.size()+1) ;           // some stack space
                context.process->write_string (tmp, v.str) ;                                 // copy the string to stack
                // NOTE: Target::write_string() doesn't append 0
                context.process->write (tmp + v.str.size(), 0, 1) ;                 // append 0
                v.type = VALUE_INTEGER ;
                v.integer = tmp ;
            } else {
                // fortran passes everything by reference, so push all values onto the stack
                if (context.language == DW_LANG_Fortran77 || context.language == DW_LANG_Fortran90 || context.language == DW_LANG_Fortran95) {
                    DIE *t = paras[i]->get_type() ;
                    // if the struct is small and in a register then it needs to be copied to memory
                    // constants are also pushed
                    if ((t->is_struct() && v.type == VALUE_REG) || args[i]->is_constant()) {            // already a reference?
                        int size = t->get_size() ;
                        Address tmp = arch->stack_space (context.process, size) ;
                        context.process->write (tmp, v.integer, size) ;
                        v.integer = tmp ;
                    }
                } 
            }
        }
    }

    // it appears that the use of floating point stuff requires 16 byte alignment on x86_64
    arch->align_stack(context.process) ;
    Address startsp = context.process->get_reg ("sp") ;

    int num_fp_args = 0 ;

    // note, the argvalues vector has the arguments in reverse order
    for (int i = numargs-1 ; i >= 0 ; i--) {
        Value &v = argvalues[numargs - 1 - i] ;
        bool pass_aggregate_by_value = false ;
        bool is_aggregate = false ;
        // fortran structs are passed by reference, not value.  All others are passed by value by
        // copying them to the stack
        if (args[i]->get_type()->is_struct()) {
            is_aggregate = true ;
            pass_aggregate_by_value = true ;
            if (context.language == DW_LANG_Fortran77 || context.language == DW_LANG_Fortran90 || context.language == DW_LANG_Fortran95) {       // fortran passes by reference
                pass_aggregate_by_value = false ;
            } else if (i < (int)paras.size()) {
                if (paras[i]->get_tag() == DW_TAG_reference_type) {                     // references are passed by reference (duh)
                    pass_aggregate_by_value = false ;
                }
            }
        }

        if (is_aggregate && pass_aggregate_by_value) {
            int size = args[i]->get_type()->get_real_size(context) ;
            if (arch->is_small_struct (size)) {
                args[i]->get_type()->check_loaded() ;
                Address addr = v.integer ;
                // XXX: this is intel specific.  Move it to the Architecture objects
                int cls = arch->classify_struct (context, args[i]->get_type()) ;
                //printf ("struct class = %d\n", cls); 
                if (cls == X8664_AC_MEMORY) {
                    std::string struct_value = context.process->read_string (addr, size) ;
                    arch->write_call_arg (context.process, i + arg_adjust, struct_value.data(), size) ;
                } else {
                    int nregs = (size - 1) / arch->main_register_set_properties()->size_of_register() + 1 ;               // probably 1 or 2
                    for (int j = nregs-1 ; j >= 0 ; j--) {
                        int size = arch->main_register_set_properties()->size_of_register();
                        Address part = context.process->read (addr + j * size, size);
                        arch->write_call_arg (context.process, i + arg_adjust + j, part, cls == X8664_AC_SSE) ;
                        num_fp_args += cls == X8664_AC_SSE ;
                    }
                    arg_adjust += nregs - 1 ;
                }
            } else {
                std::string struct_value = context.process->read_string (v.integer, size) ;
                arch->write_call_arg (context.process, i + arg_adjust, struct_value.data(), size) ;
            }
        } else {
            switch (v.type) { 
            case VALUE_INTEGER:
               arch->write_call_arg (context.process, i + arg_adjust, v.integer, false) ;
               break ;
            case VALUE_REAL:
               arch->write_call_arg (context.process, i + arg_adjust, v.integer, true) ;
               break ;
            default:
                throw Exception ("Can't assign argument value") ;
            }
        }
    }

    // write dummy first arg for struct return
    if (is_struct_return) {
        arch->write_call_arg (context.process, 0, struct_return, false) ;
    }

    // the architecture might require an alignment of the stack.  If the top of the
    // stack is not of the correct alignment, move the whole thing to the aligned
    // address
    Address endsp = context.process->get_reg ("sp") ;
    arch->align_stack(context.process) ;
    Address tmp = context.process->get_reg ("sp") ;
    if (endsp != tmp) {                 // has the stack moved?
        int size = startsp - endsp ;
        std::string contents = context.process->read_string (endsp, size) ;
        context.process->write_string (tmp, contents) ;
    }
    
    // XXX: this is x86 dependent.  Move it to Architecture
    context.process->set_reg (arch->get_return_reg(1), num_fp_args) ;

    Address sp = context.process->get_reg ("sp") ;

    // set the program counter to the start of the call
    context.process->set_reg ("pc", target) ;

    context.process->docont() ;                   // continue execute of the debuggee at the call address
    context.process->wait() ;                   // wait for it to finish
    Address newsp = context.process->get_reg ("sp") ;

    // detect a breakpoint hit in the function being called
    if (newsp < sp) {
        context.os.print ("Detected breakpoint hit in called function, spawning new command interpreter.\n") ;
        context.process->spawn_cli (sp) ;       
        // when this returns, the function will have returned
        context.os.print ("Function returned, restoring original command interpreter\n") ;
    }

    Value v ;                   // return value
    if (is_struct_return) {
        v = struct_return ;             // Note, this is on the stack, below the calling frame
    } else if (small_struct) {
        if (return_class == X8664_AC_SSE) {
            Address p = context.process->get_fpreg (arch->get_return_fpreg()) ;
            context.process->write (struct_return, p, 8 /*arch->get_fpreg_size()*/) ;   //XXX: fp regs are 16 bytes
        } else {
            int size = arch->main_register_set_properties()->size_of_register();
            Address p1 = context.process->get_reg (arch->get_return_reg(1)) ; 
            Address p2 = context.process->get_reg (arch->get_return_reg(2)) ; 
            context.process->write(struct_return, p1, size);
            context.process->write(struct_return + size, p2, size);
        }
        v = struct_return ;
    } else {
        // read the return value of the call
        if (type != NULL && type->is_real()) {
            v = context.process->get_fpreg (arch->get_return_fpreg()) ; 
            if (type->get_size() == 4) {
                v.real = (double)(*(float*)&v.real) ;
            }
            v.type = VALUE_REAL ;
        } else {
            v = context.process->get_reg (arch->get_return_reg()) ; 
        }
    }

    context.process->write_string (target, buffer) ;
    context.process->restore_state (state) ;
    return v ;
}

IntrinsicExpression::IntrinsicExpression (SymbolTable *symtab, IntrinsicIdentifier *left, std::vector<Node*> & args)
    : Node(symtab),
    it(left->get_intrinsic()),
    args(args) {
    delete left ;
}  

IntrinsicExpression::~IntrinsicExpression() {
    for (uint i = 0 ; i < args.size() ; i++) {
        delete args[i] ;
    }
}

int IntrinsicExpression::num_variables() {
    int n = 0 ;
    for (uint i = 0 ; i < args.size() ; i++) {
        n += args[i]->num_variables() ;
    }

    return n ;
}

Value IntrinsicExpression::evaluate(EvalContext &context) {
    switch (it) {
    case IT_KIND:
    case IT_LEN: {
        type = symtab->new_int() ;
        if (args.size() != 1) {
            throw Exception ("Incorrect number of arguments for intrinsic (expected 1, got %d)", args.size()) ;
        }
        Value v = args[0]->evaluate (context) ;
        return args[0]->get_type()->get_real_size(context) ;               // just get the size of the type
        break ;
        }
    case IT_SIZE: {
        type = symtab->new_int() ;
        Value v = args[0]->evaluate (context) ;
        if (args[0]->get_type()->is_array()) {
            std::vector<Dimension> &dims = dynamic_cast<TypeArray*>(args[0]->get_type())->get_dims(context) ;
            if (args.size() == 1) {
                int n = 1 ;
                for (uint i = 0 ; i < dims.size() ; i++) {
                    n *= dims[i].size() ;
                }
                return n ;
            } else if (args.size() ==2) {
                Value dim = args[1]->evaluate(context) ;
                if (dim.type != VALUE_INTEGER || dim.integer < 1 || (uint) dim.integer > dims.size()) {
                    throw Exception ("Invalid dimension number for size instrinsic") ;
                }
                return dims[dim.integer-1].size() ;
            } else {
                throw Exception ("Incorrect number of arguments for intrinsic (expected 1 or 2, got %d)", args.size()) ;
            }
        } else {
            if (args.size() != 1) {
                throw Exception ("Incorrect number of arguments for intrinsic (expected 1, got %d)", args.size()) ;
            }
            return args[0]->get_type()->get_real_size(context) ;               // just get the size of the type
        }
        
        break ;
        }
    case IT_UBOUND:
        type = symtab->new_int() ;
        break ;
    case IT_LBOUND:
        type = symtab->new_int() ;
        break ;
    case IT_ALLOCATED: {
    case IT_ASSOCIATED:
        if (args.size() != 1) {
            throw Exception ("Incorrect number of arguments for intrinsic (expected 1, got %d)", args.size()) ;
        }
        Value v = args[0]->evaluate (context) ;
        type = symtab->new_scalar_type ("boolean", DW_ATE_boolean, 1) ;
        return v.integer != 0 ;
        break ;
        }
    case IT_ADDR: {
        if (args.size() != 1) {
            throw Exception ("Incorrect number of arguments for intrinsic (expected 1, got %d)", args.size()) ;
        }
        bool saved = context.addressonly ;
        context.addressonly = true ;
        Value v = args[0]->evaluate (context) ;
        context.addressonly = saved ;
        type = symtab->new_pointer_type (args[0]->get_type()) ;
        return v ;
        }
    case IT_LOC:
	break;
    }
    throw Exception("not reached");
}


ArrayExpression::ArrayExpression (SymbolTable *symtab, Node *array, std::vector<Node*> & indices)
    : Node(symtab),
    array(array),
    indices(indices) {

}

ArrayExpression::~ArrayExpression() {
    delete array ;
    for (uint i = 0 ; i < indices.size() ; i++) {
        delete indices[i] ;
    }
}

int ArrayExpression::num_variables() {
    int n = array->num_variables() ;
    for (uint i = 0 ; i < indices.size() ; i++) {
        n += indices[i]->num_variables() ;
    }

    return n ;
}

bool ArrayExpression::is_local() {
    if (array != NULL && array->is_local()) {
        return true ;
    }
    for (uint i = 0 ; i < indices.size() ; i++) {
        if (indices[i]->is_local()) {
            return true ;
        }
    }
    return false ;
}

// get the value of a single array dimension index.  
Value ArrayExpression::get_dimension_value (EvalContext &context, TypeArray *die, int dim, Value &v, Node *index) {
        if (v.type != VALUE_INTEGER) {          // must be an address
            throw Exception ("Bad type for array dimension address") ;
        }
        if (index->get_opcode() == RANGE) {
            Expression *range = dynamic_cast<Expression*>(index) ;
            int leftindex ;     
            int rightindex ;
            std::vector<Dimension> &dims = die->get_dims (context) ;
            if (range->get_left() == NULL) {
                leftindex = dims[dim].low ;
            } else {
                leftindex = range->get_left()->evaluate (context) ;
            }
            if (range->get_right() == NULL) {
                rightindex = dims[dim].high ;
            } else {
                rightindex = range->get_right()->evaluate (context) ;
            }
            if (leftindex > rightindex) {
                int tmp = leftindex ;
                leftindex = rightindex ;
                rightindex = tmp ;
            }
            std::vector<Value> val ;
            for (int j = leftindex ; j <= rightindex ; j++) {
                Value newv = die->get_index (context, dim, v.integer, j) ;
                if (!context.addressonly && type->is_real() && !type->is_complex()) {
                    if (type->get_size() == 4) {                    // float?
                        newv.real = (double)(*(float*)&newv.real) ;
                    }
                    newv.type = VALUE_REAL ;
                }
                val.push_back (newv) ;
            }
            return val ;
        } else {
            int indexval = index->evaluate (context) ;// index value
            // index the array in dimension (dim).  The result will be an address of another dimension
            // or the value of the element
            Value newv = die->get_index (context, dim, v.integer, indexval) ;                  // value of array index
            // convert to real value if necessary
            if (!context.addressonly && type->is_real() && !type->is_complex()) {
                if (type->get_size() == 4) {                    // float?
                    newv.real = (double)(*(float*)&newv.real) ;
                }
                newv.type = VALUE_REAL ;
            }
            return newv ;
        }
}

static void check_array_index (int index, Value &v) {
    if (index < 0 || (uint) index >= v.vec.size()) {
        throw Exception ("Illegal array index: %d", index) ;
    }
}

bool ArrayExpression::split_indices (EvalContext &context, DIE *ltype, Value &lvalue, std::vector<Node*> &indices, Value &result) {
    if (ltype->is_array()) {
        TypeArray *array = dynamic_cast<TypeArray*>(ltype) ;
        uint ndims = array->get_num_dims() ;
        if (ndims < indices.size()) {
            // more indices than dimensions.  The first 'ndims' indices belong to the
            // array, the remainder are split
            std::vector<Node*> array_indices ;
            std::vector<Node*> other_indices ;
            for (uint i = 0 ; i < ndims ; i++) {
                array_indices.push_back (indices[i]) ;
                indices[i] = NULL ;                             // prevent multiple deletion
            }
            for (uint i = ndims ; i < indices.size() ; i++) {
                other_indices.push_back (indices[i]) ;
                indices[i] = NULL ;                             // prevent multiple deletion
            }
            ArrayExpression aex (symtab, new ValueConstant (symtab, lvalue, ltype), array_indices) ;
            Value v = aex.evaluate (context) ;
            type = aex.get_type() ;
            split_indices (context, aex.get_type(), v, other_indices, result) ;
            return true ;
        } else {
            return false ;
        }
    } else if (ltype->is_pointer()) {
        Node* left = new ValueConstant(symtab, lvalue, ltype);
        Node *plus = new Expression (symtab, PLUS, left, indices[0]) ;
        indices[0] = NULL ;    // prevent multiple deletion
        Expression ex (symtab, CONTENTS, plus) ;
        Value v = ex.evaluate (context) ;
        if (indices.size() > 1) {
            std::vector<Node*> other_indices ;
            for (uint i = 1 ; i < indices.size() ; i++) {
                other_indices.push_back (indices[i]) ;
                indices[i] = NULL ;
            }
            split_indices (context, ex.get_type(), v, other_indices, result) ;
            return true ;
        } else {
            result = v ;
            type = ex.get_type() ;
            return true ;
        }
    } else {
        throw Exception ("Cannot subscript this type") ;
    }
}


Value ArrayExpression::evaluate(EvalContext &context) {
    Value currentvalue = array->evaluate (context) ;// address of memory to be indexed (or vector of values)
    type = array->get_type() ;          // default to the type of the left

    if (type == NULL) {
        throw Exception ("Cannot subscript a void type") ;
    }
    if (!type->is_string() && !type->is_pointer() && !type->is_array() && !type->is_struct_deref()) {
        throw Exception ("Cannot subscript scalar variables") ; 
    }

    // if the left is a struct, check for operator()
    if (type->is_struct() && indices.size() == 1) {
        Value v = indices[0]->evaluate(context) ;
        // check for STL subscripting
        if (stl::subscript_struct (symtab, context, currentvalue, type, v.integer, currentvalue, type)) {
            return currentvalue ;
        }

        std::vector<DIE*> ops ;
        type->find_operator ("operator[]", ops) ;
        if (ops.size() > 0) {
            // call the operator[] with args (array, indices[0])
            std::vector<Node *> args ;
            args.push_back (new Expression (symtab, ADDRESS, new ValueConstant (symtab, currentvalue, type))) ;
            args.push_back (indices[0]) ;
            indices.clear() ;                       // stop deletion of indices[0]
            CallExpression call (symtab, new IdentifierSet (symtab, ops), args) ; 
            Value v = call.evaluate (context) ;
            type = call.get_type() ;
            return v ;
        }
    }

    if (type->is_struct()) {
        throw Exception ("Cannot subscript this struct - no operator[] defined") ;
    }

    // if we are subscripting an array or pointer then we might need to split
    // the indices up into proper array expressions and subscripts of pointers.
    // if the split_indices function returns true then the value of the last parameter
    // is the value of the expression.  The type of this node is set by split_indices
    if (type->is_array() || type->is_pointer()) {
        Value v ;
        if (split_indices (context, type, currentvalue, indices, v)) {
            return v ;
        }
    }

    // for strings, read 1 byte at the given address, or a string of them
    if (type->is_string()) {
        int len = type->get_real_size(context) ;
        int lb = 0 ;
        if (context.language == DW_LANG_Fortran77 || context.language == DW_LANG_Fortran90 || context.language == DW_LANG_Fortran95) {
            lb = 1 ;
        }
        if (indices[0]->get_opcode() == RANGE) {
            Expression *range = dynamic_cast<Expression*>(indices[0]) ;
            int leftindex ;
            int rightindex ;
            if (range->get_left() == NULL) {
                leftindex = 0 ;
            } else {
                leftindex = range->get_left()->evaluate (context) ;
            }
            if (range->get_right() == NULL) {
                rightindex = len - 1 ;
            } else {
                rightindex = range->get_right()->evaluate (context) ;
            }
            if (leftindex > rightindex) {
                int tmp = leftindex ;
                leftindex = rightindex ;
                rightindex = tmp ;
            }
            Value v = context.process->read_string (currentvalue.integer + leftindex - lb, rightindex - leftindex + 1) ;
            type = symtab->new_string_type (rightindex - leftindex + 1) ;
            return v ;
        } else {
            int index = indices[0]->evaluate (context) ;
        
            Value v =  context.process->read (currentvalue.integer + index - lb, 1) ;           // subtract the lower bound
            type = symtab->new_scalar_type ("character", DW_ATE_unsigned_char, 1) ;
            return v ;
        }
    }

    // if we are indexing an array, then the type of this expression is the type
    // of the array subscripted by all the indices
    if (type->is_array()) {
       TypeArray *die = (TypeArray*)type ;
       type = die->get_display_type (indices.size()) ;          // type of the array index expression
    }
    int elementsize = 0 ;
    TypeArray *die = NULL ;
    int maxdims = -1 ;

    elementsize = type->get_real_size(context) ;
    if (!array->is_vector()) {
        die = (TypeArray*)array->get_type() ;           
        maxdims = die->get_dims(context).size() ;
    }
    int dim = 0 ;// current dimension number

    for (uint i = 0 ; i < indices.size(); i++) {
        if (maxdims > 0 && dim > maxdims) {
            throw Exception ("Too many indices for array, max: %d", maxdims) ;
        }
        Node *index = indices[i] ;
        // check for a slice of an array.  This is not actually a dimension of the array and is
        // treated differently
        if (currentvalue.type == VALUE_VECTOR) {
            if (index->get_opcode() == RANGE) {
                Expression *range = dynamic_cast<Expression*>(index) ;
                int leftindex ;
                int rightindex ;
                if (range->get_left() == NULL) {
                    leftindex = 0 ;
                } else {
                    leftindex = range->get_left()->evaluate (context) ;
                }
                if (range->get_right() == NULL) {
                    rightindex = currentvalue.vec.size() - 1 ;
                } else {
                    rightindex = range->get_right()->evaluate (context) ;
                }
                if (leftindex > rightindex) {
                    int tmp = leftindex ;
                    leftindex = rightindex ;
                    rightindex = tmp ;
                }
                check_array_index (leftindex, currentvalue) ;
                check_array_index (rightindex, currentvalue) ;
                std::vector<Value> val ;
                for (int j = leftindex ; j <= rightindex ; j++) {
                    val.push_back (currentvalue.vec[j]) ;
                }
                currentvalue = val ;
            } else {
                int indexval = index->evaluate(context) ;
                check_array_index (indexval, currentvalue) ;
                currentvalue = currentvalue.vec[indexval] ;
            }
        } else {
            if (die == NULL) {
                throw Exception ("Invalid array subscript expression") ;
            }
            currentvalue = get_dimension_value (context, die, dim, currentvalue, index) ;
            dim++ ;
        }
    }
    if (die != NULL) {
        type = die->get_display_type(dim) ;
    }
    return currentvalue ;          // might be a value if all the dimensions are consumed
}

void ArrayExpression::set_value (EvalContext &context, Value &value, DIE *exprtype) {
    bool save = context.addressonly ;
    context.addressonly = true ;
    Value addr = this->evaluate (context) ;                // get address of element to set
    if (addr.type != VALUE_INTEGER) {      
        throw Exception ("Address expected for ArrayExpression::set_value()") ;
    }
    exprtype->set_value (context, addr, value) ;
    context.addressonly = save ;
}


ExpressionHandler::ExpressionHandler (SymbolTable *symtab, Architecture *arch, int lang)
    : flags(0),
      symtab(symtab),
      line(""),
      ch(0),
      spelling(""),
      number(0),
      currentToken(NONE),
      current_process(NULL), 
      arch(arch),
      language(lang)
{
}

ExpressionHandler::~ExpressionHandler() {
}

int ExpressionHandler::ptrsize() {
   return arch == NULL ? 8 : arch->ptrsize() ;
}

void ExpressionHandler::error(std::string s) {
    throw s ;
}

void ExpressionHandler::error(const char *format, ...) {
    va_list ap ;
    va_start (ap, format) ;
    throw Exception (format, ap); 
}

void ExpressionHandler::warning(std::string s) {
    std::cout << "Warning: "  <<  s << '\n' ;
}


void ExpressionHandler::skip_spaces() {
    while (ch < line.size() && isspace (line[ch])) ch++ ;
}


void ExpressionHandler::nextToken() {
    currentToken = getNextToken() ;
}

bool ExpressionHandler::match(Token tok) {
    if (currentToken == tok) {
        nextToken() ;
        return true ;
    }
    return false ;
}

void ExpressionHandler::needbrack(Token b) {
    if (currentToken != b) {
        char bra = '?';
        switch (b) {
        case RPAREN:
            bra = ')' ;
            break;
        case RSQUARE:
            bra = ']' ;
            break;
	default:
	    break; // added by bos for -Wall niceness
        }
        error (std::string("'") + bra + "' expected") ;
    } else {
        nextToken() ;
    }
}

Node *ExpressionHandler::parse(std::string expr, Process *process) {
    int end ;
    return parse (expr, process, end) ;
}

Node *ExpressionHandler::parse(std::string expr, Process *process, int &end) {
    flags = 0 ;
    current_process = process ;

    line = expr ;
    ch = 0 ;
    lastch = ch ;
    nextToken() ;

    try {
        Node *expr = expression() ;
        end = lastch ;
        return expr ;
    } catch (const char *s) {
        std::cout << s << '\n' ;
        end = lastch ;
        return NULL ;
    } catch (std::string s) {
        std::cout << s << '\n' ;
        end = lastch ;
        return NULL ;
    }
}

Node *ExpressionHandler::parse_single(std::string expr, Process *process, int &end) {
    flags = 0 ;
    current_process = process ;

    line = expr ;
    ch = 0 ;
    lastch = ch ;
    nextToken() ;

    try {
        Node *expr = single_expression() ;
        end = lastch ;
        return expr ;
    } catch (const char *s) {
        std::cout << s << '\n' ;
        end = lastch ;
        return NULL ;
    } catch (std::string s) {
        std::cout << s << '\n' ;
        end = lastch ;
        return NULL ;
    }
}


//
// C language expression symtab.
//


CExpressionHandler::CExpressionHandler (SymbolTable*symtab, Architecture *arch, int language)
    : ExpressionHandler(symtab, arch, language) {

    reserved_words["sizeof"] = SIZEOF ;
    reserved_words["int"] = INT ;
    reserved_words["char"] = CHAR ;
    reserved_words["short"] = SHORT ;
    reserved_words["long"] = LONG ;
    reserved_words["unsigned"] = UNSIGNED ;
    reserved_words["signed"] = SIGNED ;
    reserved_words["float"] = FLOAT ;
    reserved_words["double"] = DOUBLE ;
    reserved_words["void"] = VOID ;
    reserved_words["struct"] = STRUCT ;
    reserved_words["union"] = UNION ;
    reserved_words["class"] = CLASS ;
    reserved_words["enum"] = ENUM ;
    reserved_words["const"] = KONST ;
    reserved_words["volatile"] = VOLATILE ;
    reserved_words["bool"] = BOOL ;
}

CExpressionHandler::~CExpressionHandler() {
}

bool CExpressionHandler::istoken(char ch) {
    char tokens[] = { '=', '<', '>', '-', '+', '*', '%', '/', '^', '~', '!', '?', ':', '&', '|', '.', '[', ']', '(', ')', ',', '@', '{', '}'} ;
    for (uint i = 0 ; i < sizeof(tokens) ; i++) {
        if (tokens[i] == ch) {
            return true ;
        }
    }
    return false ;
}

Token CExpressionHandler::getNextToken() {
    lastch = ch ;
    skip_spaces() ;
    if (ch >= line.size()) {
        return NONE ;
    }
    if (isalpha (line[ch]) || line[ch] == '_') {
        spelling = "" ;
        while (ch < line.size() && (isalnum (line[ch]) || line[ch] == '_')) {
            spelling += line[ch++] ;
        }
        KeywordMap::iterator kw = reserved_words.find (spelling) ;
        if (kw != reserved_words.end()) {
            return kw->second ;
        }
        return IDENTIFIER ;
    } else if (line[ch] == '$') {                       // register or debugger variable?
        ch++ ;
        spelling = "" ;
        if (line[ch] == '$') {
            spelling += '$' ;
            ch++ ;
        }
        while (ch < line.size() && (isalnum (line[ch]) || line[ch] == '_')) {
            spelling += line[ch++] ;
        }
        if (arch != NULL) {
			// FIXME: Should search FPU registers too?
			int num = arch->main_register_set_properties()->register_number_for_name(spelling);
			if (RegisterSetProperties::invalid_register != num)
			{
				return REGISTERNAME;
			}
        }
        spelling = "$" + spelling ;
        return DEBUGGERVAR ;
    } else if  (isdigit (line[ch])) {
        number = 0 ;
        if (line[ch+1] == 'x' || line[ch+1] == 'X') {           // hex number?
            ch += 2 ;
            while (ch < line.size() && isxdigit (line[ch])) {
                int x = toupper(line[ch]) >= 'A' ? toupper(line[ch]) - 'A' + 10 : line[ch] - '0' ;
                number = (number << 4) | x ;
                ch++ ;
            }
        } else {
            bool found_dot = false ;
            bool found_exp = false ;
            char num_buffer[1024] ;
            char *p = num_buffer ;

            while (ch < line.size()) {
                if (line[ch] == '.' && !found_dot) {
                    found_dot = true ;
                    *p++ = line[ch++] ;
                } else if ((line[ch] == 'e' || line[ch] == 'E') && !found_exp) {
                    *p++ = line[ch++] ;
                    if (line[ch] == '+' || line[ch] == '-') {
                        *p++ = line[ch++] ;
                    }
                    found_exp = true ;
                    found_dot = true ;
                } else if (isdigit (line[ch])) {
                    *p++ = line[ch++] ;
                } else {
                    break ;
                }
            }
            *p = 0 ;
            if (found_dot) {                    // floating point number?
                fpnumber = strtod (num_buffer, NULL) ;
                return FPNUMBER ;
            } else {
                number = strtoll (num_buffer, NULL, 0) ;
            }
        }
        return NUMBER ;
    } else if  (istoken (line[ch])) {
        switch (line[ch]) {
        case '+': 
            ch++ ; 
            if (line[ch] == '=') {
                return ch++, PLUSEQ ; 
            }
            if (line[ch] == '+') {
                 return ch++, PLUSPLUS ;
            }
            return PLUS ;
            break ;
        case '*': ch++ ; if (line[ch] == '=') return ch++, STAREQ ; return STAR ;
        case '/': ch++ ; if (line[ch] == '=') return ch++, SLASHEQ ; return SLASH ;
        case '%': ch++ ; if (line[ch] == '=') return ch++, PERCENTEQ ; return MOD ;
        case '^': ch++ ; if (line[ch] == '=') return ch++, XOREQ ; return BITXOR ;
        case '~': ch++ ; return ONESCOMP ;
        case '?': ch++ ; return QUESTION ;
        case ':': ch++ ; if (line[ch] == ':') return ch++, COLONCOLON ; return COLON ;
        case '.': ch++ ; return MEMBER ;
        case ']': ch++ ; return RSQUARE ;
        case '[': ch++ ; return LSQUARE ;
        case '}': ch++ ; return RBRACE ;
        case '{': ch++ ; return LBRACE ;
        case '(': ch++ ; return LPAREN ;
        case ')': ch++ ; return RPAREN ;
        case ',': ch++ ; return COMMA ;
        case '@': ch++ ; return AT ;

        case '=':
            ch++ ;
            if (line[ch] == '=') {
                ch++ ;
                return EQUAL ;
            }
            return ASSIGN ;
            break ;
        case '!':
            ch++ ;
            if (line[ch] == '=') {
                ch++ ;
                return NOTEQUAL ;
            }
            return NOT ;
        case '<':
            ch++ ;
            if (line[ch] == '=') {
                ch++ ;
                return LESSEQ ;
            } else if  (line[ch] == '<') {
                ch++ ;
                if (line[ch] == '=') {
                    ch++ ;
                    return LSHIFTEQ ;
                }
                return LSHIFT ;
            }
            return LESS ;
        case '>':
            ch++ ;
            if (line[ch] == '=') {
                ch++ ;
                return GREATEREQ ;
            } else if  (line[ch] == '>') {
                ch++ ;
                if (line[ch] == '=') {
                    ch++ ;
                    return RSHIFTEQ ;
                }
                return RSHIFT ;
            }
            return GREATER ;
        case '-':
            ch++ ;
            if (line[ch] == '>') {
                ch++ ;
                return ARROW ;
            }
            if (line[ch] == '=') {
                return ch++, MINUSEQ ;
            }
            if (line[ch] == '-') {
                return ch++, MINUSMINUS ;
            }
            return MINUS ;
        case '&':
            ch++ ;
            if (line[ch] == '&') {
                ch++ ;
                return LOGAND ;
            } else if (line[ch] == '=') {
                ch++ ;
                return ANDEQ ;
            }
            return BITAND ;
        case '|':
            ch++ ;
            if (line[ch] == '|') {
                ch++ ;
                return LOGOR ;
            } else if (line[ch] == '=') {
                ch++ ;
                return OREQ ;
            }
            return BITOR ;
        }
    } else if  (line[ch] == '"' || line[ch] == '\'') {
        spelling = "" ;
        char term = line[ch] ;
        ch++ ;
        while (ch < line.size() && line[ch] != term) {
            if (line[ch] == '\\') {
                ch++ ;
                switch (line[ch]) {
                case 'n':
                    spelling += '\n' ;
                    break ;
                case 'r':
                    spelling += '\r' ;
                    break ;
                case 'a':
                    spelling += '\a' ;
                    break ;
                default:
                    spelling += line[ch] ;
                }
            } else {
                spelling += line[ch] ;
            }
            ch++ ;
        }
        if (ch == line.size()) {
            error ("Unterminated string/character constant") ;
        }
        ch++ ;
        if (term == '\'' && spelling.size() == 1) {
            number = (int)spelling[0] ;
            return CHARCONST ;
        }
        return STRING ;
    } else {
        error ("Illegal character") ;
        ch++ ;
    }
    throw Exception("not reached");
}


DIE *CExpressionHandler::parse_typespec() {
    TypeStack stack ;
    DIE *t = type() ;
    declaration (stack) ;

    while (!stack.empty()) {
       DIE *decl = stack.top() ;
       decl->addAttribute (DW_AT_type, t) ;
       t = decl ;
       stack.pop() ;
    }
    return t ;
}

// parse a sequence of type keywords and build a DIE for it
DIE *CExpressionHandler::type() {
    bool found_unsigned = false ;
    bool found_signed = false ;
    bool found_int = false ;
    bool found_short = false ;
    bool found_const = false ;
    bool found_volatile = false ;
    bool found_char = false ;
    bool found_long = false ;
    bool found_float = false ;
    bool found_double = false ;
    bool found_long_long = false ;
    bool found_bool = false ;
    bool done = false ;
    bool found_error = false ;
    while (!done && !found_error) {
        switch (currentToken) {
        default:
            done = true ;
            break ;
        case INT:
            if (found_bool || found_float || found_double || found_int) {
                found_error = true ;
                break ;
            }
            found_int = true ;
            nextToken() ;
            break ;
        case LONG:
            if (found_bool || found_float || found_long) {
                found_long_long = true ;
            } else if (found_long_long) {
                found_error = true ;
                break ;
            }
            nextToken() ;
            found_long = true ;
            break ;
        case CHAR:
            if (found_bool || found_float || found_double || found_char || found_int || found_long || found_short) {
                found_error = true ;
                break ;
            }
            nextToken() ;
            found_char = true ;
            break ;
        case SHORT:
            if (found_bool || found_char || found_long || found_short) {
                found_error = true ;
                break ;
            }
            nextToken() ;
            found_short = true ;
            break ;
        case BOOL:
            if (found_bool || found_float || found_char || found_int || found_double || found_long || found_short) {
                found_error = true ;
                break ;
            }
            nextToken() ;
            found_bool = true ;
            break ;
        case FLOAT:
            if (found_bool || found_float || found_char || found_int || found_double || found_long || found_short) {
                found_error = true ;
                break ;
            }
            nextToken() ;
            found_float = true ;
            break ;
        case DOUBLE:
            if (found_bool || found_float || found_char || found_int || found_double || found_short) {
                found_error = true ;
                break ;
            }
            nextToken() ;
            found_double = true ;
            break ;
        case UNSIGNED:
            if (found_bool || found_unsigned || found_signed || found_float || found_double) {
                found_error = true ;
                break ;
            }
            nextToken() ;
            found_unsigned = true ;
            break ;
        case SIGNED:
            if (found_bool || found_unsigned || found_signed || found_float || found_double) {
                found_error = true ;
                break ;
            }
            nextToken() ;
            found_signed = true ;
            break ;
        case KONST:
            if (found_const) {
                found_error = true ;
                break ;
            }
            nextToken() ;
            found_const = true ;
            break ;
        case VOLATILE:
            if (found_volatile) {
                found_error = true ;
                break ;
            }
            nextToken() ;
            found_volatile = true ;
            break ;
        case VOID:
            if (found_bool || found_int || found_char || found_short || found_long || found_signed ||
                        found_unsigned || found_const || found_volatile ) {
                found_error = true ;
                break ;
            }
            nextToken() ;
            return NULL ;                       // void has NULL DIE
            break ;
        case STRUCT:
        case CLASS:
        case UNION:
        case ENUM: {
            Token tok = currentToken ;
            if (found_bool || found_int || found_char || found_short || found_long || found_signed ||
                        found_unsigned ) {
                found_error = true ;
                break ;
            }
            nextToken() ;
            if (currentToken != IDENTIFIER) {
                error ("Need an identifier for a struct/union or enum") ;
            } else {
                std::string name = spelling ;
                nextToken() ;
                DIE *sym = current_process->find_scope (name, false) ;
                if (sym == NULL) {
                    error ("No such struct/union/enum tag: %s", name.c_str()) ;
                } else {
                    int tag = sym->get_tag() ;
                    switch (tok) {
                    case STRUCT:
                    case CLASS:
                        if (tag != DW_TAG_structure_type &&
                            tag != DW_TAG_class_type) {
                            error ("Need a structure tag") ;
                        }
                        break ;
                    case UNION:
                        if (tag != DW_TAG_union_type) {
                            error ("Need a union tag") ;
                        }
                        break ;
                    case ENUM:
                        if (tag != DW_TAG_enumeration_type) {
                            error ("Need an enum tag") ;
                        }
                        break ;
		    default:
			break; // added by bos for -Wall niceness
                    }
                    if (found_const) {
                        sym = symtab->new_const_type (sym) ;
                    }
                    if (found_volatile) {
                        sym = symtab->new_volatile_type (sym) ;
                    }
                    return sym ;                // all ok, return DIE found
                }
            }
            break ;
            }
        case IDENTIFIER: {
            if (found_bool || found_int || found_char || found_short || found_long || found_signed ||
                        found_unsigned || found_const || found_volatile) {
                found_error = true ;
                break ;
            }
            std::string name = spelling ;
            SymbolValue val = current_process->find_symbol (name, 0) ;
            if (val.type != SV_DIEVEC) {
                done = true ;
            } else {
                DIE *sym = val.dievec[0] ;              // XXX: present menu
                int tag = sym->get_tag() ;
                if (tag != DW_TAG_typedef) {
                    throw false ;               // not a typedef
                }
                nextToken() ;
                if (found_const) {
                    sym = symtab->new_const_type (sym) ;
                }
                if (found_volatile) {
                    sym = symtab->new_volatile_type (sym) ;
                }
                return sym ;
            }
            break ;
            }
        }
    }

    // if we didn't find a type we have to tell caller.  We can't return NULL because that means
    // we found void.  Throw a boolean exception
    if (!(found_int || found_char || found_short || found_float || found_double || found_long || found_unsigned || found_signed
             || found_const || found_volatile)) {
        throw false ;
    }

    if (found_error) {
        error ("Malformed type specification") ;
    }
    // if we get here we have a base type with possible qualifiers
    int encoding = 0 ;
    std::string name = "int" ;
    int size = 0 ;
    bool int_encoding = false ;

    if (found_long_long) {
        size = 8 ;
        int_encoding = true ;
        name = "long long" ;
    } else if (found_long) {
        size = arch->ptrsize() ;
        int_encoding = true ;
        name = "long" ;
    } else if (found_bool) {
        size = 1 ;
        name = "bool" ;
        encoding = DW_ATE_boolean ;
    } else if (found_short) {
        size = 2 ;
        int_encoding = true ;
        name = "short" ;
    } else if (found_char) {
        size = 1 ;
        if (found_unsigned) {
            encoding = DW_ATE_unsigned_char ;
            name = "unsigned char" ;
        } else if (found_signed) {
            encoding = DW_ATE_signed_char ;
            name = "signed char" ;
        } else {
            encoding = DW_ATE_signed_char ;
            name = "char" ;
        }
    } else if (found_float) {
        size = 4 ;
        encoding = DW_ATE_float ;
        name = "float" ;
    } else if (found_double) {
        size = 8 ;
        encoding = DW_ATE_float ;
        name = "double" ;
    } else {
        size = 4 ;
        int_encoding = true ;
    }
    if (int_encoding) {
        if (found_unsigned) {
            name = "unsigned " + name ;
            encoding = DW_ATE_unsigned ; 
        } else if (found_signed) {
            name = "signed " + name ;
            encoding = DW_ATE_signed ; 
        } else {
            encoding = DW_ATE_signed ;
        }
    }

    if (encoding == 0) {
        error ("Malformed type specification") ;
    }

    DIE *die = symtab->new_scalar_type (name, encoding, size) ;
    if (found_const) {
        die = symtab->new_const_type (die) ;
    }
    if (found_volatile) {
        die = symtab->new_volatile_type (die) ;
    }
    return die ;
}

// parse the base of a type construct
void CExpressionHandler::base(TypeStack &stack) {
    if (match (LPAREN)) {
        declaration(stack) ;
        needbrack (RPAREN) ;
    }
}

// parse a function or array
void CExpressionHandler::funcarray(TypeStack &stack) {
    base(stack) ;
    if (match (LPAREN)) {
        DIE *subroutine = symtab->new_subroutine_type() ;
        while (currentToken != RPAREN) {
            DIE *type = parse_typespec() ;
            subroutine->addChild (symtab->new_formal_parameter (type)) ;
            if (!match (COMMA)) {
                break ;
            }
        }
        needbrack (RPAREN) ;
        stack.push (subroutine) ;
    } else if (match (LSQUARE)) {
        std::vector<DIE*> dims ;
        for (;;) {
            if (currentToken == NUMBER) {
                int dim = number ;
                nextToken() ;
                DIE *dimdie = symtab->new_subrange_type (0, dim-1) ;
                dims.push_back (dimdie) ;
            } else {
                if (currentToken == RSQUARE) {
                    DIE *dimdie = symtab->new_subrange_type() ;
                    dims.push_back (dimdie) ;
                } else {
                    error ("Invalid array dimension") ;
                }
            }
            needbrack (RSQUARE) ;
            if (!match (LSQUARE)) {
                break ;
            }
        }
        DIE *array = symtab->new_array_type (NULL) ;
        for (uint i = 0 ; i < dims.size() ; i++) {
            array->addChild (dims[i]) ;
        }
        stack.push (array) ;
    }
}

// parse a pointer construct
void CExpressionHandler::pointer(TypeStack &stack) {
    if (match (STAR)) {
        pointer(stack) ;
        stack.push (symtab->new_pointer_type (NULL)) ;
    } else {
        funcarray(stack) ;
    }
}

// parse a declaration
void CExpressionHandler::declaration(TypeStack &stack) {
    pointer(stack) ;
}



Node *CExpressionHandler::primary() {
    if (found_lparen || match (LPAREN)) {
        Node *expr = expression() ;
        needbrack (RPAREN) ;
        return expr ;
    } else if  (currentToken == IDENTIFIER || currentToken == COLONCOLON) {
        bool topdown = currentToken == COLONCOLON ;
        if (topdown) {
            nextToken() ;
            if (currentToken != IDENTIFIER) {
                error ("Expected identifier after :: operator") ;
            }
        }
        std::string name = spelling ;
        nextToken() ;
        Address reqpc = 0 ;

        // lexical scoping operator
        if (match (AT)) {
            if (currentToken == NUMBER) {
                reqpc = current_process->lookup_line (number) ;
                nextToken() ;
            } else {
                error ("Expected @line for lexical scoping operator") ;
            }
        }
        std::vector<DIE*> syms ;

        if (currentToken == COLONCOLON) {
            DIE *sym = current_process->find_scope (name, topdown) ;
            if (sym == NULL) {
                error ("No symbol \"" + name + "\" in current context.") ;
            }
            syms.push_back (sym) ;
        } else {
            SymbolValue val = current_process->find_symbol (name, reqpc) ;
            if (val.type == SV_ADDRESS) {
                return new IntConstant (symtab, (Address)val, ptrsize()) ;
            } else {
                if (val.type  == SV_NONE) {
                    // the symbol is not found directly.  Let's try looking at 'this'.
                    // pathCC has a bug (#3572) that omits a constructor from the containing class.  This 
                    // code works around this bug
                    if (language == DW_LANG_C_plus_plus) {
                        SymbolValue thisval = current_process->find_symbol ("this") ;
                        if (thisval.type == SV_DIEVEC) {
                            DIE *thisdie = thisval.dievec[0] ;
                            while (thisdie != NULL && thisdie->get_tag() != DW_TAG_structure_type &&
                               thisdie->get_tag() != DW_TAG_class_type) {
                                thisdie = thisdie->get_type() ;
                            }
                            if (thisdie != NULL) {
                                bool found = false ;
                                try {
                                    DIE *member = thisdie->find_member(name) ;
                                    if (member != NULL) {
                                        found = true ;
                                    }
                                } catch (DIE *d) {
                                    found = true ;                      // only get here is it is found in base class
                                }
                                if (found) {
                                    return new MemberExpression (symtab, new Expression (symtab, CONTENTS, new Identifier (symtab, thisval.dievec[0])), name) ;
                                }
                            }
                        }
                    }
                    error ("No symbol \"" + name + "\" in current context.") ;
                } 
                syms = val.dievec ;
            }
        }
        if (syms[0]->get_tag() == DW_TAG_member || syms[0]->is_member_function()) {               // directly to a member means we have to use this
            SymbolValue thisval = current_process->find_symbol ("this") ;
            if (thisval.type == SV_NONE) {
                error ("Cannot find 'this' variable.") ;
            } else {        
                return new MemberExpression (symtab, new Expression (symtab, CONTENTS, new Identifier (symtab, thisval.dievec[0])), name) ;
            }
        } else {
            std::string context = name ;
            DIE *sym = syms[0] ;                // for the time being
            while (match (COLONCOLON)) {
                context += "::" ;
                if (currentToken == IDENTIFIER) {
                    name = spelling ;
                    nextToken() ;
                    if (match (AT)) {
                        if (currentToken == NUMBER) {
                            reqpc = current_process->lookup_line (number) ;
                            nextToken() ;
                        } else {
                            error ("Expected @line for lexical scoping operator") ;
                        }
                    }
                    if (currentToken == COLONCOLON) {
                        sym = sym->find_scope (name) ;
                    } else {
                        sym = sym->find_symbol (name, reqpc) ;
                    }
                    if (sym == SV_NONE) {
                        error ("No symbol \"%s\" in context %s", name.c_str(), context.c_str()) ;
                    }
                    context += name ;
                } else {
                    error ("Expected identifier after :: operator") ;
                }
            }
            if (sym != syms[0]) {
                syms[0] = sym ;
            }
            if (syms.size() == 1) {
                return new Identifier (symtab, syms[0]) ;
            } else {
                return new IdentifierSet (symtab, syms) ;
            }
        }
    } else if (currentToken == REGISTERNAME) {
        std::string regname = spelling ;
        nextToken() ;
        return new RegisterExpression (symtab, regname) ;
    } else if (currentToken == DEBUGGERVAR) {
        std::string name = spelling ;
        nextToken() ;
        DebuggerVar *var = current_process->find_debugger_variable (name) ;
        if (var == NULL) {
            if (isdigit (name[0])) {
                char buf[100] ;
                snprintf (buf, sizeof(buf), "History has not yet reached $%s.", name.c_str()) ;
                error (buf) ; 
            }
        }
        return new DebuggerVarExpression (symtab, name, var) ;
    } else if  (currentToken == NUMBER) {
        int64_t n = number ;
        nextToken() ;
        return new IntConstant (symtab, n, ptrsize()) ;
    } else if  (currentToken == FPNUMBER) {
        double n = fpnumber ;
        nextToken() ;
        return new RealConstant (symtab, n, 8) ;
    } else if  (currentToken == STRING) {
        std::string s = spelling ;
        nextToken() ;
        if (match (COLONCOLON)) {
            // s is a file name
            DIE *sym = current_process->find_compilation_unit (s) ;
            if (sym == NULL) {
                error ("Unable to find file %s in scoping operation", s.c_str()) ;
            }
            Address reqpc = 0 ;
            std::string context = std::string ("'") + s + "'" ;
            for (;;) {
                context += "::" ;
                if (currentToken == IDENTIFIER) {
                    s = spelling ;
                    nextToken() ;
                    if (match (AT)) {
                        if (currentToken == NUMBER) {
                            reqpc = current_process->lookup_line (number) ;
                            nextToken() ;
                        } else {
                            error ("Expected @line for lexical scoping operator") ;
                        }
                    }
                    sym = sym->find_symbol (s, reqpc) ;
                    if (sym == SV_NONE) {
                        error ("No symbol \"%s\" in context %s", s.c_str(), context.c_str()) ;
                    }
                    context += s ;
                } else {
                    error ("Expected identifier after :: operator") ;
                }
                if (!match (COLONCOLON)) {
                    break ;
                }
            }
            return new Identifier (symtab, sym) ;
        } else {
            // string concatenation
            while (currentToken == STRING) {
                s += spelling ;
                nextToken() ;
            }
            return new StringConstant (symtab, s) ;
        }
    } else if  (currentToken == CHARCONST) {
        char n = number ;
        nextToken() ;
        return new CharConstant (symtab, n) ;
    } else if (currentToken == LBRACE) {                // vector literal
        nextToken() ;
        std::vector<Node *> v ;
        try {
            while (currentToken != RBRACE) {
                Node *expr = expression() ;
                v.push_back (expr) ;
                if (!match (COMMA)) {
                    break ;
                }
            }
            needbrack (RBRACE) ;
        } catch (...) {
            for (uint i = 0 ; i < v.size() ; i++) {
                delete v[i] ;
            }
            throw ;
        }
        return new VectorExpression (symtab, v) ;
    } else {
        try {           // allow a type specification
            DIE *die = parse_typespec() ;
            return new CastExpression (symtab, new IntConstant (symtab, 0, 4), die) ;
        } catch (bool b) {
            error ("Expression syntax error") ;
        }
    }
    throw Exception("not reached");
}


Node *CExpressionHandler::postfix() {
    Node *left = primary() ;
    try {
        for (;;) {
            if (match (LPAREN)) {           // function call
                std::vector<Node*> args ;
                // now collect the real args
                try {
                    while (currentToken != RPAREN) {
                        Node *arg = single_expression() ;
                        args.push_back (arg) ;
                        if (!match (COMMA)) {
                            break ;
                        }
                    }
                    needbrack (RPAREN) ;
                } catch (...) {
                    for (uint i = 0 ; i < args.size() ; i++) {
                        delete args[i] ;
                    }
                    throw ;
                }
                left = new CallExpression (symtab, left, args) ;
            } else if  (match (LSQUARE)) {
                std::vector<Node *> indices ;
                try {
                    for (;;) {                      // collect the indices
                        if (match (COLON)) {
                            nextToken() ;
                            if (currentToken != RSQUARE) {
                                indices.push_back (new Expression (symtab, RANGE, NULL, expression())) ;
                            } else {
                                indices.push_back (new Expression (symtab, RANGE, NULL, NULL)) ;
                            }
                        } else {
                            Node *expr = expression() ;
                            if (match (COLON)) {
                                if (currentToken == RSQUARE) {
                                    expr = new Expression (symtab, RANGE, expr, NULL) ;
                                } else {
                                    expr = new Expression (symtab, RANGE, expr, expression()) ;
                                }
                            }
                            indices.push_back (expr) ;
                        }
                        needbrack (RSQUARE) ;
                        if (currentToken != LSQUARE) {
                           break ;
                        }
                        nextToken() ;         // consume the [
                    }
                } catch (...) {
                    for (uint i = 0 ; i < indices.size() ; i++) {
                        delete indices[i] ;
                    }
                    throw ;
                }

                left = new ArrayExpression (symtab, left, indices) ;
            
            } else if  (match (MEMBER)) {
                if (currentToken == IDENTIFIER) {
                    std::string membername = spelling ;
                    nextToken() ;
                    while (match (COLONCOLON)) {
                        if (currentToken == IDENTIFIER) {
                            membername += "::" + spelling ;
                            nextToken() ;
                        } else {
                            error ("Identifier expected after :: operator") ;
                        }
                    }
                    left = new MemberExpression (symtab, left, membername) ;
                } else {
                    error ("struct/union member name expected") ;
                }
            } else if  (match (ARROW)) {
                if (currentToken == IDENTIFIER) {
                    std::string membername = spelling ;
                    nextToken() ;
                    while (match (COLONCOLON)) {
                        if (currentToken == IDENTIFIER) {
                            membername += "::" + spelling ;
                            nextToken() ;
                        } else {
                            error ("Identifier expected after :: operator") ;
                        }
                    }
                    left = new MemberExpression (symtab, new Expression (symtab, CONTENTS, left), membername) ;
                } else {
                    error ("struct/union member name expected") ;
                }
            } else if (match (PLUSPLUS)) {
                left = new Expression (symtab, POSTINC, left) ;
            } else if (match (MINUSMINUS)) {
                left = new Expression (symtab, POSTDEC, left) ;
            } else {
                return left ;
            }
        }
    } catch (...) {
        delete left ;
        throw ;
    }
}

Node *CExpressionHandler::unary() {
    if (match (MINUS)) {
         Node *left = unary() ;
         return new Expression (symtab,UMINUS, left) ;
    } else if  (match (PLUS)) {
         Node *left = unary() ;
         return new Expression (symtab,UPLUS, left) ;
    } else if  (match (STAR)) {
         Node *left = unary() ;
         return new Expression (symtab,CONTENTS, left) ;
    } else if  (match (BITAND)) {
         Node *left = unary() ;
         return new Expression (symtab,ADDRESS, left) ;
    } else if  (match (NOT)) {
         Node *left = unary() ;
         return new Expression (symtab,NOT, left) ;
    } else if  (match (ONESCOMP)) {
         Node *left = unary() ;
         return new Expression (symtab,ONESCOMP, left) ;
    } else if  (match (PLUSPLUS)) {
         Node *left = unary() ;
         return new Expression (symtab,PLUSPLUS, left) ;
    } else if  (match (MINUSMINUS)) {
         Node *left = unary() ;
         return new Expression (symtab,MINUSMINUS, left) ;
    } else if  (match (SIZEOF)) {
         if (match (LPAREN)) {
             try {
                 DIE *die = parse_typespec() ;
                 needbrack (RPAREN) ;
                 return new SizeofExpression (symtab, die) ;
             } catch (bool b) {
                 Node *left = expression() ;
                 needbrack (RPAREN) ;
                 return new Expression (symtab,SIZEOF, left) ;
             }
         } else {
             try {
                 DIE *die = parse_typespec() ;
                 return new SizeofExpression (symtab, die) ;
             } catch (bool b) {
                 error ("Bad sizeof operand: expected type specification") ;
             }
         }
    } else if (match (LPAREN)) {
        try {
            DIE *die = parse_typespec() ;
            needbrack (RPAREN) ;
            Node *left = expression() ;
            return new CastExpression (symtab, left, die) ;
        } catch (bool b) {
            // no typespec
            found_lparen = true ;
            return postfix() ;
        }
    } else {
         return postfix() ;
    }
    throw Exception("not reached");
}

Node *CExpressionHandler::mult() {
    Node *left = unary() ;
    try {
        for (;;) {
            if (match (STAR)) {
                Node *right = unary() ;
                left = new Expression (symtab,STAR, left, right) ;
            } else if  (match (SLASH)) {
                Node *right = unary() ;
                left = new Expression (symtab,SLASH, left, right) ;
            } else if  (match (MOD)) {
                Node *right = unary() ;
                left = new Expression (symtab,MOD, left, right) ;
            } else {
                return left ;
            }
        }
    } catch (...) {
        delete left ;
        throw ;
    }
}

Node *CExpressionHandler::add() {
    Node *left = mult() ;
    try {
        for (;;) {
            if (match (PLUS)) {
                Node *right = mult() ;
                left = new Expression (symtab,PLUS, left, right) ;
            } else if  (match (MINUS)) {
                Node *right = mult() ;
                left = new Expression (symtab,MINUS, left, right) ;
            } else {
                return left ;
            }
        }
    } catch (...) {
        delete left ;
        throw ;
    }
}

Node *CExpressionHandler::shift() {
    Node *left = add() ;
    try {
        for (;;) {
            if (match (LSHIFT)) {
                Node *right = add() ;
                left = new Expression (symtab,LSHIFT, left, right) ;
            } else if  (match (RSHIFT)) {
                Node *right = add() ;
                left = new Expression (symtab,RSHIFT, left, right) ;
            } else {
                return left ;
            }
        }
    } catch (...) {
        delete left ;
        throw ;
    }
}

Node *CExpressionHandler::comparison() {
    Node *left = shift() ;
    try {
        for (;;) {
            if (match (LESS)) {
                Node *right = shift() ;
                left = new Expression (symtab,LESS, left, right) ;
            } else if  (match(LESSEQ)) {
                Node *right = shift() ;
                left = new Expression (symtab,LESSEQ, left, right) ;
            } else if  (match(GREATER)) {
                Node *right = shift() ;
                left = new Expression (symtab,GREATER, left, right) ;
            } else if  (match(GREATEREQ)) {
                Node *right = shift() ;
                left = new Expression (symtab,GREATEREQ, left, right) ;
            } else {
                return left ;
            }
        }
    } catch (...) {
        delete left ;
        throw ;
    }
}

Node *CExpressionHandler::equality() {
    Node *left = comparison() ;
    try {
        for (;;) {
            if (match (EQUAL)) {
                Node *right = comparison() ;
                left = new Expression (symtab,EQUAL, left, right) ;
            } else if  (match (NOTEQUAL)) {
                Node *right = comparison() ;
                left = new Expression (symtab,NOTEQUAL, left, right) ;
            } else {
                return left ;
            }
        }
    } catch (...) {
        delete left ;
        throw ;
    }
}

Node *CExpressionHandler::bit_and() {
    Node *left = equality() ;
    try {
        for (;;) {
            if (match (BITAND)) {
                Node *right = equality() ;
                left = new Expression (symtab,BITAND, left, right) ;
            } else {
                return left ;
            }
        }
    } catch (...) {
        delete left ;
        throw ;
    }
}

Node *CExpressionHandler::bit_xor() {
    Node *left = bit_and() ;
    try {
        for (;;) {
            if (match (BITXOR)) {
                Node *right = bit_and() ;
                left = new Expression (symtab,BITXOR, left, right) ;
            } else {
                return left ;
            }
        }
    } catch (...) {
        delete left ;
        throw ;
    }
}

Node *CExpressionHandler::bit_or() {
    Node *left = bit_xor() ;
    try {
        for (;;) {
            if (match (BITOR)) {
                Node *right = bit_xor() ;
                left = new Expression (symtab,BITOR, left, right) ;
            } else {
                return left ;
            }
        }
    } catch (...) {
        delete left ;
        throw ;
    }
}

Node *CExpressionHandler::logand() {
    Node *left = bit_or() ;
    try {
        for (;;) {
            if (match (LOGAND)) {
                Node *right = bit_or() ;
                left = new Expression (symtab,LOGAND, left, right) ;
            } else {
                return left ;
            }
        }
    } catch (...) {
        delete left ;
        throw ;
    }
}

Node *CExpressionHandler::logor() {
    Node *left = logand() ;
    try {
        for (;;) {
            if (match (LOGOR)) {
                Node *right = logand() ;
                left = new Expression (symtab,LOGOR, left, right) ;
            } else {
                return left ;
            }
        }
    } catch (...) {
        delete left ;
        throw ;
    }
}

Node *CExpressionHandler::conditional() {
    Node *left = logor() ;
    try {
        if (match (QUESTION)) {
            Node *e1 = single_expression() ;
            left = new Expression (symtab,QUESTION, left, e1) ;
            if (match (COLON)) {
                e1 = single_expression() ;
                return new Expression (symtab,COLON, left, e1) ;
            } else {
                error ("Missing : for conditional expression") ;
                return left ;
            }
        } else {
            return left ;
        }
    } catch (...) {
        delete left ;
        throw ;
    }
}

Node *CExpressionHandler::assignment_expression() {
    Node *left = conditional() ;
    try {
        if (match (ASSIGN)) {
            return new AssignmentExpression (symtab, ASSIGN, left, assignment_expression()) ;
        } else if (match (PLUSEQ)) {
            return new AssignmentExpression (symtab, PLUSEQ, left, assignment_expression()) ;
        } else if (match (MINUSEQ)) {
            return new AssignmentExpression (symtab, MINUSEQ, left, assignment_expression()) ;
        } else if (match (STAREQ)) {
            return new AssignmentExpression (symtab, STAREQ, left, assignment_expression()) ;
        } else if (match (SLASHEQ)) {
            return new AssignmentExpression (symtab, SLASHEQ, left, assignment_expression()) ;
        } else if (match (PERCENTEQ)) {
            return new AssignmentExpression (symtab, PERCENTEQ, left, assignment_expression()) ;
        } else if (match (ANDEQ)) {
            return new AssignmentExpression (symtab, ANDEQ, left, assignment_expression()) ;
        } else if (match (OREQ)) {
            return new AssignmentExpression (symtab, OREQ, left, assignment_expression()) ;
        } else if (match (XOREQ)) {
            return new AssignmentExpression (symtab, XOREQ, left, assignment_expression()) ;
        } else if (match (LSHIFTEQ)) {
            return new AssignmentExpression (symtab, LSHIFTEQ, left, assignment_expression()) ;
        } else if (match (RSHIFTEQ)) {
            return new AssignmentExpression (symtab, RSHIFTEQ, left, assignment_expression()) ;
        }
        return left ;
    } catch (...) {
        delete left ;
        throw ;
    }
}

Node *CExpressionHandler::single_expression() {
    found_lparen = false ;
    return assignment_expression() ;
}


Node *CExpressionHandler::expression() {
    Node *left = single_expression() ;
    for (;;) {
        if (match (COMMA)) {
            Node *right = single_expression() ;
            left = new Expression (symtab,COMMA, left, right) ;
        } else {
            return left ;
        }
    }
}

//
// Fortran language expression handler
//

FortranExpressionHandler::FortranExpressionHandler (SymbolTable *symtab, Architecture *arch, int language)
    : ExpressionHandler(symtab, arch, language) {
    operators[".LT."] = LESS ;
    operators[".GT."] = GREATER ;
    operators[".LE."] = LESSEQ ;
    operators[".GE."] = GREATEREQ ;
    operators[".EQ."] = EQUAL ;
    operators[".NE."] = NOTEQUAL ;
    operators[".NOT."] = NOT ;
    operators[".AND."] = BITAND ;
    operators[".OR."] = BITOR ;
    operators[".EQV."] = BITEQUIV ;
    operators[".NEQV."] = BITXOR ;
    operators[".TRUE."] = BITTRUE ;
    operators[".FALSE."] = BITFALSE ;

    intrinsics["KIND"] = IT_KIND ;
    intrinsics["LOC"] = IT_LOC ;
    intrinsics["ALLOCATED"] = IT_ALLOCATED ;
    intrinsics["ASSOCIATED"] = IT_ASSOCIATED ;
    intrinsics["UBOUND"] = IT_UBOUND ;
    intrinsics["LBOUND"] = IT_LBOUND ;
    intrinsics["LEN"] = IT_LEN ;
    intrinsics["SIZE"] = IT_SIZE ;
    intrinsics["ADDR"] = IT_ADDR ;

    reserved_words["INTEGER"] = INTEGER ;
    reserved_words["REAL"] = REAL ;
    reserved_words["COMPLEX"] = COMPLEX ;
    reserved_words["LOGICAL"] = LOGICAL ;
    reserved_words["DIMENSION"] = DIMENSION ;
    reserved_words["POINTER"] = POINTER ;
    reserved_words["KIND"] = KIND ;
    reserved_words["LEN"] = LEN ;
    reserved_words["CHARACTER"] = CHARACTER ;
    
}

FortranExpressionHandler::~FortranExpressionHandler() {
}

bool FortranExpressionHandler::istoken(char ch) {
    // the '_' character can be a token since getNextToken() doesn't
    // use istoken() to what characters are valid within an identifier
    char tokens[] = { '%', '=', '<', '>', '-', '+', '*', '/', '(', ')', ':', ',', '@', '{', '}', '_'} ;
    for (uint i = 0 ; i < sizeof(tokens) ; i++) {
        if (tokens[i] == ch) {
            return true ;
        }
    }
    return false ;
}

Token FortranExpressionHandler::getNextToken() {
    lastch = ch ;
    skip_spaces() ;
    if (ch >= line.size()) {
        return NONE ;
    }
    if (isalpha (line[ch])) { //fortran identifiers can't start with '_'
        spelling = "" ;
        while (ch < line.size() && (isalnum (line[ch]) || line[ch] == '_')) {
            spelling += line[ch++] ;
        }
        std::string upper ;
        for (uint i = 0 ; i < spelling.size() ; i++) {
            upper += toupper(spelling[i]) ;
        }
        KeywordMap::iterator kw = reserved_words.find (upper) ;
        if (kw != reserved_words.end()) {
            return kw->second ;
        }
        if (line[ch] == '"' || line[ch] == '\'') {              // letter prefix on number?
            number = 0 ;
            char term = line[ch] ;
            if (upper == "B") {
                ch++ ;
                while (ch < line.size() && line[ch] != term) {
                    if (line[ch] != '1' && line[ch] != '0') {
                        error ("Invalid binary number") ;
                    }
                    number = (number << 1) | (line[ch] == '1') ;
                    ch++ ;
                }
                ch++ ;
                return NUMBER ;
            } else if (upper == "O") {
                ch++ ;
                while (ch < line.size() && line[ch] != term) {
                    if (line[ch] < '0' || line[ch] > '7') {
                        error ("Invalid octal number") ;
                    }
                    number = (number << 3) | (line[ch] - '0') ;
                    ch++ ;
                }
                ch++ ;
                return NUMBER ;
            } else if (upper == "Z") {
                ch++ ;
                while (ch < line.size() && line[ch] != term) {
                    if (!isxdigit (line[ch])) {
                        error ("Invalid hex number") ;
                    }
                    number = (number << 4) | (toupper(line[ch]) >= 'A' ? toupper(line[ch]) - 'A' + 10 : line[ch] - '0') ;
                    ch++ ;
                }
                ch++ ;
                return NUMBER ;
            }
        }
        return IDENTIFIER ;
    } else if (line[ch] == '$') {                       // register or debugger variable?
        ch++ ;
        spelling = "" ;
        if (line[ch] == '$') {
            spelling += '$' ;
            ch++ ;
        }
        while (ch < line.size() && (isalnum (line[ch]) || line[ch] == '_')) {
            spelling += line[ch++] ;
        }
        if (arch != NULL) {
			int num = arch->main_register_set_properties()->register_number_for_name(spelling);
			if (RegisterSetProperties::invalid_register != num)
			{
				return REGISTERNAME;
			}
        }
        spelling = "$" + spelling ;
        return DEBUGGERVAR ;
    } else if  (isdigit (line[ch])) {
        number = 0 ;
        if (line[ch+1] == 'x' || line[ch+1] == 'X') {           // hex number?
            ch += 2 ;
            while (ch < line.size() && isxdigit (line[ch])) {
                int x = toupper(line[ch]) >= 'A' ? toupper(line[ch]) - 'A' + 10 : line[ch] - '0' ;
                number = (number << 4) | x ;
                ch++ ;
            }
        } else {
            bool found_dot = false ;
            bool found_exp = false ;
            char num_buffer[1024] ;
            char *p = num_buffer ;
            double_precision = false ;

            while (ch < line.size()) {
                if (line[ch] == '.' && !found_dot) {
                    found_dot = true ;
                    *p++ = line[ch++] ;
                } else if ((line[ch] == 'e' || line[ch] == 'E' || line[ch] == 'D' || line[ch] == 'd') && !found_exp) {
                    if (line[ch] == 'd' || line[ch] == 'D') {         
                        *p++ = 'e' ;
                        double_precision = true ;
                        ch++ ;
                    } else {
                        *p++ = line[ch++] ;
                    }
                    if (line[ch] == '+' || line[ch] == '-') {
                        *p++ = line[ch++] ;
                    }
                    found_exp = true ;
                    found_dot = true ;
                } else if (isdigit (line[ch])) {
                    *p++ = line[ch++] ;
                } else {
                    if (found_dot && isalpha (line[ch])) {              // need to allow 1.lt.2
                        if (line[ch-1] == '.') {                // allow 1.l
                            p-- ;                                                                   // move back one
                            ch-- ;
                        } else if (line[ch-2] == '.' && (line[ch-1] == 'e' || line[ch-1] == 'E')) {            // allow 1.eq (e is trapped above)
                            p -= 2 ;
                            ch -= 2 ;
                        }
                        found_dot = false ;                                                     // not a floating point number
                    }
                    break ;
                }
            }
            *p = 0 ;
            if (found_dot) {                    // floating point number?
                fpnumber = strtod (num_buffer, NULL) ;
                return FPNUMBER ;
            } else {
                number = strtoll (num_buffer, NULL, 10) ;
            }
        }
        return NUMBER ;
    } else if (line[ch] == '.') {                       // an alphanumeric operator?
        spelling = "" ;
        spelling += line[ch++] ;
        while (ch < line.size() && line[ch] != '.') {
           spelling += toupper (line[ch++]) ;
        }
        if (line[ch] != '.') {
            error ("Missing . for operator") ;
        }
        spelling += line[ch++] ;
        KeywordMap::iterator op = operators.find(spelling) ;
        if (op == operators.end()) {
            error (std::string ("No such operator: ") + spelling) ;
        }
        return op->second ;
    } else if  (istoken (line[ch])) {
        switch (line[ch]) {
        case '_': ch++ ; return UCAST ;
        case '+': ch++ ; if (line[ch] == '=') return ch++, PLUSEQ ; return PLUS ;
        case ':': ch++ ; if (line[ch] == ':') return ch++, COLONCOLON ; return COLON ;
        case ',': ch++ ; return COMMA ;
        case '@': ch++ ; return AT ;
        case '}': ch++ ; return RBRACE ;
        case '{': ch++ ; return LBRACE ;
        case '*': 
           ch++ ;
           if (line[ch] == '*') {
               ch++ ;
               return POWER ;
           } 
           if (line[ch] == '=') {
               return ch++, STAREQ ;
           }
           return STAR ;
           break ;
        case '/': 
            ch++ ; 
            if (line[ch] == '/') {
                ch++ ;
                return STRCAT ;
            } else if (line[ch] == '=') {
                ch++ ;
                return NOTEQUAL ;
            }
            if (line[ch] == '=') {
                return ch++, STAREQ ;
            } else if (line[ch] == '(') {
                return ch++, LBRACE ;                   // /( is like {
            } else if (line[ch] == ')') {       
                return ch++, RBRACE ;
            } else {
                return SLASH ;
            }
            break ;
        case '%': ch++ ; if (line[ch] == '=') return ch++, PERCENTEQ ; return MEMBER ;
        case '(': ch++ ; return LPAREN ;
        case ')': ch++ ; return RPAREN ;

        case '=':
            ch++ ;
            if (line[ch] == '=') {
                ch++ ;
                return EQUAL ;
            }
            return ASSIGN ;
            break ;
        case '<':
            ch++ ;
            if (line[ch] == '=') {
                ch++ ;
                return LESSEQ ;
            } else if  (line[ch] == '<') {
                ch++ ;
                if (line[ch] == '=') {
                    return ch++, LSHIFTEQ ;
                }
                return LSHIFT ;
            }
            return LESS ;
        case '>':
            ch++ ;
            if (line[ch] == '=') {
                ch++ ;
                return GREATEREQ ;
            } else if  (line[ch] == '>') {
                ch++ ;
                if (line[ch] == '=') {
                    return ch++, RSHIFTEQ ;
                }
                return RSHIFT ;
            }
            return GREATER ;
        case '-': ch++ ; return MINUS ;
        }
    } else if  (line[ch] == '"') {
        spelling = "" ;
        ch++ ;
        while (ch < line.size() && line[ch] != '"') {
            if (line[ch] == '\\') {
                ch++ ;
                switch (line[ch]) {
                case 'n':
                    spelling += '\n' ;
                    break ;
                case 'r':
                    spelling += '\r' ;
                    break ;
                case 'a':
                    spelling += '\a' ;
                    break ;
                default:
                    spelling += line[ch] ;
                }
            } else {
                spelling += line[ch] ;
            }
            ch++ ;
        }
        ch++ ;
        return STRING ;
    } else if  (line[ch] == '\'' || line[ch] == '"') {
        char term = line[ch++] ;
        spelling = "" ;
        while (ch < line.size() && line[ch] != term) {
            spelling += line[ch++] ;
        }
        if (ch == line.size()) {
            error ("Unterminated character string") ;
        } else {
            ch++ ;
        }
        return STRING ;
    } else {
        error ("Illegal character") ;
        ch++ ;
    }
    throw Exception("not reached");
}

DIE *FortranExpressionHandler::parse_typespec() {
    DIE *die = NULL ;
    if (match (INTEGER)) {
        int kind = get_kind(4) ;
        if (kind != 1 && kind != 2 && kind != 4 && kind != 8) {
            throw Exception ("Invalid KIND value %d in INTEGER cast", kind);
        }
        die = symtab->new_scalar_type ("INTEGER", DW_ATE_signed, kind) ;
    } else if (match (REAL)) {
        int kind = get_kind(8) ;
        if (kind != 4 && kind != 8) {
            throw Exception ("Invalid KIND value %d in REAL cast", kind);
        }
        die = symtab->new_scalar_type ("REAL", DW_ATE_float, kind) ;
    } else if (match (COMPLEX)) {
        int kind = get_kind(8) ;
        if (kind != 4 && kind != 8) {
            throw Exception ("Invalid KIND value %d in COMPLEX cast", kind);
        }
        die = symtab->new_scalar_type ("COMPLEX", DW_ATE_complex_float, kind) ;
    } else if (match (LOGICAL)) {
        int kind = get_kind(1) ;
        if (kind != 1 && kind != 2 && kind != 4 && kind != 8) {
            throw Exception ("Invalid KIND value %d in LOGICAL cast", kind);
        }
        die = symtab->new_scalar_type ("LOGICAL", DW_ATE_boolean, kind) ;
    } else if (match (CHARACTER)) {
        int len = get_kind(1) ;
	(void) len;
        // XXX: do this
    } else {
        throw false ;
    }

    if (die != NULL) {
        bool dims_set = false ;
        bool pointer_set = false ;
        while (match (COMMA)) {
            if (match (DIMENSION)) {
                if (dims_set) {
                    error ("Malformed type specification, multiple dimension attributes") ;
                }
                dims_set = true ;
                needbrack (LPAREN) ;
                std::vector<DIE*> subranges ;
                while (currentToken != RPAREN) {
                    if (match (COLON)) {                // (:)
                        subranges.push_back (symtab->new_subrange_type()) ;
                    } else {
                        if (currentToken != NUMBER) {
                            error ("Expected number for array dimension") ;
                        } 
                        int lb = number ;
                        nextToken() ;
                        int ub ;
                        if (match(COLON)) {
                            if (currentToken != NUMBER) {
                                error ("Expected number for array upper bound dimension") ;
                            }
                            ub = number ;
                            nextToken(); 
                        } else {
                            ub = lb ;
                            lb = 1 ;
                        }
                        subranges.push_back (symtab->new_subrange_type (lb, ub)) ;
                    }
                    if (!match (COMMA)) {
                        break ;
                    }
                }
                needbrack (RPAREN) ;
                die = symtab->new_array_type (die) ;
                for (uint i = 0 ; i < subranges.size() ; i++) {
                    die->addChild (subranges[i]) ;
                }
            } else if (match (POINTER)) {
                if (pointer_set) {
                    error ("Malformed type specification, multiple pointer attributes") ;
                }
                die = symtab->new_pointer_type (die) ;
                pointer_set = true ;
            }
        }
    }
    return die ;
}

int FortranExpressionHandler::get_kind(int def) {
    if (match (LPAREN)) {
        if (match (KIND)) {
            if (match (ASSIGN)) {
                if (currentToken == NUMBER) {
                    int n = number ;
                    nextToken() ;
                    needbrack (RPAREN) ;
                    return n ;
                } else {
                    error ("Expected KIND=number") ;
                }
            } else {
                error ("KIND syntax error") ;
            }
        } else if (match (LEN)) {
            if (match (ASSIGN)) {
                if (currentToken == NUMBER) {
                    int n = number ;
                    nextToken() ;
                    needbrack (RPAREN) ;
                    return n ;
                } else {
                    error ("Expected LEN=number") ;
                }
            } else {
                error ("LEN syntax error") ;
            }
        }
    } else if (match (STAR)) {
        if (currentToken == NUMBER) {
            int n = number ;
            nextToken() ;
            return n ;
        } else {
            error ("Invalid * syntax") ;
        }
    }
    return def ;
}


Node *FortranExpressionHandler::primary() {
    if (found_lparen || match (LPAREN)) {
        Node *expr = expression() ;
        needbrack (RPAREN) ;
        return expr ;
    } else if (currentToken == KIND || currentToken == LEN) {           // these are reserved words and also instrinsics
        if (currentToken == KIND) {
            nextToken() ;
            return new IntrinsicIdentifier (symtab, "KIND", IT_KIND) ;
        } else {
            nextToken() ;
            return new IntrinsicIdentifier (symtab, "LEN", IT_LEN) ;
        }
    } else if  (currentToken == IDENTIFIER) {
        std::string name = spelling ;
        nextToken() ;
        Address reqpc = 0 ;

        // lexical scoping operator
        if (match (AT)) {
            if (currentToken == NUMBER) {
                reqpc = current_process->lookup_line (number) ;
                nextToken() ;
            } else {
                error ("Expected @line for lexical scoping operator") ;
            }
        }
  
        SymbolValue val = current_process->find_symbol (name, reqpc) ;
        if (val.type == SV_ADDRESS) {
            return new IntConstant (symtab, (Address)val, ptrsize()) ;
        } else {
            if (val.type  == SV_NONE) {
                std::string uppername = name ;
                for (uint i = 0 ; i < uppername.size() ; i++) {
                    uppername[i] = toupper(uppername[i]) ;
                }
                IntrinsicMap::iterator i = intrinsics.find (uppername) ;
                if (i == intrinsics.end()) {
                    error ("No symbol \"" + name + "\" in current context.") ;
                } else {
                    return new IntrinsicIdentifier (symtab, uppername, i->second) ;
                }
            } 
            DIE *sym = val.dievec[0] ;
            std::string context = name ;
            while (match (COLONCOLON)) {
                context += "::" ;
                if (currentToken == IDENTIFIER) {
                    name = spelling ;
                    nextToken() ;
                    if (match (AT)) {
                        if (currentToken == NUMBER) {
                            reqpc = current_process->lookup_line (number) ;
                            nextToken() ;
                        } else {
                            error ("Expected @line for lexical scoping operator") ;
                        }
                    }
                    sym = sym->find_symbol (name, reqpc) ;
                    if (sym == SV_NONE) {
                        error ("No symbol \"%s\" in context %s", name.c_str(), context.c_str()) ;
                    }
                    context += name ;
                } else {
                    error ("Expected identifier after :: operator") ;
                }
            }
            return new Identifier (symtab, sym) ;
        }
    } else if (currentToken == REGISTERNAME) {
        std::string regname = spelling ;
        nextToken() ;
        return new RegisterExpression (symtab, regname) ;
    } else if (currentToken == DEBUGGERVAR) {
        std::string name = spelling ;
        nextToken() ;
        DebuggerVar *var = current_process->find_debugger_variable (name) ;
        if (var == NULL) {
            if (isdigit (name[0])) {
                char buf[100] ;
                snprintf (buf, sizeof(buf), "History has not yet reached $%s.", name.c_str()) ;
                error (buf) ; 
            }
        }
        return new DebuggerVarExpression (symtab, name, var) ;
    } else if  (currentToken == NUMBER) {
        int64_t n = number ;
        nextToken() ;
        return new IntConstant (symtab, n, 4) ;
    } else if  (currentToken == FPNUMBER) {
        double n = fpnumber ;
        nextToken() ;
        if (double_precision) {
            return new RealConstant (symtab, n, 8) ;
        } else {
            return new RealConstant (symtab, n, 4) ; 
        }
    } else if  (currentToken == STRING) {
        std::string s = spelling ;
        nextToken() ;
        if (match (COLONCOLON)) {
            // s is a file name
            DIE *sym = current_process->find_compilation_unit (s) ;
            if (sym == NULL) {
                error ("Unable to find file %s in scoping operation", s.c_str()) ;
            }
            Address reqpc = 0 ;
            std::string context = std::string ("'") + s + "'" ;
            for (;;) {
                context += "::" ;
                if (currentToken == IDENTIFIER) {
                    s = spelling ;
                    nextToken() ;
                    if (match (AT)) {
                        if (currentToken == NUMBER) {
                            reqpc = current_process->lookup_line (number) ;
                            nextToken() ;
                        } else {
                            error ("Expected @line for lexical scoping operator") ;
                        }
                    }
                    sym = sym->find_symbol (s, reqpc) ;
                    if (sym == SV_NONE) {
                        error ("No symbol \"%s\" in context %s", s.c_str(), context.c_str()) ;
                    }
                    context += s ;
                } else {
                    error ("Expected identifier after :: operator") ;
                }
                if (!match (COLONCOLON)) {
                    break ;
                }
            }
            return new Identifier (symtab, sym) ;
        } else {
            return new StringConstant (symtab, s) ;
        }
    } else if  (currentToken == CHARCONST) {
        char n = number ;
        nextToken() ;
        return new CharConstant (symtab, n) ;
    } else if  (currentToken == BITTRUE) {
        nextToken() ;
        return new IntConstant (symtab, 1, 1) ;
    } else if  (currentToken == BITFALSE) {
        nextToken() ;
        return new IntConstant (symtab, 0, 1) ;
    } else if (currentToken == LBRACE) {                // vector literal
        nextToken() ;
        std::vector<Node *> v ;
        while (currentToken != RBRACE) {
            Node *expr = single_expression() ;
            v.push_back (expr) ;
            if (!match (COMMA)) {
                break ;
            }
        }
        needbrack (RBRACE) ;
        return new VectorExpression (symtab, v) ;
    } else {
        try {           // allow a type specification
            DIE *die = parse_typespec() ;
            return new CastExpression (symtab, new IntConstant (symtab, 0, 4), die) ;
        } catch (bool b) {
            error ("Expression syntax error") ;
        }
    }
    throw Exception("not reached");
}


Node *FortranExpressionHandler::member() {
    Node *left = primary() ;
    try {
        for (;;) {
            if (match (MEMBER)) {
                if (currentToken == IDENTIFIER) {
                    std::string membername = spelling ;
                    nextToken() ;
                    left = new MemberExpression (symtab, left, membername) ;
                } else {
                    error ("defined type member name expected") ;
                }
            } else {
                return left ;
            }
        }
    } catch (...) {
         delete left ;
         throw ;
    }
}

Node *FortranExpressionHandler::array() {
    Node *left = member() ; 
    try {
        if (match (LPAREN)) {
            if (left->is_intrinsic()) {
                std::vector<Node*> args ;
                while (currentToken != RPAREN) {
                    Node *arg = single_expression() ;
                    args.push_back (arg) ;
                    if (!match (COMMA)) {
                        break ;
                    }
                }
                needbrack (RPAREN) ;
                left = new IntrinsicExpression (symtab, dynamic_cast<IntrinsicIdentifier*>(left), args) ;
            } else {
                DIE *type = left->get_type() ;              // static type only
		DIE *tmps = type->find_member (((MemberExpression *)left)->membername) ;
		if (tmps)
			type = tmps->get_type() ;
                // if we don't have a type for the left, assume it's an array
                if (type->get_tag() == DW_TAG_array_type || type->get_tag() == DW_TAG_string_type) {
                    std::vector<Node*> indices ;
                    try {
                        for (;;) {
                            if (match (COLON)) {
                                if (currentToken == COMMA || currentToken == RPAREN) {
                                    indices.push_back (new Expression (symtab, RANGE, NULL, NULL)) ;
                                } else {
                                    indices.push_back (new Expression (symtab, RANGE, NULL, single_expression())) ;
                                }
                            } else if (currentToken == COMMA || currentToken == RPAREN) {
                                indices.push_back (new Expression (symtab, RANGE, NULL, NULL)) ;
                            } else {
                                Node *ex = single_expression() ;
                                if (match (COLON)) {
                                    if (currentToken != COMMA && currentToken != RPAREN) {
                                        ex = new Expression (symtab, RANGE, ex, single_expression()) ;
                                    } else {
                                        ex = new Expression (symtab, RANGE, ex, NULL) ;
                                    }
                                }
                                indices.push_back (ex) ;
                            }
                            if (!match (COMMA)) {
                                break ;
                            }
                        }
                        needbrack (RPAREN) ;
                    } catch (...) {
                        for (uint i = 0 ; i < indices.size() ; i++) {
                            delete indices[i] ;
                        }
                        throw ;
                    }
                    if (type->get_tag() == DW_TAG_string_type && indices.size() > 1) {
                       error ("Too many dimensions for string subscript operation") ;
                    }
                    return new ArrayExpression (symtab, left, indices) ;
                } else if (type->get_tag() == DW_TAG_subprogram) {
                    std::vector<Node*> args ;
                    try {
                        while (currentToken != RPAREN) {
                            Node *arg = single_expression() ;
                            args.push_back (new Expression (symtab, ADDRESS, arg)) ;    // take address of arg
                            if (!match (COMMA)) {
                                break ;
                            }
                        }
                        needbrack (RPAREN) ;
                    } catch (...) {
                        for (uint i = 0 ; i < args.size() ; i++) {
                            delete args[i] ;
                        }
                        throw ;
                    }
                    left = new CallExpression (symtab, left, args) ;
                } else if (type->get_tag() == DW_TAG_structure_type ||
                           type->get_tag() == DW_TAG_class_type) {
                    std::vector<Node*> values ;
                    try {
                        while (currentToken != RPAREN) {
                            Node *arg = single_expression() ;
                            values.push_back (arg) ;
                            if (!match (COMMA)) {
                                break ;
                            }
                        }
                        needbrack (RPAREN) ;
                    } catch (...) {
                        for (uint i = 0 ; i < values.size() ; i++) {
                            delete values[i] ;
                        }
                        throw ;
                    }
                    left = new ConstructorExpression (symtab, left, values) ;
                } else {
                     error ("Invalid use of () - not an array or callable") ;           // XXX: intrinsics
                }
            }
        }
        return left ;
    } catch (...) {
         delete left ;
         throw ;
    }

}

Node *FortranExpressionHandler::monadic_defined() {             // XXX: implement this
    return array() ;
}

Node *FortranExpressionHandler::cast() {
    if (match (LPAREN)) {
        try {
            DIE *die = parse_typespec() ;
            needbrack (RPAREN) ;
            Node *left = cast() ;
            return new CastExpression (symtab, left, die) ;
        } catch (bool b) {
            // no typespec
            found_lparen = true ;
            return monadic_defined() ;
        }
    }

    Node* left = monadic_defined();
    if (match (UCAST)) {
        Node* right = cast() ;
        return new TypeCastExpression (symtab, left, right) ;
    }

    return left;
}


// this is right-recursive according to the spec
Node *FortranExpressionHandler::power() {
    Node *left = cast() ;
    try {
        if (match (POWER)) {
            Node *right = power() ;
            return new Expression (symtab, POWER, left, right) ;
        } else {
            return left ;
        }
    } catch (...) {
         delete left ;
         throw ;
    }
}

Node *FortranExpressionHandler::mult() {
    Node *left = power() ;
    try {
        for (;;) {
            if (match (STAR)) {
                Node *right = power() ;
                left = new Expression (symtab,STAR, left, right) ;
            } else if  (match (SLASH)) {
                Node *right = power() ;
                left = new Expression (symtab,SLASH, left, right) ;
            } else {
                return left ;
            }
        }
    } catch (...) {
         delete left ;
         throw ;
    }
}

Node *FortranExpressionHandler::unary() {
    if (match (MINUS)) {
         Node *left = unary() ;
         return new Expression (symtab,UMINUS, left) ;
    } else if  (match (PLUS)) {
         Node *left = unary() ;
         return new Expression (symtab,UPLUS, left) ;
    } else {
         return mult() ;
    }
}

Node *FortranExpressionHandler::add() {
    Node *left = unary() ;
    try {
        for (;;) {
            if (match (PLUS)) {
                Node *right = unary() ;
                left = new Expression (symtab,PLUS, left, right) ;
            } else if  (match (MINUS)) {
                Node *right = unary() ;
                left = new Expression (symtab,MINUS, left, right) ;
            } else {
                return left ;
            }
        }
    } catch (...) {
         delete left ;
         throw ;
    }
}

Node *FortranExpressionHandler::strcat() {
    Node *left = add() ;
    try {
        for (;;) {
            if (match (STRCAT)) {
                Node *right = add() ;
                left = new Expression (symtab,STRCAT, left, right) ;
            } else {
                return left ;
            }
        }
    } catch (...) {
         delete left ;
         throw ;
    }
}


Node *FortranExpressionHandler::comparison() {
    Node *left = strcat() ;
    try {
        for (;;) {
            if (match (EQUAL)) {
                Node *right = strcat() ;
                left = new Expression (symtab,EQUAL, left, right) ;
            } else if  (match (NOTEQUAL)) {
                Node *right = strcat() ;
                left = new Expression (symtab,NOTEQUAL, left, right) ;
            } else if (match (LESS)) {
                Node *right = strcat() ;
                left = new Expression (symtab,LESS, left, right) ;
            } else if  (match(LESSEQ)) {
                Node *right = strcat() ;
                left = new Expression (symtab,LESSEQ, left, right) ;
            } else if  (match(GREATER)) {
                Node *right = strcat() ;
                left = new Expression (symtab,GREATER, left, right) ;
            } else if  (match(GREATEREQ)) {
                Node *right = strcat() ;
                left = new Expression (symtab,GREATEREQ, left, right) ;
            } else {
                return left ;
            }
        }
    } catch (...) {
         delete left ;
         throw ;
    }
}

Node *FortranExpressionHandler::bit_not() {
    if (match (NOT)) {
        Node *left = bit_not() ;
        return new Expression (symtab, NOT, left) ;
    } else {
        return comparison() ;
    }
}

Node *FortranExpressionHandler::bit_and() {
    Node *left = bit_not() ;
    try {
        for (;;) {
            if (match (BITAND)) {
                Node *right = bit_not() ;
                left = new Expression (symtab,BITAND, left, right) ;
            } else {
                return left ;
            }
        }
    } catch (...) {
         delete left ;
         throw ;
    }
}

Node *FortranExpressionHandler::bit_or() {
    Node *left = bit_and() ;
    try {
        for (;;) {
            if (match (BITOR)) {
                Node *right = bit_and() ;
                left = new Expression (symtab,BITOR, left, right) ;
            } else {
                return left ;
            }
        }
    } catch (...) {
         delete left ;
         throw ;
    }
}

Node *FortranExpressionHandler::bit_equiv() {
    Node *left = bit_or() ;
    try {
        for (;;) {
            if (match (BITXOR)) {
                Node *right = bit_or() ;
                left = new Expression (symtab,BITXOR, left, right) ;
            } else if (match (BITEQUIV)) {
                Node *right = bit_or() ;
                left = new Expression (symtab,BITEQUIV, left, right) ;
            } else {
                return left ;
            }
        }
    } catch (...) {
         delete left ;
         throw ;
    }
}

Node *FortranExpressionHandler::dyadic_defined() {             // XXX: implement this
    return bit_equiv() ;
}

Node *FortranExpressionHandler::assignment_expression() {
    Node *left = dyadic_defined() ;
    try {
        if (match (ASSIGN)) {
            return new AssignmentExpression (symtab, ASSIGN, left, assignment_expression()) ;
        } else if (match (PLUSEQ)) {
            return new AssignmentExpression (symtab, PLUSEQ, left, assignment_expression()) ;
        } else if (match (MINUSEQ)) {
            return new AssignmentExpression (symtab, MINUSEQ, left, assignment_expression()) ;
        } else if (match (STAREQ)) {
            return new AssignmentExpression (symtab, STAREQ, left, assignment_expression()) ;
        } else if (match (SLASHEQ)) {
            return new AssignmentExpression (symtab, SLASHEQ, left, assignment_expression()) ;
        } else if (match (PERCENTEQ)) {
            return new AssignmentExpression (symtab, PERCENTEQ, left, assignment_expression()) ;
        } else if (match (ANDEQ)) {
            return new AssignmentExpression (symtab, ANDEQ, left, assignment_expression()) ;
        } else if (match (OREQ)) {
            return new AssignmentExpression (symtab, OREQ, left, assignment_expression()) ;
        } else if (match (XOREQ)) {
            return new AssignmentExpression (symtab, XOREQ, left, assignment_expression()) ;
        } else if (match (LSHIFTEQ)) {
            return new AssignmentExpression (symtab, LSHIFTEQ, left, assignment_expression()) ;
        } else if (match (RSHIFTEQ)) {
            return new AssignmentExpression (symtab, RSHIFTEQ, left, assignment_expression()) ;
        }
        return left ;
    } catch (...) {
         delete left ;
         throw ;
    }
}

Node *FortranExpressionHandler::single_expression() {
    found_lparen = false ;
    return assignment_expression() ;
}


// this is not really fortran, but is useful nonetheless
Node *FortranExpressionHandler::expression() {
    Node *left = single_expression() ;
    for (;;) {
        if (match (COMMA)) {
            Node *right = single_expression() ;
            left = new Expression (symtab,COMMA, left, right) ;
        } else {
            return left ;
        }
    }
    return left ;
}


