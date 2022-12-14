#ifndef FE_UTILS_H
#define FE_UTILS_H

#include <string>

namespace fe_utils {
   /**
    * Like printf, but automatically prepend a timestamp.
    */ 
   void ts_printf(const char *format, ...);

   /**
    * Return a string like "1.23GiB" for human-readable data sizes.
    */ 
   std::string format_bytes(int num_bytes);
};

#endif