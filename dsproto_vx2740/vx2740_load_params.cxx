#include "vx2740_wrapper.h"
#include "stdio.h"
#include <sys/time.h>
#include <cstring>
#include <string>
#include <vector>
#include <fstream>
#include <map>
#include <algorithm>
#include <unistd.h>
#include "CAEN_FELib.h"

/**
 * This program connects to a VX2740 board, and loads a set of parameters
 * that were previously dumped using the `vx2740_dump_params` program.
 * 
 * If --take-data is specified, a short run of 10s will be started/stopped
 * after the parameters have been loaded.
 * 
 * E.g.
 * vx2740_dump_params vx02 -a > params.txt
 * vx2740_load_params vx02 params.txt
 */

void usage(char *prog_name) {
   printf("Usage: %s <hostname> <filename> [--take-data]\n", prog_name);
}

int main(int argc, char **argv) {
   bool any_help = false;

   for (int i = 1; i < argc; i++) {
      if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
         any_help = true;
         break;
      }
   }

   if (argc < 2 || argc > 4 || any_help) {
      usage(argv[0]);
      return 0;
   }

   VX2740 vx;
   vx.connect(argv[1], false, false);

   vx.stop_acq();

   std::string filename = "";
   bool take_data = false;

   for (int i = 2; i < argc; i++) {
      std::string arg(argv[i]);

      if (arg == "--take-data") {
         take_data = true;
      } else if (filename == "") {
         filename = arg;
      } else {
         usage(argv[0]);
         return 0;
      }
   }

   std::ifstream file(filename);

   if (!file.is_open()) {
      printf("Failed to open %s\n", filename.c_str());
      return 1;
   }

   std::string line;

   std::vector<std::string> known_ch_skip = {
      "/par/adctovolts",
      "/par/chstatus",
      "/par/gainfactor"
   };

   std::vector<std::string> known_skip = {
      "/endpoint/par/activeendpoint",
      "/par/acquisitionstatus",
      "/par/adc_nbit",
      "/par/adc_samplrate",
      "/par/boardready",
      "/par/cupver",
      "/par/dutycyclesensdcdc",
      "/par/errorflags",
      "/par/familycode",
      "/par/formfactor",
      "/par/fpga_fwver",
      "/par/freqsenscore",
      "/par/fwtype",
      "/par/gateway",
      "/par/inputrange",
      "/par/inputtype",
      "/par/ioutsensdcdc",
      "/par/ipaddress",
      "/par/ledstatus",
      "/par/license",
      "/par/licenseremainingtime",
      "/par/licensestatus",
      "/par/maxrawdatasize",
      "/par/modelcode",
      "/par/modelname",
      "/par/netmask",
      "/par/numch",
      "/par/opendataaddress",
      "/par/opendatamode",
      "/par/opendatasize",
      "/par/pbcode",
      "/par/pcbrev_mb",
      "/par/pcbrev_pb",
      "/par/serialnum",
      "/par/speedsensfan1",
      "/par/speedsensfan2",
      "/par/tempsensadc0",
      "/par/tempsensadc7",
      "/par/tempsensairin",
      "/par/tempsensairout",
      "/par/tempsenscore",
      "/par/tempsensdcdc",
      "/par/tempsensfirstadc",
      "/par/tempsenshottestadc",
      "/par/tempsenslastadc",
      "/par/vinsensdcdc",
      "/par/voutsensdcdc",
      "/par/zin"
   };

   while (getline(file, line)) {
      size_t split_pos = line.find("=");
      if (split_pos == std::string::npos || line.find("/") != 0) {
         continue;
      }

      std::string param = line.substr(0, split_pos - 1);
      std::string value = line.substr(split_pos + 2, std::string::npos);

      uint64_t param_handle = 0;

      if (value.find("** UNKNOWN **") == 0) {
         continue;
      }

      if (std::find(known_skip.begin(), known_skip.end(), param) != known_skip.end()) {
         continue;
      }

      if (param.find("/ch") == 0) {
         bool skip_chan_param = false;

         for (auto& ch_skip : known_ch_skip) {
            if (param.find(ch_skip) != std::string::npos) {
               skip_chan_param = true;
               break;
            }
         }

         if (skip_chan_param) {
            continue;
         }
      }

      char name[32];
      CAEN_FELib_NodeType_t type;
      int status = CAEN_FELib_GetHandle(vx._get_root_handle(), param.c_str(), &param_handle);

      if (status != CAEN_FELib_Success) {
         printf("Failed to get handle for %s\n", param.c_str());
         continue;
      }

      status = CAEN_FELib_GetNodeProperties(param_handle, "", name, &type);

      if (status != CAEN_FELib_Success) {
         printf("Failed to get properties of %s\n", param.c_str());
         continue;
      }

      if (type != CAEN_FELib_PARAMETER) {
         continue;
      }
      
      if (param == "/par/lvdstrgmask") {
         continue;
      }

      if (vx._set(vx._get_root_handle(), param, value) != SUCCESS) {
         printf("Failed to set %s to %s\n", param.c_str(), value.c_str());
      }
   }

   file.close();
   printf("Set parameters...\n");

   if (take_data) {
      if (vx.setup_data_handle(true) != SUCCESS) {
         printf("Failed to setup data handle\n");
         return 2;
      } 

      printf("Reading raw data for 5s...\n");

      vx.start_acq();
      size_t num_bytes_read = 0;
      uint8_t* buffer = (uint8_t*)calloc(1000000, sizeof(uint8_t));
      int num_read = 0;
      int num_empty = 0;
      int num_err = 0;

      timeval start = {};
      gettimeofday(&start, NULL);

      while (true) {
         int status = vx.get_raw_data(100, buffer, num_bytes_read);

         if (status == SUCCESS) {
            num_read++;
         } else if (status == VX_NO_EVENT) {
            num_empty++;
         } else {
            num_err++;
         }

         usleep(1000);
         timeval now = {};
         gettimeofday(&now, NULL);

         if (now.tv_sec - start.tv_sec > 5) {
            break;
         }
      }

      vx.stop_acq();
      printf("Read %d events in raw mode\n", num_read);

      free(buffer);
   }

   return 0;
}

