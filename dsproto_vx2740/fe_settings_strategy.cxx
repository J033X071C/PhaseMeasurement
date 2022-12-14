#include "fe_settings_strategy.h"
#include "fe_utils.h"
#include "caen_exceptions.h"
#include <algorithm>

VX2740FeSettingsODB::VX2740FeSettingsODB(std::string _custom_set_dir, std::string _custom_rdb_dir) :
   custom_set_dir(_custom_set_dir), 
   custom_rdb_dir(_custom_rdb_dir) {}

void VX2740FeSettingsODB::init(bool _single_fe_mode, int _this_group_index, HNDLE _hDB) {
   odb.set_db_handle(_hDB);

   single_fe_mode = _single_fe_mode;
   this_group_index = _this_group_index;
   char cpath[255];

   if (single_fe_mode) {
      if (custom_set_dir == "") {
         snprintf(cpath, 255, "/Equipment/VX2740_Config_%03d/Settings", this_group_index);
         defaults_odb_path = cpath;
         group_odb_path = cpath;
      } else {
         defaults_odb_path = custom_set_dir;
         group_odb_path = custom_set_dir;
      }

      if (custom_rdb_dir == "") {
         snprintf(cpath, 255, "/Equipment/VX2740_Config_%03d/Readback", this_group_index);
         group_readback_odb_path = cpath;
      } else {
         group_readback_odb_path = custom_rdb_dir;
      }

      if (custom_err_dir == "") {
         snprintf(cpath, 255, "/Equipment/VX2740_Errors_%03d/Variables", this_group_index);
         group_errors_odb_path = cpath;
      } else {
         group_errors_odb_path = custom_err_dir;
      }
   } else {
      defaults_odb_path = "/VX2740 defaults";

      if (custom_set_dir == "") {
         snprintf(cpath, 255, "/Equipment/VX2740_Config_Group_%03d/Settings", this_group_index);
         group_odb_path = cpath;
      } else {
         group_odb_path = custom_set_dir;
      }

      if (custom_rdb_dir == "") {
         snprintf(cpath, 255, "/Equipment/VX2740_Config_Group_%03d/Readback", this_group_index);
         group_readback_odb_path = cpath;
      } else {
         group_readback_odb_path = custom_rdb_dir;
      }

      if (custom_err_dir == "") {
         snprintf(cpath, 255, "/Equipment/VX2740_Errors_Group_%03d/Variables", this_group_index);
         group_errors_odb_path = cpath;
      } else {
         group_errors_odb_path = custom_err_dir;
      }
   }

   setup_group_odb();
   setup_default_odb();
   setup_board_params();
}

std::string VX2740FeSettingsODB::get_board_subdir_name(int board_id) {
   char dirname[32];
   snprintf(dirname, 31, "Board%02d", board_id);
   return dirname;
}

std::string VX2740FeSettingsODB::get_board_odb_path(int board_id) {
   if (single_fe_mode) {
      return group_odb_path;
   } else {
      char board_path[255];
      snprintf(board_path, 255, "%s/%s", group_odb_path.c_str(), get_board_subdir_name(board_id).c_str());
      return board_path;
   }
}

std::string VX2740FeSettingsODB::get_board_odb_readback_path(int board_id) {
   if (single_fe_mode) {
      return group_readback_odb_path;
   } else {
      char board_path[255];
      snprintf(board_path, 255, "%s/%s", group_readback_odb_path.c_str(), get_board_subdir_name(board_id).c_str());
      return board_path;
   }
}

std::string VX2740FeSettingsODB::get_board_error_key_name(int board_id, std::string param_name) {
   char board_path[255];

   if (single_fe_mode) {
      snprintf(board_path, 255, "%s", param_name.c_str());
   } else {
      snprintf(board_path, 255, "%s %s", get_board_subdir_name(board_id).c_str(), param_name.c_str());
   }

   return board_path;
}

std::vector<std::string> VX2740FeSettingsODB::get_history_names() {
   std::vector<std::string> history_names;
   history_names.push_back("Board status");
   history_names.push_back("Temp air in (C)");
   history_names.push_back("Temp hottest ADC (C)");
   history_names.push_back("Error flags");

   return history_names;
}

std::vector<std::string> VX2740FeSettingsODB::get_deprecated_key_names() {
   std::vector<std::string> deprecated_keys;
   deprecated_keys.push_back("Chan over thresh majority"); // name changed
   deprecated_keys.push_back("Ch over thresh multiplicity"); // now support A and B triggers
   deprecated_keys.push_back("Ch over thresh en mask (31-0)"); // now support A and B triggers
   deprecated_keys.push_back("Ch over thresh en mask (63-32)"); // now support A and B triggers
   deprecated_keys.push_back("Enable chan over thresh trigger"); // now support A and B triggers
   deprecated_keys.push_back("Enable external trigger"); // name changed
   deprecated_keys.push_back("Enable software trigger"); // name changed
   deprecated_keys.push_back("Enable user trigger"); // name changed
   deprecated_keys.push_back("Enable test pulse trigger"); // name changed
   
   return deprecated_keys;
}

std::vector<std::string> VX2740FeSettingsODB::get_deprecated_user_reg_names() {
   std::vector<std::string> deprecated_user_regs;
   deprecated_user_regs.push_back("0x300"); // Implementation where user specifies hex values as keys no longer used
   deprecated_user_regs.push_back("0x600"); // Implementation where user specifies hex values as keys no longer used

   return deprecated_user_regs;
}

void VX2740FeSettingsODB::setup_default_odb() {
   // Create defaults directory

   hDefaults = odb.find_or_create_dir(0, defaults_odb_path);
   hDefaultRegs = odb.find_or_create_dir(hDefaults, "User registers");

   // Delete keys that are no longer needed
   for (auto k: get_deprecated_key_names()) {
      if (odb.has_key(hDefaults, k)) {
         odb.delete_key(hDefaults, k);
      }
   }

   for (auto k: get_deprecated_user_reg_names()) {
      if (odb.has_key(hDefaultRegs, k)) {
         odb.delete_key(hDefaultRegs, k);
      }
   }

   // Add keys that are needed
   BoardSettings settings;
   HNDLE subkey = 0;

   for (auto& s : settings.strings) {
      odb.ensure_string_exists(hDefaults, s.first, s.second);
   }
   for (auto& s : settings.bools) {
      odb.ensure_bool_exists(hDefaults, s.first, s.second);
   }
   for (auto& s : settings.uint16s) {
      odb.ensure_key_exists_with_type(hDefaults, s.first, (void*)&s.second, sizeof(s.second), 1, TID_UINT16);
   }
   for (auto& s : settings.uint32s) {
      odb.ensure_key_exists_with_type(hDefaults, s.first, (void*)&s.second, sizeof(s.second), 1, TID_UINT32);
   }
   for (auto& s : settings.int32s) {
      odb.ensure_key_exists_with_type(hDefaults, s.first, (void*)&s.second, sizeof(s.second), 1, TID_INT32);
   }
   for (auto& s : settings.doubles) {
      odb.ensure_key_exists_with_type(hDefaults, s.first, (void*)&s.second, sizeof(s.second), 1, TID_DOUBLE);
   }
   for (auto& s : settings.vec_bools) {
      // bool to BOOL
      std::vector<BOOL> val;
      for (size_t i = 0; i < s.second.size(); i++) {
         val.push_back(s.second[i]);
      }
      odb.ensure_key_exists_with_type(hDefaults, s.first, val.data(), sizeof(val[0]) * val.size(), val.size(), TID_BOOL);
   }
   for (auto& s : settings.vec_int16s) {
      odb.ensure_key_exists_with_type(hDefaults, s.first, s.second.data(), sizeof(s.second[0]) * s.second.size(), s.second.size(), TID_INT16);
   }
   for (auto& s : settings.vec_uint16s) {
      odb.ensure_key_exists_with_type(hDefaults, s.first, s.second.data(), sizeof(s.second[0]) * s.second.size(), s.second.size(), TID_UINT16);
   }
   for (auto& s : settings.vec_uint32s) {
      odb.ensure_key_exists_with_type(hDefaults, s.first, s.second.data(), sizeof(s.second[0]) * s.second.size(), s.second.size(), TID_UINT32);
   }
   for (auto& s : settings.vec_int32s) {
      odb.ensure_key_exists_with_type(hDefaults, s.first, s.second.data(), sizeof(s.second[0]) * s.second.size(), s.second.size(), TID_INT32);
   }
   for (auto& s : settings.vec_strings) {
      if (!odb.has_key(hDefaults, s.first)) {
         odb.set_value_string_array(hDefaults, s.first, s.second, 32);
      } else {
         odb.set_num_values(hDefaults, s.first, s.second.size());
      }
   }
   for (auto& s : settings.vec_floats) {
      odb.ensure_key_exists_with_type(hDefaults, s.first, s.second.data(), sizeof(s.second[0]) * s.second.size(), s.second.size(), TID_FLOAT);
   }
}

void VX2740FeSettingsODB::setup_group_odb() {
   hGroup = odb.find_or_create_dir(0, group_odb_path);

   if (!single_fe_mode) {
      int32_t init_num_boards = 1;
      odb.ensure_key_exists_with_type(hGroup, "Num boards (restart on change)", (void*)&init_num_boards, sizeof(init_num_boards), 1, TID_INT32);
      odb.ensure_bool_exists(hGroup, "Merge data using event ID", true);
   }

   odb.ensure_bool_exists(hGroup, "Debug data", false);
   odb.ensure_bool_exists(hGroup, "Debug settings", false);
   odb.ensure_bool_exists(hGroup, "Debug ring buffers", false);
   odb.ensure_bool_exists(hGroup, "Multi-threaded readout", true);

   odb.set_value_string_array(hGroup, "Names", get_history_names(), 32);
}

void VX2740FeSettingsODB::setup_board_params() {
   hReadback = odb.find_or_create_dir(0, group_readback_odb_path);
   hErrors = odb.find_or_create_dir(0, group_errors_odb_path);

   hBoardReadback.clear();
   hBoardSettings.clear();

   if (single_fe_mode) {
      num_boards = 1;
      hBoardReadback[0] = odb.find_or_create_dir(0, get_board_odb_readback_path(0));
   } else {
      odb.get_value(hGroup, "Num boards (restart on change)", &num_boards, sizeof(num_boards), TID_INT32, FALSE);

      // Create ODB dirs for the boards we're controlling
      std::vector<std::string> board_dirs;

      for (int i = 0; i < num_boards; i++) {
         std::string board_name = get_board_subdir_name(i);
         board_dirs.push_back(board_name);
         hBoardSettings[i] = odb.find_or_create_dir(hGroup, board_name);
         hBoardReadback[i] = odb.find_or_create_dir(0, get_board_odb_readback_path(i));

         odb.ensure_string_exists(hBoardSettings[i], "Hostname (restart on change)", "set hostname here");
      }

      // Delete ODB dirs of boards no longer needed
      std::vector<std::string> boards_in_odb = odb.find_subdirs_starting_with(hGroup, "Board");

      for (auto& it : boards_in_odb) {
         if (std::find(board_dirs.begin(), board_dirs.end(), it) == board_dirs.end()) {
            fe_utils::ts_printf("Deleting parameters for %s that are no longer needed\n", it.c_str());
            odb.delete_key(hGroup, it);
         }
      }
   }
}

void VX2740FeSettingsODB::fill_group_settings_struct(GroupSettings& group_settings) {
   odb.get_value(hGroup, "Num boards (restart on change)", &group_settings.num_boards, sizeof(int32_t), TID_INT32, FALSE);

   odb.get_value_bool(hGroup, "Debug data", &group_settings.debug_data);
   odb.get_value_bool(hGroup, "Debug settings", &group_settings.debug_settings);
   odb.get_value_bool(hGroup, "Debug ring buffers", &group_settings.debug_ring_buffers);
   odb.get_value_bool(hGroup, "Multi-threaded readout", &group_settings.multithreaded_readout);

   if (odb.has_key(hGroup, "Merge data using event ID")) {
      odb.get_value_bool(hGroup, "Merge data using event ID", &group_settings.merge_data_using_event_id);
   } else {
      group_settings.merge_data_using_event_id = false;
   }
}

bool VX2740FeSettingsODB::has_board_override(std::string param_name, int board_id) {
   return odb.has_key(hBoardSettings[board_id], param_name);
}

HNDLE VX2740FeSettingsODB::get_board_setting_handle(std::string param_name, int board_id) {
   HNDLE base_dir = get_board_setting_base_handle(param_name, board_id);
   return odb.find_key(base_dir, param_name);
}

HNDLE VX2740FeSettingsODB::get_board_setting_base_handle(std::string param_name, int board_id) {
   if (has_board_override(param_name, board_id)) {
      return hBoardSettings[board_id];
   } else {
      return hDefaults;
   }
}

void VX2740FeSettingsODB::fill_board_settings_struct(BoardSettings& board_settings, int board_id) {
   HNDLE hBase;

   for (auto& s : board_settings.strings) {
      hBase = get_board_setting_base_handle(s.first, board_id);
      odb.get_value_string(hBase, s.first, 0, &s.second);
   }
   for (auto& s : board_settings.bools) {
      hBase = get_board_setting_base_handle(s.first, board_id);
      odb.get_value_bool(hBase, s.first, &board_settings.bools[s.first]);
   }
   for (auto& s : board_settings.uint16s) {
      hBase = get_board_setting_base_handle(s.first, board_id);
      odb.get_value(hBase, s.first, &s.second, sizeof(s.second), TID_UINT16, FALSE);
   }
   for (auto& s : board_settings.uint32s) {
      hBase = get_board_setting_base_handle(s.first, board_id);
      odb.get_value(hBase, s.first, &s.second, sizeof(s.second), TID_UINT32, FALSE);
   }
   for (auto& s : board_settings.int32s) {
      hBase = get_board_setting_base_handle(s.first, board_id);
      odb.get_value(hBase, s.first, &s.second, sizeof(s.second), TID_INT32, FALSE);
   }
   for (auto& s : board_settings.doubles) {
      hBase = get_board_setting_base_handle(s.first, board_id);
      odb.get_value(hBase, s.first, &s.second, sizeof(s.second), TID_DOUBLE, FALSE);
   }
   for (auto& s : board_settings.vec_bools) {
      hBase = get_board_setting_base_handle(s.first, board_id);
      std::vector<BOOL> tmp_bool(s.second.size());
      odb.get_value(hBase, s.first, (void*)tmp_bool.data(), sizeof(BOOL) * tmp_bool.size(), TID_BOOL);

      for (size_t i = 0; i < tmp_bool.size(); i++) {
         s.second[i] = tmp_bool[i];
      }
   }
   for (auto& s : board_settings.vec_uint32s) {
      hBase = get_board_setting_base_handle(s.first, board_id);
      odb.get_value(hBase, s.first, (void*)s.second.data(), sizeof(s.second[0]) * s.second.size(), TID_UINT32);
   }
   for (auto& s : board_settings.vec_int16s) {
      hBase = get_board_setting_base_handle(s.first, board_id);
      odb.get_value(hBase, s.first, (void*)s.second.data(), sizeof(s.second[0]) * s.second.size(), TID_INT16);
   }
   for (auto& s : board_settings.vec_uint16s) {
      hBase = get_board_setting_base_handle(s.first, board_id);
      odb.get_value(hBase, s.first, (void*)s.second.data(), sizeof(s.second[0]) * s.second.size(), TID_UINT16);
   }
   for (auto& s : board_settings.vec_int32s) {
      hBase = get_board_setting_base_handle(s.first, board_id);
      odb.get_value(hBase, s.first, (void*)s.second.data(), sizeof(s.second[0]) * s.second.size(), TID_INT32);
   }
   for (auto& s : board_settings.vec_strings) {
      hBase = get_board_setting_base_handle(s.first, board_id);

      for (size_t i = 0; i < s.second.size(); i++) {
         odb.get_value_string(hBase, s.first, i, &s.second[i]);
      }
   }
   for (auto& s : board_settings.vec_floats) {
      hBase = get_board_setting_base_handle(s.first, board_id);
      odb.get_value(hBase, s.first, (void*)s.second.data(), sizeof(s.second[0]) * s.second.size(), TID_FLOAT);
   }
}

void VX2740FeSettingsODB::handle_board_readback_struct(BoardReadback& board_readback, int board_id) {
   HNDLE subkey = 0;

   for (auto& s : board_readback.strings) {
      odb.set_value_string(hBoardReadback[board_id], s.first, s.second);
   }
   for (auto& s : board_readback.bools) {
      odb.set_value_bool(hBoardReadback[board_id], s.first, s.second);
   }
   for (auto& s : board_readback.uint16s) {
      odb.set_value(hBoardReadback[board_id], s.first, &s.second, sizeof(s.second), 1, TID_UINT16);
   }
   for (auto& s : board_readback.uint32s) {
      odb.set_value(hBoardReadback[board_id], s.first, &s.second, sizeof(s.second), 1, TID_UINT32);
   }
   for (auto& s : board_readback.int32s) {
      odb.set_value(hBoardReadback[board_id], s.first, &s.second, sizeof(s.second), 1, TID_INT32);
   }
   for (auto& s : board_readback.doubles) {
      odb.set_value(hBoardReadback[board_id], s.first, &s.second, sizeof(s.second), 1, TID_DOUBLE);
   }
   for (auto& s : board_readback.vec_bools) {
      std::vector<BOOL> tmp_bool;
      for (size_t i = 0; i < s.second.size(); i++) {
         tmp_bool.push_back(s.second[i]);
      }

      odb.set_value(hBoardReadback[board_id], s.first, tmp_bool.data(), sizeof(BOOL) * tmp_bool.size(), tmp_bool.size(), TID_BOOL);
   }
   for (auto& s : board_readback.vec_int16s) {
      odb.set_value(hBoardReadback[board_id], s.first, s.second.data(), sizeof(s.second[0]) * s.second.size(), s.second.size(), TID_INT16);
   }
   for (auto& s : board_readback.vec_uint16s) {
      odb.set_value(hBoardReadback[board_id], s.first, s.second.data(), sizeof(s.second[0]) * s.second.size(), s.second.size(), TID_UINT16);
   }
   for (auto& s : board_readback.vec_uint32s) {
      odb.set_value(hBoardReadback[board_id], s.first, s.second.data(), sizeof(s.second[0]) * s.second.size(), s.second.size(), TID_UINT32);
   }
   for (auto& s : board_readback.vec_int32s) {
      odb.set_value(hBoardReadback[board_id], s.first, s.second.data(), sizeof(s.second[0]) * s.second.size(), s.second.size(), TID_INT32);
   }
   for (auto& s : board_readback.vec_strings) {
      odb.set_value_string_array(hBoardReadback[board_id], s.first, s.second, 32);
   }
   for (auto& s : board_readback.vec_floats) {
      odb.set_value(hBoardReadback[board_id], s.first, s.second.data(), sizeof(s.second[0]) * s.second.size(), s.second.size(), TID_FLOAT);
   }
}

void VX2740FeSettingsODB::handle_board_errors_struct(BoardErrors& board_errors, int board_id) {
   odb.set_value(hErrors, get_board_error_key_name(board_id, "Error flags"), &board_errors.bitmask, sizeof(uint32_t), 1, TID_UINT32);
   odb.set_value_string(hErrors, get_board_error_key_name(board_id, "Error flags text"), board_errors.message);
}