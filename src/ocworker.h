#include "ocsched/ocsched.h"
#include "ocdata/ocdata.h"
#include "ocmemfd/ocmemfd.h"
#include <pthread.h>

struct __ocworkerWatchdog;
struct __ocworkerContext;
struct __ocworkerJob;
struct __ocworker;

typedef struct __ocworkerWatchdog ocworkerWatchdog;
typedef struct __ocworkerContext  ocworkerContext;
typedef struct __ocworkerJob      ocworkerJob;
typedef struct __ocworker         ocworker;

#define OCWORKER_KILL_SIG 42

struct __ocworkerContext{
  int64_t         lastjobid;
  List*           jobs;
  List*           jobsDone;
  List*           worker;
  ocdataFile*     loadedfile;
  ocMemfdContext* memfd;
  pthread_t       scheduler;
  pthread_mutex_t lock;
};

struct __ocworkerJob{
  int64_t         jobid;
  ocdataResult*   result;
};

struct __ocworker{
  ocschedProcessContext*  ctx;
  ocworkerJob*            next_job;
  ocworkerJob*            cur_job;
  ocworkerJob*            last_job;
  int64_t                 status;
  pthread_t               watchdog;
};

struct __ocworkerWatchdog{
  ocworkerContext*  ctx;
  ocworker*         myworker;
};

typedef enum __ocworkerStatus{
  OCWORKER_FAILURE = -1,
  OCWORKER_OK,
  OCWORKER_IS_RUNNING,
  OCWORKER_NO_JOBS,
  OCWORKER_SCHEDULED,
  OCWORKER_NOT_EXECUTED,
} ocworkerStatus;

ocworkerStatus
ocworker_start(
  int               worker_amt,
  ocworkerContext** ctx
);

ocworkerStatus
ocworker_schedule_job(
  ocworkerContext*  ctx,
  ocdataFile*       file,
  ocdataCodec*      codec,
  int64_t*          jobid
);

ocworkerStatus
ocworker_schedule_jobs(
  ocworkerContext*  ctx,
  ocdataFile*       file,
  List*             codecs,
  List**            jobids
);


ocworkerStatus
ocworker_retrieve_job(
  ocworkerContext*  ctx,
  int64_t           jobid,
  ocworkerJob**     job
);

ocworkerStatus
ocworker_retrieve_jobs(
  ocworkerContext*  ctx,
  List*             jobids,
  List**            jobs
);

ocworkerStatus
ocworker_unref_job(
  ocworkerContext*  ctx,
  ocworkerJob**     job
);

ocworkerStatus
ocworker_is_running(
  ocworkerContext* ctx
);

ocworkerStatus
ocworker_kill(
  ocworkerContext*  ctx
);

void child_process(ocschedProcessContext* parent, void* data);
