/**
 * This File is part of openCompressBench
 * Copyright (C) 2016 Michael Strassberger <saremox@linux.com>
 *
 * openCompressBench is free software: you can redistribute
 * it and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation,
 * version 2 of the License.
 *
 * Some open source application is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @license GPL-2.0 <https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html>
 */

#include <unistd.h>
#include <mqueue.h>

#ifndef OCSCHEDH
#define OCSCHEDH

typedef struct {
  int pipe_in;
  int pipe_out;
} ocschedCommunicator;

typedef struct {
  pid_t pid;
  char* name;
  mqd_t workQueue;
  ocschedCommunicator comm;
} ocschedProcessContext;

typedef void (*ocschedFunction)(ocschedProcessContext*,void*);

typedef enum {
  OCSCHED_SUCCESS,
  OCSCHED_FAILURE = -1
} ocschedStatus;

/** forks the process, asign childname as new processname
  * and calls work_function for child
  */
ocschedProcessContext *
ocsched_fork_process(
  ocschedFunction work_function, /**< [in] function thats called by the child */
  char* childname, /**< [in] process name of the child */
  void* data /**< [in] pointer with addional data for the child */
);

/** send n bytes of buf to the given valid context
  */
size_t /**< [out] bytes written or OCSCHED_FAILURE on failure */
ocsched_sendto(
  ocschedProcessContext * ctx, /**< [in] context which will be send to */
  char * buf, /**< [in] buffer which will be send */
  size_t n /**< [in] number of bytes which will be send */
);

/** printf wrapper for usage with ocschedProcessContext
  * fmtstr behaved like fmtstr in printf
  */
ocschedStatus
ocsched_printf(
  ocschedProcessContext * ctx, /**< [in] context which will be send to */
  char * fmtstr,
  ...
);

/** revc n bytes into buf from the given valid context
  *
  */
size_t /**< [out] bytes readed or OCSCHED_FAILURE on failure */
ocsched_recvfrom(
  ocschedProcessContext * ctx, /**< [in] context which will be recv from */
  char * buf, /**< [out] buffer in which will be recv */
  size_t n /**< [in] maximum byte count which will be recv */
);

/** clean up context. closes open file descriptors and free's allocated memory
  * only the parent need to call this function. children get automatically
  * cleaned up
  */
ocschedStatus
ocsched_destroy_context(
  ocschedProcessContext ** ctx /**< [in] context which will be recv from */
);

/** unlinks the global ipc message queue
  */
ocschedStatus
ocsched_destroy_global_mqueue();
#endif
