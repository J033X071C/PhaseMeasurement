#ifndef VX2740_FE_CLASS_H
#define VX2740_FE_CLASS_H

#include "midas.h"
#include "odbxx.h"
#include "vx2740_wrapper.h"
#include "fe_settings.h"
#include "fe_settings_strategy.h"
#include <map>
#include <cmath>
#include <mutex>
#include <sstream>
#include <thread>
#include <stdexcept>

class VX2740GroupFrontend {
public:
   VX2740GroupFrontend(std::shared_ptr<VX2740FeSettingsStrategyBase> _strategy, bool _use_single_fe_mode, bool _enable_data_readout=true);
   
   virtual ~VX2740GroupFrontend();

   virtual INT init(int group_idx, HNDLE hDB=0, bool enable_jrpc=true);

   INT begin_of_run(INT run_num, char* error);
   INT end_of_run(INT run_num, char* error);

   bool is_event_ready();
   int write_data(char* pevent);
   int write_metadata(char* pevent);
   int check_errors(char* pevent);

   void *thread_data_readout(int board_id);
   INT jrpc_handler(int index, void** params);

   // Getters for vertical slice system that uses this class to
   // configure the boards.
   int get_num_boards();
   bool is_scope_mode(int board_id);
   bool is_open_fw(int board_id);
   std::map<int, VX2740*> get_boards();
   std::map<int, std::string> get_board_names();

   bool should_read_from_board(int board_id);


protected:
   INT setup_ring_buffers();
   virtual INT connect_to_boards(char* error);
   virtual INT validate_firmare_version(int board_id, char* error);

   INT configure_board(int board_id);
   INT arm_board(int board_id);
   INT read_into_rb(int board_id, DWORD read_timeout_ms, uint16_t* tmp_waveform);

   INT force_write_settings(char* error);

   int peek_rb_event_id(int board_id);


   // Empty a midas ring buffer, so the write pointer and read pointer
   // are in the same place.
   INT empty_ring_buffer(int rb_handle, int max_event_size_bytes);

   VX2740FeSettings settings;

   bool enable_data_readout;
   bool single_fe_mode;

   int this_group_index = -1;

   int event_id_to_write = -1;
   bool in_end_of_run = false;
   bool warned_corruption = false;

   bool ready_to_arm_acq = false;
   bool abort_arming = false;

   std::map<int, VX2740*> boards;
   std::map<int, std::string> board_names;
   std::map<int, std::thread*> readout_threads;
   std::map<int, INT> readout_status;
   std::map<int, int> readout_rbs;
   std::map<int, DWORD> max_bytes_per_read;
   std::map<int, std::mutex> vx_mutexes;
   std::map<int, bool> scope_mode;
   std::map<int, bool> open_fw;
};


#endif
