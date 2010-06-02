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
