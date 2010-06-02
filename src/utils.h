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
