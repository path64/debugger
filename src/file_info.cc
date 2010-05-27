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

File::File (DirectoryTable &dt, std::string cdir, std::string name, int dir, int mtime, int size)
    : name(name),
      dir(dir),
      mtime(mtime),
      size(size),
      dirtable(dt),
      stream(NULL),
      comp_dir(cdir),
      time(0),
      warned(false)
{
    basename = name ;
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
       std::string dirname = dirlist[i] ;
       if (dirname == "$cwd") {
           dirname = getenv ("PWD") ;
       } else if (dirname == "$cdir") {
           dirname = comp_dir ;
       }
       if (file_exists (dirname + "/" + name)) {
           pathname = dirname + '/' + name ;
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

