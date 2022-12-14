#include "fe_utils.h"
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <tuple>
#include <chrono>
#include <vector>
#include <string>
#include <cstdarg>

void fe_utils::ts_printf(const char *format, ...) {
   // Handle va args for message
   va_list argptr;
   char message[10000];
   va_start(argptr, format);
   vsnprintf(message, 10000, (char *) format, argptr);
   va_end(argptr);

   // Handle timestamp
   std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

   // Get number of milliseconds for the current second
   int milli = (std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000).count();

   // Convert main timestamp to broken time for text formatting
   time_t timer = std::chrono::system_clock::to_time_t(now);
   tm buf;
   char formatted_time[80];
   strftime(formatted_time, 80, "%Y-%m-%d %H:%M:%S", localtime_r(&timer, &buf));

   // Do the actual print
   printf("%s.%03d %s", formatted_time, milli, message);
}

std::string fe_utils::format_bytes(int num_bytes) {
   double size = num_bytes;

   int idx = 0;
   std::vector<std::string> prefixes = {"", "ki", "Mi", "Gi", "Ti", "Pi"};

   while (size >= 1024) {
      idx++;
      size /= 1024;
   }

   char res[100];

   if (num_bytes < 1024) {
      // Integer exact bytes
      snprintf(res, 100, "%dB", num_bytes);
   } else {
      // 2 d.p. with prefix
      snprintf(res, 100, "%.2f%sB", size, prefixes[idx].c_str());
   }
   return res;
}