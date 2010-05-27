
#include <stdlib.h>
#include <stdio.h>

#include "dwf_abbrv.h"
#include "dwf_names.h"
#include "dwf_info.h"
#include "dwf_cunit.h"
#include "dwf_entry.h"

/* Constructor needs to read in form and attempt
 * to calculate a constant entry size 
 */

DwAbbrv::DwAbbrv(DwInfo* info) {
   dwinfo = info;
   is_elf64 = info->is_elf64();
   is_dwf64 = info->is_dwf64();
}

/* Read in an abbreviation from .debug_abbrv 
 */
void
DwAbbrv::read(BStream& bs) {
   /* read the entry tag */
   tag = (DwTagId)bs.read_uleb();

   /* read sibling status */
   int poppy = bs.read1u();
   switch (poppy) {
   case DW_CHILDREN_yes: with_kids = true; break;
   case DW_CHILDREN_no:  with_kids = false; break;
   default: throw Exception("invalid DWARF abbreviation");
   }

   /* read attribute list */
   while (!bs.eof()) {
      DwAbbrvP pair;

      pair.attr = (DwAttrId)bs.read_uleb();
      pair.form = (DwFormId)bs.read_uleb();
      if (pair.attr == DW_AT_null &&
          pair.form == DW_FORM_null) {
         break;
      }

      data.push_back(pair); 
   }
 
   size = calc_size();

   /* sibling is optimized */
   with_sib = has(DW_AT_sibling);
}

/*  Do a readelf style dump of this attribute value
 */
void
DwAbbrv::dump_entry(BStream& bs, DwCUnit* cu) {
   Offset off = bs.offset();
   std::list<DwAbbrvP>::iterator i;

   for (i=data.begin(); i!=data.end(); i++) {
      DwFormId f_id = i->form;
      const char* at_name;

      if (f_id == DW_FORM_indirect) {
         f_id = (DwFormId)bs.read_uleb();
      }

      at_name = globl_dwf_names.get(i->attr);
      printf("     %-17s : ", at_name);

      switch (f_id) {
      /* handle the null forms */
      case DW_FORM_indirect:
      case DW_FORM_null:
      {
         printf("(NULL FORM)");
         break;
      }

      /* handle all address forms */
      case DW_FORM_addr:
      {
         Address val = scan_int(bs, f_id);
         /*
          * XXX - the casts are because SLES doesn't know these are
          * actually long longs already.  Stupid SLES.
          */
         printf("0x%llx %lld", (long long)val, (long long)val);
         break;
      }

      /* handle all integral forms */
      case DW_FORM_flag:
      case DW_FORM_data1:
      case DW_FORM_data2:
      case DW_FORM_data4:
      case DW_FORM_data8:
      case DW_FORM_sdata:
      case DW_FORM_udata:
      {
         Address val = scan_int(bs, f_id);
         /*
          * XXX - the cast is because SLES doesn't know this is
          * actually a long long already.  Stupid SLES.
          */
         printf("%lld", (long long)val);
         break;
      }

      /* handle all block forms */
      case DW_FORM_block1:
      case DW_FORM_block2:
      case DW_FORM_block4:
      case DW_FORM_blockv:
      {
         BVector val = scan_blk(bs, f_id);
         for (unsigned i=0; i<val.length(); i++) {
            byte a = val[i];
            printf("0x%02x ", (int)a);
         }
         break;
      }

      /* handle all string forms */
      case DW_FORM_string:
      case DW_FORM_strp:
      {
         const char* s = scan_str(bs, f_id);
         printf("%s", s);
         break;
      }

      /* handle all reference forms */
      case DW_FORM_ref_addr:
      case DW_FORM_ref_udata:
      case DW_FORM_ref1:
      case DW_FORM_ref2:
      case DW_FORM_ref4:
      case DW_FORM_ref8:
      {
         Offset val = scan_off(bs, f_id);
         if (f_id != DW_FORM_ref_addr) {
            val += cu->get_sec_offset();
         }
         /*
          * XXX - the cast is because SLES doesn't know this is
          * actually a long long already.  Stupid SLES.
          */
         printf("<%llx>", (long long)val);
         break;
      }

      /* no default to encourage warnings */
      }

      /* push to file */
      putchar('\n');
      fflush(stdout);
   }

   bs.seek(off);
}

/*  Do a readelf style dump of this attribute map
 */
void
DwAbbrv::dump_abbrv() {
   const char* tag_name;

   tag_name = globl_dwf_names.get(tag);
   printf("   %-7d %-22s ", (int)id_num, tag_name);

   if (with_kids) {
      printf("[has children]\n");
   } else {
      printf("[no children]\n");
   }

   std::list<DwAbbrvP>::iterator i;
   for (i=data.begin(); i!=data.end(); i++) {
      const char* at_name;
      const char* at_form;

      at_name = globl_dwf_names.get(i->attr);
      at_form = globl_dwf_names.get(i->form);
      printf("    %-20s %s\n", at_name, at_form);
   }
}


/* Only a couple forms require special functions.  Most
 * are simple enough that they can just be done using a
 * small entry in a switch statement. The following are
 * the few special cases.
 */


const char*
DwAbbrv::pull_string(BStream& bs) {
   const byte *m;
   char c;

   m = bs.address();
   do {
      c = bs.read1u();
   } while (c != '\0');

   return (char*)m;
}

const char*
DwAbbrv::pull_strp(BStream& bs) {
   Offset off;

   if (is_dwf64) {
      off = bs.read8u();
   } else {
      off = bs.read4u();
   }

   const char* s;
   s = dwinfo->get_string(off);
   if (s == NULL) {
      return NULL;
   }

   return s;
}

void
DwAbbrv::seek_string(BStream& bs) {
   char c;

   do {
      c = bs.read1u();
   } while (c != '\0');
}


/* Read in attribute values, after stream has been
 * placed at the proper offset position.
 */

int64_t
DwAbbrv::scan_int(BStream& bs, DwFormId id) {
   /* special case address */
   if (id == DW_FORM_addr) {
      if (is_elf64) {
         return bs.read8u();
      } else {
         return bs.read4u();
      }
   }

   /* others are predicatble */
   switch (id) {
   case DW_FORM_data1:  return bs.read1u();
   case DW_FORM_data2:  return bs.read2u();
   case DW_FORM_data4:  return bs.read4u();
   case DW_FORM_data8:  return bs.read8u();
   case DW_FORM_flag:   return bs.read1u();
   case DW_FORM_udata:  return bs.read_uleb();
   case DW_FORM_sdata:  return bs.read_sleb();
   default: throw Exception("invalid DWARF data form");
   }
}

const char*
DwAbbrv::scan_str(BStream& bs, DwFormId id) {
   switch (id) {
   case DW_FORM_string:  return pull_string(bs);
   case DW_FORM_strp:    return pull_strp(bs);
   default: throw Exception("invalid DWARF data form"); 
   }
}

BVector
DwAbbrv::scan_blk(BStream& bs, DwFormId id) {
   const byte* data;
   long L;

   switch (id) {
   case DW_FORM_block1: L = bs.read1u(); break;
   case DW_FORM_block2: L = bs.read2u(); break;
   case DW_FORM_block4: L = bs.read4u(); break;
   case DW_FORM_blockv: L = bs.read_uleb(); break;
   default: throw Exception("invalid DWARF data form");
   }

   data = bs.address();
   bs.seek(L, BSTREAM_CUR);

   return BVector(data, L);
}

Offset
DwAbbrv::scan_off(BStream& bs, DwFormId id) {
   Offset P;

   /* special case address */
   if (id == DW_FORM_ref_addr) {
      if (is_dwf64) {
         P = bs.read8u();
      } else {
         P = bs.read4u();
      }
      return P;
   }

   /* others are predictable */
   switch (id) {
   case DW_FORM_ref1: P = bs.read1u(); break;
   case DW_FORM_ref2: P = bs.read2u(); break;
   case DW_FORM_ref4: P = bs.read4u(); break;
   case DW_FORM_ref8: P = bs.read8u(); break;
   case DW_FORM_ref_udata: P = bs.read_uleb(); break;
   default: throw Exception("invalid DWARF data form");
   }

   return P;
}

/* Scan through an attribute value without
 * actually storing the value itself. 
 */

void
DwAbbrv::seek_thru(BStream& bs, DwFormId id) {

   /* special case address */
   if (id == DW_FORM_addr) {
      if (is_elf64) {
         bs.seek(8, BSTREAM_CUR);
      } else {
         bs.seek(4, BSTREAM_CUR);
      }
      return;
   }

   /* special case DIE address */
   if (id == DW_FORM_ref_addr) {
      if (is_dwf64) {
         bs.seek(8, BSTREAM_CUR);
      } else {
         bs.seek(4, BSTREAM_CUR);
      }
      return;
   }

   /* special case str address */
   if (id == DW_FORM_strp) {
      if (is_dwf64) {
         bs.seek(8, BSTREAM_CUR);
      } else {
         bs.seek(4, BSTREAM_CUR);
      }
      return;
   }

   switch (id) {
   /* seek through integer forms */
   case DW_FORM_data1:  bs.seek(1, BSTREAM_CUR); break;
   case DW_FORM_data2:  bs.seek(2, BSTREAM_CUR); break;
   case DW_FORM_data4:  bs.seek(4, BSTREAM_CUR); break;
   case DW_FORM_data8:  bs.seek(8, BSTREAM_CUR); break;
   case DW_FORM_flag:   bs.seek(1, BSTREAM_CUR); break;
   case DW_FORM_udata:  bs.read_uleb(); break;
   case DW_FORM_sdata:  bs.read_sleb(); break;

   /* seek through string forms */
   case DW_FORM_block1: bs.seek(bs.read1u(),BSTREAM_CUR); break;
   case DW_FORM_block2: bs.seek(bs.read2u(),BSTREAM_CUR); break;
   case DW_FORM_block4: bs.seek(bs.read4u(),BSTREAM_CUR); break;
   case DW_FORM_blockv: bs.seek(bs.read_uleb(),BSTREAM_CUR); break;

   /* seek through reference forms */
   case DW_FORM_ref1: bs.seek(1, BSTREAM_CUR); break;
   case DW_FORM_ref2: bs.seek(2, BSTREAM_CUR); break;
   case DW_FORM_ref4: bs.seek(4, BSTREAM_CUR); break;
   case DW_FORM_ref8: bs.seek(8, BSTREAM_CUR); break;
   case DW_FORM_ref_udata: bs.read_uleb(); break;

   /* seek through string forms */
   case DW_FORM_string: seek_string(bs); break;

   /* unrecognized attribute form */
   default: throw Exception("invalid DWARF data form"); 
   }
}

/* Calculate the size of a DWARF Form
 */

int
DwAbbrv::calc_form_size(DwFormId id) {

   /* special case address */
   if (id == DW_FORM_addr) {
      if (is_elf64) {
	 return 8;
      } else {
	 return 4;
      }
   }

   /* special case DIE address */
   if (id == DW_FORM_ref_addr) {
      if (is_dwf64) {
         return 8;
      } else {
         return 4;
      }
   }

   /* special case str address */
   if (id == DW_FORM_strp) {
      if (is_dwf64) {
         return 8;
      } else {
         return 4;
      }
   }

   switch (id) {
   /* seek through integer forms */
   case DW_FORM_data1:  return 1; break;
   case DW_FORM_data2:  return 2; break;
   case DW_FORM_data4:  return 4; break;
   case DW_FORM_data8:  return 8; break;
   case DW_FORM_flag:   return 1; break;
   case DW_FORM_udata:  return -1; break;
   case DW_FORM_sdata:  return -1; break;

   /* seek through string forms */
   case DW_FORM_block1: return -1; break;
   case DW_FORM_block2: return -1; break;
   case DW_FORM_block4: return -1; break;
   case DW_FORM_blockv: return -1; break;

   /* seek through reference forms */
   case DW_FORM_ref1: return 1; break;
   case DW_FORM_ref2: return 2; break;
   case DW_FORM_ref4: return 4; break;
   case DW_FORM_ref8: return 8; break;
   case DW_FORM_ref_udata: return -1; break;

   /* seek through string forms */
   case DW_FORM_string: return -1; break;

   /* unrecognized attribute form */
   default: throw Exception("invalid DWARF data form"); 
   }
}

int
DwAbbrv::calc_size()
{
   std::list<DwAbbrvP>::iterator i;
   int tsize = 0;

   for (i=data.begin(); i!=data.end(); i++) {
      int t = calc_form_size(i->form);
      if (t == -1) return -1;

      tsize += t;
   }

   return tsize;
}


/* Starting from the attribute list, scan through
 * any attributes between first and the specified
 * attribute.
 */

bool
DwAbbrv::seek_to(BStream& bs, DwAttrId id, DwFormId& fi) {
   std::list<DwAbbrvP>::iterator i,j;

   for (i=data.begin(); i!=data.end(); i++) {
      if (i->attr == id) goto found_it;
   }

   return true;

found_it:
   for (j=data.begin(); j!=i; j++) {
      seek_thru(bs, j->form);
   }

   fi = i->form;
   return false;
}

/* Starting from the attribute list, scan through
 * all abbreviation attributes.
 */

void
DwAbbrv::seek_all(BStream& bs) {
   std::list<DwAbbrvP>::iterator i;

   for (i=data.begin(); i!=data.end(); i++) {
      seek_thru(bs, i->form);
   }
}

/* Return the value of a specified attribute
 */

bool
DwAbbrv::has(DwAttrId id) {
   std::list<DwAbbrvP>::iterator i;

   for (i=data.begin(); i!=data.end(); i++) {
      if (i->attr == id) return true;
   }

   return false;
}

int64_t
DwAbbrv::get_int(BStream& bs, DwAttrId id) {
   Offset off = bs.offset();
   DwFormId f_id;

   if (seek_to(bs, id, f_id)) {
       return 0;
   }

   if (f_id == DW_FORM_indirect) {
      f_id = (DwFormId)bs.read_uleb();
   }

   int64_t ret = scan_int(bs, f_id);
   bs.seek(off, BSTREAM_SET);

   return ret;  
}

const char*
DwAbbrv::get_str(BStream& bs, DwAttrId id) {
   Offset off = bs.offset();
   DwFormId f_id;

   if (seek_to(bs, id, f_id)) {
      return NULL;
   }

   if (f_id == DW_FORM_indirect) {
      f_id = (DwFormId)bs.read_uleb();
   }

   const char* s = scan_str(bs, f_id);
   bs.seek(off, BSTREAM_SET);

   return s;
}

BVector
DwAbbrv::get_blk(BStream& bs, DwAttrId id) {
   Offset off = bs.offset();
   DwFormId f_id;

   if (seek_to(bs, id, f_id)) {
      return BVector();
   }

   if (f_id == DW_FORM_indirect) {
      f_id = (DwFormId)bs.read_uleb();
   }

   BVector blk = scan_blk(bs, f_id);
   bs.seek(off, BSTREAM_SET);

   return blk;
}

Offset
DwAbbrv::get_off(BStream& bs, DwAttrId id, DwCUnit* cu) {
   Offset off = bs.offset();
   DwFormId f_id;

   if (seek_to(bs, id, f_id)) {
      return 0;
   }

   if (f_id == DW_FORM_indirect) {
      f_id = (DwFormId)bs.read_uleb();
   }

   Offset ret = scan_off(bs, f_id);
   if (f_id != DW_FORM_ref_addr) {
      ret += cu->get_sec_offset();
   }

   bs.seek(off, BSTREAM_SET);

   return ret;
}

DwEntry*
DwAbbrv::get_die(BStream& bs, DwAttrId id, DwCUnit* cu) {
   Offset P = get_off(bs, id, cu);
   return cu->get_die(P);
}

/* DwAbbrvTab is a table of abbreviations, each compilation
 * unit will have a pointer to exactly one DwAbbrvTab.
 */
void
DwAbbrvTab::dump() {
   std::map<long,DwAbbrvMap*>::iterator i;
   for (i=data.begin(); i!=data.end(); i++) {
      DwAbbrvMap* cu_map = i->second;

      printf(" <0x%lx>   Id    TAG\n", i->first);
      DwAbbrvMap::iterator j;
      for (j=cu_map->begin(); j!=cu_map->end(); j++) {
         j->second->dump_abbrv();
      }
   }
}

void
DwAbbrvTab::read(BStream& bs) {
   DwAbbrvMap* cu_abbrv = NULL;
   long cu_spos = 0;

   while (!bs.eof()) {
      DwAbbrv* abb;
      long spos;
      int id_num;

      spos = bs.offset(); 
      id_num = bs.read_uleb();  

      if (id_num == 0) {
         if (cu_abbrv != NULL) {
            data[cu_spos] = cu_abbrv;
            cu_abbrv = NULL;
         } continue;
      }

      if (cu_abbrv == NULL) {
         cu_abbrv = new DwAbbrvMap();
         cu_spos = spos;
      }

      abb = new DwAbbrv(dwarf);
      abb->setid(id_num);
      abb->read(bs);

      (*cu_abbrv)[id_num] = abb;
   }

   if (cu_abbrv != NULL) {
      data[cu_spos] = cu_abbrv;
   }
}

