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

file: cli_history.h
created on: Tue Jan 11 22:45:44 PST 2005
author: James Strother <jims@pathscale.com>

*/

#ifndef _CLI_HISTORY_H_
#define _CLI_HISTORY_H_

#include <string>
#include <list>

#include "pstream.h"

class CliHistory {
public:
   CliHistory(PStream& os);

   /* load history from file */
   void set_file(const std::string&);

   /* set maximum list size */
   void set_size(unsigned);

   /* add a command to history */
   void push_back(const std::string&); 

   /* write back to file */
   void save_file();

   /* print to screen */
   void show(int num);

   /* get the core list of lines */
   std::list<std::string>* get_list() {
      return &lines;
   }

private:
   /* state variables */
   PStream& os;
   char* fname;
   unsigned max_size;

   /* big list of lines */
   std::list<std::string> lines;
};

#endif
