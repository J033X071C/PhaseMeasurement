#include "vx2740_wrapper.h"
#include "stdio.h"
#include <cstring>
#include <string>
#include <vector>
#include <map>

/**
 * This program connects to a VX2740 board, reads all the parameters that
 * are currently set, and prints them to screen. The user can control a
 * little bit of filtering and/or verbosity in the output.
 */

void usage(char *prog_name) {
   printf("Usage: %s <hostname> [-a] [-v] [<only_beneath_this_path>]\n", prog_name);
   printf("-a means dump all channels, not just channel 0.\n");
   printf("-v means print attributes of each node as well as the value.\n");
   printf("Hostname must be the first argument.\n");
   printf("E.g. : %s vx02\n", prog_name);
   printf("E.g. : %s vx02 -v /lvds/2\n", prog_name);
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
   vx.connect(argv[1], false, true);

   std::string beneath = "";
   bool only_params = true;
   bool all_chans = false;

   for (int i = 2; i < argc; i++) {
      std::string arg(argv[i]);

      if (arg == "-a") {
         all_chans = true;
      } else if (arg == "-v") {
         only_params = false;
      } else if (beneath == "") {
         beneath = arg;
      } else {
         usage(argv[0]);
         return 0;
      }
   }

   printf("%s\n", vx.params().get_param_list_human(beneath, all_chans, {}, only_params).c_str());

   return 0;
}
