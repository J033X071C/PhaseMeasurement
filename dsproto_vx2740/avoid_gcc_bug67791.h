#include <pthread.h>
#include <cstdio>

/*
 * This file helps you avoid a critical bug we can trigger in gcc:
 *      https://gcc.gnu.org/bugzilla/show_bug.cgi?id=67791
 *
 * The CAEN Dig2 library gets loaded dynamically by CAEN_FELib, and uses
 * threads. If the main executable DOES NOT link against pthread but DOES
 * link against libstdc++, then we trigger the above bug, and get a segfault
 * when Dig2 tries to spawn a thread.
 *
 * The bug affects many versions of gcc (including 7.5, which is on Ubuntu 20.04).
 * This bug does not affect clang.
 *
 * Your usage of pthread must be sufficient that the compiler doesn't optimize
 * it away and inject its own dummy pthread_create(). Simply creating a pthread_t
 * object is not enough to avoid the bug. It seems that you must actually spawn a
 * thread to avoid it...
 *
 * This namespace provides a simple function to help you spawn a thread that
 * doesn't do any real work (prints an empty string), but will not get optimized
 * out.
 *
 * Usage is as simple as putting this in your program: `avoid_gcc_bug67791::go()`.
 *
 * For Darkside, we do this automatically when creating a VX2740 object.
 */
namespace avoid_gcc_bug67791 {
   inline void *dummy_thread(void *arg) {
      printf("%s", "");
      return NULL;
   }

   inline void go() {
#ifdef __GNUC__
      // Only need to do anything if using gcc...
      pthread_t* pt = new pthread_t;
      pthread_create(pt, NULL, dummy_thread, NULL);
      pthread_join(*pt, NULL);
      delete pt;
   }
#endif
}
