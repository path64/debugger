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

file: cli_param.h
created on: Wed Jan  5 20:51:26 PST 2005
author: James Strother <jims@pathscale.com>

*/

#include <assert.h>
#include <algorithm>
#include "cli_param.h"
#include <cstring>
/*  This describes all of the options which are available to the
 *  user through the command line interface.  The first column is
 *  the relevent enumerated type, the second is the command name,
 *  the third is the type, and the last column is the default for
 *  the option.  If the value is an enumerated type, the default
 *  is an array of strings.  The first value in the array is the
 *  default value for that option.
 */


/* The following are lists of strings to be used as possible
 * values for enumerated types.  These lists are referenced
 * in the param_list global variable.  The order of these
 * lists much match the order in cli_param.h
 */ 

static const char* fork_enum[] = {
  "parent", "child", "both", "ask", NULL
};

static const char* lang_enum[] = {
  "auto", "c", "c++", "fortran", NULL
};

static const char* endn_enum[] = {
  "auto", "little", "big", NULL
};

static const char* anno_enum[] = {
  "0", "1"
};


/* The following parameters are listed mostly by type and then
 * alphabetically by the enumerated type name.  The exceptions
 * are those hideously long parameters which are stuck at the
 * end for aesthetic reasons.
 */

#define TRUE  ((long)1)
#define FALSE ((long)0)
#define P(X)  ((long)X)

CliParamEntry param_list[] = {
   {PRM_AS_SHELL,  PARAM_BOOL,   FALSE, "shell-mode",
       "Executing unknown commands with shell"
   },
   {PRM_CONFIRM,   PARAM_BOOL,   TRUE,  "confirm",
       "Confirmation of dangerous commands"
   },
   {PRM_HEIGHT,    PARAM_INT,    0,     "height",
      "Height of the display screen"
   },
   {PRM_LIST_LN,   PARAM_INT,    10,    "listsize",
      "Number of source lines to display"
   },
   {PRM_MULTI_PR,  PARAM_BOOL,   FALSE, "multi-process",
      "Support for multi-processing"
   },
   {PRM_P_7BIT,    PARAM_BOOL,   TRUE,  "print sevenbit-strings",
      "Printing 8-bit characters in octal"
   },
   {PRM_P_ADDR,    PARAM_BOOL,   TRUE,  "print address",
      "Printing address of reference type"
   },
   {PRM_P_CLIST,   PARAM_BOOL,   TRUE,  "print std-list",
      "Pretty printing of C++ lists" 
   },
   {PRM_P_CMAP,    PARAM_BOOL,   TRUE,  "print std-map",
      "Pretty printing of C++ maps"
   },
   {PRM_P_CSTR,    PARAM_BOOL,   TRUE,  "print std-string",
      "Pretty printing of C++ strings"
   }, 
   {PRM_P_CVEC,    PARAM_BOOL,   TRUE,  "print std-vector",
      "Pretty printing of C++ vectors"
   },
   {PRM_P_ELEM,    PARAM_INT,    100,   "print elements",
      "Number of array elements to print"
   },
   {PRM_P_NSTOP,   PARAM_BOOL,   FALSE, "print null-stop",
      "Printing char arrays as strings"
   },
   {PRM_P_OBJECT,  PARAM_BOOL,   TRUE,  "print objects",
      "Printing of dynamically typed objects"
   },
   {PRM_P_PRETTY,  PARAM_BOOL,   TRUE,  "print pretty",
      "Pretty print of structures"
   },
   {PRM_P_REPS,    PARAM_INT,    10,    "print repeats",
      "Number of printed repeats allowed"
   },
   {PRM_P_SDIGS,   PARAM_INT,    10,    "print sigdigits",
      "Number of significant digits to use"
   },
   {PRM_P_STATIC,  PARAM_BOOL,   TRUE,  "print static-members",
      "Printing of static members"
   },
   {PRM_P_TPLATE,  PARAM_BOOL,   TRUE,  "print simplify-templates",
      "Printing simplified templates"
   },
   {PRM_P_ARRAY,   PARAM_BOOL,   TRUE,  "print array",
      "Pretty printing of arrays"
   },
   {PRM_P_UNION,   PARAM_BOOL,   TRUE,  "print union",
      "Printing of union members"
   },
   {PRM_PAGINATE,  PARAM_BOOL,   TRUE,  "pagination",
      "Paging of printed output"
   },
   {PRM_STL_STEP,  PARAM_BOOL,   TRUE,  "std-step",
      "Stepping over C++ STL operators"
   },
   {PRM_WIDTH,     PARAM_INT,  0,       "width",
      "Width of the display screen"
   },
   {PRM_VERBOSE,   PARAM_BOOL,   FALSE, "verbose",
      "Verbose output"
   },
   {PRM_HSTSIZE,   PARAM_INT,    1000,  "history size",
      "Number of commands in history"
   },
   {PRM_HSTSAVE,   PARAM_BOOL,   TRUE,  "history save",
      "Saving history to file on exit"
   },
   {PRM_ARGS,      PARAM_STR,    P(""),  "args",
      "Argument list passed to program"
   },
   {PRM_PROMPT,    PARAM_STR,    P("pathdb> "), "prompt",
      "Prompt used for command interface"
   },
   {PRM_FOL_FORK,  PARAM_ENUM,   P(fork_enum), "follow-fork-mode",
      "Process to follow after a fork event"
   },
   {PRM_LANGUAGE,  PARAM_ENUM,   P(lang_enum), "language",
      "Current program source language"
   },
   {PRM_ENDIAN,    PARAM_ENUM,   P(endn_enum), "endian",
      "Endianness of the host platform"
   },
   {PRM_ANNOTE,    PARAM_ENUM,   P(anno_enum), "annotate",
      "Current annotation level"
   },
   {PRM_HSTFILE,   PARAM_STR,    P("~/.pathdb_history"), "history filename",
      "The file to save command history"
   },
   {PRM_STOP_SL,   PARAM_BOOL,   FALSE, "stop-on-solib-events",
      "Catch shared library events"
   },
   {PRM_USE_HW,    PARAM_BOOL,   TRUE, "can-use-hw-watchpoints",
      "Support for hardware watchpoints"
   },
   {PRM_NIL, PARAM_BOOL, 0, NULL, NULL}
};


/* The following members functions construct a hash out of
 * the data which is listed above.
 */

CliParam::CliParam(PStream& _os) : os(_os) {
   CliParamEntry* e = param_list;

   /* read in common data */
   while (e->name != NULL) {
#ifdef DEBUG
      std::map<int,long>::iterator i;
      i = valmap.find(e->ident);
      assert(i == valmap.end());
#endif

      /* set type that get checks */
      typmap[e->ident] = e->type;

      /* make vector to sort show */
      nmvec.push_back(e->name);

      /* deconvolute the storage */
      if (e->type == PARAM_STR) {
         const char* c= (char*)e->dflt;
         long v = (long)strdup(c);
         valmap[e->ident] = v;
         e++; continue;
      }

      if (e->type == PARAM_ENUM) {
	 valmap[e->ident] = 0;
         e++; continue;
      }

      valmap[e->ident] = e->dflt;
      e++;
   }

   /* sort the vector of names */
   std::sort(nmvec.begin(), nmvec.end());
}

CliParam::~CliParam() {
   std::map<int,long>::iterator i;

   for (i=valmap.begin(); i!=valmap.end(); i++) {
      if (typmap[i->first] == PARAM_STR) {
         free((void*)i->second);
      }
   }
}

#define WARN_INVALID   "unknown user parameter name `%s\'\n"
#define WARN_NOT_INT   "parameter requires positive integer argument\n"
#define WARN_NOT_BOOL  "parameter requires boolean (on/off) argument\n"

bool
CliParam::set(const char* key, const char* val) {
   CliParamEntry* e;

   /* search for string */
   e = name2entry(key);

   /* check if found */
   if (e == NULL) {
      return true;
   }

   if (e->type == PARAM_BOOL) {
      if (on_bool(val)) {
         valmap[e->ident] = 1;
      } else if (off_bool(val)) {
         valmap[e->ident] = 0;
      } else {
         os.print(WARN_NOT_BOOL);
      }
      return false;
   }

   if (e->type == PARAM_INT) {
      char* endptr;
      long int dec;

      dec = strtol(val,&endptr,10);
      if (*endptr == '\0' && dec >= 0) {
         valmap[e->ident] = dec;
      } else {
         os.print(WARN_NOT_INT);
      }
      return false;
   }

   if (e->type == PARAM_STR) {
      char* c = strdup(val);
      if (c == NULL) return false;

      free((void*)valmap[e->ident]);
      valmap[e->ident] = (long)c;
      return false;
   } 

   if (e->type == PARAM_ENUM) {
      unsigned int i = 0;
      const char** f;

      f = (const char**)e->dflt;

      /* scan for key */
      while (f[i] != NULL) {
         if (!strcmp(f[i],val)) {
            break; 
         } i++;
      }

      /* fail so nicely */
      if (f[i] == NULL) {
         list_enum(f);
         return false;
      }

      /* rely on ordering */
      valmap[e->ident] = i; 
   }

   return false;
}

void
CliParam::show_values() {
   for (unsigned i=0; i<nmvec.size(); i++) {
      const char* name = nmvec[i].c_str();
      os.print("%s: ", name);
      show_value(name);
   }
}

void
CliParam::show_value(const char* key) {
   CliParamEntry* e = name2entry(key);
   if (e == NULL) {
      os.print(WARN_INVALID, key);
      return;
   }

   long val = valmap[e->ident];

   os.print("%s is ", e->help);
   switch (e->type) {
   case PARAM_ENUM:
      show_enum(val,e->dflt);
      break;
   case PARAM_BOOL:
      show_bool(val);
      break;
   case PARAM_INT:
      show_int(val);
      break;
   case PARAM_STR:
      show_str(val);
      break;
   }

   /* special case auto settings */
   if (e->ident == PRM_ENDIAN &&
       (EndianType)val == ENDIAN_AUTO) {
      os.print(" (currently little endian)");
   }

   /* cap off the output line */
   os.print("\n");
}

void
CliParam::complete(const char* key,
   std::list<std::string>& l) {
   unsigned n = strlen(key);

   for (unsigned i=0; i<nmvec.size(); i++) {
       const char* c = nmvec[i].c_str();

       /* skip shorter names */
       if (strlen(c) < n) continue;

       /* compare first chars */
       if (!strncmp(c,key,n)) {
          l.push_back(c);
       }
   }

}

CliParamEntry*
CliParam::id2entry(int id) {
   CliParamEntry* e = param_list;

   while (e->name != NULL) {
      if (e->ident == id) break;
      e++;
   }

   if (e->name == NULL)
      return NULL;

   return e;
}

CliParamEntry*
CliParam::name2entry(const char* s) {
   CliParamEntry* e = param_list;

   while (e->name != NULL) {
      if (!strcmp(e->name,s)) break;
      e++;
   }

   if (e->name == NULL)
      return NULL;

   return e;
}

bool
CliParam::on_bool(const char* v) {
   return !strcmp(v, "1") ||
          !strcmp(v, "on") ||
          !strcmp(v, "true") ||
          !strcmp(v, "yes") ||
          !strcmp(v, "enable") ||
          !strcmp(v, "enabled");
}

bool
CliParam::off_bool(const char* v) {
   return !strcmp(v, "0") ||
          !strcmp(v, "off") ||
          !strcmp(v, "false") ||
          !strcmp(v, "no") ||
          !strcmp(v, "disable") ||
          !strcmp(v, "disabled");
}

void
CliParam::list_enum(const char** v) {
   os.print("invalid value, choices are: ");
   os.print("%s", *v++);
   while (*(v+1) != NULL) {
      os.print(", %s", *v++);
   }
   if (*v != NULL) {
      os.print(", or %s", *v++);
   }
   os.print("\n");
}

void
CliParam::show_enum(long val, long chc) {
   char** p = (char**) chc;
   os.print("%s", p[val]);
}

void
CliParam::show_bool(long val) {
   if (val) {
      os.print("enabled");
   } else {
      os.print("disabled");
   }
}

void
CliParam::show_int(long val) {
   os.print("%ld", val);
}

void
CliParam::show_str(long val) {
   const char* c = (char*)val;
   os.print("\"%s\"", c); 
}

