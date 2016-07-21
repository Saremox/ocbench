#include <unistd.h>
#include <mqueue.h>
#include <squash.h>

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

typedef struct {
  const char * codec;
  SquashOptions * options;
} ocschedJob;

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

/** STUB
  *
  */
ocschedStatus
ocsched_schedule_job(
  ocschedJob * job /**< [in] job which get scheduled */
);

/** STUB
  *
  */
ocschedStatus
ocsched_get_job(
  ocschedJob * job /**< [out] pointer in which we store the job */
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
