#include <stdio.h>
#include "tmfe.h"
#include "fe_settings_strategy.h"
#include "vx2740_fe_class.h"

// Object for interacting with registers.
// Here we specify to use "group fe mode"
VX2740GroupFrontend vx_group(std::make_shared<VX2740FeSettingsODB>(), false);

class EqData : public TMFeEquipment {
public:
   EqData(const char* eqname, const char* eqfilename) : TMFeEquipment(eqname, eqfilename) {
      fEqConfReadConfigFromOdb = true;
      fEqConfEventID = 1;
      fEqConfBuffer = "";
      fEqConfPeriodMilliSec = 0; // in milliseconds
      fEqConfLogHistory = 0;
      fEqConfReadOnlyWhenRunning = true;
      fEqConfWriteEventsToOdb = false;
      fEqConfEnablePoll = true; // enable polled equipment

      fEvent = (char*)malloc(320*1024*1024);
   }

   ~EqData() {
      free(fEvent);
   }

   TMFeResult HandleBeginRun(int run_number) override {
      char error[255];
      INT status = vx_group.begin_of_run(run_number, error);

      if (status == SUCCESS) {
         return TMFeOk();
      } else {
         return TMFeResult(status, error);
      }
   };

   TMFeResult HandleEndRun(int run_number) override {
      char error[255];
      INT status = vx_group.end_of_run(run_number, error);

      if (status == SUCCESS) {
         return TMFeOk();
      } else {
         return TMFeResult(status, error);
      }
   };

   TMFeResult HandleStartAbortRun(int run_number) override {
      return HandleEndRun(run_number);
   }

   bool HandlePoll() override {
      /* Polling routine for events. Returns TRUE if event is available */
      return vx_group.is_event_ready();
   }

   void HandlePollRead() override {
      ComposeEvent(fEvent, sizeof(fEvent));
      vx_group.write_data(fEvent);
      EqSendEvent(fEvent);
   }

private:
   char* fEvent = nullptr;
};

class EqConfig : public TMFeEquipment {
public:
   EqConfig(const char* eqname, const char* eqfilename)  : TMFeEquipment(eqname, eqfilename) {
      fEqConfReadConfigFromOdb = true;
      fEqConfEventID = 103;
      fEqConfBuffer = "";
      fEqConfPeriodMilliSec = 6000;
      fEqConfLogHistory = 0;
      fEqConfReadOnlyWhenRunning = true;
      fEqConfWriteEventsToOdb = false;
   }

   void HandlePeriodic() override {
      char* buf = (char*)calloc(1000, sizeof(char));
      ComposeEvent(buf, sizeof(buf));
      vx_group.write_metadata(buf);
      EqSendEvent(buf);
   }
};

class EqErrors : public TMFeEquipment {
public:
   EqErrors(const char* eqname, const char* eqfilename) : TMFeEquipment(eqname, eqfilename) {
      fEqConfReadConfigFromOdb = true;
      fEqConfEventID = 105;
      fEqConfBuffer = "";
      fEqConfPeriodMilliSec = 1000;
      fEqConfLogHistory = 0;
      fEqConfReadOnlyWhenRunning = true;
      fEqConfWriteEventsToOdb = false;
   }

   void HandlePeriodic() override {
      char* buf = (char*)calloc(1000, sizeof(char));
      ComposeEvent(buf, sizeof(buf));
      vx_group.check_errors(buf);
      EqSendEvent(buf);
   }
};

class FeVX2740Group: public TMFrontend
{
public:
   FeVX2740Group() {
      /* register with the framework */
      FeSetName("VX2740_Group_%02d");
      FeAddEquipment(new EqConfig("VX2740_Config_Group_%03d", __FILE__));
      FeAddEquipment(new EqData("VX2740_Data_Group_%03d", __FILE__));
      FeAddEquipment(new EqErrors("VX2740_Errors_Group_%03d", __FILE__));
   }

   TMFeResult HandleArguments(const std::vector<std::string>& args) override { 
      if (fFeIndex < 0) {
         cm_msg(MERROR, __FUNCTION__, "You must run this frontend with the -i argument to specify the frontend index.");
         return TMFeResult(FE_ERR_DRIVER, "Specify frontend index >= 0");
      }
      
      return TMFeOk(); 
   };

   TMFeResult HandleFrontendInit(const std::vector<std::string>& args) override {
      INT status = vx_group.init(fFeIndex, fMfe->fDB);

      if (status != SUCCESS) {
         return TMFeResult(status, "Failed to init");
      }

      return TMFeOk();
   };
   
   TMFeResult HandleFrontendReady(const std::vector<std::string>& args) override {
      /* called after equipment HandleInit(), anything that needs to be done
       * before starting the main loop goes here */
      return TMFeOk();
   };
   
   void HandleFrontendExit() override {};
};

int main(int argc, char* argv[]) {
   FeVX2740Group fe;
   fe.fFeIndex = -1; // Default to -1 so we can see if user passed a -i argument or not
   return fe.FeMain(argc, argv);
}