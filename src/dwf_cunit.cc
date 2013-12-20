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

file: dwf_cunit.cc
created on: Fri Aug 13 11:07:34 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#include "dwf_cunit.h"
#include "dwf_stab.h"
#include "dwf_names.h"
#include "dwf_info.h"
#include "dbg_dwarf.h"

#include "type_array.h"
#include "type_pointer.h"
#include "type_struct.h"
#include "type_qual.h"

#include <ios>

DwCUnit::DwCUnit (DwInfo *_dwarf) : dwarf(_dwarf), mainstream(BVector(),0), abbrev(NULL)
 {
}

DwCUnit::~DwCUnit() {
    // delete all the dies
    for (OffsetMap::iterator die = dies.begin() ; die != dies.end() ; die++) {
        delete die->second ;
    }

    // delete the files
    for (uint i = 0 ; i < file_table.size() ; i++) {
        delete file_table[i] ;
    }

    // delete the abbreviation
    delete abbrev ;

    // delete the line matrix
    for (uint i = 0 ; i < line_matrix.size() ; i++) {
        delete line_matrix[i] ;
    }
}

void DwCUnit::register_symbol (std::string name, DIE *die) {
    dwarf->register_symbol (name, die) ;
}

void DwCUnit::register_subprogram (std::string name, DIE *die) {
    int has_pc = die->getAttribute (DW_AT_low_pc, false) ;
    if (has_pc) {
        dwarf->register_subprogram (name, die) ;
    }
}

bool DwCUnit::is_64bit() {
  return dwarf->is_elf64();
}

/*
 * Note: the casts in the printfs are because SLES doesn't know these
 * are already long longs.  Stupid SLES.
 */
void DwCUnit::dump_dbg_info() {
   printf("  Compilation Unit @ %llx:\n", (unsigned long long)get_sec_offset());
   printf("   Length:        %lld\n", (long long)get_sec_length());
   printf("   Version:       %d\n", get_dwf_ver());
   printf("   Abbrev Offset: %lld\n", (long long)get_abb_offset());
   printf("   Pointer Size:  %d\n", get_ptr_size());

   dbg_info->dump();
}


void DwCUnit::add_die(Offset offset, DIE * die) {
    dies[offset] = die ;
    dofixup (offset, die) ;
    if (die->get_parent() == cu_die) {               // top level (global or file static) DIE?
        int isdecl = die->getAttribute (DW_AT_declaration, false) ;
        int tag = die->get_tag() ;
        
        if (!isdecl) {
            switch (tag) {
            case DW_TAG_variable:
            case DW_TAG_subprogram:
            case DW_TAG_typedef:
                //if (die->get_name() == "<unknown>") {
                //}
                register_symbol (die->get_name(), die) ;
                symbols[die->get_name()] = die ;
                break ;
            case DW_TAG_enumeration_type:
            case DW_TAG_structure_type:
            case DW_TAG_class_type:
            case DW_TAG_union_type:
                dwarf->register_struct (die->get_name(), die) ;
                break ;
            }
        }
    }
    
    if (die->get_tag() == DW_TAG_subprogram) {
        register_subprogram (die->get_name(), die) ;
        subprograms[die->get_name()] = die ;
    }

}

Address
DwCUnit::get_base_pc() {
   DIE* die = get_cu_die();

   AttributeValue av;
   av = die->getAttribute(DW_AT_low_pc);

   return av.integer; 
}

int DwCUnit::getAddrSize() {
    return addr_size ;
}

Abbreviation * DwCUnit::getAbbreviation(int num) {
       return abbrev->getAbbreviation (num) ;
}

DIE * DwCUnit::get_die(Attribute * attr, Offset off) {
    OffsetMap::iterator die = dies.find (off) ;
    if (die == dies.end()) {
       addFixup (attr, off) ;
       return NULL ;
    } else {
       return die->second ;
    }
}

void DwCUnit::addFixup(Attribute * attr, Offset offset) {
       FixupMap::iterator entry = fixups.find(offset) ;
       if (entry == fixups.end()) {
           AttrVec v ;
           v.push_back (attr) ;
           fixups[offset] = v ;
       } else {
           entry->second.push_back (attr) ;
       }
}

void DwCUnit::dofixup(Offset offset, DIE * die) {
    FixupMap::iterator entry = fixups.find (offset) ;
    if (entry != fixups.end()) {
       if (entry->second.size() == 0) {
           throw Exception ("empty fixup list") ;
       }
       for (uint i = 0 ; i < entry->second.size(); i++) {
           Attribute *fixup = entry->second[i];
           fixup->fixup (die) ;
       }
       fixups.erase(entry) ; 
    }
}

Address DwCUnit::get_base() {
    return dwarf->get_base() ;
}

// prints the following as an example
/*
Current source file is t.c
Compilation directory is /home/dallison/bk/buffy/debugger
Located in /home/dallison/bk/buffy/debugger/t.c
Contains 104 lines.
Source language is c.
Compiled with DWARF 2 debugging format.
Does not include preprocessor macro info.
*/
void DwCUnit::info (PStream &os) {
    std::string name = cu_die->getAttribute (DW_AT_name) ;
    os.print ("Current source file is %s\n", name.c_str()) ;
    if (comp_dir != "") {
        os.print ("Compilation directory is %s\n", comp_dir.c_str()) ;
    }
    File *file = NULL ;
    for (uint i = 1 ; i < file_table.size() ; i++) {
        if (file_table[i]->basename == name) {
            file = file_table[i] ;
            break ;
        }
    }
    if (file != NULL) {
        os.print ("Located in %s\n", file->pathname.c_str()) ;
        os.print ("Contains %d lines.\n", file->nlines) ;
    } else {
        os.print ("File not found.\n") ;
    }
    os.print ("Source language is ") ;
    switch (get_language() & 0xff) {
    case DW_LANG_C:
    case DW_LANG_C89:
        os.print ("c") ;
        break ;
    case DW_LANG_C_plus_plus:
        os.print ("c++") ;
        break ;
    case DW_LANG_Fortran77:
    case DW_LANG_Fortran90:
    case DW_LANG_Fortran95:
        os.print ("fortran") ;
        break ;
    case DW_LANG_Ada83:
        os.print ("ada") ;
        break ;
    case DW_LANG_Cobol74:
    case DW_LANG_Cobol85:
        os.print ("cobol") ;
        break ;
    case DW_LANG_Pascal83:
        os.print ("pascal") ;
        break ;
    case DW_LANG_Modula2:
        os.print ("modula2") ;
        break ;
    default:
        os.print ("unknown") ;
    }
    os.print (".\n") ;
    os.print ("Compiled with DWARF 2 debugging format.\n") ;            // XXX DWARF 3?
    os.print ("Does not include preprocessor macro info.\n") ;          // XXX: someday maybe
}

// look for a symbol in all the complation units
DIE *DwCUnit::find_symbol (std::string name) {
    return dwarf->find_symbol (name, false) ;
}


// find a global structure tag in all compilation units
DIE *DwCUnit::find_struct (std::string name) {
    return dwarf->find_struct (name) ;
}


void DwCUnit::read(BStream &stream) {
       start_offset = stream.offset() ;
       mainstream = stream ;
       debug_info_length = stream.read4u() ;
       if (debug_info_length == 0) {
           return ;
       }
       dwarf_version = stream.read2u() ;
       abbrev_offset = stream.read4u() ;
       addr_size = stream.read1u() ;

       abbrev = dwarf->find_cu_abbreviation (abbrev_offset) ;
       if (abbrev == NULL) {
           throw Exception ("Can't find compilation unit abbreviation at this offset ") ;
       }
       int num = stream.read_uleb() ;                      // abbreviation number
       (void) num;
       Offset offset = stream.offset() ;
       cu_die = make_die (this, NULL, abbrev) ;
       add_die (offset - start_offset, cu_die) ;
       cu_die->read (this, stream) ;
       AttributeValue cdir = cu_die->getAttribute (DW_AT_comp_dir) ;
       if (cdir.type != AV_NONE) {
           comp_dir = cdir.str ;
           std::string::size_type colon = comp_dir.find (":") ;
           if (colon != std::string::npos) {
               comp_dir = comp_dir.substr (colon+1) ;
           }
       }
       stream.seek (start_offset + debug_info_length + 4) ;             // seek to the correct address
       //cu_die->dump(0) ;
}

void DwCUnit::dump() {
    for (OffsetMap::iterator i = dies.begin() ; i != dies.end() ; i++) {
        DIE *die = i->second ;
        if (die->get_parent() == NULL) {
            die->dump (0) ;
        }
    }

}

int DwCUnit::get_language() {
     AttributeValue &lang = cu_die->getAttribute (DW_AT_language) ;
     if (lang.type == AV_NONE) {
         throw Exception ("No language specifed in the compilation unit") ;
     }
     return (int)lang ;
}

void DwCUnit::show_symbols() {
       int lang = cu_die->getAttribute (DW_AT_language) ;
       (void) lang;
       for (DIEMap::iterator i = symbols.begin() ; i != symbols.end() ; i++) {
           // DIE *symbol = i->second ;
           std::cout << "symbol: "  <<  i->first << '\n' ;
       }
}

static std::string readstring (BStream &stream) {
  std::string s = "" ;
  while (!stream.eof()) {
      char ch = stream.read1u() ;
      if (ch == 0) {
          break ;
      }
      s +=  ch ;
  }
  return s ;
}

void DwCUnit::build_line_matrix(BVector data) {
      if (cu_die == NULL) {
          return ;
      }
      AttributeValue av = cu_die->getAttribute (DW_AT_stmt_list) ;
      if (av.type == AV_NONE) {
          return ;
      }

      bool debug = false;                      // make true to print debug info

      Offset offset = av.integer ;
      BStream stream (data, do_swap());
      stream.seek (offset) ;

      // prolog variables
      int total_length = stream.read4u() ;
      int version = stream.read2u() ;
      (void) version;
      int prolog_length = stream.read4u() ;
      int end_prolog = stream.offset() ;// offset at end of prolog
      int min_instruction_length = stream.read1u() ;
      int default_is_stmt = stream.read1u() ;
      int line_base = stream.read1s() ;
      int line_range = stream.read1u() ;
      int opcode_base = stream.read1u() ;

      //std::cout << "line_base = " << line_base << "\n" ;
      // read opcode lengths
      for (int i = 1 ; i < opcode_base ; i++) { ;
          opcode_lengths.push_back (stream.read1u()) ;
      }

      // read directory table
      directory_table.push_back ("") ;           // first one is empty

      while (!stream.eof()) {
          std::string dirname = readstring(stream) ;
          if (dirname == "") {
             break ;
          }
          directory_table.push_back (dirname) ;
      }

      // read the file name table
      file_table.push_back (NULL) ;                // first one is empty
      while (!stream.eof()) {
          std::string filename = readstring(stream) ;
          if (filename == "") {
             break ;
          }
          int dir = stream.read_uleb() ;
          int mtime = stream.read_uleb() ;
          int size = stream.read_uleb() ;
          file_table.push_back (new File (directory_table, comp_dir, filename, dir, mtime, size)) ;
      }

      // state variables
      Address address = get_base() ;
      int file = 1 ;
      int line = 1 ;
      int column = 0 ;
      bool is_stmt = false ;
      bool basic_block = false ;

#define resetvars() { \
          address = get_base() ; \
          file = 1 ; \
          line = 1 ; \
          column = 0 ; \
          is_stmt = default_is_stmt ; \
          basic_block = false ; \
      }

      // we may need to skip to the start of the program
      int program_start = end_prolog + prolog_length ;
      stream.seek (program_start) ;

      Offset endoffset = offset + total_length + 4 ;// according to spec, the total length doesn't include the field

      int specop255 = (255 - opcode_base) / line_range ;

      resetvars() ;
      while (stream.offset() < endoffset) {
          int byte = stream.read1u() ;
          if (byte == 0) {          // extended opcode?
              //System.print ("extended opcode: " )
              int len = stream.read1u() ;// the spec is really bad here!  What is the length?
	      (void) len;
              DwSLineId opcode = (DwSLineId)stream.read1u() ;

              if (debug) {
                 std::cout << globl_dwf_names.get(opcode) << ":\n";
              } 

              switch (opcode) {
              case DW_LNE_end_sequence:
                  line_matrix.push_back (new LineInfo (this, address, file, line,
                      column, is_stmt, basic_block))  ;
                  resetvars() ;
                  break ;
              case DW_LNE_set_address: {
                  Address addr = read_address (stream) ;
                  //System.println (format ("0x%x", addr))
                  address = addr + get_base() ;
                  break ;
                  }
              case DW_LNE_define_file: {
                  std::string filename = readstring(stream) ;
                  int dir = stream.read_uleb() ;
                  int mtime = stream.read_uleb() ;
                  int size = stream.read_uleb() ;
                  file_table.push_back (new File (directory_table, comp_dir,
                     filename, dir, mtime, size)) ;
                  if (debug) {
                     std::cout <<  "name: " << filename << " dir: ";
                     std::cout << dir << "mtime: " << mtime + " size: " << size << "\n" ;
                  }
                  break ;
                  }
              default: /* XXX: probably should actually support the rest */;
              }

          } else if  (byte < opcode_base) {     // standard opcode
              //std::cout << "standard opcode " << "\n" ;
              DwSLineId opcode = (DwSLineId) byte ;

              if (debug) {
                 std::cout << globl_dwf_names.get(opcode);
              }

              switch (opcode) {
              case DW_LNS_copy:
                  line_matrix.push_back (new LineInfo (this, address, file,
                      line, column, is_stmt, basic_block))  ;
                  basic_block = false ;
                  break ;
              case DW_LNS_advance_pc: {
                  int operand = stream.read_uleb() ;
                  address += (operand * min_instruction_length) ;
                  if (debug) {
                     std::cout << ": " << operand << " to 0x";
                     std::cout << std::hex<< address << std::dec << "\n" ;
                  }
                  break ;
                  }
              case DW_LNS_advance_line: {
                  int operand = stream.read_sleb() ;
                  line += operand ;
                  if (debug) {
                      std::cout << ": " << operand << " to " <<  line << "\n" ;
                  }
                  break ;
                  }
              case DW_LNS_set_file: {
                  int operand = stream.read_uleb() ;
                  file = operand ;
                  if (debug) {
                     std::cout <<  ": " << operand;
                     std::cout << " (" << file_table[operand]->name << ")" << "\n" ;
                  }
                  break ;
                  }
              case DW_LNS_set_column: {
                  int operand = stream.read_uleb() ;
                  column = operand ;
                  if (debug) std::cout <<  ": " <<  operand << "\n" ;
                  break ;
                  }
              case DW_LNS_negate_stmt:
                  is_stmt = !is_stmt ;
                  if (debug) std::cout <<  ": " << is_stmt << "\n" ;
                  break ;
              case DW_LNS_set_basic_block:
                  basic_block = true ;
                  if (debug) std::cout <<  "\n" ;
                  break ;
              case DW_LNS_const_add_pc:
                  address += specop255 ;
                  if (debug) {
                      std::cout <<  ": " <<std::hex<<address << std::dec << "\n" ;
                  }
                  break ;
              case DW_LNS_fixed_advance_pc: {
                  int operand = stream.read2u() ;
                  address += operand ;
                  if (debug) {
                      std::cout <<  ": 0x" << std::hex << address << std::dec << "\n";
                  }
                  break ;
                  }
              default: /* XXX: probably should actually support the rest */;
              }
          } else {                          // special opcode
             int adjusted_opcode = byte - opcode_base ;
             int addrinc = (adjusted_opcode / line_range) * min_instruction_length ;
             int lineinc = line_base + (adjusted_opcode % line_range) ;
             address += addrinc ;
             line += lineinc ;
             if (debug) {
                std::cout <<  "special opcode " << adjusted_opcode;
                std::cout << ": increment address by " << addrinc;
                std::cout << " to 0x" << std::hex <<address;
                std::cout << std::dec << " and increment line by " << lineinc;
                std::cout << " to " << line << "\n" ;
             }
             line_matrix.push_back (new LineInfo (this, address, file,
                line, column, is_stmt, basic_block))  ;
             basic_block = false ;
          }
      }
}

void DwCUnit::show_line_matrix() {
      for (uint line = 0 ; line < line_matrix.size(); line++) {
          line_matrix[line]->print () ;
      }
}

bool DwCUnit::is_little_endian() {
    return dwarf->is_little_endian() ;
}

DwSTab *DwCUnit::get_string_table() {
    return dwarf->get_string_table() ;
}

DIE *DwCUnit::make_die (DwCUnit *cu, DIE *parent, Abbreviation *_abbrev) {
    return dwarf->make_die (cu, parent, _abbrev) ;
}

Address DwCUnit::read_address (BStream &bs) {
    switch (addr_size) {
    case 4: return bs.read4u(); 
    case 8: return bs.read8u();
    }

    throw Exception("Invalid address size %s", addr_size);
}


DwLocExpr DwCUnit::evaluate_location (DwCUnit* cu,
 Address frame_base, AttributeValue& attr, Process* process, Address top_addr) {
    DwLocEval eval(cu,process,frame_base,attr);
    eval.push(top_addr);
    return eval.execute();
}

DwLocExpr DwCUnit::evaluate_location (DwCUnit* cu,
 Address frame_base, AttributeValue& attr, Process* process) {
    DwLocEval eval(cu,process,frame_base,attr);
    return eval.execute();
}

void DwCUnit::disassemble_location(DwCUnit *cu, BVector location) {
    dwarf->disassemble_location(cu, location) ;
}

DIEMap &DwCUnit::get_symbols() {
       return symbols ;
}

DIEMap & DwCUnit::get_subprograms() {
       return subprograms ;
}

LineMatrix & DwCUnit::get_line_matrix() {
       return line_matrix ;
}

DirectoryTable & DwCUnit::get_directory_table() {
       return directory_table ;
}

FileTable &DwCUnit::get_file_table() {
       return file_table ;
}

DIE * DwCUnit::new_scalar_type(std::string name, int encoding, int size) {
        DIE * die = new Base_type (this, NULL, DW_TAG_base_type) ;
        die->addAttribute (DW_AT_name, name) ;
        die->addAttribute (DW_AT_encoding, encoding) ;
        if (size == 0) {
            switch (encoding) {
            case DW_ATE_address:
               size = getAddrSize() ;
               break ;
            case DW_ATE_boolean:
               size = 1 ;
               break ;
            case DW_ATE_complex_float:
               std::cout << "complex float not implemented yet" ;
               break ;
            case DW_ATE_float:
               size = 8 ;
               break ;
            case DW_ATE_signed:                     // regular signed integer
               size = 4 ;
               break ;
            case DW_ATE_signed_char:
               size = 1 ;
               break ;
            case DW_ATE_unsigned:                   // unsigned integer
               size = 4 ;
               break ;
            case DW_ATE_unsigned_char:
               size = 1 ;
               break ;
            }
        }
        die->addAttribute (DW_AT_byte_size, size) ;
        return add_temp_die (die) ;
}

DIE * DwCUnit::new_pointer_type(DIE * to) {
        DIE * die = new TypePointer (this, NULL, DW_TAG_pointer_type) ;
        if (to != NULL) {
            die->addAttribute (DW_AT_type, to) ;
        }
        die->addAttribute (DW_AT_byte_size, getAddrSize()) ;
        return add_temp_die (die) ;
}

DIE * DwCUnit::new_const_type(DIE * to) {
        DIE * die = new TypeConst(this, NULL, DW_TAG_const_type) ;
        die->addAttribute (DW_AT_type, to) ;
        return add_temp_die (die) ;
}

DIE * DwCUnit::new_volatile_type(DIE * to) {
        DIE * die = new TypeVolatile(this, NULL, DW_TAG_volatile_type) ;
        die->addAttribute (DW_AT_type, to) ;
        return add_temp_die (die) ;
}

DIE * DwCUnit::new_array_type(DIE * to) {
        DIE * die = new TypeArray (this, NULL, DW_TAG_array_type) ;
        if (to != NULL) {
            die->addAttribute (DW_AT_type, to) ;
        }
        return add_temp_die (die) ;
}

DIE * DwCUnit::new_subrange_type(int lb, int ub) {
        DIE *subrange = new Subrange_type (this, NULL, DW_TAG_subrange_type) ;
        subrange->addAttribute (DW_AT_lower_bound, lb) ;
        subrange->addAttribute (DW_AT_upper_bound, ub) ;
        return add_temp_die(subrange) ;
}

DIE * DwCUnit::new_subrange_type() {
        DIE *subrange = new Subrange_type (this, NULL, DW_TAG_subrange_type) ;
        return add_temp_die (subrange) ;
}

DIE * DwCUnit::new_string_type(int len) {
        DIE *s = new String_type (this, NULL, DW_TAG_string_type) ;
        s->addAttribute (DW_AT_byte_size, len) ;
        return add_temp_die(s );
}

DIE * DwCUnit::new_subroutine_type() {
        DIE *s = new Subroutine_type (this, NULL, DW_TAG_subroutine_type) ;
        return add_temp_die(s) ;
}

DIE * DwCUnit::new_formal_parameter(DIE *type) {
        DIE *s = new Formal_parameter (this, NULL, DW_TAG_formal_parameter) ;
        s->addAttribute (DW_AT_type, type) ;
        return add_temp_die(s) ;
}


DIE *DwCUnit::add_temp_die (DIE *die) {
    temp_dies.push_back (die) ;
    return die ;
}

void DwCUnit::delete_temp_dies() {
    for (std::list<DIE*>::iterator i = temp_dies.begin() ; i != temp_dies.end() ; i++) {
        delete *i ;
    }
    temp_dies.clear() ;
}

void DwCUnit::keep_temp_dies(std::vector<DIE*> &buffer) {
    for (std::list<DIE*>::iterator i = temp_dies.begin() ; i != temp_dies.end() ; i++) {
        buffer.push_back (*i) ;
    }
    temp_dies.clear() ;
}

// keep a temp die if it is indeed temp
void DwCUnit::keep_temp_die(DIE *die) {
    for (std::list<DIE*>::iterator i = temp_dies.begin() ; i != temp_dies.end() ; i++) {
        if (*i == die) {
            temp_dies.erase (i) ;
            return ;
        }
    }
}

// insert at start of list so that newer ones (nested) are found first
void DwCUnit::add_lazy_die (DIE *die, Offset start, Offset end) {
    lazy_dies.push_front (new LazyDIE (die, start, end)) ;
}

// XXX: make this non-linear
DIE *DwCUnit::find_lazy_die (Offset offset) {
    for (std::list<LazyDIE*>::iterator i = lazy_dies.begin() ; i != lazy_dies.end() ; i++) {
        LazyDIE *ld = *i ;
        if (offset >= ld->start && offset < ld->end) {
            return ld->die ;
        }
    }
    return NULL ;
}


