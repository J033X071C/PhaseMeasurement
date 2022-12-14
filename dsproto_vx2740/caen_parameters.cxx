#include <stdlib.h>
#include <inttypes.h>
#include <algorithm>
#include <stdio.h>
#include <arpa/inet.h>
#include "midas.h"
#include "CAEN_FELib.h"
#include "caen_parameters.h"
#include "caen_exceptions.h"
#include <cinttypes>
#include <cstring>
#include <sstream>

std::string CaenParameters::merge_string_options(std::vector<std::string> opts) {
   std::string retval = "";

   for (DWORD s = 0; s < opts.size(); s++) {
      if (s > 0) {
         retval += "|";
      }

      retval += opts[s];
   }

   return retval;
}

std::vector<std::string> CaenParameters::split_string_options(std::string opts) {
   std::vector<std::string> retval;
   char *c_opts = strdup(opts.c_str());
   char *token = strtok(c_opts, "|"); 

   while (token) { 
      if (strlen(token) > 0) {
         retval.push_back(token);
      } 
      
      token = strtok(NULL, "|"); 
   }

   free(c_opts);

   return retval;
}


std::string CaenParameters::get_param_list_json() {
   size_t size = 1024*1024*10;

   char* js = (char*)malloc(size);

   int wrote_size = CAEN_FELib_GetDeviceTree(dev->get_root_handle(), js, size);

   std::string retval(js);
   free(js);

   return retval;
}

std::vector<std::string> CaenParameters::recurse_get_param_list_human(std::string base_path, bool all_channels, std::map<std::string, std::vector<std::string>> extra_children, bool only_params) {
   std::vector<std::string> retval;

   uint64_t handles[1000];

   int num_children = CAEN_FELib_GetChildHandles(dev->get_root_handle(), base_path.c_str(), handles, 1000);

   if (num_children < 0) {
      char prefix[1024];
      snprintf(prefix, 1024, "Failed to get children of %s: ", base_path.c_str());
      dev->print_last_error(prefix);
   }

   std::vector<std::string> children_seen;

   for (int i = 0; i < num_children; i++) {
      char name[32];
      CAEN_FELib_NodeType_t type;
      int status = CAEN_FELib_GetNodeProperties(handles[i], "", name, &type);

      if (status != CAEN_FELib_Success) {
         char prefix[1024];
         snprintf(prefix, 1024, "Failed to get properties of child %d of %s", i, base_path.c_str());
         dev->print_last_error(prefix);
         continue;
      }

      children_seen.push_back(name);
      std::string path_str = base_path + "/" + name;
      std::string path_value = path_str;

      if (type == CAEN_FELib_PARAMETER || type == CAEN_FELib_FEATURE || type == CAEN_FELib_ATTRIBUTE) {
         char value[256];
         path_value += " = ";

         if (path_str == "/par/lvdstrgmask") {
            // Special logic for LVDS Trg Mask
            // Contains 16 entries; specify which entry to read by
            // pre-setting the value.
            for (int line = 0; line < 16; line++) {
               if  (line > 0) {
                  path_value += ", ";
               }

               sprintf(value, "%d", line);
               CAEN_FELib_GetValue(handles[i], "", value);

               path_value += value;
            }
         } else {
            status = CAEN_FELib_GetValue(handles[i], "", value);

            if (status == CAEN_FELib_Success) {
               path_value += value;
            } else {
               path_value += "** UNKNOWN ** (failed to read value!)";
            }
         }
      }

      if (!only_params || type == CAEN_FELib_PARAMETER) {
         retval.push_back(path_value);
      }

      if (all_channels || path_str.find("/ch/") == std::string::npos || path_str.find("/ch/0/") != std::string::npos || path_str == "/ch/0") {
         // Recurse down if not a channel-level key, or it's channel 0, or user asked for all channels
         std::vector<std::string> subkeys = recurse_get_param_list_human(path_str, all_channels, extra_children, only_params);

         for (auto it : subkeys) {
            retval.push_back(it);
         }
      }
   }

   // Handle any children that the calling code thinks should exist,
   // but which might not have been returned to us by GetChildHandles().
   for (auto child : extra_children[base_path]) {
      if (std::find(children_seen.begin(), children_seen.end(), child) == children_seen.end()) {
         std::string path_str = base_path + "/" + child;
         std::vector<std::string> subkeys = recurse_get_param_list_human(path_str, all_channels, extra_children, only_params);

         for (auto it : subkeys) {
            retval.push_back(it);
         }
      }
   }

   return retval;
}

std::string CaenParameters::get_param_list_human(std::string beneath, bool all_channels, std::map<std::string, std::vector<std::string>> extra_children, bool only_params) {
   std::vector<std::string> list = recurse_get_param_list_human(beneath, all_channels, extra_children, only_params);
   std::sort(list.begin(), list.end());

   std::string retval;

   for (auto it : list) {
      retval += it + "\n";
   }

   return retval;
}

template<> void CaenParameters::set(std::string full_param_path, std::string val) {
   if (debug) {
      printf("Setting %s to %s on %s\n", full_param_path.c_str(), val.c_str(), dev->get_name().c_str());
   }

   int ret = CAEN_FELib_SetValue(dev->get_root_handle(), full_param_path.c_str(), val.c_str());

   if (ret != CAEN_FELib_Success) {
      throw CaenException(dev->get_last_error("Failed to set " + full_param_path + " to " + val));
   }
}

template<> void CaenParameters::set(std::string full_param_path, const char* val) {
   set<std::string>(full_param_path, val);
}

template<> void CaenParameters::set(std::string full_param_path, char* val) {
   set<std::string>(full_param_path, val);
}

template<> void CaenParameters::get(std::string full_param_path, std::string& val) {
   if (debug) {
      printf("Reading %s from %s\n", full_param_path.c_str(), dev->get_name().c_str());
   }

   char cval[256] = {0};
   strncpy(cval, val.c_str(), 255);
   int ret = CAEN_FELib_GetValue(dev->get_root_handle(), full_param_path.c_str(), cval);

   if (ret == CAEN_FELib_Success) {
      val = cval;
   } else {
      val = "";
      throw CaenException(dev->get_last_error("Failed to get " + full_param_path));
   }
}

template<> void CaenParameters::set(std::string full_param_path, bool val) {
   set<std::string>(full_param_path, val ? "true" : "false");
}

template<> void CaenParameters::set(std::string full_param_path, uint8_t val) {
   set<uint64_t>(full_param_path, val);
}

template<> void CaenParameters::set(std::string full_param_path, uint16_t val) {
   set<uint64_t>(full_param_path, val);
}

template<> void CaenParameters::set(std::string full_param_path, uint32_t val) {
   set<uint64_t>(full_param_path, val);
}

template<> void CaenParameters::set(std::string full_param_path, uint64_t val) {
   char sval[256];
   snprintf(sval, 255, "%" PRIu64, val);
   set<std::string>(full_param_path, sval);
}

template<> void CaenParameters::set(std::string full_param_path, int32_t val) {
   set<int64_t>(full_param_path, val);
}

template<> void CaenParameters::set(std::string full_param_path, int64_t val) {
   char sval[256];
   snprintf(sval, 255, "%" PRId64, val);
   set<std::string>(full_param_path, sval);
}

template<> void CaenParameters::set(std::string full_param_path, float val) {
   char sval[256];
   snprintf(sval, 255, "%f", val);
   set<std::string>(full_param_path, sval);
}

template<> void CaenParameters::set(std::string full_param_path, double val) {
   char sval[256];
   snprintf(sval, 255, "%lf", val);
   set<std::string>(full_param_path, sval);
}

template<> void CaenParameters::get(std::string full_param_path, bool& val) {
   std::string sval;
   get(full_param_path, sval);

   if (sval == "true" || sval == "True") {
      val = 1;
   } else if (sval == "false" || sval == "False") {
      val = 0;
   } else {
      throw CaenSetParamException(full_param_path, "Read unexpected value for boolean");
   }
}

template<> void CaenParameters::get(std::string full_param_path, uint8_t& val) {
   uint64_t full_int = val;
   get(full_param_path, full_int);

   if (full_int > (1<<8)) {
      throw CaenSetParamException(full_param_path, "Read unexpectedly large value (>8bit)");
   } else {
      val = full_int;
   }
}

template<> void CaenParameters::get(std::string full_param_path, uint16_t& val) {
   uint64_t full_int = val;
   get(full_param_path, full_int);

   if (full_int > (1<<16)) {
      throw CaenSetParamException(full_param_path, "Read unexpectedly large value (>16bit)");
   } else {
      val = full_int;
   }
}

template<> void CaenParameters::get(std::string full_param_path, uint32_t& val) {
   uint64_t full_int = val;
   get(full_param_path, full_int);

   if (full_int > ((uint64_t)1<<32)) {
      throw CaenSetParamException(full_param_path, "Read unexpectedly large value (>32bit)");
   } else {
      val = full_int;
   }
}

template<> void CaenParameters::get(std::string full_param_path, uint64_t& val) {
   char cval[256] = {0};
   snprintf(cval, 255, "%" PRIu64, val);
   std::string sval = cval;
   
   val = 0;
   
   get(full_param_path, sval);

   int n = sscanf(sval.c_str(), "%" PRIu64, &val);

   if (n == 0) {
      throw CaenGetParamException(full_param_path, "Failed to parse value as an unsigned integer");
   }
}

template<> void CaenParameters::get(std::string full_param_path, int32_t& val) {
   int64_t full_int = val;
   get(full_param_path, full_int);

   if (full_int > ((int64_t)1<<32) || full_int < (int64_t)-1 * ((int64_t)1<<32)) {
      throw CaenGetParamException(full_param_path, "Read unexpectedly large value (> 32bit)");
   } else {
      val = full_int;
   }
}

template<> void CaenParameters::get(std::string full_param_path, int64_t& val) {
   char cval[256] = {0};
   snprintf(cval, 255, "%" PRId64, val);
   std::string sval = cval;
   
   val = 0;

   get(full_param_path, sval);
   
   int n = sscanf(sval.c_str(), "%" PRId64, &val);

   if (n == 0) {
      throw CaenGetParamException(full_param_path, "Failed to parse value as a signed integer");
   }
}

template<> void CaenParameters::get(std::string full_param_path, float& val) {
   char cval[256] = {0};
   snprintf(cval, 255, "%f", val);
   std::string sval = cval;
   
   val = 0;

   get(full_param_path, sval);

   int n = sscanf(sval.c_str(), "%f", &val);

   if (n == 0) {
      throw CaenGetParamException(full_param_path, "Failed to parse value as a float");
   }
}

template<> void CaenParameters::get(std::string full_param_path, double& val) {
   char cval[256] = {0};
   snprintf(cval, 255, "%lf", val);
   std::string sval = cval;
   
   val = 0;

   get(full_param_path, sval);

   int n = sscanf(sval.c_str(), "%lf", &val);

   if (n == 0) {
      throw CaenGetParamException(full_param_path, "Failed to parse value as a double");
   }
}

void CaenParameters::get_allowed_values(std::string full_param_path, std::vector<std::string> &allowed_values) {
   allowed_values.clear();
   full_param_path += "/allowedvalues";

   uint64_t handles[1000];

   int num_children = CAEN_FELib_GetChildHandles(dev->get_root_handle(), full_param_path.c_str(), handles, 1000);

   for (int i = 0; i < num_children; i++) {
      char value[256] = {0};
      CAEN_FELib_GetValue(handles[i], "", value);
      allowed_values.push_back(value);
   }
}

std::string CaenParameters::get_dig_path(std::string param_name) {
   return "/par/" + param_name;
}

std::string CaenParameters::get_chan_path(int chan, std::string param_name) {
   char param_path[256];
   snprintf(param_path, 255, "/ch/%d/par/%s", chan, param_name.c_str());
   return param_path;
}

std::string CaenParameters::get_lvds_path(int quartet, std::string param_name) {
   char param_path[256];
   snprintf(param_path, 255, "/lvds/%d/par/%s", quartet, param_name.c_str());
   return param_path;
}


std::string CaenParameters::get_vga_path(int group, std::string param_name) {
   char param_path[256];
   snprintf(param_path, 255, "/vga/%d/par/%s", group, param_name.c_str());
   return param_path;
}

std::string CaenParameters::get_all_chan_path(std::string param_name) {
   return "/ch/0..63/par/" + param_name;
}

void CaenParameters::set_user_register(uint32_t reg, uint32_t val) {
   if (debug) {
      printf("Setting user register 0x%x to %u on %s\n", reg, val, dev->get_name().c_str());
   }

   int ret = CAEN_FELib_SetUserRegister(dev->get_root_handle(), reg, val);

   if (ret != CAEN_FELib_Success) {
      char name[255];
      snprintf(name, 255, "User register 0x%x", reg);
      throw CaenSetParamException(name);
   }
}

template<> void CaenParameters::get_user_register<uint16_t>(uint32_t reg, uint16_t& val) {
   uint32_t full_int = 0;
   get_user_register<uint32_t>(reg, full_int);
   
   if (full_int > (1<<16)) {
      char name[255];
      snprintf(name, 255, "User register 0x%x", reg);
      throw CaenGetParamException(name, "Read unexpectedly large value (>16bit)");
   } else {
      val = full_int;
   }
}

template<> void CaenParameters::get_user_register<int16_t>(uint32_t reg, int16_t& val) {
   uint32_t full_int = 0;
   get_user_register<uint32_t>(reg, full_int);
   val = full_int;
}

template<> void CaenParameters::get_user_register<uint32_t>(uint32_t reg, uint32_t& val) {
   if (debug) {
      printf("Getting user register 0x%x from %s\n", reg, dev->get_name().c_str());
   }

   int ret = CAEN_FELib_GetUserRegister(dev->get_root_handle(), reg, &val);

   if (ret != CAEN_FELib_Success) {
      char name[255];
      snprintf(name, 255, "User register 0x%x", reg);
      throw CaenGetParamException(name);
   }
}

void CaenParameters::print_param(std::string full_param_path) {
   std::string val;
   get(full_param_path, val);
   printf("%s = %s\n", full_param_path.c_str(), val.c_str());
}

void CaenParameters::print_user_register(uint32_t reg) {
   uint32_t val;
   get_user_register(reg, val);
   printf("0x%x = %u (0x%x)\n", reg, val, val);
}


void CaenParameters::get_firmware_type(std::string& fw_type) {
   get_dig("FwType", fw_type);
}

void CaenParameters::get_firmware_version(std::string& fw_ver) {
   get_dig("cupver", fw_ver);
}

void CaenParameters::get_model_name(std::string& fw_type) {
   get_dig("ModelName", fw_type);
}

void CaenParameters::get_serial_number(std::string& fw_type) {
   get_dig("SerialNum", fw_type);
}

void CaenParameters::set_nim() {
   set_nim_ttl(true);
}

void CaenParameters::set_ttl() {
   set_nim_ttl(false);
}

void CaenParameters::set_nim_ttl(bool is_nim_io) {
   set_dig("IOlevel", std::string(is_nim_io ? "NIM": "TTL"));
}

void CaenParameters::get_nim_ttl(bool& is_nim_io) {
   std::string sval;
   get_dig("IOlevel", sval);

   if (sval == "NIM" || sval == "nim") {
      is_nim_io = 1;
   } else if (sval == "TTL" || sval == "ttl") {
      is_nim_io = 0;
   } else {
      throw CaenGetParamException(get_dig_path("IOlevel"), "Read unexpected value (not nim/ttl)");
   }
}

void CaenParameters::set_start_sources(bool sw, bool clkin, bool sin_level, bool sin_edge, bool lvds, bool first_trig, bool p0) {
   std::vector<std::string> sources;

   if (sw) {
      sources.push_back("SWcmd");
   }

   if (clkin) {
      sources.push_back("EncodedClkIn");
   }

   if (sin_level) {
      sources.push_back("SINlevel");
   }

   if (sin_edge) {
      sources.push_back("SINedge");
   }

   if (lvds) {
      sources.push_back("LVDS");
   }

   if (first_trig) {
      sources.push_back("FirstTrigger");
   }

   if (p0) {
      sources.push_back("P0");
   }

   return set_dig("StartSource", merge_string_options(sources));
}

void CaenParameters::get_start_sources(bool& sw, bool& clkin, bool& sin_level, bool& sin_edge, bool& lvds, bool& first_trig, bool& p0) {
   std::string sources;
   sw = 0;
   clkin = 0;
   sin_level = 0;
   sin_edge = 0;
   lvds = 0;
   first_trig = 0;
   p0 = 0;

   get_dig("StartSource", sources);

   if (sources.find("SWcmd") != std::string::npos) {
      sw = 1;
   }

   if (sources.find("EncodedClkIn") != std::string::npos) {
      clkin = 1;
   }

   if (sources.find("SINlevel") != std::string::npos) {
      sin_level = 1;
   }

   if (sources.find("SINedge") != std::string::npos) {
      sin_edge = 1;
   }

   if (sources.find("LVDS") != std::string::npos) {
      lvds = 1;
   }

   if (sources.find("FirstTrigger") != std::string::npos) {
      first_trig = 1;
   }

   if (sources.find("P0") != std::string::npos) {
      p0 = 1;
   }
}

bool CaenParameters::is_sw_start_enabled() {
   std::string sources;
   get_dig("StartSource", sources);
   return (sources.find("SWcmd") != std::string::npos);
}

void CaenParameters::set_waveform_length_samples(uint32_t nsamp) {
   set_dig("RecordLengthS", nsamp);
}

void CaenParameters::get_waveform_length_samples(uint32_t &nsamp) {
   get_dig("RecordLengthS", nsamp);
}

void CaenParameters::set_channel_enable_mask(uint32_t mask_31_0, uint32_t mask_63_32) {
   for (DWORD i = 0; i < 32; i++) {
      set_chan<bool>(i, "ChEnable", mask_31_0 & ((uint32_t)1<<i));
      set_chan<bool>(i + 32, "ChEnable", mask_63_32 & ((uint32_t)1<<i));
   }
}

void CaenParameters::get_channel_enable_mask(uint32_t &mask_31_00, uint32_t &mask_63_32) {
   mask_31_00 = 0;
   mask_63_32 = 0;

   for (DWORD i = 0; i < 32; i++) {
      bool bool_31_00, bool_63_32;
      get_chan(i, "ChEnable", bool_31_00);
      get_chan(i + 32, "ChEnable", bool_63_32);

      if (bool_31_00) {
         mask_31_00 |= ((uint32_t)1<<i);
      }

      if (bool_63_32) {
         mask_63_32 |= ((uint32_t)1<<i);
      }
   }
}

void CaenParameters::set_use_test_data_source(bool use_test_data) {
   for (DWORD i = 0; i < 64; i++) {
      set_chan(i, "WaveDataSource", std::string(use_test_data ? "ADC_TEST_SIN" : "ADC_DATA"));
   }
}

void CaenParameters::set_enable_dc_offsets(bool enable) {
   set_dig("EnOffsetCalibration", enable);
}

void CaenParameters::get_enable_dc_offsets(bool& enable) {
   // Assumes all channels have the same config. Just read from channel 0.
   get_dig("EnOffsetCalibration", enable);
}

void CaenParameters::set_channel_dc_offset(int channel, float dc_offset_pct) {
   set_chan(channel, "DcOffset", dc_offset_pct);
}

void CaenParameters::get_channel_dc_offset(int channel, float &dc_offset_pct) {
   get_chan(channel, "DcOffset", dc_offset_pct);
}

void CaenParameters::set_pre_trigger_samples(uint16_t nsamp) {
   set_dig("PreTriggerS", nsamp);
}

void CaenParameters::get_pre_trigger_samples(uint16_t &nsamp) {
   get_dig("PreTriggerS", nsamp);
}

void CaenParameters::set_channel_trigger_threshold(int channel, bool relative, int32_t threshold, bool rising_edge, uint32_t width_ns) {
   set_chan(channel, "TriggerThrMode", std::string(relative ? "Relative" : "Absolute"));
   set_chan(channel, "TriggerThr", threshold);
   set_chan(channel, "SelfTriggerEdge", std::string(rising_edge ? "RISE" : "FALL"));
   set_chan(channel, "SelfTriggerWidth", width_ns);
}

void CaenParameters::get_channel_trigger_threshold(int channel, bool& relative, int32_t &threshold, bool &rising_edge, uint32_t &width_ns) {
   get_chan(channel, "TriggerThr", threshold);
   get_chan(channel, "SelfTriggerWidth", width_ns);

   std::string sval;
   get_chan(channel, "TriggerThrMode", sval);

   if (sval == "Relative") {
      relative = 1;
   } else if (sval == "Absolute") {
      relative = 0;
   } else {
      throw CaenGetParamException(get_chan_path(channel, "TriggerThrMode"), "Read unexpected value (expect Relative/Absolute)");
   }

   get_chan(channel, "SelfTriggerEdge", sval);

   if (sval == "RISE") {
      rising_edge = true;
   } else if (sval == "FALL") {
      rising_edge = false;
   } else {
      throw CaenGetParamException(get_chan_path(channel, "SelfTriggerEdge"), "Read unexpected value (expect RISE/FALL)");
   }
}

void CaenParameters::set_trigger_sources(bool ch_over_thresh_A, bool ch_over_thresh_B, bool ch_over_thresh_A_and_B, bool external, bool software, bool user, bool test_pulse, bool lvds) {
   std::vector<std::string> sources;

   if (ch_over_thresh_A) {
      sources.push_back("ITLA");
   }

   if (ch_over_thresh_B) {
      sources.push_back("ITLB");
   }

   if (ch_over_thresh_A_and_B) {
      sources.push_back("ITLA_AND_ITLB");
   }

   if (external) {
      sources.push_back("TrgIn");
   }

   if (software) {
      sources.push_back("SwTrg");
   }

   if (test_pulse) {
      sources.push_back("TestPulse");
   }

   if (lvds) {
      sources.push_back("LVDS");
   }

   if (user) {
      sources.push_back("UserTrg");
   }

   set_dig("AcqTriggerSource", merge_string_options(sources));
}

void CaenParameters::get_trigger_sources(bool &ch_over_thresh_A, bool &ch_over_thresh_B, bool &ch_over_thresh_A_and_B, bool &external, bool &software, bool &user, bool &test_pulse, bool &lvds) {
   std::string source_merged;
   ch_over_thresh_A = 0;
   ch_over_thresh_B = 0;
   ch_over_thresh_A_and_B = 0;
   external = 0;
   software = 0;
   user = 0;
   lvds = 0;
   test_pulse = 0;

   get_dig("AcqTriggerSource", source_merged);

   std::vector<std::string> sources = split_string_options(source_merged);

   for (auto source : sources) {
      if (source == "ITLA") {
         ch_over_thresh_A = 1;
      } else if (source == "ITLB") {
         ch_over_thresh_B = 1;
      } else if (source == "ITLA_AND_ITLB") {
         ch_over_thresh_A_and_B = 1;
      } else if (source == "TrgIn") {
         external = 1;
      } else if (source == "SwTrg") {
         software = 1;
      } else if (source == "UserTrg") {
         user = 1;
      } else if (source == "LVDS") {
         lvds = 1;
      } else if (source == "TestPulse") {
         test_pulse = 1;
      } else {
         throw CaenGetParamException(get_dig_path("AcqTriggerSource"), "Read unexpected trigger source");
      }
   }
}

void CaenParameters::get_trigout_allowed_modes(std::vector<std::string>& modes) {
   get_allowed_values(get_dig_path("TrgOutMode"), modes);
}

void CaenParameters::set_trigout_mode(std::string mode) {
   std::vector<std::string> allowed;
   get_trigout_allowed_modes(allowed);

   if (!validate_allowed_value(allowed, mode, "TrgOutMode")) {
      throw CaenSetParamException(get_dig_path("TrgOutMode"), "Invalid value provided");
   }

   set_dig("TrgOutMode", mode);
}

void CaenParameters::get_trigout_mode(std::string& mode) {
   get_dig("TrgOutMode", mode);
}

void CaenParameters::get_trigger_id_allowed_modes(std::vector<std::string>& modes) {
   get_allowed_values(get_dig_path("TriggerIDMode"), modes);
}

void CaenParameters::set_trigger_id_mode(std::string mode) {
   std::vector<std::string> allowed;
   get_trigger_id_allowed_modes(allowed);

   if (!validate_allowed_value(allowed, mode, "TriggerIDMode")) {
      throw CaenSetParamException(get_dig_path("TriggerIDMode"), "Invalid value provided");
   }

   set_dig("TriggerIDMode", mode);
}

void CaenParameters::get_trigger_id_mode(std::string& mode) {
   get_dig("TriggerIDMode", mode);
}

void CaenParameters::set_channel_over_threshold_trigger_A_enable_mask(uint32_t multiplicity, uint32_t mask_0_31, uint32_t mask_32_63) {
   std::string main_logic = multiplicity <= 1 ? "OR" : "Majority";
   uint64_t mask = (uint64_t)mask_0_31 | (((uint64_t)mask_32_63) << 32);
   
   set_dig("ITLAMainLogic", main_logic);
   set_dig("ITLAPairLogic", std::string("NONE"));
   set_dig("ITLAMajorityLev", multiplicity);
   set_dig("ITLAMask", mask);
}

void CaenParameters::get_channel_over_threshold_trigger_A_enable_mask(uint32_t &multiplicity, uint32_t &mask_31_0, uint32_t &mask_63_32) {
   get_dig("ITLAMajorityLev", multiplicity);

   uint64_t mask = 0;
   get_dig("ITLAMask", mask);

   mask_31_0 = mask;
   mask_63_32 = mask >> 32;
}

void CaenParameters::set_channel_over_threshold_trigger_B_enable_mask(uint32_t multiplicity, uint32_t mask_0_31, uint32_t mask_32_63) {
   std::string main_logic = multiplicity <= 1 ? "OR" : "Majority";
   uint64_t mask = (uint64_t)mask_0_31 | (((uint64_t)mask_32_63) << 32);
   
   set_dig("ITLBMainLogic", main_logic);
   set_dig("ITLBPairLogic", std::string("NONE"));
   set_dig("ITLBMajorityLev", multiplicity);
   set_dig("ITLBMask", mask);
}

void CaenParameters::get_channel_over_threshold_trigger_B_enable_mask(uint32_t &multiplicity, uint32_t &mask_31_0, uint32_t &mask_63_32) {
   get_dig("ITLBMajorityLev", multiplicity);

   uint64_t mask;
   get_dig("ITLBMask", mask);

   mask_31_0 = mask;
   mask_63_32 = mask >> 32;
}

void CaenParameters::set_num_trigger_delay_samples(uint32_t nsamp) {
   set_dig("TriggerDelayS", nsamp);
}

void CaenParameters::get_num_trigger_delay_samples(uint32_t &nsamp) {
   get_dig("TriggerDelayS", nsamp);
}

void CaenParameters::set_enable_fake_sine_data(bool enable) {
   std::string data = enable ? "ADC_TEST_SIN" : "ADC_DATA";

   for (int i = 0; i < 64; i++) {
      set_chan(i, "WaveDataSource", data);
   }
}

void CaenParameters::get_enable_fake_sine_data(bool &enable) {
   std::string data;

   for (int i = 0; i < 64; i++) {
      get_chan(i, "WaveDataSource", data);
      bool this_enable = str_to_lower(data) == "adc_test_sin";

      if (i > 0 && this_enable != enable) {
         std::stringstream s;
         s << "Mismatched WaveDataSource for channel" << i;
         throw CaenGetParamException("WaveDataSource", s.str());
      }

      enable = this_enable;
   }
}

void CaenParameters::set_use_external_clock(bool external) {
   std::string data = external ? "FPClkIn" : "Internal";
   set_dig("ClockSource", data);
}

void CaenParameters::get_use_external_clock(bool &external) {
   std::string data;
   get_dig("ClockSource", data);
   external = str_to_lower(data) == "fpclkin";
}

void CaenParameters::set_enable_clock_out(bool enable) {
   set_dig("EnClockOutFP", enable);
}

void CaenParameters::get_enable_clock_out(bool &enable) {
   get_dig("EnClockOutFP", enable);
}

void CaenParameters::set_gpio_mode(std::string mode) {
   set_dig("GPIOMode", mode);
}

void CaenParameters::get_gpio_mode(std::string &mode) {
   get_dig("GPIOMode", mode);
}

void CaenParameters::set_busy_in_source(std::string source) {
   set_dig("BusyInSource", source);
}

void CaenParameters::get_busy_in_source(std::string &source) {
   get_dig("BusyInSource", source);
}

void CaenParameters::set_sync_out_mode(std::string source) {
   set_dig("SyncOutMode", source);
}

void CaenParameters::get_sync_out_mode(std::string &source) {
   get_dig("SyncOutMode", source);
}

void CaenParameters::set_run_start_delay_cycles(uint32_t delay) {
   set_dig("RunDelay", delay);
}

void CaenParameters::get_run_start_delay_cycles(uint32_t &delay) {
   get_dig("RunDelay", delay);
}

void CaenParameters::set_enable_trigger_overlap(bool enable) {
   set_dig("EnTriggerOverlap", enable);
}

void CaenParameters::get_enable_trigger_overlap(bool &enable) {
   get_dig("EnTriggerOverlap", enable);
}

void CaenParameters::set_veto_params(std::string source, bool active_high, uint32_t width_ns) {
   set_dig("VetoSource", source);
   set_dig("VetoWidth", width_ns);
   set_dig("VetoPolarity", std::string(active_high ? "ActiveHigh" : "ActiveLow"));
}

void CaenParameters::get_veto_params(std::string &source, bool &active_high, uint32_t &width_ns) {
   get_dig("VetoSource", source);
   get_dig("VetoWidth", width_ns);

   std::string pol;
   get_dig("VetoPolarity", pol);

   active_high = (str_to_lower(pol) == "activehigh");
}

void CaenParameters::set_test_pulse(double period_ms, uint32_t width_ns, uint16_t low_level_adc, uint16_t high_level_adc) {
   // Board accepts ns, not ms for period
   set_dig("TestPulsePeriod", period_ms * 1e6);
   set_dig("TestPulseWidth", width_ns);
   set_dig("TestPulseLowLevel", low_level_adc);
   set_dig("TestPulseHighLevel", high_level_adc);
}

void CaenParameters::get_test_pulse(double& period_ms, uint32_t& width_ns, uint16_t& low_level_adc, uint16_t& high_level_adc) {
   uint32_t period_ns;
   get_dig("TestPulsePeriod", period_ns);
   period_ms = (double)period_ns / 1e6;
   
   get_dig("TestPulseWidth", width_ns);
   get_dig("TestPulseLowLevel", low_level_adc);
   get_dig("TestPulseHighLevel", high_level_adc);
}

void CaenParameters::set_lvds_quartet_params(int quartet, bool is_input, std::string mode) {
   set_lvds(quartet, "LVDSDirection", std::string(is_input ? "Input" : "Output"));
   set_lvds(quartet, "LVDSMode", mode);
}

void CaenParameters::get_lvds_quartet_params(int quartet, bool &is_input, std::string &mode) {
   std::string direction;
   get_lvds(quartet, "LVDSDirection", direction);
   is_input = str_to_lower(direction) == "input";

   get_lvds(quartet, "LVDSMode", mode);
}

void CaenParameters::set_lvds_trigger_mask(int line, uint32_t mask_31_0, uint32_t mask_63_32) {
   uint64_t mask = (uint64_t)mask_31_0 | (((uint64_t)mask_63_32) << 32);

   // Special syntax for LVDSTrgMask
   // Format is <line>=<value>
   char sval[256];
   snprintf(sval, 255, "%d=%" PRIu64, line, mask);
   set_dig("LVDSTrgMask", std::string(sval));
}

void CaenParameters::get_lvds_trigger_mask(int line, uint32_t &mask_31_0, uint32_t &mask_63_32) {
   uint64_t mask = line;
   get_dig("LVDSTrgMask", mask);
   mask_31_0 = mask & 0xFFFFFFFF;
   mask_63_32 = mask >> 32;
}

void CaenParameters::set_lvds_io_register(uint16_t val) {
   set_dig("LVDSIOReg", val);
}

void CaenParameters::get_lvds_io_register(uint16_t& val) {
   get_dig("LVDSIOReg", val);
}

void CaenParameters::set_vga_gain(int group, float gain) {
   set_vga(group, "VGAGain", gain);
}

void CaenParameters::get_vga_gain(int group, float& gain) {
   get_vga(group, "VGAGain", gain);
}

void CaenParameters::get_acquisition_status(uint32_t &value) {
   get_dig("AcquisitionStatus", value);
}

void CaenParameters::get_temperatures(float &temp_air_in, float &temp_air_out, float &temp_hosttest_adc) {
   get_dig("TempSensAirIn", temp_air_in);
   get_dig("TempSensAirOut", temp_air_out);
   get_dig("TempSensHottestADC", temp_hosttest_adc);
}

void CaenParameters::get_max_raw_bytes_per_read(uint32_t& num_bytes) {
   get_dig("MaxRawDataSize", num_bytes);
}

bool CaenParameters::validate_allowed_value(std::vector<std::string>& allowed, std::string requested, std::string param_name) {
   bool ok = false;

   for (auto it : allowed) {
      if (str_to_lower(it) == str_to_lower(requested)) {
         ok = true;
         break;
      }
   }

   if (!ok) {
      printf("Unexpected value for %s: %s. Not one of: ", param_name.c_str(), requested.c_str());
      for (auto it : allowed) {
         printf("%s, ", it.c_str());
      }
      printf("\n");
   }

   return ok;
}

void CaenParameters::get_error_flags(uint32_t& mask) {
   get_dig("ErrorFlags", mask);
}

void CaenParameters::get_error_flags(std::string& text) {
   uint32_t mask;
   get_error_flags(mask);
   text = error_to_text(mask);
}

std::string CaenParameters::error_to_text(uint32_t mask) {
   std::string retval = "";

   for (auto& bits : error_bit_to_text) {
      uint32_t bit_num = bits.first;
      if (mask & (1<<bit_num)) {
         retval += bits.second + ". ";
      }
   }

   return retval;
}

std::string CaenParameters::str_to_lower(std::string str) {
   for (auto i = 0; i < str.size(); i++) {
      str[i] = tolower(str[i]);
   }

   return str;
}