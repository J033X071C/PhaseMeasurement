#include "caen_device.h"
#include "CAEN_FELib.h"
#include "midas.h"
#include "avoid_gcc_bug67791.h"
#include <sstream>

CaenDevice::CaenDevice() {}

CaenDevice::~CaenDevice() {
   if (connected) {
      disconnect();
   }
}

void CaenDevice::set_debug(bool _debug) {
   debug = _debug;
}

std::string CaenDevice::get_last_error(std::string prefix) {
   char msg[1024];
   int got_err_msg = CAEN_FELib_GetLastError(msg);
   std::stringstream s;

   if (prefix == "") {
      prefix = "CAEN FELib operation failed";
   }

   if (got_err_msg == CAEN_FELib_Success) {
      s << prefix << ": " << msg;
   } else {
      s << prefix << ": Failed to get error message from CAEN FELib." << msg;
   }

   return s.str();
}

void CaenDevice::print_last_error(std::string prefix) {
   char msg[1024];
   int got_err_msg = CAEN_FELib_GetLastError(msg);

   if (prefix == "") {
      prefix = "CAEN FELib operation failed";
   }

   if (got_err_msg == CAEN_FELib_Success) {
      printf("%s: %s\n", prefix.c_str(), msg);
   } else {
      printf("%s: Failed to get error message from CAEN FELib.\n", prefix.c_str());
   }
}


INT CaenDevice::connect(std::string hostname, bool do_reset, bool monitor_only) {
   // Avoid bug that can cause us to segfault when using gcc.
   avoid_gcc_bug67791::go();

   std::string connection = "Dig2:" + hostname;

   if (monitor_only) {
      // Monitoring clients connect as "Dig2:hostname/?monitor";
      if (connection[connection.size() - 1] != '/') {
         connection += "/";
      }
      connection += "?monitor";
   }

   dev_path = hostname;

   int result = CAEN_FELib_Open(connection.c_str(), &dev_handle);

   if (result != CAEN_FELib_Success) {
      print_last_error("Failed to connect to " + connection);
      return FE_ERR_DRIVER;
   }

   if (do_reset) {
      if (reset() != SUCCESS) {
         return FE_ERR_DRIVER;
      }
   }

   connected = true;
   return SUCCESS;
}

INT CaenDevice::disconnect() {
   connected = false;

   int result = CAEN_FELib_Close(dev_handle);

   if (result != CAEN_FELib_Success) {
      print_last_error("Failed to disconnect from " + dev_path);
      return FE_ERR_DRIVER;
   }

   return SUCCESS;
}

bool CaenDevice::is_connected() {
   return connected;
}

INT CaenDevice::reset() {
   return run_cmd("reset");
}

uint64_t CaenDevice::get_root_handle() {
   return dev_handle;
}

INT CaenDevice::run_cmd(std::string cmd_name) {
   if (debug) {
      printf("Running cmd %s on %s\n", cmd_name.c_str(), dev_path.c_str());
   }

   std::string full_cmd_path = "/cmd/" + cmd_name;
   int ret = CAEN_FELib_SendCommand(dev_handle, full_cmd_path.c_str());

   if (ret == CAEN_FELib_Success) {
      return SUCCESS;
   } else {
      print_last_error("Failed to run command " + full_cmd_path);
      return FE_ERR_DRIVER;
   }
}