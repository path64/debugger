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
