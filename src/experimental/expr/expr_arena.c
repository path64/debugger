/****************************************************
 * This file contains routines for putting together *
 * and accessing an expression evaluation context.  *
 ***************************************************/

#include "expr_arena.h"
#include <stdlib.h>

ex_arena_t*
ex_arena_new() {
   ex_arena_t* ret;

   ret = (ex_arena_t*)malloc(sizeof(ex_arena_t));

   ret->cur_tmp = 0;
   return ret;
}

