#ifndef VX2740_WRAPPER_H
#define VX2740_WRAPPER_H

#include "caen_device.h"
#include "caen_parameters.h"
#include "caen_commands.h"
#include "caen_data.h"
#include "midas.h"
#include <inttypes.h>
#include <stdlib.h>

// Helper class for interacting with the VX2740.
// Every function in this class uses midas-style return codes
// (where 1 indicates SUCCESS).
class VX2740 {
public:
   virtual ~VX2740(){};

   // Connect to the VX2740 and do some sanity-checking.
   // If `monitor_only` is true, you will not be able to read data
   // or control the run (start/stop/arm), but you will be able to
   // read/write settings.
   //
   // Only one "full" connection is allowed per device (if you connect
   // in non-monitor mode to a device that is already connected, you will
   // steal the connection! The other program that is connected will lose
   // their connection to the device).
   //
   // Up to 2 programs may be connected as "monitoring" clients, in addition
   // to the 1 program with the full connection.
   INT connect(std::string hostname, bool do_reset = false, bool monitor_only = false);
   
   INT disconnect() {
      return dev->disconnect();
   }

   bool is_connected() {
      return dev->is_connected();
   }

   std::string get_name() {
      return dev->get_name();
   }

   CaenParameters& params() { return params_helper; }
   CaenCommands& commands() { return commands_helper; }
   CaenData& data() { return data_helper; }

protected:
   std::shared_ptr<CaenDevice> dev = nullptr;
   CaenParameters params_helper;
   CaenCommands commands_helper;
   CaenData data_helper;

};

#endif
