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

file: gen_loc.h
created on: Mon May  2 13:08:35 PDT 2005
author: James Strother <jims@pathscale.com>

*/

#ifndef _GEN_LOC_H_
#define _GEN_LOC_H_

#include "file_info.h"

class SymbolTable;
class LineInfo;
class Process;
class Frame;
class Symbol;
class DIE;

class FunctionLocation {
public:
   FunctionLocation(SymbolTable *_symtab, Address _address,
        Symbol* _symbol, Address _startaddr, Address _endaddr) {
      symtab = _symtab;
      address = _address;
      symbol = _symbol;
      startaddr = _startaddr;
      endaddr = _endaddr;
   }

   ~FunctionLocation() {
      /* nothing */
   }

   std::string get_name();

   Address get_start_address() {
      return startaddr;      
   }
   Address get_end_address() {
      return endaddr;
   }

   bool at_first_line(Address);
   void print();
   Symbol* symbol; 
   Address address; 
   Address startaddr; 
   Address endaddr; 
private:
   SymbolTable *symtab;
};

class Location {
public:
   Location() {
      address = 0;
      file = NULL;
      line = -1;
      linfo = NULL;
      stab = NULL;
      func = NULL;
      offset = 0;
   }
   ~Location() { 
      /* nothing */
   }

   void set_addr(Address addr) { address = addr; }
   Address get_addr() const { return address; }

   void set_file(File* fl) { file = fl; }
   File* get_file() const { return file; }

   void set_line(int _line) { line = _line; }
   int get_line() const { return line; }

   void set_symname(const std::string& s) { funcname = s; }
   const std::string& get_symname() const { return funcname; }

   void set_lineinfo(LineInfo* _linfo) { linfo = _linfo; }
   LineInfo* get_lineinfo() const { return linfo; }

   void set_symtab(SymbolTable* _stab) { stab = _stab; }
   SymbolTable* get_symtab() const { return stab; }

   DIE* get_subp_die();
   void set_subp_die(DIE* _d) { subp_die = _d; }

   void set_funcloc(FunctionLocation* _func) { func = _func; }
   FunctionLocation* get_funcloc() const { return func; }

   void set_dirlist(DirectoryTable* _dt) { dirlist = _dt; }
   DirectoryTable* get_dirlist() const { return dirlist; } 

   void set_offset(int off) { offset = off; }
   int get_offset() const { return offset; }


   bool is_known() const {
      return (file != NULL && line != -1);
   }

   std::string get_filename() const {
      if (file != NULL) {
         return file->name;
      }
      return "<unknown>";
   }

   bool has_debug_info() {
     return file != NULL && func != NULL ;
   }

   bool equiv(const Location& ref) {
      return (is_known() && ref.is_known() &&
              get_filename() == ref.get_filename() &&
              line == ref.line);
  }

   void show_line(PStream &os, bool emacs_mode);

private:
   SymbolTable* stab;
   DirectoryTable* dirlist;
   FunctionLocation* func;
   LineInfo* linfo;
   Address address;
   File *file;
   int line;
   DIE* subp_die;
   std::string funcname;
   int offset;
};

#endif
