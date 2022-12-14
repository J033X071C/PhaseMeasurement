#include "CAEN_FELib.h"
#include "stdio.h"
#include <string>
#include "avoid_gcc_bug67791.h"

/*
 * This program demonstrates a critical issue we can trigger in gcc:
 *      https://gcc.gnu.org/bugzilla/show_bug.cgi?id=67791
 *
 * The CAEN Dig2 library gets loaded dynamically by CAEN_FELib, and uses
 * threads. If the main executable DOES NOT link against pthread but DOES
 * link against libstdc++, then we trigger the above bug, and get a segfault
 * when Dig2 tries to spawn a thread.
 *
 * In this example code:
 * > SPAWN_THREAD = 0, USE_STRING = 0 : No crash, as we don't link either pthread or libstdc++
 * > SPAWN_THREAD = 0, USE_STRING = 1 : Crash, as we link libstdc++ but not pthread
 * > SPAWN_THREAD = 1, USE_STRING = 1 : No crash, as the main exe links pthread
 * > SPAWN_THREAD = 1, USE_STRING = 0 : No crash, as the main exe links pthread
 *
 * Your usage of pthread must be sufficient that the compiler doesn't inject its
 * own dummy pthread_create. Simply creating a pthread_t object is not enough to
 * avoid the bug. It seems that you must actually spawn a thread to avoid it...
 *
 * The bug affects many versions of gcc (including 7.5, which is on Ubuntu 20.04).
 * This bug does not affect clang.
 */

#define SPAWN_THREAD 1
#define USE_STRING 1

void usage(char *prog_name) {
   printf("Usage: %s <device_path>\n", prog_name);
   printf("E.g. : %s Dig2:vx02\n", prog_name);
}

#if SPAWN_THREAD
void *dummy_thread(void *arg) {
   printf("Spawned dummy thread\n");
   return NULL;
}
#endif

int main(int argc, char **argv) {
   if (argc != 2) {
      usage(argv[0]);
      return 0;
   }

#if SPAWN_THREAD
   avoid_gcc_bug67791::go();
#endif

#if USE_STRING
#if SPAWN_THREAD
   std::string s = "Hopefully will avoid segfault...";
#else
   std::string s = "Might segfault...";
#endif
   printf("%s\n", s.c_str());
#endif

   uint64_t dev_handle;
   int result = CAEN_FELib_Open(argv[1], &dev_handle);

   if (result == 0) {
      printf("Connected!\n");
   } else {
      char msg[1024];
      int got_err_msg = CAEN_FELib_GetLastError(msg);

      if (got_err_msg == CAEN_FELib_Success) {
         printf("%s\n", msg);
      } else {
         printf("Failed to get error message from CAEN FELib.\n");
      }
   }

   CAEN_FELib_Close(dev_handle);

   return 0;
}
