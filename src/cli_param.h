/*

   Copyright (c) 2004-2005 PathScale, Inc.  All rights reserved.
   PathDB is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation version 3

   PathDB is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with PathDB; see the file COPYING.  If not, write to the Free
   Software Foundation, 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

file: cli_param.h
created on: Wed Jan  5 20:51:26 PST 2005
author: James Strother <jims@pathscale.com>

*/

#ifndef _CLI_PARAM_H_
#define _CLI_PARAM_H_

#include <stdlib.h>
#include <assert.h>
#include <vector>
#include <string>
#include <list>
#include <map>

#include "pstream.h"

/* The following are enumerated types to mirror
 * the enumerated types that are present in the
 * command line interpreter parameters
 */

enum ForkType {
   FORK_PARENT=0,
   FORK_CHILD=1,
   FORK_BOTH=2,
   FORK_ASK=3
};

enum LangType {
   LANG_AUTO=0,
   LANG_C=1,
   LANG_CXX=2,
   LANG_F90=3
};

enum EndianType {
   ENDIAN_AUTO=0,
   ENDIAN_LITTLE=1,
   ENDIAN_BIG=2
};

enum AnnoteType {
   ANNOTATE_NORMAL=0,
   ANNOTATE_FULLN=1
};


/* The following are enumerated types used to
 * describe each command line parameter 
 */

enum CliParamType {
   PARAM_BOOL,
   PARAM_ENUM,
   PARAM_INT,
   PARAM_STR
};

enum CliParamId {
   PRM_AS_SHELL,   PRM_CONFIRM,    PRM_HEIGHT,
   PRM_LIST_LN,    PRM_MULTI_PR,   PRM_P_7BIT,
   PRM_P_ADDR,     PRM_P_CLIST,    PRM_P_CMAP,
   PRM_P_CSTR,     PRM_P_CVEC,     PRM_P_ELEM,
   PRM_P_NSTOP,    PRM_P_OBJECT,   PRM_P_PRETTY,
   PRM_P_REPS,     PRM_P_SDIGS,    PRM_P_STATIC,
   PRM_P_TPLATE,   PRM_P_ARRAY,    PRM_P_UNION,
   PRM_PAGINATE,   PRM_STL_STEP,   PRM_WIDTH,
   PRM_ARGS,       PRM_PROMPT,     PRM_FOL_FORK,
   PRM_LANGUAGE,   PRM_ENDIAN,     PRM_STOP_SL,
   PRM_USE_HW,     PRM_ANNOTE,     PRM_VERBOSE,
   PRM_HSTFILE,    PRM_HSTSIZE,    PRM_HSTFSIZE,
   PRM_HSTSAVE,    PRM_NIL
};


/* The following are classes which are used to
 * access the information on each parameter
 */

struct CliParamEntry {
   CliParamId ident;
   CliParamType type;
   long int dflt;
   const char* name;
   const char* help;
};


class CliParam {
public:
   CliParam(PStream& os);
   ~CliParam();

   /* assign a new parameter value, returns
    * true if the parameter is not found */
   bool set(const char* key, const char* val);

   /* assign directly to value */
   void set(CliParamId id, long val) {
      valmap[id] = val;
   }

   /* access the current value */
   char* get_str(CliParamId id) {
      assert(typmap[id]==PARAM_STR);
      return (char*)valmap[id];
   }
   long get_int(CliParamId id) {
      assert(typmap[id]!=PARAM_STR);
      return (long)valmap[id];
   }

   /* show current values */
   void show_values();
   void show_value(const char*);

   /* completion function */
   void complete(const char*,
     std::list<std::string>&);
 
private:
   /* handy functions to have */
   CliParamEntry* id2entry(int id);
   CliParamEntry* name2entry(const char*);

   bool on_bool(const char* v);
   bool off_bool(const char* v);

   void list_enum(const char** v);

   void show_enum(long v, long c);
   void show_bool(long v);
   void show_int(long v);
   void show_str(long v);

   /* data descriptors */
   std::map<int,long> valmap;
   std::vector<std::string> nmvec;
   std::map<int,CliParamType> typmap;

   /* output stream */
   PStream& os; 
};

#endif

