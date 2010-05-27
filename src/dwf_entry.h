
#ifndef _DWF_ENTRY_H_
#define _DWF_ENTRY_H_

#include <stdlib.h>
#include <stdio.h>

#include "dwf_abbrv.h"

class DwEntry {

/* iterator class for linked list */
public:
class iterator {
public:
   iterator(void) { cur = NULL; }
   iterator(DwEntry* e) { cur = e; }

   DwEntry* operator*() {
      return cur;
   }
   iterator operator++() {
      if (cur != NULL) {
         cur = cur->sibling;
      }
      return cur;
   }
   iterator operator++(int) {
      DwEntry* r = cur;
      if (cur != NULL) {
         cur = cur->sibling;
      }
      return r;
   }
   bool operator==(iterator e) {
      return (cur == e.cur);
   }
   bool operator!=(iterator e) {
      return (cur != e.cur);
   }
private:
   DwEntry* cur;
};

public: /* XXX: these should  be protected */
   DwEntry(DwCUnit*,DwAbbrv*);

   /* funny functions for tree traversal */
   void read(BStream&, bool try_skip, Offset base=0);
   static DwEntry* read_tree(DwCUnit* cu,
      DwEntry* parent, BStream& bs, Offset base=0);
   void read_children();

   void dump(int level=0);
   void set_sec_off(Offset p) { sec_off = p; }
   void set_bvector(const BVector& p) { bvec = p; }
   void set_child(DwEntry* p) { child = p; }
   void set_parent(DwEntry* p) { parent = p; }
   void set_sibling(DwEntry* p) { sibling = p; }

public:
   int64_t get_int(DwAttrId id);
   const char* get_str(DwAttrId id);
   BVector get_blk(DwAttrId id);
   DwEntry* get_die(DwAttrId);

   DwEntry* get_parent() { return parent; }
   bool has(DwAttrId id) { return abbrv->has(id); }
   DwTagId get_tag() { return abbrv->get_tag(); }

   iterator children() { 
      if ( !c_loaded ) { read_children(); }
      return iterator(child);
   }
   iterator end() { return nil_iter; }

private:
   DwEntry* parent;
   DwEntry* sibling;
   DwEntry* child;

   DwAbbrv* abbrv;
   DwCUnit* cunit;
   BVector bvec;
   bool c_loaded;
   Offset sec_off;

   static iterator nil_iter;
};

#endif

