/****************************************************
 * This file contains routines for lowering C99 ops *
 * into the mid-level representation.               *
 ***************************************************/

#ifndef _EXPR_C99_LOWER_H_
#define _EXPR_C99_LOWER_H_

#include "expr_node.h"
#include "expr_arena.h"

ex_node_t* ex_c99_lower(ex_node_t*,ex_arena_t*);

#endif

