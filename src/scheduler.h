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

#include "job.h"
#include "ocsched/ocsched.h"
#include "ocmemfd/ocmemfd.h"
#include <pthread.h>

#ifndef OCWORKER_H
#define OCWORKER_H

struct __schedulerContext;

typedef struct __schedulerContext  schedulerContext;


struct __schedulerContext{
  int64_t         lastjobid;
  List*           jobs;
  List*           jobsDone;
  List*           watchdogs;
  ocdataFile*     loadedfile;
  ocMemfdContext* memfd;
  pthread_t       scheduler;
  pthread_mutex_t lock;
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
  schedulerContext** ctx
);

ocworkerStatus
ocworker_schedule_job(
  schedulerContext*  ctx,
  ocdataFile*       file,
  ocdataCodec*      codec,
  int64_t*          jobid
);

ocworkerStatus
ocworker_schedule_jobs(
  schedulerContext*  ctx,
  ocdataFile*       file,
  List*             codecs,
  List**            jobids
);


ocworkerStatus
ocworker_retrieve_job(
  schedulerContext*  ctx,
  int64_t           jobid,
  Job**     job
);

ocworkerStatus
ocworker_retrieve_jobs(
  schedulerContext*  ctx,
  List*             jobids,
  List**            jobs
);

ocworkerStatus
ocworker_unref_job(
  schedulerContext*  ctx,
  Job**     job
);

ocworkerStatus
ocworker_is_running(
  schedulerContext* ctx
);

ocworkerStatus
ocworker_kill(
  schedulerContext*  ctx
);

ocworkerStatus
ocworker_force_kill(
  schedulerContext*  ctx
);


#endif
