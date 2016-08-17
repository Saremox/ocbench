// Inspired by http://c.learncodethehardway.org/book/ex20.html

#ifndef __dbg_h__
#define __dbg_h__

#include <stdio.h>
#include <errno.h>
#include <string.h>

enum verbosityLevels {
  OCDEBUG_ERROR = 0,
  OCDEBUG_WARN  = 1,
  OCDEBUG_INFO  = 2,
  OCDEBUG_DEBUG = 3
};

extern int verbosityLevel;

#ifdef NDEBUG
#define debug(M, ...)
#else
#define debug(M, ...) if(verbosityLevel >= OCDEBUG_DEBUG) fprintf(stderr,\
     "[DEBUG] (%s:%4d): " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#endif

#define clean_errno() (errno == 0 ? "None" : strerror(errno))

#define log_err(M, ...) if(verbosityLevel >= OCDEBUG_ERROR) fprintf(stderr,\
   "[ERROR] (%s:%4d): errno: %s) " M "\n",\
    __FILE__, __LINE__, clean_errno(), ##__VA_ARGS__)

#define log_warn(M, ...) if(verbosityLevel >= OCDEBUG_WARN) fprintf(stderr,\
   "[WARN]  (%s:%4d): errno: %s) " M "\n",\
    __FILE__, __LINE__, clean_errno(), ##__VA_ARGS__)

#define log_info(M, ...) if(verbosityLevel >= OCDEBUG_INFO) fprintf(stderr,\
   "[INFO]  (%s:%4d) " M "\n",\
    __FILE__, __LINE__, ##__VA_ARGS__)

#define check(A, M, ...) if(!(A)) { log_err(M, ##__VA_ARGS__);\
   errno=0; goto error; }

#define sentinel(M, ...)  { log_err(M, ##__VA_ARGS__); errno=0; goto error; }

#define check_mem(A) check((A), "Out of memory.")

#define check_debug(A, M, ...) if(!(A)) \
  { debug(M, ##__VA_ARGS__); errno=0; goto error; }

#endif
