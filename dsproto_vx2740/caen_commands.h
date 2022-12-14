#ifndef CAEN_COMMANDS_H
#define CAEN_COMMANDS_H

#include "midas.h"
#include "caen_device.h"
#include <inttypes.h>
#include <stdlib.h>
#include <map>
#include <memory>
#include <exception>

class CaenCommands {
   public:
      CaenCommands() {}
      ~CaenCommands() {}

      void set_device(std::shared_ptr<CaenDevice> _dev) {
         dev = _dev;
      }

      // Enable/disable triggering. start_acq() resets trigger/clock counts etc.
      // start_acq() will always arm the acquisition. It will only actually start the acquisition
      // if the "sw" param was true when calling set_start_sources().
      INT start_acq(bool sw_start);
      INT stop_acq();

      // Issue a software trigger.
      INT issue_software_trigger();

   protected:
      std::shared_ptr<CaenDevice> dev = nullptr;
      bool debug = false;
};

#endif