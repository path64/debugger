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

file: expr.h
created on: Fri Aug 13 11:02:30 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#ifndef expr_h_included
#define expr_h_included

#include "dbg_elf.h"
#include "pstream.h"
#include "dwf_spec.h"
#include "exp_value.h"
#include "exp_fmt.h"

#include <stack>

// classes

class Process ;
typedef std::vector<Address> LocationStack ;
class DIE ;
class TypeArray ;
class SymbolTable ;
class ExpressionHandler ;
class Architecture ;

class EvalContext {
public:
    EvalContext(Process *process, Address fb, int language, PStream &os) ;
    EvalContext (int language, PStream &os)
       : ignore_expr(false), language(language), os(os),
         process(NULL), fb(0), show_contents(true),
         show_reference(true), truncate_aggregates(false), pretty(true) {} 
    ~EvalContext() ; 
    bool addressonly ; 
    bool ignore_expr ;
    int language ; 
    PStream &os; 
    Process *process ; 
    Address fb ; 

    // output formatting 
    Format fmt ;
    bool show_contents ;                // show the contents of structs/unions
    bool show_reference ;               // show the contents of a reference
    bool truncate_aggregates ;          // show truncated aggregates
    bool pretty ;                       // pretty print structs and arrays
    bool trunc_digs;                    // only show non-zero digits
    int num_sigdigs;                    // number of signifcant digits
} ;

enum Token {
        NONE,
        NUMBER,
        IDENTIFIER,
        STRING,
        CHARCONST,
        FPNUMBER,
        SIZEOF,
        PLUS,
        MINUS,
        STAR,
        SLASH,
        MOD,
        LSHIFT,
        RSHIFT,
        EQUAL,
        NOTEQUAL,
        LESS,
        LESSEQ,
        GREATER,
        GREATEREQ,
        BITAND,
        BITOR,
        LOGAND,
        LOGOR,
        QUESTION,
        COLON,
        ONESCOMP,
        BITXOR,
        MEMBER,
        ARROW,
        LPAREN,
        RPAREN,
        LBRACE,
        RBRACE,
        LSQUARE,
        RSQUARE,
        COMMA,
        NOT,
        UMINUS,
        UPLUS,
        CONTENTS,
        ADDRESS,
        PLUSPLUS, MINUSMINUS, POSTINC, POSTDEC,
        POWER,
        BITEQUIV,
        BITTRUE,
        BITFALSE,
        STRCAT, 
        RANGE,
        REGISTERNAME,
        DEBUGGERVAR,
        ASSIGN,
        AT,
        COLONCOLON,
        TOREAL,
        TOINT,
        UCAST,
        PLUSEQ, MINUSEQ, STAREQ, SLASHEQ, PERCENTEQ, ANDEQ, OREQ, XOREQ, LSHIFTEQ, RSHIFTEQ,
        // C type keywords
        INT, CHAR, SHORT, LONG, UNSIGNED, SIGNED, VOID, FLOAT, DOUBLE, STRUCT, UNION, CLASS, ENUM,
        KONST, VOLATILE, BOOL,

        // Fortran types
        INTEGER, DIMENSION, POINTER, REAL, COMPLEX, LOGICAL, KIND, LEN, CHARACTER
  
} ;


std::ostream &operator<< (std::ostream &os, const Value &v) ;


Value operator+ (const Value &v1, const Value &v2) ;
Value operator- (const Value &v1, const Value &v2) ;
Value operator* (const Value &v1, const Value &v2) ;
Value operator/ (const Value &v1, const Value &v2) ;
Value operator% (const Value &v1, const Value &v2) ;
Value operator< (const Value &v1, const Value &v2) ;
Value operator> (const Value &v1, const Value &v2) ;
Value operator<= (const Value &v1, const Value &v2) ;
Value operator>= (const Value &v1, const Value &v2) ;
Value operator== (const Value &v1, const Value &v2) ;
bool operator== (const Value &v1, int v2) ;
bool operator!= (const Value &v1, int v2) ;
Value operator!= (const Value &v1, const Value &v2) ;
Value operator<< (const Value &v1, const Value &v2) ;
Value operator>> (const Value &v1, const Value &v2) ;
Value operator^ (const Value &v1, const Value &v2) ;
Value operator& (const Value &v1, const Value &v2) ;
Value operator| (const Value &v1, const Value &v2) ;
Value operator- (const Value &v) ;
Value operator~ (const Value &v) ;
Value operator! (const Value &v) ;

// some fortran intrinsic functions that are handled directly by the expression handler
enum Intrinsic {
    IT_KIND, IT_LOC, IT_ALLOCATED, IT_ASSOCIATED, IT_UBOUND, IT_LBOUND, IT_LEN, IT_SIZE, IT_ADDR
} ;


// a variable created by the debugger

class DebuggerVar {
    public:
    DebuggerVar (Value &val, DIE *type) : value(val), type(type) {}
    Value value ;
    DIE *type ;
} ;

class Node {
public:
    Node(SymbolTable *symtab) ;
    virtual ~Node() ; 
    DIE *get_type () ;
    void set_type (DIE *t) { type = t ; }
    virtual Value evaluate (EvalContext &context) = 0 ;
    virtual void set_value (EvalContext &context, Value &value, DIE *type) { throw Exception ("Cannot set the value of this expression") ; }
    virtual bool is_intrinsic() { return false ; }
    virtual bool is_generic() { return false ; }
    virtual Token get_opcode() { return NONE ; }
    virtual bool is_vector() { return false ; }
    virtual bool is_local() { return false ; }          // is the expression local to a function (contains local variables)
    virtual Node * check_operator_overload() { return NULL ;} 
    virtual bool is_member_expression() { return false ; }
    virtual bool is_identifier_set() { return false ; }
    virtual bool is_constant() { return false ; }
    virtual int num_variables() { return 0 ; }
protected:
    DIE *type ; 
    SymbolTable *symtab ;
private:
} ;

class ConstantNode: public Node  {
public:
    ConstantNode(SymbolTable *symtab) ;
    ~ConstantNode() ; 
    virtual Value evaluate (EvalContext &context) = 0 ;
    bool is_constant() { return true ; }
protected:
private:
} ;

class IntConstant: public ConstantNode  {
public:
    IntConstant(SymbolTable *symtab, int64_t v, int size) ;
    ~IntConstant() ; 
    Value evaluate (EvalContext &context) ;
protected:
private:
    int64_t v ; 
    int size ;          // in bytes
} ;

class ValueConstant: public ConstantNode  {
public:
    ValueConstant(SymbolTable *symtab, Value &v, DIE *t): ConstantNode (symtab), value(v) { type = t ; } 
    ~ValueConstant() {}
    Value evaluate (EvalContext &context) { return value ; }
protected:
private:
    Value value ;
} ;

class StringConstant: public ConstantNode  {
public:
    StringConstant(SymbolTable *symtab, std::string v) ;
    ~StringConstant() ; 
    Value evaluate (EvalContext &context) ;
protected:
private:
    std::string v ; 
} ;

class CharConstant: public ConstantNode  {
public:
    CharConstant(SymbolTable *symtab, char ch) ;
    ~CharConstant() ; 
    Value evaluate (EvalContext &context) ;
protected:
private:
    char ch ; 
} ;

class RealConstant: public ConstantNode  {
public:
    RealConstant(SymbolTable *symtab, double r, int size) ;
    ~RealConstant() ; 
    Value evaluate (EvalContext &context) ;
protected:
private:
    double r ; 
} ;

class Identifier: public Node  {
public:
    Identifier(SymbolTable *symtab, DIE *sym) ;
    ~Identifier() ; 
    virtual Value evaluate (EvalContext &context) ;
    DIE *get_symbol() { return sym ; }
    void set_value (EvalContext &context, Value &value, DIE *type) ;
    bool is_local()  ;
    int num_variables() { return 1 ; }
protected:
private:
    DIE *sym ; 
} ;

class IdentifierSet: public Node  {
public:
    IdentifierSet(SymbolTable *symtab, std::vector<DIE *> &syms) ;
    ~IdentifierSet() ; 
    virtual Value evaluate (EvalContext &context) ;
    void set_value (EvalContext &context, Value &value, DIE *type) ;
    bool is_local()  ;
    bool is_identifier_set() { return true ; }
    std::vector<DIE*> &get_dies() { return syms ; }
    int num_variables() { return syms.size() ; }
protected:
private:
    std::vector<DIE *> syms ; 
} ;

class IntrinsicIdentifier: public Node  {
public:
    IntrinsicIdentifier(SymbolTable *symtab, std::string name, Intrinsic it) ;
    ~IntrinsicIdentifier() ;
    std::string get_name() { return name ; }
    bool is_intrinsic() { return true ; }
    Intrinsic get_intrinsic() { return it ; }
    Value evaluate (EvalContext &context) { return Value() ; }
protected:
private:
    std::string name ;
    Intrinsic it ;
} ;

class RegisterExpression : public Node {
public:
    RegisterExpression(SymbolTable *symtab, std::string name) ;
    ~RegisterExpression() ; 
    virtual Value evaluate (EvalContext &context) ;
    void set_value (EvalContext &context, Value &value, DIE *type) ;
private:
    std::string regname ;
} ;

class DebuggerVarExpression : public Node {
public:
    DebuggerVarExpression(SymbolTable *symtab, std::string nm, DebuggerVar *var) ;
    ~DebuggerVarExpression() ; 
    virtual Value evaluate (EvalContext &context) ;
    void set_value (EvalContext &context, Value &value, DIE *type) ;
    bool is_generic() { return true ; }
private:
    DebuggerVar *var ;
    std::string name ;
} ;

class CastExpression: public Node  {
public:
    CastExpression(SymbolTable *symtab, Node *left, DIE *casttype) ;
    ~CastExpression() ; 
    virtual Value evaluate (EvalContext &context) ;
    void set_value (EvalContext &context, Value &value, DIE *type) { throw Exception ("Casts are not lvalues") ; }
    bool is_local()  ;
    int num_variables() ;
protected:
private:
   Node *left ;
   DIE *casttype ;
} ;

class TypeCastExpression: public Node {
public:
   TypeCastExpression(SymbolTable *symtab, Node *left, Node *right) ;
   ~TypeCastExpression() ;
   virtual Value evaluate (EvalContext &context) ;
   void set_value (EvalContext &context, Value &value, DIE *type) { throw Exception ("Casts are not lvalues") ; }
   bool is_local() ;
   int num_variables() ;
private:
   Node *left;
   Node *right;
} ;

class SizeofExpression: public Node  {
public:
    SizeofExpression(SymbolTable *symtab, DIE *t) ;
    ~SizeofExpression() ; 
    virtual Value evaluate (EvalContext &context) ;
    void set_value (EvalContext &context, Value &value, DIE *type) { throw Exception ("sizeof is not an lvalue") ; }
protected:
private:
} ;

class VectorExpression: public Node  {
public:
    VectorExpression(SymbolTable *symtab, std::vector<Node*> &v) ;
    ~VectorExpression() ; 
    virtual Value evaluate (EvalContext &context) ;
    void set_value (EvalContext &context, Value &value, DIE *type) { throw Exception ("Vector expressions are not lvalues") ; }
    bool is_vector() { return true ; }
    int num_variables() ;
protected:
private:
    std::vector<Node*> values ;
} ;


class MemberExpression : public Node {
public:
    MemberExpression (SymbolTable *symtab, Node *left, std::string membername) ;
    ~MemberExpression() ;
    virtual Value evaluate (EvalContext &context) ;
    virtual Value evaluate (EvalContext &context, DIE *member) ;
    void set_value (EvalContext &context, Value &value, DIE *type) ;
    bool is_local()  ;
    bool is_member_expression() { return true ; }
    Node *get_this_pointer() ;          // get the base of the member expression
    void resolve (EvalContext &ctx, std::vector<DIE*> &result) ;
    int num_variables() ;
private:
    Node *left ;
    std::string membername ;
} ;

class Expression: public Node  {
public:
    Expression(SymbolTable *symtab, Token opcode, Node *left = NULL, Node *right = NULL) ;
    Expression (const Expression &ex) ;
    ~Expression() ; 
    virtual Value evaluate (EvalContext &context) ;
    void set_value (EvalContext &context, Value &value, DIE *type) ;
    Token get_opcode() { return opcode ; }
    Node *get_left() { return left ; }
    Node *get_right() { return right ; }
    bool is_local()  ;
    bool check_operator_overload(EvalContext &context, DIE *ltype, Value &l, DIE *rtype, Value &r, Value &result) ;
    void set_left (Node *l) { left = l ; }
    void set_right (Node *r) { right = r ; }
    void set_opcode (Token op) { opcode = op ;}
    void set_type (DIE *t) { type = t ; }
    int num_variables() ;
protected:
    Node *left ;
    Node *right ;
private:
    void convert(EvalContext &context, DIE *ltype, Value &l, DIE *rtype, Value &r) ;
    bool scale(EvalContext &context, DIE *ltype, Value &l, DIE *rtype, Value &r, Value &result) ;
    Token opcode ; 
} ;

class CallExpression: public Node  {
public:
    CallExpression(SymbolTable *symtab, Node *left, std::vector<Node *> & args) ;
    ~CallExpression() ; 
    virtual Value evaluate (EvalContext &context) ;
    bool is_local()  ;
    int num_variables() ;
protected:
private:
    Node *left ; 
    std::vector<Node *> args ; 
    bool is_func ;
    DIE *resolve_overloads (EvalContext &ctx, bool thisadded, std::vector<DIE*> &overloads) ;
} ;

class IntrinsicExpression: public Node  {
public:
    IntrinsicExpression(SymbolTable *symtab, IntrinsicIdentifier *left, std::vector<Node *> & args) ;
    ~IntrinsicExpression() ; 
    virtual Value evaluate (EvalContext &context) ;
    int num_variables() ;
protected:
private:
    Intrinsic it ;
    std::vector<Node *> args ; 
} ;

class AssignmentExpression: public Node  {
public:
    AssignmentExpression(SymbolTable *symtab, Token tok, Node *left, Node *right) ;
    ~AssignmentExpression() ; 
    virtual Value evaluate (EvalContext &context) ;
    bool is_local()  ;
    int num_variables() ;
protected:
private:
    Value convert (EvalContext &context, Value &rhs) ;
    Token tok ;
    Node *left ; 
    Node *right ; 
} ;

// this is used for construction of fortran types e.g. foo(1,2,'hello')
class ConstructorExpression : public Node {
public:
    ConstructorExpression (SymbolTable *symtab, Node *structnode, std::vector<Node *> & values) ;
    ~ConstructorExpression() ;
    virtual Value evaluate (EvalContext &context) ;
    void set_value (EvalContext &context, Value &value, DIE *type) { throw Exception ("Constructors are not lvalues") ; }
    int num_variables() ;
protected:
    Node *structnode ;
    std::vector<Node*> values ;
} ;

class ArrayExpression: public Node  {
public:
    ArrayExpression(SymbolTable *symtab, Node *array, std::vector<Node *> & indices) ;
    ~ArrayExpression() ; 
    virtual Value evaluate (EvalContext &context) ;
    void set_value (EvalContext &context, Value &value, DIE *type) ;
    bool is_local()  ;
    int num_variables() ;
protected:
private:
    Node *array ; 
    std::vector<Node *> indices ; 
    Value get_dimension_value (EvalContext &context, TypeArray *die, int dim, Value &v, Node *index) ;
    bool split_indices (EvalContext &context, DIE *ltype, Value &addr, std::vector<Node *> &indices, Value &result) ;
} ;

class ExpressionHandler {
    friend class RegisterExpression ;
    friend class CallExpression ;
    friend class Expression ;
    friend class MemberExpression ;
    friend class DebuggerVarExpression ;

public:
    ExpressionHandler(SymbolTable *symtab, Architecture *arch, int language) ;
    virtual ~ExpressionHandler() ; 
    void error (std::string s) ;
    void error (const char *format, ...) ;
    void warning (std::string s) ;
    Node *parse (std::string expr, Process *process, int &end) ; 
    Node *parse (std::string expr, Process *process) ; 
    Node *parse_single (std::string expr, Process *process, int &end) ; 

protected:
    int flags ; 
    SymbolTable *symtab ; 
    void skip_spaces () ;
    virtual bool istoken (char ch) = 0 ;
    virtual Token getNextToken () = 0 ;
    void nextToken ()  ;
    bool match (Token tok) ;
    void needbrack (Token b) ;
    std::string line ; // whole line
    uint ch ; // current position
    uint lastch ;                        // last position
    std::string spelling ; // spelling for identifiers and strings
    int64_t number ; // value of int
    double fpnumber ;   // value of real
    Token currentToken ; 
    typedef std::map<std::string, Token> KeywordMap ;
    KeywordMap reserved_words ; 
    Process *current_process ; 
    Architecture *arch ;
    virtual Node *expression() = 0 ;
    virtual Node *single_expression() = 0 ;
    int language ;
    int ptrsize() ;
private:
    
} ;

class CExpressionHandler: public ExpressionHandler  {
public:
    CExpressionHandler(SymbolTable *symtab, Architecture *arch, int language) ;
    ~CExpressionHandler() ; 
protected:
private:
    bool istoken (char ch) ;
    Token getNextToken ()  ;
    Node *primary () ;
    Node *unary () ;
    Node *postfix () ;
    Node *mult () ;
    Node *add () ;
    Node *shift () ;
    Node *comparison () ;
    Node *equality () ;
    Node *bit_and () ;
    Node *bit_xor () ;
    Node *bit_or () ;
    Node *logand () ;
    Node *logor () ;
    Node *conditional () ;
    Node *single_expression () ;
    Node *assignment_expression() ;
    Node *expression () ;

    typedef std::stack<DIE*> TypeStack ;

    DIE *parse_typespec() ;             // parse a type specification
    DIE *type() ;
    void declaration (TypeStack &stack) ;
    void funcarray (TypeStack &stack) ;
    void pointer (TypeStack &stack) ;
    void base (TypeStack &stack) ;

    bool found_lparen ;

} ;

class FortranExpressionHandler: public ExpressionHandler  {
public:
    FortranExpressionHandler(SymbolTable *symtab, Architecture *arch, int language) ;
    ~FortranExpressionHandler() ; 
protected:
private:
    bool istoken (char ch) ;
    Token getNextToken ()  ;
    KeywordMap operators ;

    typedef std::map<std::string, Intrinsic> IntrinsicMap ;
    IntrinsicMap intrinsics ;

    Node *monadic_defined() ;
    Node *primary () ;
    Node *member () ;
    Node *array () ;
    Node *cast () ;
    Node *power () ;
    Node *mult () ;
    Node *unary () ;
    Node *add () ;
    Node *strcat () ;
    Node *comparison () ;
    Node *bit_not(); 
    Node *bit_and(); 
    Node *bit_or(); 
    Node *bit_equiv(); 
    Node *dyadic_defined() ;
    Node *assignment_expression() ;
    Node *expression() ;
    Node *single_expression () ;

    DIE *parse_typespec() ;             // parse a type specification
    int get_kind (int def) ;

    bool found_lparen ;
    bool double_precision ;             // floating point number is double precision
} ;


// functions

#endif
