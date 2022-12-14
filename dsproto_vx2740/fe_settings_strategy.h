#ifndef FE_SETTINGS_STRATEGY_H
#define FE_SETTINGS_STRATEGY_H

#include "caen_parameters.h"
#include "vx2740_wrapper.h"
#include "fe_settings_structs.h"
#include "odb_wrapper.h"
#include "midas.h"
#include <map>
#include <string>
#include <sstream>

class VX2740FeSettingsStrategyBase {
public:
   virtual ~VX2740FeSettingsStrategyBase() {};
   virtual void init(bool _single_fe_mode, int _this_group_index, HNDLE _hDB) = 0;

   virtual void fill_group_settings_struct(GroupSettings& group_settings) = 0;
   virtual void fill_board_settings_struct(BoardSettings& board_settings, int board_id) = 0;
   virtual void handle_board_readback_struct(BoardReadback& board_readback, int board_id) = 0;
   virtual void handle_board_errors_struct(BoardErrors& board_errors, int board_id) = 0;
};

class VX2740FeSettingsManual : public VX2740FeSettingsStrategyBase {
public:
   virtual ~VX2740FeSettingsManual() {};
   virtual void init(bool _single_fe_mode, int _this_group_index, HNDLE _hDB) override {};

   virtual void fill_group_settings_struct(GroupSettings& group_settings) override { group_settings = manual_group_settings; };
   virtual void fill_board_settings_struct(BoardSettings& board_settings, int board_id) override { board_settings = manual_board_settings[board_id]; };
   virtual void handle_board_readback_struct(BoardReadback& board_readback, int board_id) override {};
   virtual void handle_board_errors_struct(BoardErrors& board_errors, int board_id) override {};

   GroupSettings manual_group_settings;
   std::map<int, BoardSettings> manual_board_settings;
};

class VX2740FeSettingsODB : public VX2740FeSettingsStrategyBase {
public:
   VX2740FeSettingsODB(std::string _custom_set_dir="", std::string _custom_rdb_dir="");
   virtual ~VX2740FeSettingsODB() {};

   virtual void init(bool _single_fe_mode, int _this_group_index, HNDLE _hDB) override;

   virtual void fill_group_settings_struct(GroupSettings& group_settings) override;
   virtual void fill_board_settings_struct(BoardSettings& board_settings, int board_id) override;
   virtual void handle_board_readback_struct(BoardReadback& board_readback, int board_id) override;
   virtual void handle_board_errors_struct(BoardErrors& board_errors, int board_id) override;

protected:
   std::vector<std::string> get_history_names();
   std::vector<std::string> get_deprecated_key_names();
   std::vector<std::string> get_deprecated_user_reg_names();

   virtual void setup_default_odb();
   virtual void setup_group_odb();
   virtual void setup_board_params();

   std::string get_board_subdir_name(int board_id);
   std::string get_board_odb_path(int board_id);
   std::string get_board_odb_readback_path(int board_id);
   std::string get_board_error_key_name(int board_id, std::string param_name);

   bool has_board_override(std::string param_name, int board_id);
   HNDLE get_board_setting_handle(std::string param_name, int board_id);
   HNDLE get_board_setting_base_handle(std::string param_name, int board_id);


   bool single_fe_mode = false;
   int this_group_index = -1;
   int num_boards = 1;

   std::string custom_set_dir;
   std::string custom_rdb_dir;
   std::string custom_err_dir;

   std::string defaults_odb_path;
   std::string group_odb_path;
   std::string group_readback_odb_path;
   std::string group_errors_odb_path;

   ODBWrapper odb;
   HNDLE hDefaults;
   HNDLE hDefaultRegs;
   HNDLE hGroup;
   HNDLE hReadback;
   HNDLE hErrors;
   std::map<int, HNDLE> hBoardSettings;
   std::map<int, HNDLE> hBoardReadback;
};

#endif