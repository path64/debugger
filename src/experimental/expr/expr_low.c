/****************************************************
 * This file contains routines for constructing low *
 * level operators.
 ***************************************************/

#include "expr_low.h"
#include <stdlib.h>

ex_node_t*
ex_new_ldid(int id) {
  ex_node_t* r;

  r = ex_new_op0(EX_LO_LDID);
  r->_sym_id = id;

  return r;
}

ex_node_t*
ex_new_stid(ex_node_t* a, int id) {
  ex_node_t* r;

  r = ex_new_op1(EX_LO_STID, a);
  r->_sym_id = id;

  return r;
}

