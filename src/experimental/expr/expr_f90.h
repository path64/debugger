/***************************************************
 * This file contains routines for lowering simple *
 * syntatic constructs into low-level operations   *
 ***************************************************/

#ifndef _EXPR_F90_H_
#define _EXPR_F90_H_

#include "expr_node.h"

void ex_f90_trim_input(char*);
ex_node_t* ex_f90_new_str(const char*, int);
ex_node_t* ex_f90_new_float(const char*, int);
ex_node_t* ex_f90_new_integ(const char*, int, int);
ex_node_t* ex_f90_kind_cast(ex_node_t*,ex_node_t*);
ex_node_t* ex_f90_do_range(ex_node_t*,ex_node_t*,ex_node_t*);

void ex_f90_chk_syntax(ex_node_t*);

#endif
