/*

Copyright (c) 2004 PathScale, Inc.  All rights reserved.
Unpublished -- rights reserved under the copyright laws of the United
States. USE OF A COPYRIGHT NOTICE DOES NOT IMPLY PUBLICATION OR
DISCLOSURE. THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND TRADE
SECRETS OF PATHSCALE, INC. USE, DISCLOSURE, OR REPRODUCTION IS
PROHIBITED WITHOUT THE PRIOR EXPRESS WRITTEN PERMISSION OF PATHSCALE,
INC.

U.S. Government Restricted Rights:
The Software is a "commercial item," as that term is defined at 48
C.F.R. 2.101 (OCT 1995), consisting of "commercial computer software"
and "commercial computer software documentation," as such terms are used
in 48 C.F.R. 12.212 (SEPT 1995).  Consistent with 48 C.F.R. 12.212 and
48 C.F.R. 227-7202-1 through 227-7202-4 (JUNE 1995), all U.S. Government
End Users acquire the Software with only those rights set forth in the
accompanying license agreement. PathScale, Inc. 477 N. Mathilda Ave;
Sunnyvale, CA 94085.

file: cli_history.cc
created on: Tue Jan 11 22:45:44 PST 2005
author: James Strother <jims@pathscale.com>

*/

#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <cstring>
#include <string.h>

#include "utils.h"
#include "cli_history.h"

CliHistory::CliHistory(PStream& str) 
 : os(str), fname(NULL), max_size(0) {
}

void CliHistory::set_file(const std::string& f) {
    /* qualify name */
    std::string path(f);
    Utils::expand_path(path);

    /* kill array */
    lines.clear();

    /*  kill fname */
    if (fname != NULL) {
       free(fname);
    }

    /* make new string */
    fname = strdup(path.c_str());
    if (fname == NULL) {
       return;
    }

    /* open up history file */
    std::ifstream in (fname);
    if (in == NULL) return;

    /* read in file */
    std::string s;
    std::getline(in, s);
    unsigned nread = 0;
    while (in.good()) {
       if (nread++ > max_size) {
          lines.pop_front();
       }
       lines.push_back(s);
       std::getline(in, s);
    }

    /* file automatically closed */
}

void CliHistory::set_size(unsigned new_size) {
    /* set the member data */
    max_size = new_size;

    /* remove spurious entries */
    while (lines.size() > max_size) {
       lines.pop_front(); 
    }
}

void CliHistory::push_back(const std::string& s) {
   /* skip whitespace only */
   std::string::size_type t;
   t = s.find_first_not_of(" \t");
   if (t==std::string::npos) return;

   /* remove extra elements */
   if (lines.size() >= max_size) {
      lines.pop_front();
   }

   /* add it to list, calling base */
   lines.push_back(s);
}

void CliHistory::save_file() {
   /* open up history file */
   std::ofstream out (fname);
   if (!out || out.bad()) return;

   /* dump contents */
   std::list<std::string>::iterator i;
   for (i = lines.begin(); i != lines.end(); i++) {
      out << *i << std::endl;
   }

   /* file closed automatically */
}

void CliHistory::show(int num) {
   std::list<std::string>::iterator i;
   unsigned count = lines.size();

   /* start iterator at end */
   i = lines.end();

   /* scan to proper position */
   while ( num > 0 ) {
      count--; i--; 
   }

   /* catch unlimited list */
   if (i == lines.end()) {
      i = lines.begin(); 
      count = 1;
   }

   /* match forward thru list */
   for (i=lines.begin(); i!=lines.end(); i++) {
      os.print("%5d  %s\n", count++, i->c_str());
   }
}

