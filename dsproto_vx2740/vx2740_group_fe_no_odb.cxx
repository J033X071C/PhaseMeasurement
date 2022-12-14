#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <iostream>
#include <cstdio>
#include <sys/time.h>
#include <stdarg.h>
#include <pthread.h>
#include "midas.h"
#include "msystem.h"
#include "mfe.h"
#include "fe_settings_strategy.h"
#include "vx2740_fe_class.h"

/*-- Globals -------------------------------------------------------*/

/* The frontend name (client name) as seen by other MIDAS clients   */
const char *frontend_name = "VX2740_Group_";
/* The frontend file name, don't change it */
const char *frontend_file_name = __FILE__;

/* frontend_loop is called periodically if this variable is TRUE    */
BOOL frontend_call_loop = TRUE;

/* a frontend status page is displayed with this frequency in ms */
INT display_period = 0;

#define BUFFER_SIZE 1000000000 // 1000MB (board has 2GB total)
#define MAX_EV_SIZE 320000000  // 320MB

/* maximum event size produced by this frontend */
INT max_event_size = MAX_EV_SIZE + 100;

/* maximum event size for fragmented events (EQ_FRAGMENTED) */
INT max_event_size_frag = 5 * 1024 * 1024;

/* buffer size to hold events */
INT event_buffer_size = BUFFER_SIZE;

/*-- Function declarations -----------------------------------------*/

INT frontend_init();
INT frontend_exit();
INT begin_of_run(INT run_number, char *error);
INT end_of_run(INT run_number, char *error);
INT pause_run(INT run_number, char *error);
INT resume_run(INT run_number, char *error);
INT frontend_loop();
INT poll_event(INT source, INT count, BOOL test);
INT interrupt_configure(INT cmd, INT source, PTYPE adr);

INT read_metadata(char *pevent, INT off);
INT read_waveforms(char *pevent, INT off);
INT read_errors(char *pevent, INT off);

// Object for interacting with registers.
// Here we specify to use "group fe mode"
std::shared_ptr<VX2740FeSettingsManual> fake_odb;
std::shared_ptr<VX2740GroupFrontend> vx_group;

// Frontend/board index.
int gFrontendIndex;

/*-- Equipment list ------------------------------------------------*/
#undef USE_INT

BOOL equipment_common_overwrite = FALSE;

EQUIPMENT equipment[] = {
   { "VX2740_Errors_Group_%03d", /* equipment name */
      { 105, 0, /* event ID, trigger mask */
      "", /* write events to system buffer */
      EQ_PERIODIC, /* equipment type */
      0, /* event source */
      "MIDAS", /* format */
      TRUE, /* enabled */
      RO_ALWAYS, /* write events only when running */
      1000, /* read every so many msec */
      0, /* stop run after this event limit */
      0, /* number of sub events */
      0, /* log history */
      "", "", "", },
      read_errors, /* readout routine */
   },
   
   { "VX2740_Config_Group_%03d", /* equipment name */
      { 103, 0, /* event ID, trigger mask */
      "", /* write events to system buffer */
      EQ_PERIODIC, /* equipment type */
      0, /* event source */
      "MIDAS", /* format */
      TRUE, /* enabled */
      RO_RUNNING, /* write events only when running */
      6000, /* read every so many msec */
      0, /* stop run after this event limit */
      0, /* number of sub events */
      1, /* log history */
      "", "", "", },
      read_metadata, /* readout routine */
   },

   { "VX2740_Data_Group_%03d", /* equipment name */
      { 104, 0, /* event ID, trigger mask */
      "SYSTEM", /* write events to system buffer */
      EQ_POLLED, /* equipment type */
      0, /* event source */
      "MIDAS", /* format */
      TRUE, /* enabled */
      RO_RUNNING, /* write events only when running */
      1000, /* read every so many msec */
      0, /* stop run after this event limit */
      0, /* number of sub events */
      0, /* log history */
      "", "", "", },
      read_waveforms, /* readout routine */
   },

   { "" }
};

// Readout event for metadata from registers (deadtime,
// lost triggers etc). Stored as doubles so we can include
// a trigger rate as well.
INT read_metadata(char *pevent, INT off) {
   return vx_group->write_metadata(pevent);
}

INT read_errors(char *pevent, INT off) {
   return vx_group->check_errors(pevent);
}

// Polling to check whether events are ready to be written to midas buffer.
// We require the ring buffer to contain at least 1 full event.
INT poll_event(INT source, INT count, BOOL test) {
   if (test) {
      for (int i = 0; i < count - 1; i++) {
         ss_sleep(2);
      }

      return 0;
   }

   return vx_group->is_event_ready();
}

// Place the most recently-read event into a midas bank.
// Currently we also print some debugging information.
INT read_waveforms(char *pevent, INT off) {
   return vx_group->write_data(pevent);
}

// Called when FE starts. Connect to FPGA fabric, setup ODB structure, and
// write board register values to ODB.
INT frontend_init() {
   gFrontendIndex = get_frontend_index();

   if (gFrontendIndex < 0) {
      cm_msg(MERROR, __FUNCTION__, "You must run this frontend with the -i argument to specify the frontend index.");
      return FE_ERR_DRIVER;
   }

   if (gFrontendIndex == 0) {
      // Edinburgh test
      fake_odb = std::make_shared<VX2740FeSettingsManual>();
      fake_odb->manual_group_settings.num_boards = 1;
      fake_odb->manual_group_settings.multithreaded_readout = false;

      BoardSettings settings;
      settings.bools.at("Scope mode (restart on change)") = true;
      settings.strings.at("Hostname (restart on change)") = "192.168.0.254";

      settings.bools.at("Trigger on ch over thresh A") = false;
      settings.bools.at("Trigger on ch over thresh B") = false;
      settings.bools.at("Trigger on ch over thresh A&&B") = false;
      settings.bools.at("Trigger on external signal") = false;
      settings.bools.at("Trigger on software signal") = false;
      settings.bools.at("Trigger on user mode signal") = false;
      settings.bools.at("Trigger on test pulse") = true;
      settings.bools.at("Trigger on LVDS") = false;
      settings.doubles.at("Test pulse period (ms)") = 500;

      settings.uint32s.at("Readout channel mask (31-0)") = 0x1;
      settings.uint32s.at("Readout channel mask (63-32)") = 0;

      settings.uint32s.at("Waveform length (samples)") = 1000;
      settings.uint32s.at("Trigger delay (samples)") = 100;
      settings.uint16s.at("Pre-trigger (samples)") = 100;

      settings.vec_uint16s.at("User registers/Waveform length (samples)") = std::vector<uint16_t>(64, 1000);
      settings.vec_uint16s.at("User registers/Pre-trigger (samples)") = std::vector<uint16_t>(64, 100);
      settings.uint32s.at("User registers/Trigger on global (31-0)") = 0x1; // Global trigger for chan 0

      fake_odb->manual_board_settings[0] = settings;
   } else {
      // TRIUMF test
      fake_odb = std::make_shared<VX2740FeSettingsManual>();
      fake_odb->manual_group_settings.num_boards = 2;
      fake_odb->manual_group_settings.multithreaded_readout = true;

      BoardSettings settings;
      settings.bools.at("Scope mode (restart on change)") = false;
      settings.strings.at("Hostname (restart on change)") = "vx01";

      settings.bools.at("Trigger on ch over thresh A") = false;
      settings.bools.at("Trigger on ch over thresh B") = false;
      settings.bools.at("Trigger on ch over thresh A&&B") = false;
      settings.bools.at("Trigger on external signal") = false;
      settings.bools.at("Trigger on software signal") = false;
      settings.bools.at("Trigger on user mode signal") = false;
      settings.bools.at("Trigger on test pulse") = true;
      settings.bools.at("Trigger on LVDS") = false;
      settings.doubles.at("Test pulse period (ms)") = 500;

      settings.uint32s.at("Readout channel mask (31-0)") = 0x1;
      settings.uint32s.at("Readout channel mask (63-32)") = 0;

      settings.uint32s.at("Waveform length (samples)") = 1000;
      settings.uint32s.at("Trigger delay (samples)") = 100;
      settings.uint16s.at("Pre-trigger (samples)") = 100;

      settings.vec_uint16s.at("User registers/Waveform length (samples)") = std::vector<uint16_t>(64, 1000);
      settings.vec_uint16s.at("User registers/Pre-trigger (samples)") = std::vector<uint16_t>(64, 100);
      settings.uint32s.at("User registers/Trigger on global (31-0)") = 0x1; // Global trigger for chan 0

      fake_odb->manual_board_settings[0] = settings;

      BoardSettings settings2 = settings;
      settings2.strings.at("Hostname (restart on change)") = "vx02";

      fake_odb->manual_board_settings[1] = settings2;
   }

   vx_group = std::make_shared<VX2740GroupFrontend>(fake_odb, false);
   cm_register_transition(TR_STARTABORT, end_of_run, 500);

   return vx_group->init(gFrontendIndex);
}

INT frontend_exit() {
   return SUCCESS;
}

// Write ODB settings to board, and initialize for acquisition.
INT begin_of_run(INT run_number, char *error) {
   return vx_group->begin_of_run(run_number, error);
}

INT end_of_run(INT run_number, char *error) {
   return vx_group->end_of_run(run_number, error);
}

INT pause_run(INT run_number, char *error) {
   return SUCCESS;
}

INT resume_run(INT run_number, char *error) {
   return SUCCESS;
}

INT frontend_loop() {
   return SUCCESS;
}

/*-- Interrupt configuration ---------------------------------------*/

INT interrupt_configure(INT cmd, INT source, PTYPE adr) {
   switch (cmd) {
   case CMD_INTERRUPT_ENABLE:
      break;
   case CMD_INTERRUPT_DISABLE:
      break;
   case CMD_INTERRUPT_ATTACH:
      break;
   case CMD_INTERRUPT_DETACH:
      break;
   }
   return SUCCESS;
}
