
#ifndef _DWF_ABBRV_H_
#define _DWF_ABBRV_H_

#include "bstream.h"
#include "dwf_spec.h"

#include <list>

class DwEntry;
class DwInfo;
class DwCUnit;

struct DwAbbrvP {
   DwAttrId attr;
   DwFormId form;
};

class DwAbbrv {
private: /* not allowed */
   DwAbbrv();

public: /* XXX: should be protected */
   DwAbbrv(DwInfo* info);
   void read(BStream&);
   void dump_entry(BStream& bs,DwCUnit*);
   void dump_abbrv(void);

   void setid(int n) { id_num = n; }
   int get_size() { return size; }
   bool has_kids() { return with_kids; }
   Offset get_off(BStream&, DwAttrId, DwCUnit*);
   void seek_all(BStream&);

public:
   bool has (DwAttrId);
   int getid(void) { return id_num; }
   DwTagId get_tag() { return tag; }
   bool has_sibling() { return with_sib; }

   int64_t get_int(BStream&, DwAttrId);
   const char* get_str(BStream&, DwAttrId);
   BVector get_blk(BStream&, DwAttrId);
   DwEntry* get_die(BStream&, DwAttrId, DwCUnit*);

private:
   /* scan through strings */
   const char* pull_string(BStream&);
   const char* pull_strp(BStream&);
   void seek_string(BStream&);

   /* read in one attribute entry */
   int64_t scan_int(BStream&, DwFormId);
   const char* scan_str(BStream&, DwFormId);
   BVector scan_blk(BStream&, DwFormId);
   Offset scan_off(BStream&, DwFormId);

   /* seek stream over one attribute  */
   void seek_thru(BStream&, DwFormId);

   /* calculate a form size */
   int calc_form_size(DwFormId);
   int calc_size();

   /* scan to specified attribute, sets form  */
   bool seek_to(BStream&, DwAttrId, DwFormId&);

   /* data members */
   std::list<DwAbbrvP> data;
   long id_num;
   DwTagId tag;
   bool with_sib;
   bool with_kids;
   DwInfo* dwinfo;

   signed int size;
   bool is_elf64;
   bool is_dwf64;
};

typedef std::map<long,DwAbbrv*> DwAbbrvMap;

class DwAbbrvTab {
public:
   DwAbbrvTab(DwInfo* dw) : dwarf(dw) {}
   void read(BStream& bs);

   DwAbbrvMap* get(long pos) {
      std::map<long,DwAbbrvMap*>::iterator i;
      if ((i = data.find(pos)) != data.end()) {
         return i->second;
      }
      return NULL;
   }

   void dump();
private:
   std::map<long,DwAbbrvMap*> data;
   DwInfo* dwarf;
};


#endif
