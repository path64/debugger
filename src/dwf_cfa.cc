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

file: dwf_cfa.cc
created on: Fri Aug 13 11:07:34 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#include "dbg_dwarf.h"
#include "dwf_info.h"
#include "err_nice.h"
#include "dwf_cfa.h"

CFAEntry::CFAEntry(DwInfo * dwarf)
 : instructions(), dwarf(dwarf)
{
}



CFAEntry::~CFAEntry()
{
}

void
CFAEntry::read_instructions(BStream& stream, int len)
{
   instructions = BVector(stream.address(), len);
   stream.seek(len, BSTREAM_CUR);
}

BVector
CFAEntry::get_instructions()
{
   return instructions;
}

CIE::CIE(DwInfo * dwarf, Offset cieoffset)
 :  CFAEntry(dwarf), cieoffset(cieoffset),
    code_align(0), data_align(0), retaddr(0),
    has_z_aug(false)
{
}

CIE::~CIE()
{
}

void
CIE::read(Section* section, BStream & stream,
	  int id, int length, bool is_eh)
{
   Offset aug_end = 0;
   unsigned ap = 0;
   Offset offset;

   offset = stream.offset();
   version = stream.read1u();

   /* read augmentation string */
   while (!stream.eof()) {
      char ch = stream.read1u();
      if (ch == '\0') break;

      if (ap < MAX_AUGSTR) {
         augstr[ap++] = ch;   
      } else {
         throw Exception("Invalid CIE augmentation string");
      }
   }

   /* cap off augmentation */
   augstr[ap] = '\0'; ap = 0;

   /* catch GCC 2.x "eh" augmentation   */
   /* Note: data BEFORE standard values */
   if (augstr[ap] == 'e' && augstr[ap+1] == 'h') {
      eh_ptr = dwarf->read_address(stream);
      ap += 2;
   }
      
   /* get alignment factors */
   code_align = stream.read_uleb();
   data_align = stream.read_sleb();

   /* depends on DWARF version */
   if (dwarf->get_ver() == 2) {
      retaddr = stream.read1u();
   } else {
      retaddr = stream.read_uleb();
   }

   /* calculate the encoding scheme */
   if (dwarf->is_elf64()) {
      fde_enc = DW_EH_PE_udata8;
   } else {
      fde_enc = DW_EH_PE_udata4;
   }

   /* catch GCC 2.x 'z' augmentation */
   /* Note: data AFTER standard values */
   if (augstr[ap] == 'z') {
      int asize = stream.read_uleb();
      aug_end = stream.offset() + asize;
      has_z_aug = true;
      ++ap;
   } 

   /* catch miscellaneous augmentation */
   /* Note: data AFTER standard values */
   while (augstr[ap] != '\0') {
      switch (augstr[ap]) {
      case 'R': { /* FDE address encoding */
         fde_enc = stream.read1u();
         break;
      } 
      case 'L': { /* not handled, drop value */
         stream.read1u();
         break;
      }
      case 'P': { /* not handled, drop values */
         int val = stream.read1u() & 0x07;
         switch (val) {
         case DW_EH_PE_absptr:
	    dwarf->read_address(stream);
	    break;
	 case DW_EH_PE_udata2:
	    stream.read2u();
	    break;
	 case DW_EH_PE_udata4:
	    stream.read4u();
	    break;
	 case DW_EH_PE_udata8:
	    stream.read8u();
	    break;
	 }
         break;
      }
      default:
	 throw Exception("Unrecognized CIE augmentation, %c", (char)augstr[ap]);
      }

      /* next */
      ap++;
   }

   /* use z_aug to find instructions */
   if (has_z_aug && aug_end != 0) {
      stream.seek(aug_end);
   }

   /* done with augmentation, read instrs */
   Offset codesize = length - stream.offset() + offset;
   if (codesize < 0) {
     throw Exception("Invalid CIE instruction length");
   }
   read_instructions(stream, codesize);
}

Offset
CIE::get_offset()
{
   return cieoffset;
}

int
CIE::get_code_align()
{
   return code_align;
}

int
CIE::get_data_align()
{
   return data_align;
}

int
CIE::get_ra()
{
   return retaddr;
}

void
CIE::print()
{
   std::cout << "\tCIE at offset " << cieoffset << '\n';
   std::cout << "\t\taugmentation: " << augstr << '\n';
   std::cout << "\t\tcaf: " << code_align << '\n';
   std::cout << "\t\tdaf: " << data_align << '\n';
   std::cout << "\t\treturn addr reg: " << retaddr << '\n';
}

// Frame Description Entry
FDE::FDE(DwInfo * dwarf)
:  CFAEntry(dwarf), cie(NULL), start_address(0), end_address(0)
{
}

FDE::~FDE()
{
}

Address
FDE::read_value(Section * section, CIE * cie,
		BStream & stream, int mask)
{
   int encoding = cie->get_fde_encoding() & mask;
   Address baseaddr = 0;
   switch (encoding & 0x70) {
   case DW_EH_PE_absptr:
      break;
   case DW_EH_PE_pcrel:
      // printf ("pcrel: section: 0x%llx, base: 0x%llx\n",
      // section->get_addr(), dwarf->get_base()) ;
      baseaddr = section->get_addr() + dwarf->get_base();
      baseaddr += stream.offset();
      break;
   case DW_EH_PE_datarel:
      baseaddr = dwarf->GOT_address();
      break;
   }

   switch (encoding & 0x0f) {
   case DW_EH_PE_udata2:
      return baseaddr + stream.read2u();
   case DW_EH_PE_udata4:
      return baseaddr + stream.read4u();
   case DW_EH_PE_udata8:
      return baseaddr + stream.read8u();
   case DW_EH_PE_sdata2:
      return baseaddr + stream.read2s();
   case DW_EH_PE_sdata4:
      return baseaddr + stream.read4s();
   case DW_EH_PE_sdata8:
      return baseaddr + stream.read8s();
   }
   printf("Unable to determine encoding for FDE\n");
   throw Exception("Unable to determine encoding for FDE");
}

void
FDE::read(Section * section, BStream& stream,
	  int id, Offset offset, int length, bool is_eh)
{
   if (is_eh) {
      /* for an eh_frame, the CIE address is the difference
       * between the address of the id and the FDE address */
      id = offset - id;
   }

  
   CIEMap::iterator x = dwarf->cies.find(id);
   if (x == dwarf->cies.end()) {
      throw Exception("Unable to find CIE %d", id);
   }
   cie = x->second;

   start_address = read_value(section, cie, stream, 0xff);
   Address len = read_value(section, cie, stream, 0x0f);
   end_address = start_address + len;

   if (stream.offset() > offset + length) {
      throw Exception("Bad FDE entry, too long");
   }

   /* use z_aug to find instructions */
   if (cie->is_z_aug()) {
      Address len = stream.read_uleb();
      stream.seek(len, BSTREAM_CUR);
   }

   Offset codesize = length - stream.offset() + offset;
   if (codesize < 0) {
      throw Exception("Invalid FDE instruction length");
   }
   read_instructions(stream, codesize);
}

Address
FDE::get_start_address()
{
   return start_address;
}

void
FDE::print()
{
   std::cout << "FDE at address 0x" << std::
       hex << start_address << "...0x" << end_address << std::dec << '\n';
   cie->print();
}

CIE *
FDE::get_cie()
{
   return cie;
}

