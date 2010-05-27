/****************************************************
 * This file contains routines for constructing low *
 * level operators.
 ***************************************************/

#ifndef _EXPR_LOW_H_
#define _EXPR_LOW_H_

#include "expr_node.h"

ex_node_t* ex_new_ldid(int);
ex_node_t* ex_new_stid(ex_node_t*,int);

#endif

