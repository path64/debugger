#include <map>

#include "bstream.h"
#include "dbg_elf.h"
#include "dwf_abbrv.h"
#include "dwf_cunit.h"
#include "dwf_info.h"

void read_cunits_sect(ELF* elf, std::istream* is, DwInfo* dwarf) {
   BVector info = elf->get_section(*is, ".debug_info");
   BStream bs(info, false);

   while (!bs.eof()) {
      bool is_dwf64 = false;
      Offset cu_offset;
      Offset unit_len;
      Offset end_spos;
      Offset abb_off;
      int dwf_ver;
      int ptr_size;
      DwCUnit* cunit;

      cu_offset = bs.offset();
      unit_len = bs.read4u();
      if (unit_len == 0) continue;

      if (unit_len == 0xffffffffL) {
         is_dwf64 = true;
         unit_len = bs.read8u();
      }

      end_spos = bs.offset() + unit_len;

      dwf_ver = bs.read2u();
      if (is_dwf64) {
         abb_off = bs.read8u();
      } else {
         abb_off = bs.read4u();   
      }

      ptr_size = bs.read1u();

      cunit = new DwCUnit(dwarf); 
      cunit->set_sec_offset(cu_offset);
      cunit->set_sec_length(unit_len);
      cunit->set_dwf_ver(dwf_ver);
      cunit->set_abb_offset(abb_off);
      cunit->set_ptr_size(ptr_size);

      DwAbbrvMap* cu_abb = dwarf->get_abb_map(abb_off);
      cunit->set_abb_map(cu_abb);
      if (cu_abb == NULL) {
         throw Exception("invalid DWARF");
      }

      DwEntry* dbg_info = DwEntry::read_tree(cunit, NULL, bs);
      cunit->set_dbg_info(dbg_info);
      dwarf->add_cunit(cunit);

      bs.seek(end_spos, BSTREAM_SET);
   }
}

void read_abbrv_sect(ELF* elf, std::istream* is, DwInfo* dwarf)
{
   BVector abbrv = elf->get_section(*is, ".debug_abbrev");

   DwAbbrvTab* tab = new DwAbbrvTab(dwarf);
   if (tab == NULL) {
      throw Exception("out of memory");
   }

   BStream bs(abbrv, false);
   tab->read(bs);
   dwarf->set_abb_tab(tab);
}

int main(int argc, char* argv[])
{
   ELF* elf = new ELF(argv[1], 0);
   std::istream* is = elf->open(0);

   DwInfo dwarf(elf, *is);
   dwarf.read_string_table();

   read_abbrv_sect(elf, is, &dwarf);
   //dwarf.dump_abb_tab();

   read_cunits_sect(elf, is, &dwarf);
   dwarf.dump_dbg_info();

   //sleep(100);
}

