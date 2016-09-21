/**
 * This File is part of openCompressBench
 * Copyright (C) 2016 Michael Strassberger <saremox@linux.com>
 *
 * openCompressBench is free software: you can redistribute
 * it and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation,
 * version 2 of the License.
 *
 * openCompressBench is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with openCompressBench.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @license GPL-2.0 <https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html>
 */


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
