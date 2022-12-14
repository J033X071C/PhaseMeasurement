#include "odb_wrapper.h"
#include "caen_exceptions.h"

void ODBWrapper::create_key(HNDLE hBase, std::string key_name, DWORD type) {
   INT status = db_create_key(hDB, hBase, key_name.c_str(), type);

   if (status != SUCCESS) {
      throw SettingsException(status, "Error creating", key_name);
   }
}

KEY ODBWrapper::get_key(HNDLE hBase, std::string key_name) {
   HNDLE hKey = find_key(hBase, key_name);
   KEY retval;
   INT status = db_get_key(hDB, hKey, &retval);

   if (status != SUCCESS) {
      throw SettingsException(status, "Failed to get key for", key_name);
   }

   return retval;
}

void ODBWrapper::ensure_key_exists_with_type(HNDLE hBase, std::string key_name, const void *initial_data, INT initial_size, INT initial_num_values, DWORD type) {
   if (has_key(hBase, key_name)) {
      KEY key = get_key(hBase, key_name);

      if (key.type != type) {
         // If key exists but has wrong type, delete it
         delete_key(hBase, key_name);
      } else if (key.num_values != initial_num_values) {
         // If key has wrong array size, resize it
         set_num_values(hBase, key_name, initial_num_values);
      }
   }

   if (!has_key(hBase, key_name)) {
      // If key doesn't exist, create it
      // (either because we just deleted it, or it never existed.)
      set_value(hBase, key_name, initial_data, initial_size, initial_num_values, type, false);
   }
}

void ODBWrapper::ensure_bool_exists(HNDLE hBase, std::string key_name, bool data) {
   if (has_key(hBase, key_name)) {
      KEY key = get_key(hBase, key_name);

      if (key.type != TID_BOOL) {
         // If key exists but has wrong type, delete it
         delete_key(hBase, key_name);
      }
   }

   if (!has_key(hBase, key_name)) {
      // If key doesn't exist, create it
      // (either because we just deleted it, or it never existed.)
      set_value_bool(hBase, key_name, data, false);
   }
}

void ODBWrapper::ensure_string_exists(HNDLE hBase, std::string key_name, std::string data) {
   if (has_key(hBase, key_name)) {
      KEY key = get_key(hBase, key_name);

      if (key.type != TID_STRING) {
         // If key exists but has wrong type, delete it
         delete_key(hBase, key_name);
      }
   }

   if (!has_key(hBase, key_name)) {
      // If key doesn't exist, create it
      // (either because we just deleted it, or it never existed.)
      set_value_string(hBase, key_name, data, false);
   }
}

void ODBWrapper::set_num_values(HNDLE hBase, std::string key_name, INT num_values) {
   HNDLE subkey = find_key(hBase, key_name);
   INT status = db_set_num_values(hDB, subkey, num_values);

   if (status != SUCCESS) {
      throw SettingsException(status, "Error resizing", key_name);
   }
}

void ODBWrapper::get_value(HNDLE hBase, std::string key_name, void *data, INT size, DWORD type, BOOL create) {
   INT status = db_get_value(hDB, hBase, key_name.c_str(), data, &size, type, create);

   if (status != SUCCESS) {
      throw SettingsException(status, "Error getting value of", key_name);
   }
}

void ODBWrapper::get_value_bool(HNDLE hBase, std::string key_name, bool *data, BOOL create) {
   // bool/BOOL conversion
   BOOL tmp_bool = 0;
   get_value(hBase, key_name, &tmp_bool, sizeof(BOOL), TID_BOOL, create);
   *data = tmp_bool;
}

void ODBWrapper::get_value_string(HNDLE hBase, std::string key_name, int index, std::string *data, BOOL create) {
   INT status = db_get_value_string(hDB, hBase, key_name.c_str(), index, data, create);

   if (status != SUCCESS) {
      throw SettingsException(status, "Error getting value of", key_name);
   }
}

void ODBWrapper::set_value(HNDLE hBase, std::string key_name, const void *data, INT size, INT num_values, DWORD type, bool fix_type_if_needed) {
   if (fix_type_if_needed) {
      ensure_key_exists_with_type(hBase, key_name, data, size, num_values, type);
   }

   INT status = db_set_value(hDB, hBase, key_name.c_str(), data, size, num_values, type);

   if (status != SUCCESS) {
      throw SettingsException(status, "Error setting value of", key_name);
   }
}
   
void ODBWrapper::set_value_index(HNDLE hBase, std::string key_name, const void *data, INT size, INT index, DWORD type, BOOL truncate) {
   INT status = db_set_value_index(hDB, hBase, key_name.c_str(), data, size, index, type, truncate);

   if (status != SUCCESS) {
      throw SettingsException(status, "Error setting value of", key_name);
   }
}

void ODBWrapper::set_value_string(HNDLE hBase, std::string key_name, std::string data, bool fix_type_if_needed) {
   if (fix_type_if_needed) {
      ensure_string_exists(hBase, key_name, data);
   }

   INT status = db_set_value_string(hDB, hBase, key_name.c_str(), &data);

   if (status != SUCCESS) {
      throw SettingsException(status, "Error setting value of", key_name);
   }
}

void ODBWrapper::set_value_bool(HNDLE hBase, std::string key_name, bool data, bool fix_type_if_needed) {
   BOOL tmp_bool = data;
   set_value(hBase, key_name, &tmp_bool, sizeof(BOOL), 1, TID_BOOL, fix_type_if_needed);
}

bool ODBWrapper::has_key(HNDLE hBase, std::string key_name) {
   HNDLE subkey = 0;
   INT status = db_find_key(hDB, hBase, key_name.c_str(), &subkey);

   if (status == SUCCESS) {
      return true;
   } else if (status == DB_NO_KEY) {
      return false;
   }

   throw SettingsException(status, "Error finding key", key_name);
}

HNDLE ODBWrapper::find_key(HNDLE hBase, std::string key_name) {
   HNDLE subkey = 0;
   INT status = db_find_key(hDB, hBase, key_name.c_str(), &subkey);

   if (status != SUCCESS) {
      throw SettingsException(status, "Error finding key", key_name);
   }
   
   return subkey;
}

HNDLE ODBWrapper::find_or_create_dir(HNDLE hBase, std::string key_name) {
   if (!has_key(hBase, key_name)) {
      create_key(hBase, key_name, TID_KEY);
   }

   return find_key(hBase, key_name);
}

void ODBWrapper::delete_key(HNDLE hBase, std::string key_name, BOOL follow_links) {
   HNDLE hKey = find_key(hBase, key_name);
   INT status = db_delete_key(hDB, hKey, follow_links);

   if (status != SUCCESS) {
      throw SettingsException(status, "Error deleting", key_name);
   }
}

void ODBWrapper::resize_string(HNDLE hBase, std::string key_name, int num_values, int max_string_size) {
   INT status = db_resize_string(hDB, hBase, key_name.c_str(), num_values, max_string_size);

   if (status != SUCCESS) {
      throw SettingsException(status, "Error resizing string size of key", key_name);
   }
}

void ODBWrapper::set_value_string_array(HNDLE hBase, std::string key_name, std::vector<std::string> values, int max_string_size) {
   // Create a string if needed
   if (!has_key(hBase, key_name)) {
      set_value_string(hBase, key_name.c_str(), values[0]);
   }

   HNDLE subkey = find_key(hBase, key_name);
   
   // Resize array and string length if needed
   resize_string(hBase, key_name.c_str(), values.size(), max_string_size);

   // Set all the values
   for (size_t i = 0; i < values.size(); i++) {
      std::string val = values[i];
      val.resize(max_string_size);
      set_value_index(hBase, key_name.c_str(), val.c_str(), val.size(), i, TID_STRING, FALSE);
   }
}

std::vector<std::string> ODBWrapper::find_subdirs_starting_with(HNDLE hBase, std::string prefix) {
   std::vector<std::string> retval;

   for (int i = 0; ; i++) {
      HNDLE subkey = 0;
      db_enum_key(hDB, hBase, i, &subkey);

      if (!subkey) {
         break;
      }

      KEY key;
      db_get_key(hDB, subkey, &key);

      std::string key_name(key.name);

      if (key.type == TID_KEY && key_name.find(prefix) == 0) {
         retval.push_back(key_name);
      }
   }

   return retval;
}
