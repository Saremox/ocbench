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

ocworkerStatus
ocworker_force_kill(
  ocworkerContext*  ctx
);
