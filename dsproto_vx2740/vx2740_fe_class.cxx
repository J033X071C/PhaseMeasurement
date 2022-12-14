#include "vx2740_fe_class.h"
#include "caen_event.h"
#include "caen_exceptions.h"
#include "fe_utils.h"
#include "fe_settings.h"
#include "odbxx.h"
#include "midas.h"
#include "msystem.h"
#include <inttypes.h>
#include <algorithm>
#include <numeric>
#include <sys/time.h>
#include <sys/resource.h>
#include <thread>
#include <tuple>
#include <chrono>

#ifdef __linux__
#include <sys/sysinfo.h>
#endif

#define BUFFER_SIZE 1000000000 // 1000MB (board has 2GB total)
#define MAX_EV_SIZE 320000000  // 320MB

#define THREAD_STATUS_ERROR -1
#define THREAD_STATUS_CONFIGURING 1
#define THREAD_STATUS_CONFIGURED 2
#define THREAD_STATUS_ARMED 3

#define MIN_USER_MODE_FW 2022102602

#define MAIN_THREAD_CPU_ID 0
#define MAIN_THREAD_PRIORITY 40
#define READOUT_THREAD_PRIORITY 40

typedef struct {
   VX2740GroupFrontend *obj;
   int board_index;
} BoardThreadArgs;

// We need an object that lives a long time to back the arguments passed to
// thread spawn functions.
std::map<int, BoardThreadArgs> thread_args;
VX2740GroupFrontend *gobj;

void set_thread_cpu_and_priority(int cpu_id, int priority, std::string thread_name) {
#ifdef __linux__
   // cpu_set_t etc not available on MacOS, but developers sometimes want to compile on that platform.
   int num_cpus = get_nprocs();

   if (cpu_id >= num_cpus) {
      cm_msg(MINFO, "Unable to set %s thread to CPU %d, as this machine only has %d CPUs", thread_name.c_str(), cpu_id, num_cpus);
   } else {
      cpu_set_t cpuset;
      CPU_ZERO(&cpuset);
      CPU_SET(cpu_id, &cpuset);

      fe_utils::ts_printf("Assigning %s thread to CPU %d\n", thread_name.c_str(), cpu_id);
      pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset);
   }
#endif

   sched_param param = {};
   param.sched_priority = priority;
   fe_utils::ts_printf("Setting %s thread to realtime priority %d\n", thread_name.c_str(), priority);
   pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
}

void *thread_data_readout_helper(void *arg) {
   BoardThreadArgs *arg_cast = (BoardThreadArgs *) arg;
   VX2740GroupFrontend *obj = arg_cast->obj;

   int board_idx = arg_cast->board_index;
   int cpu_id = MAIN_THREAD_CPU_ID + 1 + board_idx;
   std::stringstream thread_name;
   thread_name << "board " << board_idx << " readout";
   set_thread_cpu_and_priority(cpu_id, READOUT_THREAD_PRIORITY, thread_name.str());

   return obj->thread_data_readout(arg_cast->board_index);
}

INT jrpc_helper(INT index, void** params) {
   return gobj->jrpc_handler(index, params);
}

VX2740GroupFrontend::VX2740GroupFrontend(std::shared_ptr<VX2740FeSettingsStrategyBase> _strategy, bool _use_single_fe_mode, bool _enable_data_readout) :
   settings(VX2740FeSettings(_strategy)), single_fe_mode(_use_single_fe_mode), enable_data_readout(_enable_data_readout) {}

VX2740GroupFrontend::~VX2740GroupFrontend() {
   for (auto it : boards) {
      delete it.second;
   }
}

INT VX2740GroupFrontend::init(int group_idx, HNDLE hDB, bool enable_jrpc) {
   gobj = this; // Needed so global-level RPC callback can use this class
   this_group_index = group_idx;

   printf("\n");
   fe_utils::ts_printf("Initialising settings\n");

   try {
      settings.init_strategy(single_fe_mode, this_group_index, hDB);
   } catch (SettingsException& e) {
      cm_msg(MERROR, __FUNCTION__, "Failure initialising settings: %s", e.what());   
      return FE_ERR_ODB;
   }

   fe_utils::ts_printf("Reading settings\n");

   try {
      settings.sync_settings_structs();
   } catch (SettingsException& e) {
      cm_msg(MERROR, __FUNCTION__, "Failure reading settings: %s", e.what());   
      return FE_ERR_ODB;
   }

   if (setup_ring_buffers() != SUCCESS) {
      return FE_ERR_DRIVER;
   }

   char error[255] = {};
   if (connect_to_boards(error) != SUCCESS) {
      return FE_ERR_DRIVER;
   }

   fe_utils::ts_printf("Writing settings to boards (may take a few seconds)\n");

   if (force_write_settings(error) != SUCCESS) {
      return FE_ERR_DRIVER;
   }

   if (enable_jrpc) {
      if (cm_register_function(RPC_JRPC, &jrpc_helper) != SUCCESS) {
         return FE_ERR_DRIVER;
      }
   }

   fe_utils::ts_printf("Frontend init complete\n");
   return SUCCESS;
}

INT VX2740GroupFrontend::setup_ring_buffers() {
   if (!enable_data_readout) {
      return SUCCESS;
   }

   rb_set_nonblocking();

   for (int i = 0; i < settings.get_num_boards(); i++) {
      INT status = rb_create(BUFFER_SIZE, MAX_EV_SIZE, &(readout_rbs[i]));

      if (status != SUCCESS) {
         cm_msg(MERROR, __FUNCTION__, "Failed to create ring buffer for %s", board_names[i].c_str());
         return status;
      }
   }

   return SUCCESS;
}

INT VX2740GroupFrontend::validate_firmare_version(int board_id, char* error) {
   INT status = SUCCESS;

   // Store the firmware version and model name (VX2740/VX2745) in the board's Readback section.
   std::string fw_ver, model_name;
   boards[board_id]->params().get_firmware_version(fw_ver);
   boards[board_id]->params().get_model_name(model_name);

   settings.set_board_firmware_info(board_id, fw_ver, model_name);

   std::string fw_type;
   boards[board_id]->params().get_firmware_type(fw_type);

   std::string hostname = settings.get_hostname(board_id);

   if (fw_type == "Scope") {
      if (!scope_mode[board_id]) {
         snprintf(error, 255, "Board %s is running firmware %s, but FE is configured for non-scope mode. Edit the 'Scope mode (restart on change)' param and restart", hostname.c_str(), fw_type.c_str());
         cm_msg(MERROR, __FUNCTION__, "%s", error);
         return FE_ERR_ODB;
      }

      // Feature only relevant for user mode
      settings.set_board_user_firmware_info(board_id, 0, 0, false);
   } else if (fw_type == "DPP_OPEN") {
      open_fw[board_id] = true;
      uint32_t fw_rev = 0;
      uint32_t reg_rev = 0;
      boards[board_id]->params().get_user_register(0x0, fw_rev);
      boards[board_id]->params().get_user_register(0x4, reg_rev);

      // Can cause board to hang if we try to access user registers
      // that aren't defined. Make sure we're running a FW version
      // we know to be compatible.
      uint64_t min_ver = MIN_USER_MODE_FW;
      uint64_t our_ver = 0;
      sscanf(fw_ver.c_str(), "%" PRIu64, &our_ver);
      if (our_ver < min_ver) {
         cm_msg(MERROR, __FUNCTION__, "Frontend code only supports user FW versions >= %" PRIu64 ". Board %d in group %d is running version %s", min_ver, board_id, this_group_index, fw_ver.c_str());
         return FE_ERR_HW;
      }

      // Eventually we can set these feature flags based on the FW version
      bool upper_mirrors_lower = true;
      settings.set_board_user_firmware_info(board_id, fw_rev, reg_rev, upper_mirrors_lower);

      if (scope_mode[board_id]) {
         snprintf(error, 255, "Board %s is running firmware %s, but FE is configured for scope mode. Edit the 'Scope mode (restart on change)' param and restart", hostname.c_str(), fw_type.c_str());
         cm_msg(MERROR, __FUNCTION__, "%s", error);
         return FE_ERR_ODB;
      }
   } else {
      snprintf(error, 255, "Board %s is running unsupported firmware type %s", hostname.c_str(), fw_type.c_str());
      cm_msg(MERROR, __FUNCTION__, "%s", error);
      return FE_ERR_ODB;
   }
   
   return SUCCESS;
}

INT VX2740GroupFrontend::connect_to_boards(char* error) {
   INT status = SUCCESS;

   for (auto i : settings.get_boards_enabled()) {
      if (boards.find(i) != boards.end() && boards[i]->is_connected()) {
         continue;
      }

      boards[i] = new VX2740();
      scope_mode[i] = settings.is_scope_mode(i);
      open_fw[i] = false;

      std::string hostname = settings.get_hostname(i);

      if (hostname == "" || hostname == "set hostname here") {
         snprintf(error, 255, "Set board's hostname/IP in ODB");
         cm_msg(MERROR, __FUNCTION__, "%s", error);
         status = FE_ERR_ODB;
         break;
      }

      fe_utils::ts_printf("Connecting to board %02d at %s\n", i, hostname.c_str());

      status = boards[i]->connect(hostname);
      board_names[i] = hostname;

      if (status != SUCCESS) {
         snprintf(error, 255, "Failed to connect to board '%s'", hostname.c_str());
         cm_msg(MERROR, __FUNCTION__, "%s", error);
         break;
      }

      status = validate_firmare_version(i, error);

      if (status != SUCCESS) {
         break;
      }
   }

   return status;
}

INT VX2740GroupFrontend::begin_of_run(INT run_num, char* error) {
   INT status = SUCCESS;
   timeval start_bor, end_connect, end_config;
   gettimeofday(&start_bor, NULL);

   abort_arming = false;
   ready_to_arm_acq = false;
   in_end_of_run = false;
   warned_corruption = false;

   try {
      settings.sync_settings_structs();
   } catch (SettingsException& e) {
      cm_msg(MERROR, __FUNCTION__, "Failure reading settings: %s", e.what());   
      return FE_ERR_ODB;
   }

   bool any_not_scope = false;

   for (auto& i : settings.get_boards_to_read_from()) {
      any_not_scope = any_not_scope || !scope_mode[i];
   }

   // Doesn't make sense to merge data in DPP_OPEN mode
   if (enable_data_readout && settings.merge_data() && any_not_scope) {
      snprintf(error, 255, "Not possible to run in 'merge data using event ID' mode with boards using open DPP firmware");
      cm_msg(MERROR, __FUNCTION__, "%s", error);
      return FE_ERR_ODB;
   }

   // Connect to any boards that were previously disabled
   if (connect_to_boards(error) != SUCCESS) {
      return FE_ERR_DRIVER;
   }

   gettimeofday(&end_connect, NULL);

   for (auto i : settings.get_boards_enabled()) {
      if (settings.multithreaded_readout()) {
         // Spawn thread to set up settings
         thread_args[i].obj = this;
         thread_args[i].board_index = i;
         readout_threads[i] = new std::thread(thread_data_readout_helper, &thread_args[i]);
         readout_status[i] = THREAD_STATUS_CONFIGURING;
      } else {
         readout_status[i] = configure_board(i);
      }
   }

   set_thread_cpu_and_priority(MAIN_THREAD_CPU_ID, MAIN_THREAD_PRIORITY, "main");

   // Wait until all boards report that they configured the board okay.
   int timeout_secs = settings.multithreaded_readout() ? 10 : 1;

   while (true) {
      bool all_okay = true;
      bool any_fail = false;

      for (auto i : settings.get_boards_enabled()) {
         if (readout_status[i] != THREAD_STATUS_CONFIGURED) {
            all_okay = false;
         }

         if (readout_status[i] == THREAD_STATUS_ERROR) {
            snprintf(error, 255, "Error when configuring %s", board_names[i].c_str());
            cm_msg(MERROR, __FUNCTION__, "%s", error);
            any_fail = true;
         }
      }

      if (any_fail) {
         abort_arming = true;
         return FE_ERR_DRIVER;
      }

      if (all_okay) {
         break;
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(1));

      timeval now;
      gettimeofday(&now, NULL);

      if (now.tv_sec - end_connect.tv_sec > timeout_secs) {
         snprintf(error, 255, "Timeout waiting for all boards to be configured");
         cm_msg(MERROR, __FUNCTION__, "%s", error);
         return FE_ERR_DRIVER;
      }
   }

   gettimeofday(&end_config, NULL);
   double ms = (end_config.tv_sec - start_bor.tv_sec) * 1e3 + (end_config.tv_usec - start_bor.tv_usec) * 1e-3;
   fe_utils::ts_printf("Took %.3lfms to configure boards\n", ms);

   // Signal the threads that we can start arming the boards
   ready_to_arm_acq = true;

   if (!settings.multithreaded_readout()) {
      for (auto i : settings.get_boards_enabled()) {
         readout_status[i] = arm_board(i);
      }
   }

   // Wait until all boards report that they configured the board okay.
   timeout_secs = settings.multithreaded_readout() ? 10 : 0;

   while (true) {
      bool all_okay = true;
      bool any_fail = false;

      for (auto i : settings.get_boards_enabled()) {
         if (readout_status[i] != THREAD_STATUS_ARMED) {
            all_okay = false;
         }

         if (readout_status[i] == THREAD_STATUS_ERROR) {
            snprintf(error, 255, "Error when arming %s", board_names[i].c_str());
            cm_msg(MERROR, __FUNCTION__, "%s", error);
            any_fail = true;
         }
      }

      if (any_fail) {
         abort_arming = true;
         return FE_ERR_DRIVER;
      }

      if (all_okay) {
         break;
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(1));

      timeval now;
      gettimeofday(&now, NULL);

      if (now.tv_sec - end_config.tv_sec > timeout_secs) {
         snprintf(error, 255, "Timeout waiting for all boards to be armed");
         cm_msg(MERROR, __FUNCTION__, "%s", error);
         return FE_ERR_DRIVER;
      }
   }

   fe_utils::ts_printf("All boards armed. End of begin-of-run procedure.\n");
   // TODO - understand initial 32-byte event sent by boards

   return SUCCESS;
}

INT VX2740GroupFrontend::jrpc_handler(int index, void** params) {
   // cm_register_function(RPC_JRPC, jrpc_handler)
   const char* cmd = (char*)params[0];
   const char* args = (char*)params[1];
   char* buf_p = (char*)params[2];
   INT max_reply_len = (*((INT*)params[3]));

   if (strcmp(cmd, "force_write") == 0) {
      return force_write_settings(buf_p);
   } else {
      return FE_ERR_ODB;
   }
}

INT VX2740GroupFrontend::force_write_settings(char* error) {
   INT status = SUCCESS;

   try {
      settings.sync_settings_structs();
   } catch (SettingsException& e) {
      cm_msg(MERROR, __FUNCTION__, "Failure reading settings: %s", e.what());   
      return FE_ERR_ODB;
   }

   // Connect to any boards that were previously disabled
   if (connect_to_boards(error) != SUCCESS) {
      return FE_ERR_DRIVER;
   }

   for (int i = 0; i < settings.get_num_boards(); i++) {
      if (!settings.is_board_enabled(i)) {
         cm_msg(MINFO, __FUNCTION__, "Board %02d from group %03d is disabled", i, this_group_index);
         continue;
      }

      std::lock_guard<std::mutex> guard(vx_mutexes[i]);

      INT fw_status = validate_firmare_version(i, error);

      if (fw_status != SUCCESS) {
         status = fw_status;
         break;
      }

      try {
         settings.write_settings_to_board(i, *boards[i]);
      } catch(CaenException& e) {
         cm_msg(MERROR, __FUNCTION__, "Failure writing settings to board for %s: %s", board_names[i].c_str(), e.what());
         status = FE_ERR_DRIVER;
         break;
      }
   }

   try {
      settings.handle_board_readback_structs();
   } catch(SettingsException& e) {
      cm_msg(MERROR, __FUNCTION__, "Failure handling board readback: %s", e.what());
      status = FE_ERR_ODB;
   }

   return status;
}

INT VX2740GroupFrontend::configure_board(int board_id) {
   VX2740& vx = *(boards[board_id]);
   INT status = SUCCESS;

   // Ensure board isn't already running
   vx.commands().stop_acq();

   // Write settings, based on defaults and/or overrides.
   // Also checks that values were set correctly.
   std::lock_guard<std::mutex> guard(vx_mutexes[board_id]);

   char error[255];
   status = validate_firmare_version(board_id, error);

   if (status != SUCCESS) {
      return status;
   }

   try {
      settings.write_settings_to_board(board_id, *boards[board_id]);
   } catch(CaenException& e) {
      cm_msg(MERROR, __FUNCTION__, "Failure writing board settings for %s: %s", board_names[board_id].c_str(), e.what());
      return THREAD_STATUS_ERROR;
   }

   guard.~lock_guard();

   int rb_handle = readout_rbs[board_id];

   // Set up raw data handle for scope mode; decoded handle for user DPP mode
   bool use_raw_handle = scope_mode[board_id];

   if (vx.data().setup_data_handle(use_raw_handle, vx.params()) != SUCCESS) {
      cm_msg(MERROR, __FUNCTION__, "Failure setting up data handle for %s", board_names[board_id].c_str());
      return THREAD_STATUS_ERROR;
   }

   if (enable_data_readout) {
      // Skip over any unread data from previous run.
      int buf_level = 0;
      status = rb_get_buffer_level(rb_handle, &buf_level);

      if (status != SUCCESS) {
         cm_msg(MERROR, __FUNCTION__, "Failure reading buffer level for %s: %d", board_names[board_id].c_str(), status);
         return THREAD_STATUS_ERROR;
      }

      fe_utils::ts_printf("Skipping over %d unused bytes in ring buffer for %s\n", buf_level, board_names[board_id].c_str());
      empty_ring_buffer(rb_handle, MAX_EV_SIZE);

      try {
         vx.params().get_max_raw_bytes_per_read(max_bytes_per_read[board_id]);
      } catch (CaenException& e) {
         cm_msg(MERROR, __FUNCTION__, "Failure max bytes per read for %s", board_names[board_id].c_str());
         return THREAD_STATUS_ERROR;
      }
   }

   return THREAD_STATUS_CONFIGURED;
}

INT VX2740GroupFrontend::arm_board(int board_id) {
   VX2740& vx = *(boards[board_id]);
   fe_utils::ts_printf("Arming %s\n", board_names[board_id].c_str());

   // Arm the acquisition
   if (vx.commands().start_acq(vx.params().is_sw_start_enabled()) != SUCCESS) {
      cm_msg(MERROR, __FUNCTION__, "Failure starting acquisition for %s", board_names[board_id].c_str());
      return THREAD_STATUS_ERROR;
   }

   fe_utils::ts_printf("Armed %s\n", board_names[board_id].c_str());
   return THREAD_STATUS_ARMED;
}

INT VX2740GroupFrontend::read_into_rb(int board_id, DWORD read_timeout_ms, uint16_t* tmp_waveform) {
   if (!enable_data_readout || !should_read_from_board(board_id)) {
      return VX_NO_EVENT;
   }

   int rb_handle = readout_rbs[board_id];

   unsigned char* wp = NULL;
   INT status = rb_get_wp(rb_handle, (void**) &wp, 0);

   if (status == DB_TIMEOUT) {
      if (settings.debug_ring_buffers()) {
         fe_utils::ts_printf("DEBUG: timeout waiting for wp for board %d\n", board_id);
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      return VX_NO_EVENT;
   } else if (status != SUCCESS) {
      cm_msg(MERROR, __FUNCTION__, "Failed to get write pointer: %d", status);
      readout_status[board_id] = THREAD_STATUS_ERROR;
      return THREAD_STATUS_ERROR;
   }

   if (settings.debug_ring_buffers()) {
      fe_utils::ts_printf("DEBUG: wp is currently %p\n", wp);
   }

   int buf_level = 0;
   status = rb_get_buffer_level(rb_handle, &buf_level);

   if (status != SUCCESS) {
      cm_msg(MERROR, __FUNCTION__, "Failed to read buffer level: %d", status);
      readout_status[board_id] = THREAD_STATUS_ERROR;
      return THREAD_STATUS_ERROR;
   }

   unsigned long int buffer_left_bytes = BUFFER_SIZE - buf_level;

   if (buffer_left_bytes <= max_bytes_per_read[board_id] + 1024) {
      return VX_NO_EVENT;
   }

   if (settings.debug_ring_buffers()) {
      fe_utils::ts_printf("RB headroom is %lu bytes, going to read out up to %u bytes\n", buffer_left_bytes, max_bytes_per_read[board_id]);
   }

   size_t read_size_bytes = 0;

   VX2740& vx = *(boards[board_id]);
   std::lock_guard<std::mutex> guard(vx_mutexes[board_id]);

   timeval start, end;
   gettimeofday(&start, NULL);

   if (scope_mode[board_id]) {
      // Read directly into ring buffer
      status = vx.data().get_raw_data(read_timeout_ms, wp, read_size_bytes);
   } else {
      // Read parsed data and convert to same format as scope data
      // Not optimal as adds some irrelevant header words!
      uint8_t channel_id = 0xFF;
      uint64_t timestamp = 0;
      size_t waveform_size = 0;
      status = vx.data().get_decoded_user_data(read_timeout_ms, channel_id, timestamp, waveform_size, tmp_waveform);

      if (status == SUCCESS) {
         read_size_bytes = vx.data().encode_user_data_to_buffer(channel_id, timestamp, waveform_size, tmp_waveform, wp);
      }
   }

   gettimeofday(&end, NULL);

   if (status == VX_NO_EVENT) {
      return VX_NO_EVENT;
   } else if (status != SUCCESS) {
      fe_utils::ts_printf("get_raw_data() returned %d. Break.\n", status);
      readout_status[board_id] = THREAD_STATUS_ERROR;
      return THREAD_STATUS_ERROR;
   }

   double elapsed_us = (end.tv_sec - start.tv_sec)*1e6 + (end.tv_usec - start.tv_usec);
   double rate = (read_size_bytes/1024./1024.)/(elapsed_us/1e6);

   if (settings.debug_rates()) {
      fe_utils::ts_printf("Read %s in %.0f us (%.1f MiB/s) from %s.\n", fe_utils::format_bytes(read_size_bytes).c_str(), elapsed_us, rate, board_names[board_id].c_str());
   }

   status = rb_increment_wp(rb_handle, read_size_bytes);

   if (status != SUCCESS) {
      cm_msg(MERROR, __FUNCTION__, "Failed to increment wp!");
      readout_status[board_id] = THREAD_STATUS_ERROR;
      return THREAD_STATUS_ERROR;
   }

   if (settings.debug_ring_buffers()) {
      rb_get_buffer_level(rb_handle, &buf_level);
      fe_utils::ts_printf("DEBUG: incremented wp; RB headroom is now %d bytes\n", BUFFER_SIZE - buf_level);
   }

   return SUCCESS;
}

void *VX2740GroupFrontend::thread_data_readout(int board_id) {
   fe_utils::ts_printf("Spawned thread to configure/readout %s (board %02d)\n", board_names[board_id].c_str(), board_id);

   readout_status[board_id] = configure_board(board_id);

   if (readout_status[board_id] != THREAD_STATUS_CONFIGURED) {
      return NULL;
   }

   // Wait until main thread tells us that all boards have been configured correctly
   // before we actually arm the board.
   while (!ready_to_arm_acq) {
      if (abort_arming) {
         return NULL;
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(1));
   }

   readout_status[board_id] = arm_board(board_id);

   if (readout_status[board_id] != THREAD_STATUS_ARMED) {
      return NULL;
   }

   timeval last_sleep;
   DWORD timeout_ms = settings.get_read_data_timeout(board_id);
   uint16_t* tmp_waveform = (uint16_t*) calloc(0x8000, sizeof(uint16_t));

   while (enable_data_readout && !in_end_of_run) {
      INT status = read_into_rb(board_id, timeout_ms, tmp_waveform);

      if (status == VX_NO_EVENT) {
         gettimeofday(&last_sleep, NULL);
         std::this_thread::sleep_for(std::chrono::milliseconds(10));
         continue;
      } else if (status != SUCCESS) {
         break;
      }

      timeval now;
      gettimeofday(&now, NULL);

      if (double(now.tv_sec - last_sleep.tv_sec) + 1e-6 * double(now.tv_usec - last_sleep.tv_usec) > 0.1) {
         // At least 100ms since we last slept. Yield to ensure other thread can
         // do some work.
         gettimeofday(&last_sleep, NULL);
         std::this_thread::sleep_for(std::chrono::milliseconds(10));
         continue;
      }
   }

   free(tmp_waveform);

   return NULL;
}

INT VX2740GroupFrontend::end_of_run(INT run_num, char* error) {
   in_end_of_run = true;

   for (auto board_id : settings.get_boards_enabled()) {
      if (readout_threads[board_id]) {
         readout_threads[board_id]->join();
         delete readout_threads[board_id];
         readout_threads[board_id] = NULL;
      }

      std::lock_guard<std::mutex> guard(vx_mutexes[board_id]);
      boards[board_id]->commands().stop_acq();
   }

   return SUCCESS;
}

int VX2740GroupFrontend::peek_rb_event_id(int board_id) {
   unsigned char* rp = NULL;
   int buf_level = 0;
   int rb_handle = readout_rbs[board_id];

   INT status = rb_get_buffer_level(rb_handle, &buf_level);

   if (status != SUCCESS) {
      cm_msg(MERROR, __FUNCTION__, "Failed to get buffer level for %s: %d", board_names[board_id].c_str(), status);
      return 0;
   }

   if (buf_level < 24) {
      // Not enough data to even parse a header.
      return -1;
   }

   status = rb_get_rp(rb_handle, (void**) &rp, 0);

   if (status != SUCCESS) {
      cm_msg(MERROR, __FUNCTION__, "Failed to get rp for %s: %d", board_names[board_id].c_str(), status);
      return -1;
   }

   if (settings.debug_ring_buffers()) {
      fe_utils::ts_printf("DEBUG: rp is currently %p\n", rp);
   }

   // Parse the event header
   CaenEventHeader header((uint64_t*)(rp));

   if (header.size_bytes() > BUFFER_SIZE) {
      if (!warned_corruption) {
         warned_corruption = true;
         cm_msg(MERROR, __FUNCTION__, "Data corruption or event size too large; event is reporting a size of %u bytes", header.size_bytes());
         char errstr[255];
         cm_transition(TR_STOP, 0, errstr, 255, TR_ASYNC, FALSE);
      }

      return -1;
   }

   if (buf_level < header.size_bytes()) {
      // Fragmented event that hasn't been fully read out yet.
      printf("%d < %d\n", buf_level, header.size_bytes());
      return -1;
   }

   return header.event_counter;
}

bool VX2740GroupFrontend::is_event_ready() {
   if (!enable_data_readout) {
      return false;
   }

   if (!settings.multithreaded_readout()) {
      uint16_t* tmp_waveform = (uint16_t*) calloc(0x8000, sizeof(uint16_t));

      for (auto& i : settings.get_boards_enabled()) {
         read_into_rb(i, settings.get_read_data_timeout(i), tmp_waveform);
      }

      free(tmp_waveform);
   }

   if (!single_fe_mode && settings.merge_data()) {
      // Need an event from all boards
      int match_id = -2;
      int mismatch_board_id = -1;
      int mismatch_missing_id = -1;

      for (auto i : settings.get_boards_to_read_from()) {
         int this_id = peek_rb_event_id(i);

         if (this_id == -1) {
            // No event from this board; can't merge data from all boards..
            return false;
         } else if (match_id < 0) {
            // First board has an event; make a note of it's ID.
            match_id = this_id;
         } else if (match_id != this_id) {
            // Event ID mismatch.
            mismatch_missing_id = std::min(match_id, this_id);
            mismatch_board_id = (mismatch_missing_id == this_id) ? i : settings.get_boards_to_read_from()[0];
         }
      }

      if (mismatch_board_id >= 0) {
         cm_msg(MERROR, __FUNCTION__, "Board %s missed trigger #%d", board_names[mismatch_board_id].c_str(), mismatch_missing_id);
         event_id_to_write = mismatch_missing_id;
      } else {
         event_id_to_write = match_id;
      }

      // We have all the data we need to write out an event.
      return true;
   } else {
      // Need an event from any board
      for (auto i : settings.get_boards_to_read_from()) {
         int this_id = peek_rb_event_id(i);

         if (this_id != -1) {
            // Board has an event
            event_id_to_write = this_id;
            return true;
         }
      }

      // No event from any boards.
      return false;
   }
}

int VX2740GroupFrontend::write_data(char* pevent) {
   if (!enable_data_readout) {
      return 0;
   }

   std::vector<int> board_ids_to_write;

   if (!single_fe_mode && settings.merge_data()) {
      // Write data from all boards (unless they missed this trigger)
      for (auto board_id : settings.get_boards_to_read_from()) {
         if (peek_rb_event_id(board_id) == event_id_to_write) {
            board_ids_to_write.push_back(board_id);
         }
      }
   } else {
      // Write data from first board with data available
      for (auto i : settings.get_boards_to_read_from()) {
         if (peek_rb_event_id(i) == event_id_to_write) {
            board_ids_to_write.push_back(i);
            break;
         }
      }
   }

   bk_init32(pevent);
   TRIGGER_MASK(pevent) = this_group_index;

   for (auto board_id : board_ids_to_write) {
      unsigned char* rp = NULL;
      int buf_level = 0;
      int rb_handle = readout_rbs[board_id];

      INT status = rb_get_rp(rb_handle, (void**) &rp, 0);

      if (status != SUCCESS) {
         cm_msg(MERROR, __FUNCTION__, "Failed to get rp: %d", status);
         return 0;
      }

      status = rb_get_buffer_level(rb_handle, &buf_level);

      if (status != SUCCESS) {
         cm_msg(MERROR, __FUNCTION__, "Failed to get buffer level");
         return 0;
      }

      // Parse the event header
      CaenEvent event((uint64_t*)rp);
      CaenEventHeader& header = event.header;

      if (settings.debug_data()) {
         if (header.format == 0x10) {
            fe_utils::ts_printf("Writing event # 0x%x from %s.\n", header.event_counter, board_names[board_id].c_str());
            fe_utils::ts_printf("  Format:       0x%x\n", header.format);
            fe_utils::ts_printf("  Size:         0x%x bytes (%s)\n", header.size_bytes(), fe_utils::format_bytes(header.size_bytes()).c_str());
            fe_utils::ts_printf("  Channel mask: 0x%llx\n", header.ch_enable_mask);
            fe_utils::ts_printf("  Trigger time: 0x%llx (%fs)\n", header.trigger_time, (double)header.trigger_time/1.25e8);
            
            // Find first enabled channel
            int chan = 0;

            for (int i = 0; i < 64; i++) {
               if (header.ch_enable_mask & ((uint64_t)1<<i)) {
                  chan = i;
                  break;
               }
            }

            // Print samples of first enabled channel
            uint16_t samples[10];
            int num_read = event.get_channel_samples(chan, samples, 10);
            fe_utils::ts_printf("First %d samples from chan %d: ", num_read, chan);
            for (int i = 0; i < num_read; i++) {
               printf("0x%x ", samples[i]);
            }
            printf("\n");
         } else {
            fe_utils::ts_printf("Writing special event from %s.\n", board_names[board_id].c_str());
            fe_utils::ts_printf("  Format:       0x%x\n", header.format);
            fe_utils::ts_printf("  Trigger time: 0x%llx (%fs)\n", header.trigger_time, (double)header.trigger_time/1.25e8);
         }
      }

      // Copy data from buffer into bank
      uint64_t* pdata;
      char bank_name[5];
      snprintf(bank_name, 5, "D%03d", board_id);

      bk_create(pevent, bank_name, TID_QWORD, (void**)&pdata);
      event.hencode(pdata);
      pdata += header.size_bytes()/sizeof(uint64_t);
      bk_close(pevent, pdata);

      rb_increment_rp(rb_handle, header.size_bytes());

      if (settings.debug_ring_buffers()) {
         rb_get_rp(rb_handle, (void**) &rp, 0);
         fe_utils::ts_printf("DEBUG: incremented rp to %p\n", rp);
      }
   }

   if (settings.debug_data()) {
      fe_utils::ts_printf("Final event size: %s\n", fe_utils::format_bytes(bk_size(pevent)).c_str());
   }

   return bk_size(pevent);
}

int VX2740GroupFrontend::write_metadata(char* pevent) {
   // Store metadata from VX2740.
   bk_init32(pevent);

   for (int board_id = 0; board_id < settings.get_num_boards(); board_id++) {
      DWORD* pdata;
      char bank_name[5];
      snprintf(bank_name, 5, "M%03d", board_id);

      // If you add more settings here, also update the list of Names in
      // setup_group_and_board_params()!
      DWORD status = 0;
      float temp_air_in = 0, temp_air_out = 0, temp_hottest_adc = 0;
      uint32_t error_flags = 0;
      std::vector<int> boards_enabled = settings.get_boards_enabled();

      if (std::find(boards_enabled.begin(), boards_enabled.end(), board_id) != boards_enabled.end()) {
         VX2740& vx = *(boards[board_id]);
         vx_mutexes[board_id].lock();
         vx.params().get_acquisition_status(status);
         vx.params().get_temperatures(temp_air_in, temp_air_out, temp_hottest_adc);
         vx.params().get_error_flags(error_flags);
         vx_mutexes[board_id].unlock();
      }

      bk_create(pevent, bank_name, TID_DWORD, (void**)&pdata);

      *pdata++ = status;
      *pdata++ = (DWORD)temp_air_in;
      *pdata++ = (DWORD)temp_hottest_adc;
      *pdata++ = error_flags;

      bk_close(pevent, pdata);

   }

   return bk_size(pevent);
}

int VX2740GroupFrontend::check_errors(char* pevent) {
   for (int board_id = 0; board_id < settings.get_num_boards(); board_id++) {
      uint16_t lvds_ioreg = 0;
      uint32_t lvds_userreg_in = 0;
      uint32_t lvds_userreg_out = 0;
      BoardErrors err;
      std::vector<int> boards_enabled = settings.get_boards_enabled();

      if (std::find(boards_enabled.begin(), boards_enabled.end(), board_id) != boards_enabled.end()) {
         VX2740& vx = *(boards[board_id]);
         std::lock_guard<std::mutex> guard(vx_mutexes[board_id]);
         vx.params().get_error_flags(err.bitmask);
         vx.params().get_lvds_io_register(lvds_ioreg);

         if (open_fw[board_id]) {
            vx.params().get_user_register(0x44, lvds_userreg_out);
            vx.params().get_user_register(0x48, lvds_userreg_in);
         }

         err.message = vx.params().error_to_text(err.bitmask);
      }

      settings.set_board_errors(board_id, err);
      settings.set_lvds_readback(board_id, lvds_ioreg, lvds_userreg_out, lvds_userreg_in);

      std::string hostname = settings.get_hostname(board_id);

      char alarm_name[255] = {};
      snprintf(alarm_name, 255, "Digitizer error - %s", hostname.c_str());

      if (err.bitmask == 0) {
         al_reset_alarm(alarm_name);
      } else {
         char message[80] = {};
         snprintf(message, 80, "%s - %s", hostname.c_str(), err.message.c_str());
         al_trigger_alarm(alarm_name, message, "Alarm", "", AT_INTERNAL);
      }
   }

   settings.handle_board_errors_structs();
   settings.handle_board_readback_structs();

   return 0;
}


INT VX2740GroupFrontend::empty_ring_buffer(int rb_handle, int max_event_size_bytes) {
   int prev_buf_level = -1;

   // Implementation of rb_increment_wp only allows incrementing by max_event_size_bytes
   // at a time. Iterate as needed.
   while (true) {
      int buf_level;
      INT status = rb_get_buffer_level(rb_handle, &buf_level);

      if (status != SUCCESS) {
         cm_msg(MERROR, __FUNCTION__, "Failed to read buffer level for rb %d: %d", rb_handle, status);
         return status;
      }

      if (buf_level == 0) {
         break;
      }

      if (buf_level == prev_buf_level) {
         cm_msg(MERROR, __FUNCTION__, "Failed to free up space in rb %d: level is stuck at %d bytes", rb_handle, buf_level);
         return FE_ERR_DRIVER;
      }

      int this_inc = std::min(buf_level, max_event_size_bytes);
      rb_increment_rp(rb_handle, this_inc);
      prev_buf_level = buf_level;
   }

   return SUCCESS;
}

std::map<int, VX2740*> VX2740GroupFrontend::get_boards() {
   return boards;
}

std::map<int, std::string> VX2740GroupFrontend::get_board_names() {
   return board_names;
}

bool VX2740GroupFrontend::should_read_from_board(int board_id) {
   for (auto& b : settings.get_boards_to_read_from()) {
      if (b == board_id) {
         return true;
      }
   }

   return false;
}
