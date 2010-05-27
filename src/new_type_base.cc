

#include "new_type_base.h"

bool Type::is_derived() {
   switch (data_type) {
   case TYPE_DWARF: {
      switch (dw_data->get_tag()) {
      case DW_TAG_enumeration_type:
      case DW_TAG_structure_type:
      case DW_TAG_union_type:
      case DW_TAG_class_type:
         return true;
      }
      return false;
    }
    default: throw Exception("invalid type format");
    }
}

bool Type::is_qualifier() {
   switch (data_type) {
   case TYPE_DWARF: {
      switch (dw_data->get_tag()) {
      case DW_TAG_const_type:
      case DW_TAG_volatile_type:
      case DW_TAG_typedef:
         return true;
      } 
      return false;
   } 
   default: throw Exception("invalid type format");
   }
}

std::string Type::get_name() {
   switch (data_type) {
   case TYPE_DWARF: {
      const char* s = dw_data->get_str(DW_AT_name);
      return std::string(s); 
   }
   default: throw Exception("invalid type format");
   }
}

Type* Type::get_type() {
   switch (data_type) {
   case TYPE_DWARF: {
      Type* t = dw_data->get_str(DW_AT_type);
      return t;
   }
   default: throw Exception("invalid type format");
   }
}


