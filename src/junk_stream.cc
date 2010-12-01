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

file: junk_stream.cc
created on: Fri Aug 13 11:07:37 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#include "utils.h"
#include "junk_stream.h"
#include "process.h"
#include "dwf_spec.h"

#include <climits>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>

void JunkStream::disassemble (EvalContext &ctx, Address a) {
   Process *proc = ctx.process;
   proc->disassemble (a, a, false);
}

void JunkStream::print (EvalContext &ctx, int i)  {
   const char *fmt = to_format_string (ctx.fmt, false);

   if (fmt != NULL) {
      PStream::print (fmt, i);
      return;
   }

   int code = ctx.fmt.code;

   if (code == 't') {
      print_binary (i, sizeof(int)*CHAR_BIT);
      return;
   }

   if (code == 'n') {
      PStream::print ("%d", i);
      return;
   }

   /* XXX: uncomments this after dumb thing works
    throw Exception ("invalid print command");
   */
}

void JunkStream::print (EvalContext &ctx, int64_t i, int size, bool issigned)  {
   /* XXX: still need to clean up this function */
   if (size != 8 && issigned) {
       i <<= 64 - size*8;
       i >>= 64 - size*8;
   }
   const char *fmt = to_format_string (ctx.fmt, size == 8);
   if (fmt == NULL) {
       switch (ctx.fmt.code) {
       case 't':                       // binary
          print_binary (i, size);
          break;
       case 'i':                       // instruction
          throw Exception ("Cannot disassemble an integer");
          break;
       case 'n':                       // native
          if (issigned) {
              PStream::print ("%lld", i);
          } else {
              PStream::print ("%llu", i);
          }
          break;
       }
   } else {
       if (ctx.fmt.code == 'f') {              // floating point
           PStream::print (fmt, *(double*)&i);
       } else {
           PStream::print (fmt, i);
       }
   }
}

void JunkStream::print (EvalContext &ctx, double d)  {
   const char *fmt = to_format_string (ctx.fmt, false);
   int ndig = ctx.num_sigdigs;

   if (fmt != NULL) {
       PStream::print (fmt, d);
       return;
   }

   int code = ctx.fmt.code;

   if (code == 't') {
      print_binary ((int64_t)d, 64);
      return;
   }

   if (code == 'n') {
      char buf[1024];
      if (ctx.trunc_digs) {
         snprintf (buf, sizeof(buf), "%%.%dg", ndig);
      } else {
         snprintf (buf, sizeof(buf), "%%.%df", ndig);
      }
      PStream::print (buf, d);
      return;
   }

   /* XXX: uncomments this after dumb thing works
    throw Exception ("invalid print command");
   */
}

void JunkStream::print (EvalContext &ctx, unsigned int i)  {
   const char *fmt = to_format_string (ctx.fmt, false);

   if (fmt != NULL) {
       PStream::print (fmt, i);
       return;
   }

   int code = ctx.fmt.code;

   if (code == 't') {
      print_binary (i, 32);
      return;
   }

   if (code == 'n') {
      PStream::print ("%u", i);
      return;
   }

   /* XXX: uncomments this after dumb thing works
   throw Exception ("invalid print command");
   */
}

void JunkStream::print (EvalContext &ctx, unsigned long i)  {
   const char *fmt = to_format_string (ctx.fmt, false);

   if (fmt != NULL) {
       PStream::print (fmt, i);
       return;
   }

   int code = ctx.fmt.code;

   if (code == 'i') {
      print_binary (i, 32);
      /* XXX: what is this supposed to do */
      return;
   }

   if (code == 'n') {
      PStream::print ("%lu", i);
      return;
   }

   /* XXX: uncomments this after dumb thing works
   throw Exception ("invalid print command");
   */
}

void JunkStream::print (EvalContext &ctx, Address i)  {
   const char *fmt = to_format_string (ctx.fmt, true);

   if (fmt != NULL) {
      PStream::print (fmt, i);
      return;
   }

   int code = ctx.fmt.code;
  
   if (code == 't') { 
      print_binary (i, 64);
     /* XXX: this is wrong, what is target size */
      return;
   }

   if (code == 'i') {
      disassemble (ctx, i);
      return;
   }

   if (code == 'n') {
      PStream::print ("0x%llx", i);
      return;
   }

   /* XXX: uncomments this after dumb thing works
   throw Exception ("invalid print command");
   */
}

void JunkStream::print_address (EvalContext &ctx, Address i)  {
   /* XXX: still need to clean up this function */

   const char *fmt = to_format_string (ctx.fmt, true);
   if (fmt == NULL) {
       switch (ctx.fmt.code) {
       case 't':                       // binary
          print_binary (i, 64);
          break;
       case 'i':                       // instruction
          disassemble (ctx, i);
          break;
       case 'n':                       // native
          PStream::print ("0x%llx", i);
          break;
       }
   } else {
       PStream::print (fmt, i);
   }

   bool printsym = true;
   Address _end = ctx.process->lookup_function ("_end");
   if (i >= _end) {
       printsym = false;
   } else {
       Address _start = ctx.process->lookup_function ("_start");
       if (i <= _start) {
           printsym = false;
       } else {
           Section *section = ctx.process->find_section_at_addr (i);
           if (section != NULL) {
               if (section->get_name() == ".rodata") {
                   printsym = false;
               }
           }
       }
       
   }
   if (printsym) {
       Location loc = ctx.process->lookup_address (i);

       if (loc.get_symname() != "") {
           if (loc.get_offset() == 0) {
               PStream::print (" <%s>", loc.get_symname().c_str());
           } else {
               PStream::print (" <%s+%d>", loc.get_symname().c_str(), loc.get_offset());
           }
       }
   }
}

void JunkStream::print (EvalContext &ctx, void *p)  {
   /* XXX: this is wrong, should be target size */
   bool is64 = sizeof(p) == 8;

   const char *fmt = to_format_string (ctx.fmt, is64);

   if (fmt != NULL) {
      PStream::print (fmt, p);
      return;
   }

   int code = ctx.fmt.code;

   if (code == 't') {
      print_binary (reinterpret_cast<int64_t>(p), is64?64:32);
      return;
   }

   if (code == 'i') {
      disassemble (ctx, reinterpret_cast<Address>(p));
      return;
   }

   if (code == 's') {
      const char* s = (char*)p;
      Utils::print_string(ctx, s);
      return;
   }

   if (code == 'n') {
      if (is64) {
         PStream::print ("0x%llx", p);
      } else {
         PStream::print ("0x%x", p);
      }
      return;
   }

   /* XXX: uncomments this after dumb thing works
   throw Exception ("invalid print command");
   */
}

static const char* unescape (int ch) {
   switch (ch) {
   case '\a':  return "\\a";
   case '\b':  return "\\b";
   case '\r':  return "\\r";
   case '\n':  return "\\n";
   case '\v':  return "\\v";
   case '\f':  return "\\f";
   case '\t':  return "\\t";
   case '\\':  return "\\\\";
   case '\'':  return "\\\'";
   case '\0':  return "\\0";
   case 27:    return "\\e";
   }

   return NULL; 
}

void JunkStream::print_char (EvalContext& ctx, int ch) {
   const char* s_esc = unescape(ch);

   if ( s_esc != NULL ) {
      PStream::print ("\'%s\'", s_esc);
      return;
   }

   if ( !isprint(ch) ) {
      PStream::print ("\'\\%03o\'", ch);
      return;
   }

   PStream::print ("\'%c\'", ch);
   return;

}

void JunkStream::print (EvalContext &ctx, signed char ch)  {
   const char *fmt = to_format_string (ctx.fmt, true);

   if (fmt != NULL) {
      PStream::print (fmt, ch);
      return;
   }

   int code = ctx.fmt.code;

   if (code == 't') {
      print_binary (ch, 8);
      return;
   }

   if (code == 'n') {
      unsigned char u_ch = ch;
      PStream::print ("%d ", ch);
      print_char(ctx, u_ch);
      return;
   }

   /* XXX: uncomments this after dumb thing works
   throw Exception ("invalid print command");
   */
}

void JunkStream::print (EvalContext &ctx, unsigned char ch)  {
   const char *fmt = to_format_string (ctx.fmt, true);

   if (fmt != NULL) {
      PStream::print (fmt, ch);
      return;
   }

   int code = ctx.fmt.code;

   if (code == 't') {
      print_binary (ch, 8);
      return;
   }

   if (code == 'n') {
      PStream::print ("%u ", ch);
      print_char(ctx, ch);
      return;
   }

   /* XXX: uncomments this after dumb thing works
   throw Exception ("invalid print command");
   */
}

void JunkStream::print (EvalContext &ctx, bool v)  {
   const char *fmt = to_format_string (ctx.fmt, false);

   if (fmt != NULL) {
      PStream::print (fmt, v);
      return;
   }

   int code = ctx.fmt.code;

   if (code == 't') {
      print_binary (v, 32);
      return;
   }

   if (code == 'n') {
      switch (ctx.language & 0xff) {
      case DW_LANG_C89:
      case DW_LANG_C:
         PStream::print (v ? "TRUE" : "FALSE");     
         break;
      case DW_LANG_C_plus_plus:
         PStream::print (v ? "true" : "false");     
         break;
      case DW_LANG_Fortran77:
      case DW_LANG_Fortran90:
         PStream::print (v ? ".true." : ".false.");     
         break;
      default:
         print (v);
         break;
      }
      return;
   }

   /* XXX: uncomments this after dumb thing works
   throw Exception ("invalid print command");
   */
}

void JunkStream::print (const Value &v) {
   switch (v.type) {
   case VALUE_NONE:
      PStream::print ("none");
      break;
   case VALUE_INTEGER:
      PStream::print ("%lld", v.integer);
      break;
   case VALUE_REAL:
      PStream::print ("%.10g", v.real);
      break;
   case VALUE_STRING:
      PStream::print ("\"%s\"", v.str.c_str());
      break;
   case VALUE_REG:
      break;
   case VALUE_BOOL:
      if (v.integer) {
         PStream::print ("on");
      } else {
         PStream::print ("off");
      }
      break;
   default:;
   }
}


// convert a format code to a printf style format string.  If the code is
// not representible by printf then we return NULL and let the caller
// deal with it


// XXX: fortran format for pointers

// this array holds the format strings for filled displays of various
// sizes
static const char *fill_formats[4][4] = {
   {"0%01o", "0%04o", "0%08o", "0%016o"},	// octal
   {"0x%01x", "0x%04x", "0x%08x", "0x%016x"},	// hex
   {"0%01llo", "0%04llo", "0%08llo", "0%016llo"},	// 64 bit octal
   {"0x%01llx", "0x%04llx", "0x%08llx", "0x%016llx"}	// 64 bit hex
};

const char *
JunkStream::to_format_string(Format & fmt, bool is64bit)
{
   int si = 2;			// size index 

   switch (fmt.size) {
   case 'b': si = 0; break;
   case 'h': si = 1; break;
   case 'w': si = 2; break;
   case 'g': si = 3; break;
   }

   if (is64bit) {
      switch (fmt.code) {
      case 'o':
	 return fmt.fill ? fill_formats[2][si] : "0%llo";
      case 'x':
	 return fmt.fill ? fill_formats[3][si] : "0x%llx";
      case 'd':
	 return "%lld";
      case 'u':
	 return "%llu";
      case 't':
	 return NULL;
      case 'f':
	 return "%.10g";
      case 'a':
	 return fmt.fill ? fill_formats[3][si] : "0x%llx";
      case 'i':
	 return NULL;
      case 'c':
	 return "'%c'";
      case 's':
	 return NULL;
      default:
	 return NULL;
      }
   } else {
      switch (fmt.code) {
      case 'o':
	 return fmt.fill ? fill_formats[0][si] : "0%o";
      case 'x':
	 return fmt.fill ? fill_formats[1][si] : "0x%x";
      case 'd':
	 return "%d";
      case 'u':
	 return "%u";
      case 't':
	 return NULL;
      case 'f':
	 return "%.10g";
      case 'a':
	 return fmt.fill ? fill_formats[1][si] : "0x%x";
      case 'i':
	 return NULL;
      case 'c':
	 return "'%c'";
      case 's':
	 return NULL;
      default:
	 return NULL;
      }
   }
}

void
JunkStream::print_binary(int64_t v, int size)
{
   bool skipzero = true;
   for (int i = size * 8 - 1; i >= 0; i--) {
      int64_t b = (v >> i) & 1;
      if (i != 0 && b == 0 && skipzero) {
	 // skip leading zero (but not last one)
      } else {
	 skipzero = false;
	 print(b ? "1" : "0");
      }
   }
}
