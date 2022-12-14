#ifndef VX_SETTINGS_STRUCTS_H
#define VX_SETTINGS_STRUCTS_H

#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <inttypes.h>

typedef struct GroupSettings {
   int32_t num_boards = 1;
   bool merge_data_using_event_id = false;
   bool debug_data = false;
   bool debug_rates = false;
   bool debug_settings = false;
   bool debug_ring_buffers = false;
   bool multithreaded_readout = true;
} GroupSettings;

typedef struct BoardErrors {
   uint32_t bitmask = 0;
   std::string message;
} BoardErrors;

// To add a new board-level setting (of an existing type):
// * Add it the BoardSettings struct
// * Implement writing it to the board in VX2740FeSettings::write_settings_to_board()
// * Add it to the webpage in custom/vx2740.js
//
// To add a new *type* of board-level setting:
// * Add a map to the BoardSettings struct
// * Implement default ODB in VX2740FeSettingsODB::setup_default_odb()
// * Implement ODB->Struct in VX2740FeSettingsODB::fill_board_settings_struct()
// * Implement Struct->ODB in VX2740FeSettingsODB::handle_board_readback_struct()
typedef struct BoardSettings {
   std::map<std::string, std::string> strings = {
      {"Hostname (restart on change)", ""},
      {"Trigger ID mode", "TriggerCnt"},
      {"Trigger out mode", "TRGIN"},
      {"GPIO mode", "Disabled"},
      {"Sync out mode", "Disabled"},
      {"Busy in source", "Disabled"},
      {"Veto source", "Disabled"}
   };

   std::map<std::string, bool> bools = {
      {"Enable", true},
      {"Read data", true},
      {"Scope mode (restart on change)", true},
      {"Use NIM IO", true},
      {"Start acq on midas run start", true},
      {"Start acq on encoded CLKIN", false},
      {"Start acq on SIN level", false},
      {"Start acq on SIN edge", false},
      {"Start acq on first trigger", false},
      {"Start acq on P0", false},
      {"Start acq on LVDS", false},
      {"Trigger on ch over thresh A", false},
      {"Trigger on ch over thresh B", false},
      {"Trigger on ch over thresh A&&B", false},
      {"Trigger on external signal", true},
      {"Trigger on software signal", true},
      {"Trigger on user mode signal", false},
      {"Trigger on test pulse", false},
      {"Trigger on LVDS", false},
      {"Allow trigger overlap", false},
      {"Enable DC offsets", true},
      {"Use external clock", false},
      {"Enable clock out", false},
      {"Veto when source is high", true},
      {"Use relative trig thresholds", false},
      {"Read fake sinewave data", false},
      {"User registers/Expert mode for trig settings", false},
      {"User registers/Enable LVDS loopback", false},
      {"User registers/Only read triggering channel", true},
      {"User registers/Trigger on falling edge", true},
      {"User registers/Upper 32 mirror raw of lower 32", true},
      {"User registers/Enable LVDS pair 12 trigger", false}
   };

   std::map<std::string, uint32_t> uint32s = {
      {"Read data timeout (ms)", 100},
      {"Readout channel mask (31-0)", 0xFFFFFFFF},
      {"Readout channel mask (63-32)", 0xFFFFFFFF},
      {"Waveform length (samples)", 1000},
      {"Trigger delay (samples)", 0},
      {"Ch over thresh A multiplicity", 1},
      {"Ch over thresh A en mask(31-0)", 0xFFFFFFFF},
      {"Ch over thresh A en mask(63-32)", 0xFFFFFFFF},
      {"Ch over thresh B multiplicity", 1},
      {"Ch over thresh B en mask(31-0)", 0xFFFFFFFF},
      {"Ch over thresh B en mask(63-32)", 0xFFFFFFFF},
      {"Veto width (ns) (0=source len)", 0},
      {"Test pulse width (ns)", 104},
      {"Run start delay (cycles)", 0},
      {"User registers/Darkside trigger en mask(31-0)", 0},
      {"User registers/Darkside trigger en mask(63-32)", 0},
      {"User registers/Trigger on threshold (31-0)", 0},
      {"User registers/Trigger on threshold (63-32)", 0},
      {"User registers/Trigger on external (31-0)", 0},
      {"User registers/Trigger on external (63-32)", 0},
      {"User registers/Trigger on internal (31-0)", 0},
      {"User registers/Trigger on internal (63-32)", 0},
      {"User registers/Trigger on global (31-0)", 0},
      {"User registers/Trigger on global (63-32)", 0},
      {"User registers/Enable FIR filter (31-0)", 0},
      {"User registers/Enable FIR filter (63-32)", 0},
      {"User registers/Write unfiltered data (31-0)", 0xFFFFFFFF},
      {"User registers/Write unfiltered data (63-32)", 0xFFFFFFFF}
   };

   std::map<std::string, uint16_t> uint16s = {
      {"Pre-trigger (samples)", 100},
      {"LVDS IO register", 0},
      {"Test pulse low level (ADC)", 0},
      {"Test pulse high level (ADC)", 1000},
      {"User registers/LVDS output", 0}
   };

   std::map<std::string, double> doubles = {
      {"Test pulse period (ms)", 100}
   };

   std::map<std::string, int32_t> int32s = {
   };

   std::map<std::string, std::vector<bool>> vec_bools = {
      {"LVDS quartet is input", std::vector<bool>(4, false)},
      {"Chan over thresh rising edge", std::vector<bool>(64, false)}
   };

   std::map<std::string, std::vector<std::string>> vec_strings = {
      {"LVDS quartet mode", std::vector<std::string>(4, "SelfTriggers")}
   };

   std::map<std::string, std::vector<int16_t>> vec_int16s = {
      {"User registers/FIR filter coefficients", std::vector<int16_t>(48, 1)}
   };

   std::map<std::string, std::vector<uint16_t>> vec_uint16s = {
      {"User registers/Waveform length (samples)", std::vector<uint16_t>(64, 1000)},
      {"User registers/Pre-trigger (samples)", std::vector<uint16_t>(64, 100)},
      {"User registers/Darkside trigger threshold", std::vector<uint16_t>(64, 32000)},
      {"User registers/Qshort length (samples)", std::vector<uint16_t>(64, 16)},
      {"User registers/Qlong length (samples)", std::vector<uint16_t>(64, 32)}
   };

   std::map<std::string, std::vector<uint32_t>> vec_uint32s = {
      {"LVDS trigger mask (31-0)", std::vector<uint32_t>(16, 0xFFFFFFFF)},
      {"LVDS trigger mask (63-32)", std::vector<uint32_t>(16, 0xFFFFFFFF)},
      {"Chan over thresh width (ns)", std::vector<uint32_t>(64, 0)}
   };

   std::map<std::string, std::vector<int32_t>> vec_int32s = {
      {"Chan over thresh thresholds", std::vector<int32_t>(64, 32768)}
   };

   std::map<std::string, std::vector<float>> vec_floats = {
      {"DC offset (pct)", std::vector<float>(64, 50)},
      {"VGA gain", std::vector<float>(4, 2.5)}
   };
} BoardSettings;

typedef struct BoardReadback : public BoardSettings {
   BoardReadback() : BoardSettings() {
      strings["Firmware version"] = "???";
      strings["Model name"] = "???";
      uint32s["User FW revision"] = 0;
      uint32s["User register revision"] = 0;
      bools["Upper 32 mirror raw of lower 32"] = false;
      uint32s["User registers/LVDS input"] = 0;
      vec_uint32s["User registers/FIR gain and discard"] = std::vector<uint32_t>(64, 0);
      uint32s["User registers/Darkside trigger en mask(31-0)"] = 0;
      uint32s["User registers/Darkside trigger en mask(63-32)"] = 0;
      vec_uint32s["User registers/Channel trigger sources"] = std::vector<uint32_t>(64, 0);
      vec_uint32s["User registers/Test signal"] = std::vector<uint32_t>(64, 0);
   }
} BoardReadback;

#endif