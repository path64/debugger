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
