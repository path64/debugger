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

file: utils.cc
created on: Fri Aug 13 17:44:18 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#include "utils.h"
#include "dbg_except.h"
#include "pstream.h"
#include "process.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>

namespace Utils {

RegularExpression::RegularExpression (std::string pattern) {
    int e = regcomp (&comp, pattern.c_str(), REG_EXTENDED) ;
    if (e != 0) {
        char errbuf[256] ;
        regerror (e, &comp, errbuf, sizeof (errbuf)) ;
        throw Exception (errbuf) ;
    }
}

RegularExpression::~RegularExpression() {
    regfree (&comp) ;
}


RegexResult RegularExpression::match (std::string s) {
    regmatch_t result[10] ;
    int e = regexec (&comp, s.c_str(), 10, result, 0) ;
    RegexResult r ;
    if (e == REG_NOMATCH) {     
        return r ;
    }
    if (e != 0) {
        char errbuf[256] ;
        regerror (e, &comp, errbuf, sizeof (errbuf)) ;
        throw Exception (errbuf) ;
    }
    for (int i = 0 ; result[i].rm_so != -1 ; i++) {
       r.push_back (Regex (result[i].rm_so, result[i].rm_eo)) ;
    }
    return r ;
}

bool RegularExpression::matches(std::string s) {
    regmatch_t result[10];
    int e = regexec (&comp, s.c_str(), 10, result, 0);
    return (e != REG_NOMATCH);
}

struct {
    const char *comp ;
    const char *simp ;
} replacements[] = {
   {"basic_string<char,std::char_traits<char>,std::allocator<char> >", "std::string"},
   {NULL, NULL}
} ;

std::string simplify_type (std::string t) {
    for (int i = 0 ; replacements[i].comp != NULL ; i++) {
        for (;;) {
            std::string::size_type index = t.find (replacements[i].comp) ;
            if (index != std::string::npos) {
                t.replace (index, strlen (replacements[i].comp),  replacements[i].simp) ;
            } else {
                break; 
            }
        }
    }

    // remove all allocators
    for (;;) {
        std::string::size_type index = t.find ("std::allocator<") ;
        if (index == std::string::npos) {
            break ;
        }
        // back to comma
        if (t[index-1] == ' ') index-- ;
        if (t[index-1] == ',') index-- ;

        std::string::size_type i2 = index ;
        while (i2 < t.size() && t[i2] != '>') i2++ ;            // skip to >
        if (t[i2+1] == ' ') i2++ ;
        t.erase (index, i2-index) ;
    }
    return t ;
}



std::string toUpper (std::string s) {
    std::string news  ;
    for (unsigned int i = 0 ; i < s.size() ; i++) {
        news += toupper (s[i]) ;
    }
    return news ;
}

void print_char (PStream &os, char ch, bool printseven) {
   /* print escaped chars */
   switch (ch) {
   case '\n': os.print ("\\n"); return;
   case '\r': os.print ("\\r"); return;
   case '\a': os.print ("\\a"); return;
   case '\t': os.print ("\\t"); return;
   case '\v': os.print ("\\v"); return;
   case '\b': os.print ("\\b"); return;
   case '\f': os.print ("\\f"); return;
   case  '"': os.print ("\\\""); return;
   case '\\': os.print ("\\\\"); return;
   case '\0': os.print ("\\0"); return;
   case   27: os.print ("\\e"); return;
   }

   /* print 7-bit in octal */
   if (!isprint(ch) && printseven) {
      os.print("\\%03o", ch & 0xff);
   } else {
      os.print("%c", ch & 0xff) ;
   }
}

void print_string (EvalContext &context, std::string s) {
    char last_value = '\0' ;
    bool lv_valid = false;
    int nrepeats = 0 ;

    bool null_stop = context.process->get_int_opt(PRM_P_NSTOP);
    bool printseven = context.process->get_int_opt(PRM_P_7BIT);
    int maxchars = context.process->get_int_opt(PRM_P_ELEM);
    int repmin = context.process->get_int_opt(PRM_P_REPS);

    if (context.truncate_aggregates) {
        if (maxchars > 100) {
            maxchars = 100 ;
        }
    }

    for (unsigned int i = 0 ; i < s.size(); i++) {
        char ch = s[i] ;
        if (ch == 0 && null_stop) {
            break ;
        }
        if (lv_valid && ch == last_value) {
            nrepeats++ ;
        } else {
            if (nrepeats > 0) {
                if (repmin > 0 && nrepeats >= repmin) {
                    context.os.print (" <repeats %d times>", nrepeats + 1) ;
                    nrepeats = 0 ;
                } else {
                    while (nrepeats-- > 0) {
                        print_char (context.os, last_value, printseven) ;
                    }
                    nrepeats = 0 ;
                }
            }
            if (maxchars-- <= 0) {
                context.os.print ("...") ;
                break ;
            }
            print_char (context.os, ch, printseven) ;
            last_value = ch ;
            lv_valid = true ;
        }
    }

    if (nrepeats > 0) {
        if (repmin > 0 && nrepeats >= repmin) {
            context.os.print (" <repeats %d times>", nrepeats + 1) ;
        } else {
            while (nrepeats-- > 0) {
                print_char (context.os, last_value, printseven) ;
            }
        }
    }
}

static bool replace_tilde(std::string& path, unsigned& i) {
   unsigned j;

   /* match only tildes */
   if (path[i] != '~')
      return false;

   /* get the full tilde token */
   for (j=i+1; j<path.size(); j++) {
      if (!isalnum(path[j])) break;
   }

   /* extract pw information */
   struct passwd* pw;
   if (j == i + 1) {
      pw = getpwuid(getuid());
   } else {
      std::string nm = path.substr(i+1,j-i-1);
      pw = getpwnam(nm.c_str());
   }

   /* get home directory */
   char* home_path;
   if (pw == NULL) {
       home_path = getenv("HOME");
   } else {
       home_path = pw->pw_dir;
   }

   /* uh, just in case */
   if (home_path == NULL) {
      home_path = "";
   }

   /* construct new path */
   path.replace(i,j-i,home_path);

   /* move over position */
   i += strlen(home_path);

   /* was processed */
   return true;
}

void expand_path(std::string& path) {
    bool in_quotes = false;

    for (unsigned i=0; i < path.size(); i++) {
       /* catch any quotes */
       if (path[i] == '\'') {
          in_quotes = !in_quotes;
          continue;
       }

       /* ignore quoted material */
       if (in_quotes) continue;

       /* replace any tildes */
       if (replace_tilde(path, i))
          continue;
    }
}

bool is_completion (const char* a, const char* b) {
   /* no points for empties */
   if (*a == '\0') return false;

   /* push pointers forward */
   while (*a != '\0' && *a == *b) {
      a++; b++;
   }

   /* check if a is done */
   return (*a == '\0');
}

}
