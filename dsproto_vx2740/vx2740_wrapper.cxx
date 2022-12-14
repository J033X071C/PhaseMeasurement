#include <stdlib.h>
#include <inttypes.h>
#include <stdio.h>
#include <cstring>
#include <arpa/inet.h>
#include "midas.h"
#include "CAEN_FELib.h"
#include "avoid_gcc_bug67791.h"
#include "caen_event.h"
#include "caen_exceptions.h"
#include "vx2740_wrapper.h"

INT VX2740::connect(std::string hostname, bool do_reset, bool monitor_only) {
   // Avoid bug that can cause us to segfault when using gcc.
   avoid_gcc_bug67791::go();

   dev = std::make_shared<CaenDevice>();
   dev->connect(hostname, do_reset, monitor_only);

   params_helper.set_device(dev);
   data_helper.set_device(dev);
   commands_helper.set_device(dev);

   std::string model_name, serial_num, fw_ver, fw_type;

   try {
      params_helper.get_firmware_type(fw_type);
      params_helper.get_firmware_version(fw_ver);
      params_helper.get_model_name(model_name);
      params_helper.get_serial_number(serial_num);
   } catch (CaenException& e) {
      return FE_ERR_DRIVER;
   }

   if (model_name != "VX2740" && model_name != "VX2745") {
      printf("Connected to a model %s not a VX2740/VX2745!", model_name.c_str());
      return FE_ERR_DRIVER;
   }

   printf("Connected to %s #%s, running %s FW version %s\n", model_name.c_str(), serial_num.c_str(), fw_type.c_str(), fw_ver.c_str());

   return SUCCESS;
}
