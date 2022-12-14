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
#include "vx2740_fe_class.h"

/*-- Globals -------------------------------------------------------*/

/* The frontend name (client name) as seen by other MIDAS clients   */
const char *frontend_name = "VX2740_";
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
// Here we specify to use "single fe mode"
VX2740GroupFrontend vx_group(std::make_shared<VX2740FeSettingsODB>(), true);

// Frontend/board index.
int gFrontendIndex;

/*-- Equipment list ------------------------------------------------*/
#undef USE_INT

BOOL equipment_common_overwrite = FALSE;

EQUIPMENT equipment[] = {
   { "VX2740_Errors_%03d", /* equipment name */
      { 100, 0, /* event ID, trigger mask */
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

   { "VX2740_Config_%03d", /* equipment name */
      { 101, 0, /* event ID, trigger mask */
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

   { "VX2740_Data_%03d", /* equipment name */
      { 102, 0, /* event ID, trigger mask */
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
   return vx_group.write_metadata(pevent);
}

INT read_errors(char *pevent, INT off) {
   return vx_group.check_errors(pevent);
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

   return vx_group.is_event_ready();
}

// Place the most recently-read event into a midas bank.
// Currently we also print some debugging information.
INT read_waveforms(char *pevent, INT off) {
   return vx_group.write_data(pevent);
}

// Called when FE starts. Connect to FPGA fabric, setup ODB structure, and
// write board register values to ODB.
INT frontend_init() {
   gFrontendIndex = get_frontend_index();

   if (gFrontendIndex < 0) {
      cm_msg(MERROR, __FUNCTION__, "You must run this frontend with the -i argument to specify the frontend index.");
      return FE_ERR_DRIVER;
   }

   cm_register_transition(TR_STARTABORT, end_of_run, 500);

   return vx_group.init(gFrontendIndex, hDB);
}

INT frontend_exit() {
   return SUCCESS;
}

// Write ODB settings to board, and initialize for acquisition.
INT begin_of_run(INT run_number, char *error) {
   return vx_group.begin_of_run(run_number, error);
}

INT end_of_run(INT run_number, char *error) {
   return vx_group.end_of_run(run_number, error);
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
