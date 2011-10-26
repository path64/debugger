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

file: file_info.cc
created on: Fri Aug 13 11:07:34 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#include "utils.h"
#include "file_info.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <fstream>

File::File (DirectoryTable &dt, std::string cdir, std::string _name, int _dir, int _mtime, int _size)
    : name(_name),
      dir(_dir),
      mtime(_mtime),
      size(_size),
      dirtable(dt),
      stream(NULL),
      comp_dir(cdir),
      time(0),
      warned(false)
{
    basename = _name ;
    if (dir != 0) {
        dirname = dirtable[dir] ;
        this->name = dirname + "/" + basename ;
    }

}

File::~File() {
}

static bool file_exists (std::string name) {
    struct stat st ;
    return stat (name.c_str(), &st) == 0 ;
}

void File::open(DirectoryTable &dirlist) {
   if (stream != NULL) {
       return ;                       // already open
   }
   pathname = name ;
   // search using the dirlist, backwards
   for (int i = (int)dirlist.size() - 1 ; i >= 0 ; i--) {
       std::string _dirname = dirlist[i] ;
       if (_dirname == "$cwd") {
           _dirname = getenv ("PWD") ;
       } else if (_dirname == "$cdir") {
           _dirname = comp_dir ;
       }
       if (file_exists (_dirname + "/" + name)) {
           pathname = _dirname + '/' + name ;
           break ;
       }
   }

   stream = new std::ifstream (pathname.c_str()) ;               
   if (stream == NULL || !stream->is_open()) {
       std::cout << "Unable to find file "  <<  name << '\n' ;
       return ;
   }   
   struct stat st ;
   if (stat (pathname.c_str(), &st) == 0) {
       time = st.st_mtime ;                     // record time
   }
   int lineno = 1 ;
   int offset = 0 ;
   while (stream->good()) {
       std::string line;
       lines[lineno] = offset ;
       getline(*stream, line);
       offset += line.size() + 1 ;
       lineno++ ;
   }
   nlines = lineno - 2 ;
   stream->clear() ;            // clear EOF condition
}

bool File::check_time(PStream &os) {
    if (warned) return false ;

    struct stat st ;
    if (stat (pathname.c_str(), &st) == 0) {
        if (time != 0 && time < st.st_mtime) {
            os.print ("Warning: file %s has been modified\n", pathname.c_str()) ;
            warned = true ;
            return true ;
        }
    }
    return false ;
}

void File::show_line(int lineno, PStream &os, bool indent, int currentline) {
          check_time (os) ;
          LineMap::iterator li = lines.find (lineno) ;
          if (li == lines.end()) {
              throw Exception ("Can't find line %d in file %s.\n", lineno, name.c_str()) ;
          } else {
              stream->clear() ;
              stream->seekg (li->second, std::ios_base::beg) ;
              std::string line ;
              std::getline (*stream, line) ;
              if (indent) {
                  if (currentline == lineno) {
                      os.print (">%d   %s\n", lineno, line.c_str()) ;
                  } else {
                      os.print (" %d   %s\n", lineno, line.c_str()) ;
                  }
              } else {
                  os.print ("%d   %s\n", lineno, line.c_str()) ;
              }
          }
}

void File::show_line(int sline, int eline, PStream &os, bool indent, int currentline) {
    check_time (os) ;
    LineMap::iterator li = lines.find (sline) ;
    if (li != lines.end()) {
        stream->clear() ;
        stream->seekg (li->second, std::ios_base::beg) ;
        int line = sline ;
        while (line < eline) {
            std::string linedata ;
            std::getline (*stream, linedata) ;
            if (indent) {
                if (currentline == line) {
                    os.print (">%d   %s\n", line, linedata.c_str()) ;
                } else {
                    os.print (" %d   %s\n", line, linedata.c_str()) ;
                }
            } else {
                os.print ("%d   %s\n", line, linedata.c_str()) ;
            }
            line++ ;
        }
    } else {
       throw Exception ("Can't find line %d in file %s.\n", sline, name.c_str()) ;
    }
}

// search from the line number for the regex and write the resulting line number and
// text for the line

bool File::search (std::string regex, int &lineno, std::string &text) {
    Utils::RegularExpression re (regex) ;
    LineMap::iterator li = lines.find (lineno) ;
    if (li != lines.end()) {
        stream->clear() ;
        stream->seekg (li->second, std::ios_base::beg) ;
        while (stream->good()) {
            std::string linedata ;
            std::getline (*stream, linedata) ;
            if (re.match (linedata).size() != 0) {
                text = linedata ;
                return true ;
            }
            lineno++ ;
        }
    } else {
        throw Exception ("No line %d in file %s", lineno, name.c_str()) ;
    }
    return false ;
}

