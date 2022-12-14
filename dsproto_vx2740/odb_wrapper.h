#include "midas.h"
#include <string>
#include <vector>

/** 
 * Wrapper of the midas db_xxx functions that will throw a SettingsException
 * if there is a problem, rather than returning a status code.
 */
class ODBWrapper {
   public:
      /** 
       * Constructor.
       * ODB handle can also be passed later with `set_db_handle()` if needed. 
       */
      ODBWrapper(HNDLE _hDB=0) : hDB(_hDB) {}

      virtual ~ODBWrapper() {}

      /** 
       * Set the base ODB handle (if it wasn't available when you called
       * the constructor).
       * 
       * @param[in] _hDB - ODB handle from `cm_get_experiment_database()`.
       */
      void set_db_handle(HNDLE _hDB) {hDB = _hDB;}

      /** 
       * Whether the ODB contains a specific key. 
       * 
       * @param[in] hBase - Where to start searching in the ODB.
       * @param[in] key_name - Path beneath hBase.
       * @exception SettingsException if there's a problem.
       * @return Whether the key exists or not.
       */
      bool has_key(HNDLE hBase, std::string key_name);

      /** 
       * Find and return a handle to a key.
       * 
       * @param[in] hBase - Where to start searching in the ODB.
       * @param[in] key_name - Path beneath hBase.
       * @exception SettingsException if there's a problem.
       * @return ODB handle of the key.
       */
      HNDLE find_key(HNDLE hBase, std::string key_name);

      /** 
       * Find and return a handle to a key.
       * Creates a directory if it doesn't already exist.
       * 
       * @param[in] hBase - Where to start searching in the ODB.
       * @param[in] key_name - Path beneath hBase.
       * @exception SettingsException if there's a problem.
       * @return ODB handle of the key.
       */
      HNDLE find_or_create_dir(HNDLE hBase, std::string key_name);

      /** 
       * Set a value in the ODB.
       * 
       * If needed, it will:
       * - create the key if it doesn't already exist
       * - resize the ODB array to match num_values
       * - change the key type (if `fix_type_if_needed` is true)
       * 
       * @param[in] hBase - Where to start searching in the ODB.
       * @param[in] key_name - Path beneath hBase.
       * @param[in] data - Pointer to data to set.
       * @param[in] size - Size of data payload in bytes.
       * @param[in] num_values - Array length of data (1 if only a single element).
       * @param[in] type - One of the midas TID_xxx values (e.g. TID_FLOAT).
       * @param[in] fix_type_if_needed - If and ODB entry already exists, but is the wrong type,
       *   whether to delete the existing entry and create a new one, or throw an exception.
       * @exception SettingsException if there's a problem.
       */
      void set_value(HNDLE hBase, std::string key_name, const void *data, INT size, INT num_values, DWORD type, bool fix_type_if_needed=true);

      /**
       * Like `set_value()` but handles bool->BOOL conversion.
       * 
       * @param[in] hBase - Where to start searching in the ODB.
       * @param[in] key_name - Path beneath hBase.
       * @param[in] data - Value to set.
       * @param[in] fix_type_if_needed - If and ODB entry already exists, but is the wrong type,
       *   whether to delete the existing entry and create a new one, or throw an exception.
       * @exception SettingsException if there's a problem.
       */
      void set_value_bool(HNDLE hBase, std::string key_name, bool data, bool fix_type_if_needed=true);

      /**
       * Like `set_value()` but handles std::string.
       * 
       * @param[in] hBase - Where to start searching in the ODB.
       * @param[in] key_name - Path beneath hBase.
       * @param[in] data - Value to set.
       * @param[in] fix_type_if_needed - If and ODB entry already exists, but is the wrong type,
       *   whether to delete the existing entry and create a new one, or throw an exception.
       * @exception SettingsException if there's a problem.
       */
      void set_value_string(HNDLE hBase, std::string key_name, std::string data, bool fix_type_if_needed=true);
   
      /**
       * Like `set_value()` but handles an array of strings.
       * 
       * @param[in] hBase - Where to start searching in the ODB.
       * @param[in] key_name - Path beneath hBase.
       * @param[in] data - Value to set.
       * @param[in] fix_type_if_needed - If and ODB entry already exists, but is the wrong type,
       *   whether to delete the existing entry and create a new one, or throw an exception.
       * @exception SettingsException if there's a problem.
       */
      void set_value_string_array(HNDLE hBase, std::string key_name, std::vector<std::string> values, int max_string_size);

      /** 
       * Set the value of a specific entry in an ODB array.
       * If needed, it will:
       * - create the key if it doesn't already exist
       * - resize the ODB array to be `index` values long (if `trucate` is true)
       * 
       * @param[in] hBase - Where to start searching in the ODB.
       * @param[in] key_name - Path beneath hBase.
       * @param[in] data - Pointer to data to set.
       * @param[in] size - Size of data payload in bytes.
       * @param[in] index - Index in the ODB array to set the value of.
       * @param[in] type - One of the midas TID_xxx values (e.g. TID_FLOAT).
       * @param[in] truncate - Whether to truncate the array to be `index` elements long.
       * @exception SettingsException if there's a problem.
       */
      void set_value_index(HNDLE hBase, std::string key_name, const void *data, INT size, INT index, DWORD type, BOOL truncate);
      
      /** 
       * Resize an ODB array.
       * 
       * @param[in] hBase - Where to start searching in the ODB.
       * @param[in] key_name - Path beneath hBase.
       * @param[in] num_values - New array length.
       * @exception SettingsException if there's a problem.
       */
      void set_num_values(HNDLE hBase, std::string key_name, INT num_values);

      /** 
       * Creates an entry in the ODB. `type` is one of the midas TID_xxx values.
       * Initial value is zero for numeric types (use `set_value()` if you want to 
       * specify a default value).
       * 
       * @param[in] hBase - Where to start searching in the ODB.
       * @param[in] key_name - Path beneath hBase.
       * @param[in] type - One of the midas TID_xxx values (e.g. TID_FLOAT).
       * @exception SettingsException if there's a problem.
       */
      void create_key(HNDLE hBase, std::string key_name, DWORD type);

      /** 
       * Like `set_value()`, but doesn't overwrite the existing value
       * if the key already exists and has the correct type.
       * 
       * @param[in] hBase - Where to start searching in the ODB.
       * @param[in] key_name - Path beneath hBase.
       * @param[in] data - Pointer to data to set.
       * @param[in] size - Size of data payload in bytes.
       * @param[in] num_values - Array length of data (1 if only a single element).
       * @param[in] type - One of the midas TID_xxx values (e.g. TID_FLOAT).
       * @exception SettingsException if there's a problem.
       */
      void ensure_key_exists_with_type(HNDLE hBase, std::string key_name, const void *initial_data, INT initial_size, INT initial_num_values, DWORD type);

      /**
       * Like `ensure_key_exists_with_type()`, but handles bool->BOOL conversion.
       * 
       * @param[in] hBase - Where to start searching in the ODB.
       * @param[in] key_name - Path beneath hBase.
       * @param[in] initial_data - Value to set if we need to create the key.
       * @exception SettingsException if there's a problem.
       */
      void ensure_bool_exists(HNDLE hBase, std::string key_name, bool initial_data);

      /**
       * Like `ensure_key_exists_with_type()`, but handles strings.
       * 
       * @param[in] hBase - Where to start searching in the ODB.
       * @param[in] key_name - Path beneath hBase.
       * @param[in] initial_data - Value to set if we need to create the key.
       * @exception SettingsException if there's a problem.
       */
      void ensure_string_exists(HNDLE hBase, std::string key_name, std::string initial_data);

      /** 
       * Resize an ODB array of strings.
       * 
       * @param[in] hBase - Where to start searching in the ODB.
       * @param[in] key_name - Path beneath hBase.
       * @param[in] num_values - Size of ODB array.
       * @param[in] max_string_size - Size of each string in the array.
       * @exception SettingsException if there's a problem.
       */
      void resize_string(HNDLE hBase, std::string key_name, int num_values, int max_string_size);
      
      /** 
       * Get the content of an ODB entry.
       * 
       * @param[in] hBase - Where to start searching in the ODB.
       * @param[in] key_name - Path beneath hBase.
       * @param[out] data - Pointer where data should be written.
       * @param[in] size - Expected data payload size in bytes.
       * @param[in] type - One of the midas TID_xxx values (e.g. TID_FLOAT).
       * @param[in] create - Whether to create the key if it doesn't exist.
       * @exception SettingsException if there's a problem.
       */
      void get_value(HNDLE hBase, std::string key_name, void *data, INT size, DWORD type, BOOL create=false);

      /** 
       * Get the content of an ODB TID_BOOL entry as a bool.
       * 
       * @param[in] hBase - Where to start searching in the ODB.
       * @param[in] key_name - Path beneath hBase.
       * @param[out] data - Pointer where data should be written.
       * @param[in] create - Whether to create the key if it doesn't exist.
       * @exception SettingsException if there's a problem.
       */
      void get_value_bool(HNDLE hBase, std::string key_name, bool *data, BOOL create=false);

      /** 
       * Get the content of an ODB TID_STRING entry.
       * 
       * @param[in] hBase - Where to start searching in the ODB.
       * @param[in] key_name - Path beneath hBase.
       * @param[in] index - Index if reading from an array of TID_STRING.
       * @param[out] data - Pointer where data should be sent.
       * @param[in] create - Whether to create the key if it doesn't exist.
       * @exception SettingsException if there's a problem.
       */
      void get_value_string(HNDLE hBase, std::string key_name, int index, std::string *data, BOOL create=false);
      
      /** 
       * Delete a key from the ODB.
       * 
       * @param[in] hBase - Where to start searching in the ODB.
       * @param[in] key_name - Path beneath hBase.
       * @param[in] follow_links - Whether to follow ODB links.
       * @exception SettingsException if there's a problem.
       */
      void delete_key(HNDLE hBase, std::string key_name, BOOL follow_links=true);

      /** 
       * Get metadata about an ODB entry (type, name etc).
       * 
       * @param[in] hBase - Where to start searching in the ODB.
       * @param[in] key_name - Path beneath hBase.
       * @exception SettingsException if there's a problem.
       * @return The ODB KEY (see midas.h for struct definition).
       */
      KEY get_key(HNDLE hBase, std::string key_name);

      /** 
       * Find all the subdirectories within a given ODB directory that
       * start with a given prefix.
       * 
       * E.g. 
       * For an ODB directory containing:
       * - "Board names" (array of string)
       * - "Board 0" (directory)
       * - "Board 1" (directory)
       * - "Some other dir" (directory)
       * 
       * Calling `find_subdirs_starting_with(hBase, "Board")` would return
       * `["Board 0", "Board 1"]`.
       * 
       * @param[in] hBase - Where to start searching in the ODB.
       * @param[in] prefix - Prefix to search for.
       * @exception SettingsException if there's a problem.
       * @return The directory names that match.
       */
      std::vector<std::string> find_subdirs_starting_with(HNDLE hBase, std::string prefix);

   protected:
      HNDLE hDB; //!< Main ODB handle.
};