
enum TypeId {
  TYPE_DWARF
};

class Type {
public:
   /* the following will pass through modifiers */
   bool is_derived();
   virtual bool is_pointer() { return false; }
   virtual bool is_array() { return false; }
   virtual bool is_function() { return false; }
   virtual bool is_scalar() { return false; }
   virtual bool is_real() { return false; }
   virtual bool is_integral() { return false; }
   virtual bool is_address() { return false; }
   virtual bool is_boolean() { return false; }
   virtual bool is_char() { return false; }
   virtual bool is_uchar() { return false; }
   virtual bool is_schar() { return false; }
   virtual bool is_signed() { return false; }
   virtual bool is_string() { return false; }
   virtual bool is_struct() { return false; }
   virtual bool is_complex() { return false; }
   virtual bool is_local_var() { return false; }
   virtual bool is_member_function() { return false; }
  
   /* the following are limited to one node */
   bool is_qualifier();
   virtual bool is_inheritance() { return false; }


   virtual int get_language() { return 0; }
   DwCUnit* get_cunit() { return cunit; }

   std::string get_name();
   Type* get_type();


   /* used to get members from derived types */
   virtual Type* get_member(const std::string& name) {
       return NULL;
   }

   /* used to get variables from lexical blocks */
   virtual Type* get_symbol(const std::string& name, Address pc) {
       return NULL;
   }

   /* used to get functions from compilation units */
   virtual Type* get_function(const std::string& name) {
       return NULL;
   }

   /* the following functions DWARF-specific 
      and thus should be thought deprecated */
   bool has(DwAttrId id) {
      return dw_data->has(id);
   } 

private:
   TypeId   data_type;
   DwEntry* dw_data;
};
