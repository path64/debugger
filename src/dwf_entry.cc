#include <stdlib.h>
#include <stdio.h>

#include "dwf_names.h"
#include "dwf_entry.h"
#include "dwf_cunit.h"

DwEntry::iterator DwEntry::nil_iter;

DwEntry::DwEntry(DwCUnit* c, DwAbbrv* a) {
   sibling = NULL;
   child = NULL;

   abbrv = a;
   cunit = c;
   c_loaded = false;
}

void
DwEntry::read(BStream& bs, bool try_skip, Offset base) {
   Offset spos;

   /* first try for a sibling DIE */
   if (try_skip && abbrv->has_sibling()) {
      spos = abbrv->get_off(bs, DW_AT_sibling, cunit);
      bs.seek(spos - base, BSTREAM_SET);
      return;
   }

   /* if here, children are loaded */
   c_loaded = true;

   /* next try fixed size seek */
   spos = abbrv->get_size();
   if (spos != -1) {
      bs.seek(spos, BSTREAM_CUR);
      return;
   }

   /* no nothing, do slow seek */
   abbrv->seek_all(bs);
}

DwEntry*
DwEntry::read_tree(DwCUnit* cu, DwEntry* parent, BStream& bs, Offset base) {
   DwEntry *first=NULL, *prev=NULL;

   while (!bs.eof()) {
      DwEntry* entry;

      Offset sec_off = base + bs.offset();

      /* store stream location */
      const byte* addr = bs.address();
      long slen = bs.remaining();
      BVector bvec(addr, slen);

      /* find the abbreviation code */
      int abb_num = bs.read_uleb();
      if (abb_num == 0) break;

      DwAbbrv* abbrv = cu->get_abb_num(abb_num);
      if (abbrv == NULL) {
         throw Exception("invalid DWARF");
      }

      /* allocate new DIE node */
      entry = new DwEntry(cu, abbrv);
      if (entry == NULL) {
         throw Exception("out of memory");
      }
      if (first == NULL) {
         first = entry;
      }

      /* perform skippy read */
      entry->read(bs, true, base);
      entry->set_bvector(bvec);
      entry->set_sec_off(sec_off);
      entry->set_parent(parent);

      /* carryover linked list */
      if (prev != NULL) {
         prev->set_sibling(entry);
      }  prev = entry;

      /* if necessary, read the children */
      if (!abbrv->has_sibling() && abbrv->has_kids()) {
         DwEntry* child = read_tree(cu, entry, bs, base);
         entry->set_child(child);
      }

      /* compile units don't zero terminate */
      if (parent == NULL) {
         break;
      }
   }

   return first;
}

void
DwEntry::read_children() {
   BStream bs(bvec, cunit->do_swap());

   if (abbrv->has_kids()) {
      DwEntry* child;

      /* skip this entry */
      bs.read_uleb();
      read(bs, false);

      child = read_tree(cunit, this, bs, sec_off);
      set_child(child);
   }
   c_loaded = true;
}

void
DwEntry::dump(int level) {
   BStream s(bvec, cunit->do_swap());
   const char* tag_name;
   int ab_id = abbrv->getid();

   tag_name = globl_dwf_names.get(abbrv->get_tag());
   /*
    * XXX - the cast is because SLES doesn't know that this is already
    * a long long.  Stupid SLES.
    */
   printf(" <%d><%llx>: Abbrev Number: %d (%s)\n",
      level, (long long)sec_off, ab_id, tag_name);

   s.read_uleb(); /* skip abbrev id */
   abbrv->dump_entry(s,cunit);

   iterator i = children();
   for (; i!=end(); i++) {
      (*i)->dump(level+1);
   }
}

int64_t
DwEntry::get_int(DwAttrId id) {
   BStream s(bvec, cunit->do_swap());
   s.read_uleb(); /* skip abbrev id */
   return abbrv->get_int(s,id);
}

const char*
DwEntry::get_str(DwAttrId id) {
   BStream s(bvec, cunit->do_swap());
   s.read_uleb(); /* skip abbrev id */
   return abbrv->get_str(s,id);
}

BVector
DwEntry::get_blk(DwAttrId id) {
   BStream s(bvec, cunit->do_swap());
   s.read_uleb(); /* skip abbrev id */
   return abbrv->get_blk(s,id);
}

DwEntry*
DwEntry::get_die(DwAttrId id) {
   BStream s(bvec, cunit->do_swap());
   s.read_uleb(); /* skip abbrev id */
   return abbrv->get_die(s,id,cunit);
}

