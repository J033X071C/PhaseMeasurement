#ifndef VX2740_FE_SETTINGS_H
#define VX2740_FE_SETTINGS_H

#include "caen_parameters.h"
#include "caen_exceptions.h"
#include "vx2740_wrapper.h"
#include "fe_settings_strategy.h"
#include "fe_settings_structs.h"
#include "midas.h"
#include <map>
#include <cmath>
#include <string>
#include <sstream>

namespace vx2740_comparisons {
   // gcc only allows explicit template specializations at the namespace level,
   // not within a class. So although users only really call VX2740GroupFrontend::compare(),
   // we need to do the actual logic in this spearate namespace.
   // Also declare them inline to avoid having multiple definition issues (or having to create
   // a separate .tpp file for the implementations).
   template <class T> inline void validate(T set_val, T rdb_val, std::string param_name, std::string board_name, bool write_midas_msg=true) {
      bool same = (set_val == rdb_val);

      if (!same) {
         std::stringstream s;
         s << "Unexpected readback value for '" << param_name << "' of board " << board_name << ". ";
         s << "Set " << set_val << ", read " << rdb_val << ".";

         if (write_midas_msg) {
            cm_msg(MERROR, __FUNCTION__, "%s", s.str().c_str());
         }

         throw CaenException(s.str());
      }
   }

   template <> inline void validate(float set_val, float rdb_val, std::string param_name, std::string board_name, bool write_midas_msg) {
      bool same = (fabs(set_val - rdb_val) < 0.001);

      if (!same) {
         std::stringstream s;
         s << "Unexpected readback value for '" << param_name << "' of board " << board_name << ". ";
         s << "Set " << set_val << ", read " << rdb_val << ".";
         
         if (write_midas_msg) {
            cm_msg(MERROR, __FUNCTION__, "%s", s.str().c_str());
         }

         throw CaenException(s.str());
      }
   }

   template <> inline void validate(std::string set_val, std::string rdb_val, std::string param_name, std::string board_name, bool write_midas_msg) {
      bool same = (CaenParameters::str_to_lower(set_val) == CaenParameters::str_to_lower(rdb_val));

      if (!same) {
         std::stringstream s;
         s << "Unexpected readback value for '" << param_name << "' of board " << board_name << ". ";
         s << "Set " << set_val << ", read " << rdb_val << ".";
         
         if (write_midas_msg) {
            cm_msg(MERROR, __FUNCTION__, "%s", s.str().c_str());
         }

         throw CaenException(s.str());
      }
   }
}

class VX2740FeSettings {
public:
   VX2740FeSettings(std::shared_ptr<VX2740FeSettingsStrategyBase> _strategy) : strategy(_strategy) {}
   
   // Initialise our settings strategy object.
   // Will throw SettingsException if there's an issue.
   void init_strategy(bool _single_fe_mode, int _this_group_index, HNDLE _hDB) {
      strategy->init(_single_fe_mode, _this_group_index, _hDB);
   }

   // Sync ODB to our GroupSettings struct and BoardSettings structs.
   // Will throw SettingsException if there's an issue.
   void sync_settings_structs();

   // Sync our BoardReadback structs to the ODB.
   // Will throw SettingsException if there's an issue.
   void handle_board_readback_structs();

   // Sync our BoardErrors structs to the ODB.
   // Will throw SettingsException if there's an issue.
   void handle_board_errors_structs();

   // Sync our BoardSettings struct to a physical board.
   // Will throw CaenException if there's an issue.
   void write_settings_to_board(int board_id, VX2740& board);

   // Call handle_board_readback_structs afterwards
   void set_board_firmware_info(int board_id, std::string firmware_version, std::string model_name);
   void set_board_user_firmware_info(int board_id, uint32_t user_fw_version, uint32_t user_reg_revision, bool user_upper_32_mirror_lower_32);
   void set_lvds_readback(int board_id, uint16_t lvds_ioreg, uint32_t lvds_userreg_out, uint32_t lvds_userreg_in);

   // Call handle_board_errors_structs afterwards
   void set_board_errors(int board_id, BoardErrors& err);

   inline int get_num_boards() {
      return group_settings.num_boards;
   }

   std::vector<int> get_boards_enabled();

   std::vector<int> get_boards_to_read_from();

   inline std::string get_hostname(int board_id) {
      return board_settings[board_id].strings.at("Hostname (restart on change)");
   }

   inline bool is_scope_mode(int board_id) {
      return board_settings[board_id].bools.at("Scope mode (restart on change)");
   }

   inline bool is_board_enabled(int board_id) {
      return board_settings[board_id].bools.at("Enable");
   }

   inline uint32_t get_read_data_timeout(int board_id) {
      return board_settings[board_id].uint32s.at("Read data timeout (ms)");
   }

   inline bool debug_settings() {
      return group_settings.debug_settings;
   }

   inline bool debug_data() {
      return group_settings.debug_data;
   }

   inline bool debug_rates() {
      return group_settings.debug_rates;
   }

   inline bool debug_ring_buffers() {
      return group_settings.debug_ring_buffers;
   }

   inline bool merge_data() {
      return group_settings.merge_data_using_event_id;
   }

   inline bool multithreaded_readout() {
      return group_settings.multithreaded_readout;
   }

protected:
   std::map<int, BoardSettings> board_settings;
   std::map<int, BoardReadback> board_readback;
   std::map<int, BoardErrors> board_errors;
   GroupSettings group_settings;

   std::shared_ptr<VX2740FeSettingsStrategyBase> strategy;

   std::tuple<INT, uint16_t, uint16_t> compute_fir_gain_and_discard(std::vector<int16_t> coeffs);

   // Will throw VX2740ReadbackException if set_vals[param_name][elem_idx] != rdb_vals[param_name][elem_idx]
   template <class T> void validate_array_element(const std::map<std::string, std::vector<T>>& set_vals, const std::map<std::string, std::vector<T>>& rdb_vals, std::string param_name, std::string board_name, int elem_idx) {
      std::stringstream full_name;
      full_name << param_name << "[" << elem_idx << "]";
      T set_val = set_vals.at(param_name)[elem_idx];
      T rdb_val = rdb_vals.at(param_name)[elem_idx];
      vx2740_comparisons::validate(set_val, rdb_val, full_name.str(), board_name);
   }

   // Will throw VX2740ReadbackException if set_vals[param_name] != rdb_vals[param_name]
   template <class T> void validate(const std::map<std::string, T>& set_vals, const std::map<std::string, T>& rdb_vals, std::string param_name, std::string board_name) {
      vx2740_comparisons::validate(set_vals.at(param_name), rdb_vals.at(param_name), param_name, board_name);
   }

   void set_and_check_trigger_sources(int board_id, VX2740& board, BoardSettings& set, BoardReadback& rdb);
   void set_and_check_start_sources(int board_id, VX2740& board, BoardSettings& set, BoardReadback& rdb);
   void set_and_check_ch_over_thresh(int board_id, VX2740& board, BoardSettings& set, BoardReadback& rdb);
   void set_and_check_readout_params(int board_id, VX2740& board, BoardSettings& set, BoardReadback& rdb);
   void set_and_check_front_panel(int board_id, VX2740& board, BoardSettings& set, BoardReadback& rdb);
   void set_and_check_dc_offsets(int board_id, VX2740& board, BoardSettings& set, BoardReadback& rdb);
   void set_and_check_busy_veto(int board_id, VX2740& board, BoardSettings& set, BoardReadback& rdb);
   void set_and_check_lvds(int board_id, VX2740& board, BoardSettings& set, BoardReadback& rdb);
   void set_and_check_test_pulse(int board_id, VX2740& board, BoardSettings& set, BoardReadback& rdb);

   void set_and_check_scope_readout(int board_id, VX2740& board, BoardSettings& set, BoardReadback& rdb);
   void set_and_check_scope_trigger(int board_id, VX2740& board, BoardSettings& set, BoardReadback& rdb);

   void set_and_check_user_registers(int board_id, VX2740& board, BoardSettings& set, BoardReadback& rdb);

   void set_and_check_2745(int board_id, VX2740& board, BoardSettings& set, BoardReadback& rdb);

};

#endif