/****************************************************
 * This file contains routines for putting together *
 * and accessing an expression evaluation context.  *
 ***************************************************/

#ifndef _EXPR_ARENA_H_
#define _EXPR_ARENA_H_

typedef struct _ex_arena_t {
   int cur_tmp;
} ex_arena_t;

ex_arena_t* ex_arena_new();

static int
ex_arena_get_tmp(ex_arena_t* a) {
   return a->cur_tmp++; 
}

#endif

