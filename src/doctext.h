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

file: doctext.h
created on: Fri Dec 17 11:03:24 PDT 2004
author: James Strother <jims@pathscale.com>

*/

typedef struct
{
  const char* name;
  unsigned int id;
} pathdb_help_entry_t;

extern const char* PATHDB_HELP_ENTRIES[];
extern pathdb_help_entry_t PATHDB_HELP_TABLE[];

