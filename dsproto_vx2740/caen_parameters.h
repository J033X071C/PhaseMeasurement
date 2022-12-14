#ifndef CAEN_PARAMETERS_H
#define CAEN_PARAMETERS_H

#include "midas.h"
#include "caen_device.h"
#include <inttypes.h>
#include <stdlib.h>
#include <map>
#include <memory>
#include <exception>

class CaenParameters {
public:
   CaenParameters() {}
   ~CaenParameters() {}

   void set_device(std::shared_ptr<CaenDevice> _dev) {
      dev = _dev;
   }

   void set_debug(bool _debug) {
      if (dev) {
         dev->set_debug(_debug);
      }

      debug = _debug;
   }

   std::string get_param_list_json();
   std::string get_param_list_human(std::string beneath="", bool all_channels=false, std::map<std::string, std::vector<std::string>> extra_children={}, bool only_params=false);



   // Firmware type - Scope or DPP_OPEN
   void get_firmware_type(std::string& fw_type);

   // Firwmare revision (CUP version)
   void get_firmware_version(std::string& fw_ver);

   // Model name, should be VX2740
   void get_model_name(std::string& fw_type);

   // Serial number of this module
   void get_serial_number(std::string& fw_type);

   // Signal(s) for starting the acquisition.
   void set_start_sources(bool sw, bool clkin, bool sin_level, bool sin_edge, bool lvds, bool first_trig, bool p0);
   void get_start_sources(bool& sw, bool& clkin, bool& sin_level, bool& sin_edge, bool& lvds, bool& first_trig, bool& p0);
   bool is_sw_start_enabled();

   // Set I/O mode to NIM or TTL.
   void set_nim();
   void set_ttl();
   void set_nim_ttl(bool is_nim_io);
   void get_nim_ttl(bool &is_nim_io);

   void set_use_test_data_source(bool use_test_data);

   // Which triggers are enabled.
   void set_trigger_sources(bool ch_over_thresh_A, bool ch_over_thresh_B, bool ch_over_thresh_A_and_B, bool external, bool software, bool user, bool test_pulse, bool lvds);
   void get_trigger_sources(bool& ch_over_thresh_A, bool& ch_over_thresh_B, bool& ch_over_thresh_A_and_B, bool &external, bool &software, bool &user, bool &test_pulse, bool &lvds);

   // What to put on trigger out
   void get_trigout_allowed_modes(std::vector<std::string>& modes);
   void set_trigout_mode(std::string mode);
   void get_trigout_mode(std::string& mode);

   // What to write in the trigger ID field
   void get_trigger_id_allowed_modes(std::vector<std::string>& modes);
   void set_trigger_id_mode(std::string mode);
   void get_trigger_id_mode(std::string& mode);

   // nsamp should be a multiple of 4!
   void set_waveform_length_samples(uint32_t nsamp);
   void get_waveform_length_samples(uint32_t &nsamp);

   // Which channels will get their waveforms read out.
   void set_channel_enable_mask(uint32_t mask_31_0, uint32_t mask_63_32);
   void get_channel_enable_mask(uint32_t &mask_31_0, uint32_t &mask_63_32);

   // Channel offsets.
   void set_enable_dc_offsets(bool enable);
   void get_enable_dc_offsets(bool &enable);
   void set_channel_dc_offset(int channel, float dc_offset_pct);
   void get_channel_dc_offset(int channel, float &dc_offset_pct);

   // Waveform pre-trigger.
   void set_pre_trigger_samples(uint16_t nsamp);
   void get_pre_trigger_samples(uint16_t &nsamp);

   // Thresholds for "channel over threshold" trigger.
   void set_channel_trigger_threshold(int channel, bool relative, int32_t threshold, bool rising_edge, uint32_t width_ns);
   void get_channel_trigger_threshold(int channel, bool &relative, int32_t &threshold, bool &rising_edge, uint32_t &width_ns);

   // Which channels can trigger in first "channel over threshold" trigger (ITLA).
   void set_channel_over_threshold_trigger_A_enable_mask(uint32_t multiplicity, uint32_t mask_31_0, uint32_t mask_63_32);
   void get_channel_over_threshold_trigger_A_enable_mask(uint32_t &multiplicity, uint32_t &mask_31_0, uint32_t &mask_63_32);

   // Which channels can trigger in second "channel over threshold" trigger (ITLB).
   void set_channel_over_threshold_trigger_B_enable_mask(uint32_t multiplicity, uint32_t mask_31_0, uint32_t mask_63_32);
   void get_channel_over_threshold_trigger_B_enable_mask(uint32_t &multiplicity, uint32_t &mask_31_0, uint32_t &mask_63_32);

   // How long to delay trigger signal.
   void set_num_trigger_delay_samples(uint32_t nsamp);
   void get_num_trigger_delay_samples(uint32_t &nsamp);

   // Whether to read sine waves or real data
   void set_enable_fake_sine_data(bool enable);
   void get_enable_fake_sine_data(bool &enable);

   // Whether to use internal or external clock
   void set_use_external_clock(bool external);
   void get_use_external_clock(bool &external);

   // Whether to enable clock out signal from front panel
   void set_enable_clock_out(bool enable);
   void get_enable_clock_out(bool &enable);

   // GPIO mode
   void set_gpio_mode(std::string mode);
   void get_gpio_mode(std::string &mode);

   // Busy signal source
   void set_busy_in_source(std::string source);
   void get_busy_in_source(std::string &source);

   // Sync output (on the 4-pin CLKOUT cable)
   void set_sync_out_mode(std::string source);
   void get_sync_out_mode(std::string &source);

   // Run start delay
   void set_run_start_delay_cycles(uint32_t delay);
   void get_run_start_delay_cycles(uint32_t &delay);

   // Whether to use internal or external clock
   void set_enable_trigger_overlap(bool enable);
   void get_enable_trigger_overlap(bool &enable);

   // When to veto
   void set_veto_params(std::string source, bool active_high, uint32_t width_ns);
   void get_veto_params(std::string &source, bool &active_high, uint32_t &width_ns);

   // Test pulse period
   void set_test_pulse(double period_ms, uint32_t width_ns, uint16_t low_level_adc, uint16_t high_level_adc);
   void get_test_pulse(double& period_ms, uint32_t& width_ns, uint16_t& low_level_adc, uint16_t& high_level_adc);

   // LVDS per-quartet parameters
   void set_lvds_quartet_params(int quartet, bool is_input, std::string mode);
   void get_lvds_quartet_params(int quartet, bool &is_input, std::string &mode);

   // LVDS trigger mask
   void set_lvds_trigger_mask(int line, uint32_t mask_31_0, uint32_t mask_63_32);
   void get_lvds_trigger_mask(int line, uint32_t &mask_31_0, uint32_t &mask_63_32);

   // LVDS IO register. Note that only quartets set to output with `set_lvds_quartet_params()`
   // have the value applied; the other quartets are read-only.
   void set_lvds_io_register(uint16_t val);
   void get_lvds_io_register(uint16_t& val);

   // VGA gain (2745 only, not 2740). Group is 0-3.
   void set_vga_gain(int group, float gain);
   void get_vga_gain(int group, float& gain);

   // Get the maximum number of bytes that a call to get_raw_data() will return.
   void get_max_raw_bytes_per_read(uint32_t& num_bytes);

   // Metadata.
   void get_acquisition_status(uint32_t &value);
   void get_temperatures(float &temp_air_in, float &temp_air_out, float &temp_hosttest_adc);

   // Get the current set of error flags, either as a bitmask or a human-readable string.
   void get_error_flags(uint32_t& mask);
   void get_error_flags(std::string& text);

   // Convert an error flag (or mask of multiple flags) into a human-readable string.
   std::string error_to_text(uint32_t mask);

   // Some parameters accept multiple options, separated by pipes (|).
   // This function creates a pipe-separated string from a vector of strings.
   std::string merge_string_options(std::vector<std::string> opts);
   std::vector<std::string> split_string_options(std::string opts);

   // Low-level functions that work for any parameter
   template<typename T> void set(std::string full_param_path, T val) = delete;
   template<typename T> void get(std::string full_param_path, T& val) = delete;

   template<typename T> void set_dig(std::string param_name, T val) {
      set(get_dig_path(param_name), val);
   }
   template<typename T> void get_dig(std::string param_name, T& val) {
      get(get_dig_path(param_name), val);
   }

   template<typename T> void set_chan(int chan, std::string param_name, T val) {
      set(get_chan_path(chan, param_name), val);
   }
   template<typename T> void get_chan(int chan, std::string param_name, T& val) {
      get(get_chan_path(chan, param_name), val);
   }

   template<typename T> void set_lvds(int quartet, std::string param_name, T val) {
      set(get_lvds_path(quartet, param_name), val);
   }
   template<typename T> void get_lvds(int quartet, std::string param_name, T& val) {
      get(get_lvds_path(quartet, param_name), val);
   }

   template<typename T> void set_vga(int group, std::string param_name, T val) {
      set(get_vga_path(group, param_name), val);
   }
   template<typename T> void get_vga(int group, std::string param_name, T& val) {
      get(get_vga_path(group, param_name), val);
   }

   std::string get_dig_path(std::string param_name);
   std::string get_chan_path(int chan, std::string param_name);
   std::string get_all_chan_path(std::string param_name);
   std::string get_lvds_path(int quartet, std::string param_name);
   std::string get_vga_path(int group, std::string param_name);

   void get_allowed_values(std::string full_param_path, std::vector<std::string> &val);

   // User registers
   void set_user_register(uint32_t reg, uint32_t val);
   template<typename T> void get_user_register(uint32_t reg, T& val) = delete;

   // Print out like "path = val\n".
   void print_param(std::string full_param_path);
   void print_user_register(uint32_t reg);

   // Simple helper function to convert a string to lower case, to allow easier
   // comparisons.
   static std::string str_to_lower(std::string str);

protected:
   bool validate_allowed_value(std::vector<std::string>& allowed, std::string requested, std::string param_name);
   std::vector<std::string> recurse_get_param_list_human(std::string base_path, bool all_channels, std::map<std::string, std::vector<std::string>> extra_children, bool only_params);
   std::shared_ptr<CaenDevice> dev = nullptr;
   bool debug = false;

   std::map<uint8_t, std::string> error_bit_to_text = {
      {0, "Power supply fail"},
      {1, "Initialization fail"},
      {2, "SI5341 PLL unlocked"},
      {3, "SI5395 PLL unlocked"},
      {4, "LMK04832 PLL unlocked"},
      {5, "JESD204B bus unlocked"},
      {6, "FPGA DDR4 Bank0 calibration fail"},
      {7, "FPGA DDR4 Bank1 calibration fail"},
      {8, "Processor DDR4 calibration fail"},
      {9, "FPGA calibration fail"},
      {10, "Board ID Card check fail"},
      {11, "ADC temperature out of range"},
      {12, "Air outlet temperature out of range"},
      {13, "FPGA temperature out of range"},
      {14, "FPGA power supply temperature out of range"},
      {15, "Clock fail"},
      {16, "ADC shutdown due to overheating"}
   };
};


template<> void CaenParameters::set<std::string>(std::string full_param_path, std::string val);
template<> void CaenParameters::set<const char*>(std::string full_param_path, const char* val);
template<> void CaenParameters::set<char*>(std::string full_param_path, char* val);
template<> void CaenParameters::set<bool>(std::string full_param_path, bool val);
template<> void CaenParameters::set<uint8_t>(std::string full_param_path, uint8_t val);
template<> void CaenParameters::set<uint16_t>(std::string full_param_path, uint16_t val);
template<> void CaenParameters::set<uint32_t>(std::string full_param_path, uint32_t val);
template<> void CaenParameters::set<uint64_t>(std::string full_param_path, uint64_t val);
template<> void CaenParameters::set<int32_t>(std::string full_param_path, int32_t val);
template<> void CaenParameters::set<int64_t>(std::string full_param_path, int64_t val);
template<> void CaenParameters::set<float>(std::string full_param_path, float val);
template<> void CaenParameters::set<double>(std::string full_param_path, double val);

template<> void CaenParameters::get<std::string>(std::string full_param_path, std::string &val);
template<> void CaenParameters::get<bool>(std::string full_param_path, bool &val);
template<> void CaenParameters::get<uint8_t>(std::string full_param_path, uint8_t &val);
template<> void CaenParameters::get<uint16_t>(std::string full_param_path, uint16_t &val);
template<> void CaenParameters::get<uint32_t>(std::string full_param_path, uint32_t &val);
template<> void CaenParameters::get<uint64_t>(std::string full_param_path, uint64_t &val);
template<> void CaenParameters::get<int32_t>(std::string full_param_path, int32_t &val);
template<> void CaenParameters::get<int64_t>(std::string full_param_path, int64_t &val);
template<> void CaenParameters::get<float>(std::string full_param_path, float &val);
template<> void CaenParameters::get<double>(std::string full_param_path, double &val);

template<> void CaenParameters::get_user_register<uint16_t>(uint32_t reg, uint16_t &val);
template<> void CaenParameters::get_user_register<int16_t>(uint32_t reg, int16_t &val);
template<> void CaenParameters::get_user_register<uint32_t>(uint32_t reg, uint32_t &val);

#endif// CAEN_PARAMETERS
