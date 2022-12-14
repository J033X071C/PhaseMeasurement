#include "caen_commands.h"
#include "CAEN_FELib.h"

INT CaenCommands::start_acq(bool sw_start) {
   int ret = dev->run_cmd("cleardata");

   if (ret == SUCCESS) {
      dev->run_cmd("armacquisition");
   }

   if (ret == SUCCESS && sw_start) {
      ret = dev->run_cmd("swstartacquisition");
   }

   return ret;
}

INT CaenCommands::stop_acq() {
   return dev->run_cmd("disarmacquisition");
}

INT CaenCommands::issue_software_trigger() {
   return dev->run_cmd("sendswtrigger");
}