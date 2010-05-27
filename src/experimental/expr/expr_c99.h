/***************************************************
 * This file contains routines for lowering simple *
 * syntatic constructs into low-level operations   *
 ***************************************************/

#ifndef _EXPR_C99_H_
#define _EXPR_C99_H_

#include "expr_node.h"

ex_node_t* ex_c99_new_str(const char*);
ex_node_t* ex_c99_new_char(const char*);
ex_node_t* ex_c99_modified_int(int);
ex_node_t* ex_c99_sizeof_expr(ex_node_t*);
ex_node_t* ex_c99_declare_list(ex_node_t*,ex_node_t*);
ex_node_t* ex_c99_reduce_dlist(ex_node_t*);
ex_node_t* ex_c99_func_pointer(ex_node_t*,ex_node_t*,ex_node_t*);
ex_node_t* ex_c99_decl_ptr_ary(ex_node_t*,ex_node_t*,ex_node_t*);
ex_node_t* ex_c99_call_tmpl(ex_node_t*,ex_node_t*,ex_node_t*);

void ex_c99_chk_syntax(ex_node_t*);

#endif
