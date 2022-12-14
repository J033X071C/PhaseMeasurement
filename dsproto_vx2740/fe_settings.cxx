#include "fe_settings.h"
#include "fe_settings_strategy.h"
#include "fe_utils.h"
#include "midas.h"
#include <numeric>
#include <cmath>

void VX2740FeSettings::set_board_firmware_info(int board_id, std::string firmware_version, std::string model_name) {
   board_readback[board_id].strings.at("Firmware version") = firmware_version;
   board_readback[board_id].strings.at("Model name") = model_name;
}

void VX2740FeSettings::set_board_user_firmware_info(int board_id, uint32_t user_fw_version, uint32_t user_reg_revision, bool user_upper_32_mirror_lower_32) {
   board_readback[board_id].uint32s.at("User FW revision") = user_fw_version;
   board_readback[board_id].uint32s.at("User register revision") = user_reg_revision;
   board_readback[board_id].bools.at("Upper 32 mirror raw of lower 32") = user_upper_32_mirror_lower_32;
}

void VX2740FeSettings::set_lvds_readback(int board_id, uint16_t lvds_ioreg, uint32_t lvds_userreg_out, uint32_t lvds_userreg_in) {
   board_readback[board_id].uint16s.at("LVDS IO register") = lvds_ioreg;
   board_readback[board_id].uint16s.at("User registers/LVDS output") = lvds_userreg_out;
   board_readback[board_id].uint32s.at("User registers/LVDS input") = lvds_userreg_in;
}

void VX2740FeSettings::set_board_errors(int board_id, BoardErrors& err) {
   board_errors[board_id] = err;
}

std::vector<int> VX2740FeSettings::get_boards_enabled() {
   std::vector<int> retval;

   for (int i = 0; i < get_num_boards(); i++) {
      if (is_board_enabled(i)) {
         retval.push_back(i);
      }
   }

   return retval;
}

std::vector<int> VX2740FeSettings::get_boards_to_read_from() {
   std::vector<int> retval;

   for (auto& b : board_settings) {
      if (is_board_enabled(b.first) && b.second.bools.at("Read data")) {
         retval.push_back(b.first);
      }
   }

   return retval;
}

void VX2740FeSettings::sync_settings_structs() {
   strategy->fill_group_settings_struct(group_settings);

   for (int i = 0; i < get_num_boards(); i++) {
      strategy->fill_board_settings_struct(board_settings[i], i);
   }
}

void VX2740FeSettings::handle_board_readback_structs() {
   for (int i = 0; i < get_num_boards(); i++) {
      strategy->handle_board_readback_struct(board_readback[i], i);
   }
}

void VX2740FeSettings::handle_board_errors_structs() {
   for (int i = 0; i < get_num_boards(); i++) {
      strategy->handle_board_errors_struct(board_errors[i], i);
   }
}

void VX2740FeSettings::write_settings_to_board(int board_id, VX2740& board) {
   if (debug_settings()) {
      fe_utils::ts_printf("Setting parameters for %s\n", board.get_name().c_str());
   }

   CaenParameters& vx = board.params();
   BoardSettings& this_board_settings = board_settings[board_id];
   BoardReadback& this_board_readback = board_readback[board_id];

   vx.set_debug(debug_settings());

   // Set the parameters in CAEN part of firmware
   set_and_check_start_sources(board_id, board, this_board_settings, this_board_readback);
   set_and_check_trigger_sources(board_id, board, this_board_settings, this_board_readback);
   set_and_check_readout_params(board_id, board, this_board_settings, this_board_readback);
   set_and_check_ch_over_thresh(board_id, board, this_board_settings, this_board_readback);
   set_and_check_front_panel(board_id, board, this_board_settings, this_board_readback);
   set_and_check_dc_offsets(board_id, board, this_board_settings, this_board_readback);
   set_and_check_busy_veto(board_id, board, this_board_settings, this_board_readback);
   set_and_check_lvds(board_id, board, this_board_settings, this_board_readback);
   set_and_check_test_pulse(board_id, board, this_board_settings, this_board_readback);

   if (is_scope_mode(board_id)) {
      // Only available in CAEN Scope FW
      set_and_check_scope_readout(board_id, board, this_board_settings, this_board_readback);
      set_and_check_scope_trigger(board_id, board, this_board_settings, this_board_readback);
   }  else {
      // Only available in User FW
      set_and_check_user_registers(board_id, board, this_board_settings, this_board_readback);
   }

   if (this_board_readback.strings.at("Model name") == "VX2745") {
      // Only available in VX2745, not VX2740
      set_and_check_2745(board_id, board, this_board_settings, this_board_readback);
   }

   if (debug_settings()) {
      fe_utils::ts_printf("Finished setting parameters for board %s\n", board.get_name().c_str());
   }
}

void VX2740FeSettings::set_and_check_start_sources(int board_id, VX2740& board, BoardSettings& set, BoardReadback& rdb) {
   board.params().set_start_sources(set.bools.at("Start acq on midas run start"),
                                    set.bools.at("Start acq on encoded CLKIN"),
                                    set.bools.at("Start acq on SIN level"),
                                    set.bools.at("Start acq on SIN edge"),
                                    set.bools.at("Start acq on LVDS"),
                                    set.bools.at("Start acq on first trigger"),
                                    set.bools.at("Start acq on P0"));
   board.params().get_start_sources(rdb.bools.at("Start acq on midas run start"),
                                    rdb.bools.at("Start acq on encoded CLKIN"),
                                    rdb.bools.at("Start acq on SIN level"),
                                    rdb.bools.at("Start acq on SIN edge"),
                                    rdb.bools.at("Start acq on LVDS"),
                                    rdb.bools.at("Start acq on first trigger"),
                                    rdb.bools.at("Start acq on P0"));
   validate(set.bools, rdb.bools, "Start acq on midas run start", board.get_name());
   validate(set.bools, rdb.bools, "Start acq on encoded CLKIN", board.get_name());
   validate(set.bools, rdb.bools, "Start acq on SIN level", board.get_name());
   validate(set.bools, rdb.bools, "Start acq on SIN edge", board.get_name());
   validate(set.bools, rdb.bools, "Start acq on LVDS", board.get_name());
   validate(set.bools, rdb.bools, "Start acq on first trigger", board.get_name());
   validate(set.bools, rdb.bools, "Start acq on P0", board.get_name());

   board.params().set_run_start_delay_cycles(set.uint32s.at("Run start delay (cycles)"));
   board.params().get_run_start_delay_cycles(rdb.uint32s.at("Run start delay (cycles)"));
   validate(set.uint32s, rdb.uint32s, "Run start delay (cycles)", board.get_name());
}

void VX2740FeSettings::set_and_check_trigger_sources(int board_id, VX2740& board, BoardSettings& set, BoardReadback& rdb) {
   // Simplify life by ignoring CAEN threshold trigger in dpp mode
   bool thresh_a = is_scope_mode(board_id) ? set.bools.at("Trigger on ch over thresh A") : false;
   bool thresh_b = is_scope_mode(board_id) ? set.bools.at("Trigger on ch over thresh B") : false;
   bool thresh_ab = is_scope_mode(board_id) ? set.bools.at("Trigger on ch over thresh A&&B") : false;

   // Set board value
   board.params().set_trigger_sources(thresh_a, 
                                      thresh_b, 
                                      thresh_ab, 
                                      set.bools.at("Trigger on external signal"), 
                                      set.bools.at("Trigger on software signal"),
                                      set.bools.at("Trigger on user mode signal"),
                                      set.bools.at("Trigger on test pulse"),
                                      set.bools.at("Trigger on LVDS"));

   // Readback board values
   board.params().get_trigger_sources(rdb.bools.at("Trigger on ch over thresh A"),
                                      rdb.bools.at("Trigger on ch over thresh B"),
                                      rdb.bools.at("Trigger on ch over thresh A&&B"),
                                      rdb.bools.at("Trigger on external signal"), 
                                      rdb.bools.at("Trigger on software signal"),
                                      rdb.bools.at("Trigger on user mode signal"),
                                      rdb.bools.at("Trigger on test pulse"),
                                      rdb.bools.at("Trigger on LVDS"));

   // Check the two agree
   vx2740_comparisons::validate(thresh_a, rdb.bools.at("Trigger on ch over thresh A"), "Trigger on ch over thresh A", board.get_name());
   vx2740_comparisons::validate(thresh_b, rdb.bools.at("Trigger on ch over thresh B"), "Trigger on ch over thresh B", board.get_name());
   vx2740_comparisons::validate(thresh_ab, rdb.bools.at("Trigger on ch over thresh A&&B"), "Trigger on ch over thresh A&&B", board.get_name());
   validate(set.bools, rdb.bools, "Trigger on external signal", board.get_name());
   validate(set.bools, rdb.bools, "Trigger on software signal", board.get_name());
   validate(set.bools, rdb.bools, "Trigger on user mode signal", board.get_name());
   validate(set.bools, rdb.bools, "Trigger on test pulse", board.get_name());
   validate(set.bools, rdb.bools, "Trigger on LVDS", board.get_name());
}

void VX2740FeSettings::set_and_check_readout_params(int board_id, VX2740& board, BoardSettings& set, BoardReadback& rdb) {
   board.params().set_channel_enable_mask(set.uint32s.at("Readout channel mask (31-0)"), set.uint32s.at("Readout channel mask (63-32)"));
   board.params().get_channel_enable_mask(rdb.uint32s.at("Readout channel mask (31-0)"), rdb.uint32s.at("Readout channel mask (63-32)"));
   validate(set.uint32s, rdb.uint32s, "Readout channel mask (31-0)", board.get_name());
   validate(set.uint32s, rdb.uint32s, "Readout channel mask (63-32)", board.get_name());
}

void VX2740FeSettings::set_and_check_ch_over_thresh(int board_id, VX2740& board, BoardSettings& set, BoardReadback& rdb) {
   board.params().set_channel_over_threshold_trigger_A_enable_mask(set.uint32s.at("Ch over thresh A multiplicity"),
                                                                   set.uint32s.at("Ch over thresh A en mask(31-0)"),
                                                                   set.uint32s.at("Ch over thresh A en mask(63-32)"));
   board.params().get_channel_over_threshold_trigger_A_enable_mask(rdb.uint32s.at("Ch over thresh A multiplicity"),
                                                                   rdb.uint32s.at("Ch over thresh A en mask(31-0)"),
                                                                   rdb.uint32s.at("Ch over thresh A en mask(63-32)"));
   validate(set.uint32s, rdb.uint32s, "Ch over thresh A multiplicity", board.get_name());
   validate(set.uint32s, rdb.uint32s, "Ch over thresh A en mask(31-0)", board.get_name());
   validate(set.uint32s, rdb.uint32s, "Ch over thresh A en mask(63-32)", board.get_name());

   board.params().set_channel_over_threshold_trigger_B_enable_mask(set.uint32s.at("Ch over thresh B multiplicity"),
                                                                   set.uint32s.at("Ch over thresh B en mask(31-0)"),
                                                                   set.uint32s.at("Ch over thresh B en mask(63-32)"));
   board.params().get_channel_over_threshold_trigger_B_enable_mask(rdb.uint32s.at("Ch over thresh B multiplicity"),
                                                                   rdb.uint32s.at("Ch over thresh B en mask(31-0)"),
                                                                   rdb.uint32s.at("Ch over thresh B en mask(63-32)"));
   validate(set.uint32s, rdb.uint32s, "Ch over thresh B multiplicity", board.get_name());
   validate(set.uint32s, rdb.uint32s, "Ch over thresh B en mask(31-0)", board.get_name());
   validate(set.uint32s, rdb.uint32s, "Ch over thresh B en mask(63-32)", board.get_name());
}

void VX2740FeSettings::set_and_check_front_panel(int board_id, VX2740& board, BoardSettings& set, BoardReadback& rdb) {
   board.params().set_nim_ttl(set.bools.at("Use NIM IO"));
   board.params().get_nim_ttl(rdb.bools.at("Use NIM IO"));
   validate(set.bools, rdb.bools, "Use NIM IO", board.get_name());

   board.params().set_trigout_mode(set.strings.at("Trigger out mode"));
   board.params().get_trigout_mode(rdb.strings.at("Trigger out mode"));
   validate(set.strings, rdb.strings, "Trigger out mode", board.get_name());

   board.params().set_use_external_clock(set.bools.at("Use external clock"));
   board.params().get_use_external_clock(rdb.bools.at("Use external clock"));
   validate(set.bools, rdb.bools, "Use external clock", board.get_name());

   board.params().set_enable_clock_out(set.bools.at("Enable clock out"));
   board.params().get_enable_clock_out(rdb.bools.at("Enable clock out"));
   validate(set.bools, rdb.bools, "Enable clock out", board.get_name());

   board.params().set_gpio_mode(set.strings.at("GPIO mode"));
   board.params().get_gpio_mode(rdb.strings.at("GPIO mode"));
   validate(set.strings, rdb.strings, "GPIO mode", board.get_name());

   board.params().set_sync_out_mode(set.strings.at("Sync out mode"));
   board.params().get_sync_out_mode(rdb.strings.at("Sync out mode"));
   validate(set.strings, rdb.strings, "Sync out mode", board.get_name());

   board.params().set_enable_fake_sine_data(set.bools.at("Read fake sinewave data"));
   board.params().get_enable_fake_sine_data(rdb.bools.at("Read fake sinewave data"));
   validate(set.bools, rdb.bools, "Read fake sinewave data", board.get_name());
}

void VX2740FeSettings::set_and_check_scope_readout(int board_id, VX2740& board, BoardSettings& set, BoardReadback& rdb) {
   board.params().set_num_trigger_delay_samples(set.uint32s.at("Trigger delay (samples)"));
   board.params().get_num_trigger_delay_samples(rdb.uint32s.at("Trigger delay (samples)"));
   validate(set.uint32s, rdb.uint32s, "Trigger delay (samples)", board.get_name());

   board.params().set_waveform_length_samples(set.uint32s.at("Waveform length (samples)"));
   board.params().get_waveform_length_samples(rdb.uint32s.at("Waveform length (samples)"));
   validate(set.uint32s, rdb.uint32s, "Waveform length (samples)", board.get_name());

   board.params().set_enable_trigger_overlap(set.bools.at("Allow trigger overlap"));
   board.params().get_enable_trigger_overlap(rdb.bools.at("Allow trigger overlap"));
   validate(set.bools, rdb.bools, "Allow trigger overlap", board.get_name());

   board.params().set_pre_trigger_samples(set.uint16s.at("Pre-trigger (samples)"));
   board.params().get_pre_trigger_samples(rdb.uint16s.at("Pre-trigger (samples)"));
   validate(set.uint16s, rdb.uint16s, "Pre-trigger (samples)", board.get_name());

   board.params().set_trigger_id_mode(set.strings.at("Trigger ID mode"));
   board.params().get_trigger_id_mode(rdb.strings.at("Trigger ID mode"));
   validate(set.strings, rdb.strings, "Trigger ID mode", board.get_name());
}

void VX2740FeSettings::set_and_check_scope_trigger(int board_id, VX2740& board, BoardSettings& set, BoardReadback& rdb) { 
   for (int c = 0; c < 64; c++) {
      bool rdb_rising = false;
      board.params().set_channel_trigger_threshold(c, 
                                                   set.bools.at("Use relative trig thresholds"), 
                                                   set.vec_int32s.at("Chan over thresh thresholds")[c],
                                                   set.vec_bools.at("Chan over thresh rising edge")[c],
                                                   set.vec_uint32s.at("Chan over thresh width (ns)")[c]);
      board.params().get_channel_trigger_threshold(c, 
                                                   rdb.bools.at("Use relative trig thresholds"), 
                                                   rdb.vec_int32s.at("Chan over thresh thresholds")[c],
                                                   rdb_rising,
                                                   rdb.vec_uint32s.at("Chan over thresh width (ns)")[c]);
      rdb.vec_bools.at("Chan over thresh rising edge")[c] = rdb_rising;

      std::stringstream param;
      param << "Use relative trig thresholds for channel " << c;
      vx2740_comparisons::validate(set.bools.at("Use relative trig thresholds"), rdb.bools.at("Use relative trig thresholds"), param.str(), board.get_name());
      validate_array_element(set.vec_int32s, rdb.vec_int32s, "Chan over thresh thresholds", board.get_name(), c);
      validate_array_element(set.vec_bools, rdb.vec_bools, "Chan over thresh rising edge", board.get_name(), c);
      validate_array_element(set.vec_uint32s, rdb.vec_uint32s, "Chan over thresh width (ns)", board.get_name(), c);
   }
}

void VX2740FeSettings::set_and_check_dc_offsets(int board_id, VX2740& board, BoardSettings& set, BoardReadback& rdb) {
   board.params().set_enable_dc_offsets(set.bools.at("Enable DC offsets"));
   board.params().get_enable_dc_offsets(rdb.bools.at("Enable DC offsets"));
   validate(set.bools, rdb.bools, "Enable DC offsets", board.get_name());

   for (int i = 0; i < 64; i++) {
      board.params().set_channel_dc_offset(i, set.vec_floats.at("DC offset (pct)")[i]);
      board.params().get_channel_dc_offset(i, rdb.vec_floats.at("DC offset (pct)")[i]);
      validate_array_element(set.vec_floats, rdb.vec_floats, "DC offset (pct)", board.get_name(), i);
   }
}

void VX2740FeSettings::set_and_check_busy_veto(int board_id, VX2740& board, BoardSettings& set, BoardReadback& rdb) {
   board.params().set_busy_in_source(set.strings.at("Busy in source"));
   board.params().get_busy_in_source(rdb.strings.at("Busy in source"));
   validate(set.strings, rdb.strings, "Busy in source", board.get_name());

   board.params().set_veto_params(set.strings.at("Veto source"),
                                  set.bools.at("Veto when source is high"),
                                  set.uint32s.at("Veto width (ns) (0=source len)"));
   board.params().get_veto_params(rdb.strings.at("Veto source"),
                                  rdb.bools.at("Veto when source is high"),
                                  rdb.uint32s.at("Veto width (ns) (0=source len)"));
   validate(set.strings, rdb.strings, "Veto source", board.get_name());
   validate(set.bools, rdb.bools, "Veto when source is high", board.get_name());
   validate(set.uint32s, rdb.uint32s, "Veto width (ns) (0=source len)", board.get_name());
}

void VX2740FeSettings::set_and_check_lvds(int board_id, VX2740& board, BoardSettings& set, BoardReadback& rdb) {
   // Per-quartet
   for (int q = 0; q < 4; q++) {
      // Shenanigans to get around vector<bool> not being like a regular vector,
      // (and you can't just get a reference to an element as a bool&).
      bool rdb_bool = false;
      board.params().set_lvds_quartet_params(q, set.vec_bools.at("LVDS quartet is input")[q], set.vec_strings.at("LVDS quartet mode")[q]);
      board.params().get_lvds_quartet_params(q, rdb_bool, rdb.vec_strings.at("LVDS quartet mode")[q]);
      
      rdb.vec_bools.at("LVDS quartet is input")[q] = rdb_bool;

      validate_array_element(set.vec_bools, rdb.vec_bools, "LVDS quartet is input", board.get_name(), q);
      validate_array_element(set.vec_strings, rdb.vec_strings, "LVDS quartet mode", board.get_name(), q);
   }

   // Per-line
   for (int l = 0; l < 16; l++) {
      board.params().set_lvds_trigger_mask(l, set.vec_uint32s.at("LVDS trigger mask (31-0)")[l], set.vec_uint32s.at("LVDS trigger mask (63-32)")[l]);
      board.params().get_lvds_trigger_mask(l, rdb.vec_uint32s.at("LVDS trigger mask (31-0)")[l], rdb.vec_uint32s.at("LVDS trigger mask (63-32)")[l]);
      validate_array_element(set.vec_uint32s, rdb.vec_uint32s, "LVDS trigger mask (31-0)", board.get_name(), l);
      validate_array_element(set.vec_uint32s, rdb.vec_uint32s, "LVDS trigger mask (63-32)", board.get_name(), l);
   }

   // IO Register value readback depends on whether each quartet
   // is set to input or output. Input quartet readback matches
   // the voltage levels present. Output quartet readback should
   // match what we set.
   board.params().set_lvds_io_register(set.uint16s.at("LVDS IO register"));
   board.params().get_lvds_io_register(rdb.uint16s.at("LVDS IO register"));

   for (int q = 0; q < 4; q++) {
      bool is_ioreg = set.vec_strings.at("LVDS quartet mode")[q] == "IORegister";
      bool is_input = set.vec_bools.at("LVDS quartet is input")[q];

      if (is_ioreg && !is_input) {
         uint16_t set_quartet = (set.uint16s.at("LVDS IO register") >> (q*4)) & 0xf;
         uint16_t rdb_quartet = (rdb.uint16s.at("LVDS IO register") >> (q*4)) & 0xf;

         std::stringstream param;
         param << "LVDS IO register quartet" << q;

         vx2740_comparisons::validate(set_quartet, rdb_quartet, param.str(), board.get_name());
      }
   }
}

void VX2740FeSettings::set_and_check_test_pulse(int board_id, VX2740& board, BoardSettings& set, BoardReadback& rdb) {
   board.params().set_test_pulse(set.doubles.at("Test pulse period (ms)"),
                                 set.uint32s.at("Test pulse width (ns)"),
                                 set.uint16s.at("Test pulse low level (ADC)"),
                                 set.uint16s.at("Test pulse high level (ADC)"));
   board.params().get_test_pulse(rdb.doubles.at("Test pulse period (ms)"),
                                 rdb.uint32s.at("Test pulse width (ns)"),
                                 rdb.uint16s.at("Test pulse low level (ADC)"),
                                 rdb.uint16s.at("Test pulse high level (ADC)"));
   validate(set.doubles, rdb.doubles, "Test pulse period (ms)", board.get_name());
   validate(set.uint32s, rdb.uint32s, "Test pulse width (ns)", board.get_name());
   validate(set.uint16s, rdb.uint16s, "Test pulse low level (ADC)", board.get_name());
   validate(set.uint16s, rdb.uint16s, "Test pulse high level (ADC)", board.get_name());
}

void VX2740FeSettings::set_and_check_2745(int board_id, VX2740& board, BoardSettings& set, BoardReadback& rdb) {
   for (int g = 0; g < 4; g++) {
      board.params().set_vga_gain(g, set.vec_floats.at("VGA gain")[g]);
      board.params().get_vga_gain(g, rdb.vec_floats.at("VGA gain")[g]);
      validate_array_element(set.vec_floats, rdb.vec_floats, "VGA gain", board.get_name(), g);
   }
}

void VX2740FeSettings::set_and_check_user_registers(int board_id, VX2740& board, BoardSettings& set, BoardReadback& rdb) {
   board.params().set_user_register(0x44, set.uint16s.at("User registers/LVDS output"));
   board.params().get_user_register(0x44, rdb.uint16s.at("User registers/LVDS output"));
   validate(set.uint16s, rdb.uint16s, "User registers/LVDS output", board.get_name());

   uint16_t set_loopback = 0;
   uint16_t rdb_loopback = 0;

   if (set.bools.at("User registers/Enable LVDS loopback")) {
      set_loopback |= 0x1;
   }

   if (set.bools.at("User registers/Enable LVDS pair 12 trigger")) {
      set_loopback |= 0x2;
   }

   board.params().set_user_register(0x50, set_loopback);
   board.params().get_user_register(0x50, rdb_loopback);
   vx2740_comparisons::validate(set_loopback, rdb_loopback, "LVDS loopback", board.get_name());
   
   rdb.bools.at("User registers/Enable LVDS loopback") = (rdb_loopback & 0x1);
   rdb.bools.at("User registers/Enable LVDS pair 12 trigger") = (rdb_loopback & 0x2);

   for (int chan = 0; chan < 64; chan++) {
      // Wavelength, Qlong, Qshort registers use "samples = reg_value * 4"
      uint16_t set_wf_conv = set.vec_uint16s.at("User registers/Waveform length (samples)")[chan] / 4;
      uint16_t set_qs_conv = set.vec_uint16s.at("User registers/Qshort length (samples)")[chan] / 4;
      uint16_t set_ql_conv = set.vec_uint16s.at("User registers/Qlong length (samples)")[chan] / 4;
      uint16_t rdb_wf_conv = 0;
      uint16_t rdb_qs_conv = 0;
      uint16_t rdb_ql_conv = 0;

      board.params().set_user_register(0x300 + chan*4, set_wf_conv);
      board.params().get_user_register(0x300 + chan*4, rdb_wf_conv);
      board.params().set_user_register(0x400 + chan*4, set_qs_conv);
      board.params().get_user_register(0x400 + chan*4, rdb_qs_conv);
      board.params().set_user_register(0x500 + chan*4, set_ql_conv);
      board.params().get_user_register(0x500 + chan*4, rdb_ql_conv);

      rdb.vec_uint16s.at("User registers/Waveform length (samples)")[chan] = rdb_wf_conv * 4;
      rdb.vec_uint16s.at("User registers/Qshort length (samples)")[chan] = rdb_qs_conv * 4;
      rdb.vec_uint16s.at("User registers/Qlong length (samples)")[chan] = rdb_ql_conv * 4;
      
      validate_array_element(set.vec_uint16s, rdb.vec_uint16s, "User registers/Waveform length (samples)", board.get_name(), chan);
      validate_array_element(set.vec_uint16s, rdb.vec_uint16s, "User registers/Qshort length (samples)", board.get_name(), chan);
      validate_array_element(set.vec_uint16s, rdb.vec_uint16s, "User registers/Qlong length (samples)", board.get_name(), chan);
   }
   
   // Filter settings
   size_t num_coeffs = set.vec_int16s.at("User registers/FIR filter coefficients").size();
   
   for (size_t coeff = 0; coeff < num_coeffs; coeff++) {
      board.params().set_user_register(0x900 + coeff*4, set.vec_int16s.at("User registers/FIR filter coefficients")[coeff]);
      board.params().get_user_register(0x900 + coeff*4, rdb.vec_int16s.at("User registers/FIR filter coefficients")[coeff]);
      validate_array_element(set.vec_int16s, rdb.vec_int16s, "User registers/FIR filter coefficients", board.get_name(), coeff);
   }

   // Tell the board to load the new FIR coefficients. Self-clears, no need to check result.
   board.params().set_user_register(0x40, 1);

   // Compute the gain and # upper bits to discard
   INT gain_comp_status;
   uint16_t gain, discard;
   std::tie(gain_comp_status, gain, discard) = compute_fir_gain_and_discard(set.vec_int16s.at("User registers/FIR filter coefficients"));
   
   if (gain_comp_status != SUCCESS) {
      throw CaenSetParamException("Unable to compute valid FIR gain/discard for the given FIR filter coefficients");
   }
   
   uint32_t set_gain_reg_val = gain | (((uint32_t)discard) << 16);

   for (int chan = 0; chan < 64; chan++) {
      uint32_t rdb_gain_reg_val;
      uint32_t gain_reg_val = gain | (((uint32_t)discard) << 16);
      board.params().set_user_register(0xC00 + chan*4, set_gain_reg_val);
      board.params().get_user_register(0xC00 + chan*4, rdb_gain_reg_val);

      rdb.vec_uint32s.at("User registers/FIR gain and discard")[chan] = rdb_gain_reg_val;
      std::stringstream param;
      param << "FIR gain and discard for chan " << chan;
      vx2740_comparisons::validate(set_gain_reg_val, rdb_gain_reg_val, param.str(), board.get_name());
   }

   for (int chan = 0; chan < 64; chan++) {
      // Pre-trigger length
      if (set.vec_uint16s.at("User registers/Pre-trigger (samples)")[chan] > 0xFFF) {
         throw CaenSetParamException("User registers/Pre-trigger (samples)", "Max user-mode pre-trigger length is 0xFFF samples.");
      }

      board.params().set_user_register(0xB00 + chan*4, set.vec_uint16s.at("User registers/Pre-trigger (samples)")[chan]);
      board.params().get_user_register(0xB00 + chan*4, rdb.vec_uint16s.at("User registers/Pre-trigger (samples)")[chan]);
      validate_array_element(set.vec_uint16s, rdb.vec_uint16s, "User registers/Pre-trigger (samples)", board.get_name(), chan);

      // Trigger settings
      board.params().set_user_register(0x200 + chan*4, set.vec_uint16s.at("User registers/Darkside trigger threshold")[chan]);
      board.params().get_user_register(0x200 + chan*4, rdb.vec_uint16s.at("User registers/Darkside trigger threshold")[chan]);
      validate_array_element(set.vec_uint16s, rdb.vec_uint16s, "User registers/Darkside trigger threshold", board.get_name(), chan);
   }

   DWORD en_ds_lo = set.uint32s.at("User registers/Darkside trigger en mask(31-0)");
   DWORD en_ds_hi = set.uint32s.at("User registers/Darkside trigger en mask(63-32)");

   uint64_t en_ovth_lo = 0;
   uint64_t en_ovth_hi = 0;
   uint64_t en_ext_lo = 0;
   uint64_t en_ext_hi = 0;
   uint64_t en_int_lo = 0;
   uint64_t en_int_hi = 0;
   uint64_t en_glob_lo = 0; 
   uint64_t en_glob_hi = 0;

   if (set.bools.at("User registers/Expert mode for trig settings")) {
      // User has specified exact config
      en_ovth_lo = set.uint32s.at("User registers/Trigger on threshold (31-0)");
      en_ovth_hi = set.uint32s.at("User registers/Trigger on threshold (63-32)");
      en_ext_lo = set.uint32s.at("User registers/Trigger on external (31-0)");
      en_ext_hi = set.uint32s.at("User registers/Trigger on external (63-32)");
      en_int_lo = set.uint32s.at("User registers/Trigger on internal (31-0)");
      en_int_hi = set.uint32s.at("User registers/Trigger on internal (63-32)");
      en_glob_lo = set.uint32s.at("User registers/Trigger on global (31-0)");
      en_glob_hi = set.uint32s.at("User registers/Trigger on global (63-32)");
   } else {
      // Use the global trigger settings to also configure the user registers.
      DWORD readout_lo = set.uint32s.at("Readout channel mask (31-0)");
      DWORD readout_hi = set.uint32s.at("Readout channel mask (63-32)");

      if (set.bools.at("Trigger on external signal") ||
            set.bools.at("Trigger on software signal") ||
            set.bools.at("Trigger on test pulse") ||
            set.bools.at("Trigger on LVDS")) {
         en_glob_lo |= readout_lo;
         en_glob_hi |= readout_hi;
      }

      if (set.bools.at("Trigger on user mode signal")) {
         en_ovth_lo |= readout_lo;
         en_ovth_hi |= readout_hi;
      }

      if (!set.bools.at("User registers/Only read triggering channel")) {
         en_int_lo |= readout_lo;
         en_int_hi |= readout_hi;
      }
   }

   uint64_t en_ovth = (en_ovth_lo) | (en_ovth_hi << 32);;
   uint64_t en_ext = (en_ext_lo) | (en_ext_hi << 32);
   uint64_t en_int = (en_int_lo) | (en_int_hi << 32);
   uint64_t en_glob = (en_glob_lo) | (en_glob_hi << 32);
   std::vector<DWORD> en_masks(64, 0);

   for (int i = 0; i < 64; i++) {
      DWORD mask = 0;
      if (en_ovth & ((uint64_t)1<<i)) mask |= 1;
      if (en_ext & ((uint64_t)1<<i)) mask |= 2;
      if (en_int & ((uint64_t)1<<i)) mask |= 4;
      if (en_glob & ((uint64_t)1<<i)) mask |= 8;
      en_masks[i] = mask;
   }

   board.params().set_user_register(0xC, en_ds_lo);
   board.params().set_user_register(0x10, en_ds_hi);

   board.params().get_user_register(0xC, rdb.uint32s.at("User registers/Darkside trigger en mask(31-0)"));
   board.params().get_user_register(0x10, rdb.uint32s.at("User registers/Darkside trigger en mask(63-32)"));

   vx2740_comparisons::validate(en_ds_lo, rdb.uint32s.at("User registers/Darkside trigger en mask(31-0)"), "Darkside trigger en mask(31-0)", board.get_name());
   vx2740_comparisons::validate(en_ds_hi, rdb.uint32s.at("User registers/Darkside trigger en mask(63-32)"), "Darkside trigger en mask(63-32)", board.get_name());

   for (int chan = 0; chan < 64; chan++) {
      board.params().set_user_register(0x600 + chan*4, en_masks[chan]);
      board.params().get_user_register(0x600 + chan*4, rdb.vec_uint32s.at("User registers/Channel trigger sources")[chan]);
      vx2740_comparisons::validate(en_masks[chan], rdb.vec_uint32s.at("User registers/Channel trigger sources")[chan], "User registers/Channel trigger sources", board.get_name());
   }

   // Test signal settings
   uint64_t en_filt_lo = set.uint32s.at("User registers/Enable FIR filter (31-0)");
   uint64_t en_filt_hi = set.uint32s.at("User registers/Enable FIR filter (63-32)");
   uint64_t write_raw_lo = set.uint32s.at("User registers/Write unfiltered data (31-0)");
   uint64_t write_raw_hi = set.uint32s.at("User registers/Write unfiltered data (63-32)");

   if (set.bools.at("User registers/Upper 32 mirror raw of lower 32")) {
      // Upper 32 channels should not have FIR filter enabled, as they're
      // displaying the raw version of the lower 32 channels.
      en_filt_hi = 0;
   }

   uint64_t en_filt = (en_filt_lo) | (en_filt_hi << 32);
   uint64_t write_raw = (write_raw_lo) | (write_raw_hi << 32);
   std::vector<uint32_t> test_signal(64);

   for (int i = 0; i < 64; i++) {
      if (en_filt & (((uint64_t)1)<<i)) {
         // Set bit 6 (use filter)
         test_signal[i] |= (1<<6);
      }

      if (set.bools.at("User registers/Trigger on falling edge")) {
         // Set bit 7 (trig on falling edge)
         test_signal[i] |= (1<<7);
      }

      if (set.bools.at("User registers/Upper 32 mirror raw of lower 32") && i >= 32) {
         // Set bit 8 (mirror lower 32)
         test_signal[i] |= (1<<8);
      }

      if (en_filt & (((uint64_t)1)<<i)) {
         // Set bit 11 (write raw data)
         test_signal[i] |= (1<<11);
      }
   }

   for (int chan = 0; chan < 64; chan++) {
      board.params().set_user_register(0x700 + chan*4, test_signal[chan]);
      board.params().get_user_register(0x700 + chan*4, rdb.vec_uint32s.at("User registers/Test signal")[chan]);
      vx2740_comparisons::validate(test_signal[chan], rdb.vec_uint32s.at("User registers/Test signal")[chan], "User registers/Test signal", board.get_name());
   }
}

std::tuple<INT, uint16_t, uint16_t> VX2740FeSettings::compute_fir_gain_and_discard(std::vector<int16_t> coeffs) {
   // Filter math is:
   // * weighted sum of coeff * value (40 bits, but only need first 38)
   // * multiplied by gain (=> 56 bits, but only need first 54)
   // * discard top N bits (=> 56-N bits)
   // * keep top 16 bits (=> 16 bits)
   //
   // Max gain is shift of 16 bits; max discard is 64 bits.
   // 
   // Ideally we would want to just divide by the weighted sum by the sum of
   // the coefficients. But we can only do a multiplication then a bitshift.
   // * ideal_factor = 1./coeff_sum_abs;
   // * our_factor = gain / (1>>(56-discard-16));
   int64_t coeff_sum = std::accumulate(coeffs.begin(), coeffs.end(), (int32_t)0);
   uint64_t coeff_sum_abs = std::abs(coeff_sum);
   uint16_t gain = 1;
   uint16_t discard = 2;
   uint16_t max_discard = 64-16;

   if (coeff_sum_abs == 0) {
      cm_msg(MERROR, __FUNCTION__, "Invalid FIR coefficients - sum of all coefficients must not equal 0!");
      return std::make_tuple(FE_ERR_ODB, 0, 0);
   }

   while (discard <= max_discard) {
      uint64_t gain_full = (uint64_t(1)<<(56-discard-16)) / coeff_sum_abs;

      if (gain_full >= 0x8000) {
         if (discard == max_discard) {
            cm_msg(MERROR, __FUNCTION__, "Invalid FIR coefficients - no suitable gain/discard settings will result in the output being at the same level as the input");
            return std::make_tuple(FE_ERR_ODB, 0, 0);
         }

         // Would need to multiply too much. Increase number of bits discarded.
         discard++;
         continue;
      }

      gain = gain_full;
      break;
   }

   if (debug_settings()) {
      double ideal_factor = 1./coeff_sum_abs;
      double our_factor = double(gain) / (uint64_t(1)<<(56-discard-16));
      double diff_pct = std::fabs(100. * (our_factor-ideal_factor) / ideal_factor);
      printf("Ideal factor is %.10lf; our approximation is %.10lf (within %.2f%%; gain of %u, discard top %d bits)\n", ideal_factor, our_factor, diff_pct, gain, discard);
   }

   return std::make_tuple(SUCCESS, gain, discard);
}
