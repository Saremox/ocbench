#ifndef OCSCHEDH
#define OCSCHEDH

typedef struct {
  int pipe_in;
  int pipe_out;
} ocschedCommunicator;

typedef struct {
  int pid;
  char* name;
  ocschedCommunicator comm;
} ocschedProcessContext;

typedef void (*ocschedFunction)(ocschedProcessContext*,void*);

typedef enum {
  OCSCHED_SUCCESS
} ocschedStatus;

ocschedProcessContext *
ocsched_fork_process(
  ocschedFunction work_function, /**< [in] function thats called by the child */
  char* childname, /**< [in] process name of the child */
  void* data /**< [in] pointer with addional data for the child */
);





#endif
