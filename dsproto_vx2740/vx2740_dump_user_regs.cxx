#include "vx2740_wrapper.h"
#include "caen_exceptions.h"
#include <cstring>
#include <string>
#include <vector>
#include <map>

/**
 * This program connects to a VX2740 board and prints the value
 * of all user registers in a specified range.
 */

void usage(char *prog_name) {
   printf("Usage: %s <hostname> <start_hex> <end_hex>\n", prog_name);
   printf("   or: %s <hostname> --status-regs\n", prog_name);
   printf("E.g. : %s vx02 0x100 0x120\n", prog_name);
   printf("Start and end values are inclusive.\n");
}

int dump_status_regs(VX2740& vx);
int dump_user_regs(VX2740& vx, char **argv);

int main(int argc, char **argv) {
   bool any_help = false;
   bool status_regs = false;

   for (int i = 1; i < argc; i++) {
      if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
         any_help = true;
         break;
      }
      if (strcmp(argv[i], "--status-regs") == 0) {
         status_regs = true;
         break;
      }
   }

   if ((status_regs && argc != 3) || (!status_regs && argc != 4) || any_help) {
      usage(argv[0]);
      return 0;
   }

   VX2740 vx;
   vx.connect(argv[1], false, true);
   int retval = 0;

   if (status_regs) {
      retval = dump_status_regs(vx);
   } else {
      retval = dump_user_regs(vx, argv);
   }

   vx.disconnect();
   return retval;
}

int dump_status_regs(VX2740& vx) {
   // Status regs in 0x100, 0x800 and 0xA00 groups
   printf("+------+----------------+----------------+----------------+\n");
   printf("| Chan | Status (0x100) | DebugA (0x800) | DebugB (0xA00) |\n");
   printf("+------+----------------+----------------+----------------+\n");

   std::vector<int> bases = {0x100, 0x800, 0xA00};

   for (int i = 0; i < 64; i++) {
      printf("|   %02d |", i);

      for (auto& base : bases) {
         uint32_t val = 0xFFFFFFFF;

         try {
            vx.params().get_user_register(base + 4*i, val);
            printf("     0x%08x |", val);
         } catch (CaenException& e) {
            printf(" ERROR READING! |");
         }
      }

      printf("\n");
   }

   printf("+------+----------------+----------------+----------------+\n");

   return 0;
}

int dump_user_regs(VX2740& vx, char **argv) {
   uint32_t start = 0;
   uint32_t end = 0;

   if (sscanf(argv[2], "0x%x", &start) != 1) {
      printf("Failed to parse start register value. Format should look like 0x100 etc.\n");
      usage(argv[0]);
      return 1;
   }

   if (sscanf(argv[3], "0x%x", &end) != 1) {
      printf("Failed to parse end register value. Format should look like 0x100 etc.\n");
      usage(argv[0]);
      return 1;
   }

   if (start % 4 != 0 || end % 4 != 0) {
      printf("Start and end registers must be multiples of 4.\n");
      usage(argv[0]);
      return 1;
   }

   for (int reg = start; reg <= end; reg += 4) {
      uint32_t val = 0xFFFFFFFF;

      try {
         vx.params().get_user_register(reg, val);
         printf("0x%x = %u\n", reg, val);
      } catch(CaenException& e) {
         printf("Failed to read register 0x%x: %s\n", reg, e.what());
      }
   }

   return 0;
}
