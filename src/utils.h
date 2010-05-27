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

file: utils.h
created on: Fri Aug 13 17:44:18 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#ifndef utils_h_included
#define utils_h_included

#include "dbg_types.h"
#include <string>
#include <regex.h>

class EvalContext ;

namespace Utils {

struct Regex {
    Regex (int s, int e) : start(s), end(e), length (e-s+1) {}
    int start ;
    int end ;
    int length ;
} ;

typedef std::vector<Regex> RegexResult ;

class RegularExpression {
public:
    RegularExpression (std::string pattern) ;
    ~RegularExpression() ;
    RegexResult match (std::string s) ;
    bool matches(std::string s);

private:
    regex_t comp ;
} ;

std::string simplify_type (std::string type) ;
void print_string (EvalContext &context, std::string s);
std::string toUpper (std::string s) ;

void expand_path(std::string& path);
bool is_completion(const char*, const char*);

}

#endif
