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

file: file_info.h
created on: Fri Aug 13 11:07:34 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#ifndef _FILE_INFO_H_
#define _FILE_INFO_H_

#include <vector>
#include <string>

class File;

typedef std::vector<std::string> DirectoryTable ;
typedef std::vector<File *> FileTable ;

#include "pstream.h"
#include "dbg_types.h"

class File {
public:
    File(DirectoryTable &dt, std::string comp_dir, std::string  name, int  dir, int  mtime, int  size) ;
    ~File() ;
    void open (DirectoryTable &dirlist) ;
    void show_line (int lineno, PStream &os, bool indent, int currentline) ;
    void show_line (int sline, int eline, PStream &os, bool indent, int currentline) ;
    bool search (std::string regex, int &lineno, std::string &text) ;

    std::string name ;
    int  dir ;
    int  mtime ;
    int  size ;
    DirectoryTable &dirtable ;
    typedef std::map<int, Offset> LineMap ;
    LineMap lines ; // map of line number vs offset
    std::ifstream *stream ;
    std::string comp_dir ;
    std::string basename ;              // base name (without dir)
    std::string dirname ;               // directory name
    std::string pathname ;              // full path
    int nlines ;                        // number of lines in file
    time_t time ;                       // time from file when first loaded
    bool check_time (PStream &os) ;
    bool warned ;
} ;

#endif
