#include "stdio.h"
#include "vx2740_wrapper.h"
#include "caen_exceptions.h"
#include <string>
#include <cstring>
#include <unistd.h>

void usage(char *prog_name) {
   printf("Lets you get/set VX2740 parameters and user registers\n\n");
   printf("Usage: %s <hostname> get <param_path_or_register>\n", prog_name);
   printf("       %s <hostname> set <param_path_or_register> <value>\n", prog_name);
   printf("       %s <hostname> swtrig <number_of_sw_triggers_to_issue>\n", prog_name);
   printf("E.g. : %s vx02 get /par/itlamajoritylev\n", prog_name);
   printf("E.g. : %s vx02 set /par/itlamajoritylev 3\n", prog_name);
   printf("E.g. : %s vx02 get 0x100\n", prog_name);
   printf("E.g. : %s vx02 set 0x100 255\n", prog_name);
   printf("E.g. : %s vx02 set 0x100 0xFF\n", prog_name);
   printf("E.g. : %s vx02 swtrig 3\n", prog_name);
   printf("E.g. : %s vx02 start_acq\n", prog_name);
   printf("E.g. : %s vx02 stop_acq\n", prog_name);
}

int main(int argc, char **argv) {
   if (argc < 3) {
      usage(argv[0]);
      return 0;
   }

   std::string hostname(argv[1]);
   std::string command(argv[2]);
   std::string path = argc > 3 ? std::string(argv[3]) : "";

   uint32_t reg_num = 0;
   bool is_reg = (path.find("0x") == 0);

   if (is_reg) {
      int n_parsed = sscanf(argv[3], "0x%x", &reg_num);

      if (n_parsed != 1) {
         printf("Failed to parse register number.\n");
         usage(argv[0]);
         return 0;
      }
   }

   VX2740 vx;
   vx.connect(argv[1], false, true);

   if (command == "get") {
      // Read value
      if (argc != 4) {
         usage(argv[0]);
         return 0;
      }

      if (is_reg) {
         vx.params().print_user_register(reg_num);
      } else {
         vx.params().print_param(path);
      }
   } else if (strcmp(argv[2], "set") == 0) {
      // Set, then readback
      if (argc != 5) {
         usage(argv[0]);
         return 0;
      }

      if (is_reg) {
         uint32_t set_val = 0;
         int n_parsed = sscanf(argv[4], "0x%x", &set_val);

         if (n_parsed == 0) {
            n_parsed = sscanf(argv[4], "%u", &set_val);

            if (n_parsed != 1) {
               printf("Failed to parse new register value.\n");
               usage(argv[0]);
               return 0;
            }
         }

         try {
            vx.params().set_user_register(reg_num, set_val);
            vx.params().print_user_register(reg_num);
         } catch(CaenException& e) {
            printf("Failed to set new register value: %s\n", e.what());
         }
      } else {
         try {
            vx.params().set(path, argv[4]);
            vx.params().print_param(path);
         } catch(CaenException& e) {
            printf("Failed to set new parameter value: %s\n", e.what());
         }
      }
   } else if (command == "swtrig")  {
      if (argc != 4) {
         usage(argv[0]);
         return 0;
      }

      uint32_t n_trig = 0;
      int n_parsed = sscanf(argv[3], "%u", &n_trig);

      if (n_parsed != 1) {
         printf("Failed to parse number of software triggers to issue.\n");
         usage(argv[0]);
         return 0;
      }

      for (uint32_t i = 0; i < n_trig; i++) {
         // Issue triggers at 100ms intervals
         printf("Issuing trigger %u\n", i);
         vx.commands().issue_software_trigger();
         usleep(100000);
      }
   } else if (command == "start_acq") {
      vx.commands().start_acq(vx.params().is_sw_start_enabled());
   } else if (command == "stop_acq") {
      vx.commands().stop_acq();
   } else {
      printf("Invalid command '%s'. Specify get/set/swtrig/start_acq/stop_acq.\n", command.c_str());
      usage(argv[0]);
      return 0;
   }

   return 0;
}
