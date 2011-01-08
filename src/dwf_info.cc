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

file: dwf_info.cc
created on: Fri Aug 13 11:07:34 PDT 2004
author: David Allison <dallison@pathscale.com>

*/
#include <algorithm>

#include "process.h"
#include "dwf_info.h"
#include "dwf_cfa.h"
#include "dwf_cunit.h"
#include "dwf_names.h"

#include "type_array.h"
#include "type_class.h"
#include "type_enum.h"
#include "type_union.h"
#include "type_nspace.h"
#include "type_pointer.h"
#include "type_qual.h"

#include <algorithm>

DwInfo::DwInfo(ELF * elffile, std::istream& _stream)
:  elffile(elffile), stream(_stream), string_table(NULL), location_table(NULL)
{

   // populate the opcode operand counts
   opcode_opcounts[DW_OP_addr] = 1;
   for (int i = DW_OP_const1u; i != DW_OP_dup; i++) {
      opcode_opcounts[i] = 1;
   }
   opcode_opcounts[DW_OP_pick] = 1;
   opcode_opcounts[DW_OP_plus_uconst] = 1;
   opcode_opcounts[DW_OP_skip] = 1;
   opcode_opcounts[DW_OP_bra] = 1;
   for (int i = DW_OP_breg0; i != DW_OP_bregx; i++) {
      opcode_opcounts[i] = 1;
   }
   opcode_opcounts[DW_OP_bregx] = 2;
   for (int i = DW_OP_piece; i != DW_OP_nop; i++) {
      opcode_opcounts[i] = 1;
   }

   Section *got_section = elffile->find_section(".got");
   if (got_section != NULL) {
      got = got_section->get_addr() + elffile->get_base();
   } else {
      got = 0;
   }
}

DwInfo::~DwInfo()
{
   // delete the compilation units
   for (uint i = 0; i < compilation_units.size(); i++) {
      delete compilation_units[i];
   }
   // don't delete the ELF file as we don't own it

   // delete the FDEs and CIEs
   for (FDEMap::iterator i = fdes.begin(); i != fdes.end(); i++) {
      delete i->second;
   }
   for (CIEMap::iterator i = cies.begin(); i != cies.end(); i++) {
      delete i->second;
   }
   delete string_table;
}

Address
DwInfo::read_address(BStream & stream)
{
   if (elffile->is_elf64()) {
      return stream.read8u();
   } else {
      return stream.read4u();
   }
}

void
DwInfo::disassemble_location(DwCUnit * cu, BVector location)
{
   BStream lstream(location, cu->do_swap());
   while (!lstream.eof()) {
      int op = lstream.read1u();
      int nargs = opcode_opcounts[op];

      DwOpcodeId op_id = (DwOpcodeId) op;
      std::cout << " " << globl_dwf_names.get(op_id);

      if (nargs == 0) {
	 std::cout << ";";
	 continue;
      }

      switch (op) {
      case DW_OP_addr:{
	    Address v = cu->read_address(lstream);
	    std::cout << std::hex << " 0x" << v << std::dec;
	    break;
	 }
      case DW_OP_const1u:
      case DW_OP_pick:
      case DW_OP_deref_size:
      case DW_OP_xderef_size:
	 std::cout << " " << lstream.read1u();
	 break;
      case DW_OP_const2u:
	 std::cout << " " << lstream.read2u();
	 break;
      case DW_OP_const4u:
	 std::cout << " " << lstream.read4u();
	 break;
      case DW_OP_const8u:
	 std::cout << " " << lstream.read8u();
	 break;
      case DW_OP_const8s:
	 std::cout << " " << lstream.read8s();
	 break;
      case DW_OP_const1s:
	 std::cout << " " << lstream.read1s();
	 break;
      case DW_OP_const2s:
      case DW_OP_skip:
      case DW_OP_bra:
	 std::cout << " " << lstream.read2s();
	 break;
      case DW_OP_const4s:
	 std::cout << " " << lstream.read4s();
	 break;
      case DW_OP_plus_uconst:
      case DW_OP_regx:
      case DW_OP_piece:
      case DW_OP_constu:
	 std::cout << " " << lstream.read_uleb();
	 break;
      case DW_OP_breg0:
      case DW_OP_breg1:
      case DW_OP_breg2:
      case DW_OP_breg3:
      case DW_OP_breg4:
      case DW_OP_breg5:
      case DW_OP_breg6:
      case DW_OP_breg7:
      case DW_OP_breg8:
      case DW_OP_breg9:
      case DW_OP_breg10:
      case DW_OP_breg11:
      case DW_OP_breg12:
      case DW_OP_breg13:
      case DW_OP_breg14:
      case DW_OP_breg15:
      case DW_OP_breg16:
      case DW_OP_breg17:
      case DW_OP_breg18:
      case DW_OP_breg19:
      case DW_OP_breg20:
      case DW_OP_breg21:
      case DW_OP_breg22:
      case DW_OP_breg23:
      case DW_OP_breg24:
      case DW_OP_breg25:
      case DW_OP_breg26:
      case DW_OP_breg27:
      case DW_OP_breg28:
      case DW_OP_breg29:
      case DW_OP_breg30:
      case DW_OP_breg31:
      case DW_OP_fbreg:
      case DW_OP_consts:
	 std::cout << " " << lstream.read_sleb();
	 break;
      case DW_OP_bregx:
	 std::cout << " " << lstream.read_uleb();
	 std::cout << " " << lstream.read_sleb();
	 break;
      }
      std::cout << ";";

   }
}

void
DwInfo::read_frame_entry(Section * section, BStream & stream,
			    bool is_eh)
{
   Offset offset = stream.offset();

   /* get length of the whole CFA entry */
   int length = stream.read4u();

   /* length of remaining after id */
   int rlen;			/* XXX: rearrange FDE and just use length */

   /*
    * zero length is null terminator 
    */
   if (length == 0) {
      stream.seek(0, BSTREAM_END);
      return;
   }

   /*
    * Please note that 64-bit DWARF *is not* 64-bit ELF.  The spec states
    * that the following is 32-bit in 32-bit DWARF, and 64-bit in 64-bit
    * DWARF.  This classfication is independent of whether the ELF file is 
    * 64-bit or 32-bit. 
    */
   Address id;
   if (is_dwf64()) {
      id = stream.read8u();
      rlen = length - 8;
   } else {
      id = stream.read4u();
      rlen = length - 4;
   }

   /*
    * CIE id is -1 anywhere or 0 in .eh_frame 
    */
   if (id == -1 || (is_eh && id == 0)) {
      CIE *entry = new CIE(this, offset);
      entry->read(section, stream, id, rlen, is_eh);
      cies[offset] = entry;
   } else {
      FDE *entry = new FDE(this);
      entry->read(section, stream, id, offset + 4, length, is_eh);
      fdes[entry->get_start_address()] = entry;
      fdevec.push_back(entry);
   }

}


void
DwInfo::read_frames(std::string section_name)
{
   BVector debug_frame;
   Section *section = NULL;

   section = elffile->find_section(section_name);
   if (section != NULL) {
      debug_frame = section->get_contents(stream);
   } else {
      // std::cout << "No " << section_name << " frames present" << '\n' ;
      return;
   }
   BStream stream(debug_frame, do_swap());
   bool is_eh = section_name == ".eh_frame";
   while (!stream.eof()) {
      try {
	 read_frame_entry(section, stream, is_eh);
      }
      catch(Exception e) {
	 e.report(std::cout);
      }
      catch(const char *s) {
	 std::cerr << s << "\n";
      } catch(...) {
	 std::cerr << "unknown exception in read_frame_entry: \n";
      }
   }
}

class Compare_fde {
 public:
   bool operator() (FDE * f1, FDE * f2) {
      return f1->get_start_address() < f2->get_start_address();
}};

void
DwInfo::read_frames()
{
   try {
      read_frames(".eh_frame");
      read_frames(".debug_frame");
   } catch(Exception e) {
      std::cout << "exception in read_frames: ";
      e.report(std::cout);
   } catch(const char *s) {
      std::cerr << "exception in read_frames: " << s << "\n";
   } catch(std::string s) {
      std::cerr << "exception in read_frames: " << s << "\n";
   } catch(...) {
      std::cerr << "unknown exception in read_frames: \n";
   }
   std::stable_sort(fdevec.begin(), fdevec.end(), Compare_fde());
}


// find an FDE containing the given address using a binary search

FDE *
DwInfo::find_fde(Address addr)
{
   int start = 0;
   int end = fdevec.size() - 1;
   if (end < 0) {
      return NULL;
   }
   FDE *fde = NULL;
   int mid = 0;
   while (start <= end) {
      mid = (end + start) / 2;
      fde = fdevec[mid];
      if (addr >= fde->get_start_address()
	  && addr < fde->get_end_address()) {
	 return fde;
      }
      if (addr < fde->get_start_address()) {
	 end = mid - 1;
      } else {
	 start = mid + 1;
      }
   }
   return NULL;

#if 0
   // this looks wrong
   if (end < 0) {
      end = 0;
   }
   fde = fdevec[end];
   if (end < fdevec.size() - 1 && addr > fde->get_start_address()
       && addr <= fdevec[end + 1]->get_end_address()) {
      return fde;
   }
   return NULL;
#endif
}


Abbreviation *
DwInfo::find_cu_abbreviation(Offset offset)
{
   AbbreviationMap::iterator i =
       compilationunit_abbreviations.find(offset);
   if (i == compilationunit_abbreviations.end()) {
      return NULL;
   }
   return i->second;
}

void
DwInfo::read_abbreviations()
{
   BVector abbrev;

   try {
      abbrev = elffile->get_section(stream, ".debug_abbrev");
   } catch(...) {
      return;
   }

   BStream str(abbrev, do_swap());
   Abbreviation *cu = NULL;	// current compilation unit
   while (!str.eof()) {
      int offset = str.offset();
      int num = str.read_uleb();
      if (num != 0) {
	 int tag = str.read_uleb();
	 int children = str.read_uleb();
	 Abbreviation *abb = new Abbreviation(num, tag, children);
	 if (cu == NULL) {
	    cu = abb;
	    compilationunit_abbreviations[offset] = abb;
	 } else {
	    cu->addAbbreviation(num, abb);
	 }
	 abb->read(str);
	 // printf ("@0x%x\n", offset) ;
	 // abb->print () ;
      } else {
	 // printf ("@0x%x\n", offset) ;
	 // printf ("NULL\n") ;
	 Abbreviation *abb = new Abbreviation(0, 0, false);
	 if (cu == NULL) {
	    compilationunit_abbreviations[offset] = abb;
	 } else {
	    cu->addAbbreviation(0, abb);
	 }
	 cu = NULL;
      }
   }
}

DIE *
DwInfo::make_die(DwCUnit * cu, DIE * parent,
		    Abbreviation * abbrev)
{
   switch (abbrev->getTag()) {
   case 0:
      return NULL;
   case DW_TAG_array_type:
      return new TypeArray(cu, parent, abbrev);
   case DW_TAG_class_type:
      return new TypeClass(cu, parent, abbrev);
   case DW_TAG_entry_point:
      return new Entry_point(cu, parent, abbrev);
   case DW_TAG_enumeration_type:
      return new TypeEnum(cu, parent, abbrev);
   case DW_TAG_formal_parameter:
      return new Formal_parameter(cu, parent, abbrev);
   case DW_TAG_imported_declaration:
      return new Imported_declaration(cu, parent, abbrev);
   case DW_TAG_label:
      return new Label(cu, parent, abbrev);
   case DW_TAG_lexical_block:
      return new Lexical_block(cu, parent, abbrev);
   case DW_TAG_member:
      return new Member(cu, parent, abbrev);
   case DW_TAG_pointer_type:
      return new TypePointer(cu, parent, abbrev);
   case DW_TAG_reference_type:
      return new Reference_type(cu, parent, abbrev);
   case DW_TAG_compile_unit:
      return new Compile_unit(cu, parent, abbrev);
   case DW_TAG_string_type:
      return new String_type(cu, parent, abbrev);
   case DW_TAG_structure_type:
      return new TypeStruct(cu, parent, abbrev);
   case DW_TAG_subroutine_type:
      return new Subroutine_type(cu, parent, abbrev);
   case DW_TAG_typedef:
      return new TypeTypedef(cu, parent, abbrev);
   case DW_TAG_union_type:
      return new TypeUnion(cu, parent, abbrev);
   case DW_TAG_unspecified_parameters:
      return new Unspecified_parameters(cu, parent, abbrev);
   case DW_TAG_variant:
      return new Variant(cu, parent, abbrev);
   case DW_TAG_common_block:
      return new Common_block(cu, parent, abbrev);
   case DW_TAG_common_inclusion:
      return new Common_inclusion(cu, parent, abbrev);
   case DW_TAG_inheritance:
      return new Inheritance(cu, parent, abbrev);
   case DW_TAG_inlined_subroutine:
      return new Inlined_subroutine(cu, parent, abbrev);
   case DW_TAG_module:
      return new Module(cu, parent, abbrev);
   case DW_TAG_ptr_to_member_type:
      return new Ptr_to_member_type(cu, parent, abbrev);
   case DW_TAG_set_type:
      return new Set_type(cu, parent, abbrev);
   case DW_TAG_subrange_type:
      return new Subrange_type(cu, parent, abbrev);
   case DW_TAG_with_stmt:
      return new With_stmt(cu, parent, abbrev);
   case DW_TAG_access_declaration:
      return new Access_declaration(cu, parent, abbrev);
   case DW_TAG_base_type:
      return new Base_type(cu, parent, abbrev);
   case DW_TAG_catch_block:
      return new Catch_block(cu, parent, abbrev);
   case DW_TAG_const_type:
      return new TypeConst(cu, parent, abbrev);
   case DW_TAG_constant:
      return new Constant(cu, parent, abbrev);
   case DW_TAG_enumerator:
      return new Enumerator(cu, parent, abbrev);
   case DW_TAG_file_type:
      return new File_type(cu, parent, abbrev);
   case DW_TAG_friend:
      return new Friend(cu, parent, abbrev);
   case DW_TAG_namelist:
      return new Namelist(cu, parent, abbrev);
   case DW_TAG_namelist_item:
      return new Namelist_item(cu, parent, abbrev);
   case DW_TAG_packed_type:
      return new Packed_type(cu, parent, abbrev);
   case DW_TAG_subprogram:
      return new Subprogram(cu, parent, abbrev);
   case DW_TAG_template_type_param:
      return new Template_type_param(cu, parent, abbrev);
   case DW_TAG_template_value_param:
      return new Template_value_param(cu, parent, abbrev);
   case DW_TAG_thrown_type:
      return new Thrown_type(cu, parent, abbrev);
   case DW_TAG_try_block:
      return new Try_block(cu, parent, abbrev);
   case DW_TAG_variant_part:
      return new Variant_part(cu, parent, abbrev);
   case DW_TAG_variable:
      return new Variable(cu, parent, abbrev);
   case DW_TAG_volatile_type:
      return new TypeVolatile(cu, parent, abbrev);
   case DW_TAG_namespace:
      return new EntryNSpace(cu, parent, abbrev);
   case DW_TAG_lo_user:
      return new Lo_user(cu, parent, abbrev);
   case DW_TAG_hi_user:
      return new Hi_user(cu, parent, abbrev);
   case DW_TAG_imported_module:
      return new imported_module(cu, parent, abbrev);
   default:
      throw Exception("Unknown DIE type");
   }
}

void
DwInfo::dump()
{
   for (uint cu = 0; cu < compilation_units.size(); cu++) {
      compilation_units[cu]->dump();
   }

}

void
DwInfo::show_symbols()
{
   for (uint cu = 0; cu < compilation_units.size(); cu++) {
      compilation_units[cu]->show_symbols();
   }
}

void
DwInfo::read_debug_info(PStream * os, bool reporterror)
{
   BVector info;

   try {
      info = elffile->get_section(stream, ".debug_info");
   } catch(...) {
      if (reporterror) {
	 if (os != NULL) {
	    os->print("(no symbol information)...");
	 } else {
	    printf("(no symbol information)");
	 }
      }
      return;
   }
   BVector linedata = elffile->get_section(stream, ".debug_line");
   BStream str(info, do_swap());
   while (!str.eof()) {
      DwCUnit *cu = new DwCUnit(this);
      cu->read(str);
      compilation_units.push_back(cu);
      cu->build_line_matrix(linedata);
      // cu->show_line_matrix () ;
   }


}

void
DwInfo::read_string_table()
{
   BVector data;
   try {
      data = elffile->get_section(stream, ".debug_str");
   } catch(Exception e) {
      return;
   }
   catch(const char *s) {
      return;
   }
   catch(std::string s) {
      return;
   }

   string_table = new DwSTab(data);
}

void
DwInfo::read_location_table(DwCUnit * cu)
{
   BVector data;
   try {
      data = elffile->get_section(stream, ".debug_loc");
   } catch(Exception e) {
      return;
   }
   catch(const char *s) {
      return;
   }
   catch(std::string s) {
      return;
   }

   location_table = new LocationListTable(cu, data);
}


CUVec & DwInfo::get_compilation_units()
{
   return compilation_units;
}

void
DwInfo::register_symbol(std::string name, DIE * die)
{
}

void
DwInfo::register_struct(std::string name, DIE * die)
{
}

void
DwInfo::register_subprogram(std::string name, DIE * die)
{
}

BVector
DwInfo::get_loc_expr(DwCUnit* cu, Address offset, Address pc) {
   if (location_table == NULL) {
      read_location_table(cu);
   }

   return location_table->getexpr(cu, offset, pc);
}

