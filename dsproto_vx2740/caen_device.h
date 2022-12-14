#ifndef CAEN_DEVICE_H
#define CAEN_DEVICE_H

#include "midas.h"
#include <inttypes.h>
#include <stdlib.h>
#include <map>
#include <exception>

class CaenDevice {
   public:
      CaenDevice();
      ~CaenDevice();

      INT connect(std::string hostname, bool do_reset, bool monitor_only);
      INT disconnect();
      INT reset();
      bool is_connected();
      void set_debug(bool _debug);

      std::string get_last_error(std::string prefix);
      void print_last_error(std::string prefix);
      uint64_t get_root_handle();

      std::string get_name() {
         return dev_path;
      }

      INT run_cmd(std::string cmd_name);

   private:
      bool debug = false;
      bool connected = false;
      uint64_t dev_handle = 0;
      std::string dev_path;
};

#endif