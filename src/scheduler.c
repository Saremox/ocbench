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

#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <wait.h>
#include "debug.h"
#include "job.h"
#include "scheduler.h"
#include "ocsched/ocsched.h"
#include "ocutils/list.h"
#include "watchdog.h"
#include "worker.h"

void* ocworker_schedule_worker(void* data)
{
  schedulerContext* ctx = (schedulerContext*) data;\

  pthread_setname_np(pthread_self(),"Job Scheduler");

  while (true) {
    struct timespec sleeptimer = {0,25000000};
    nanosleep(&sleeptimer,NULL);

    int running_jobs = 0;
    pthread_mutex_lock(&ctx->lock);
    if(ctx->watchdogs == NULL)
      goto unlock_mutex;
    ocutils_list_foreach_f(ctx->watchdogs, curwatchdog, WatchdogContext*)
    {
      if (curwatchdog->myworker->cur_job != NULL)
        running_jobs++;
    }
    if(running_jobs != 0)
      goto unlock_mutex;


    for (size_t i = 0; i < ctx->watchdogs->items; i++)
    {
      WatchdogContext* watchdog = (WatchdogContext*) ocutils_list_get(ctx->watchdogs, i);
      if(watchdog->myworker->last_job != NULL)
      {
        ocutils_list_add(ctx->jobsDone, watchdog->myworker->last_job);
        watchdog->myworker->last_job = NULL;
      }
    }

    ListNode* job =  ocutils_list_head(ctx->jobs);
    if (job != NULL) {
      // Check if we need to load a new file into our memfd
      if (((Job*)job->value)->result->file_id != ctx->loadedfile) {
        ocmemfd_load_file(ctx->memfd, ((Job*)job->value)->result->file_id->path);
        ctx->loadedfile = ((Job*)job->value)->result->file_id;
      }
    }
    else
    {
      // No jobs are in queue continue waiting
      goto unlock_mutex;
    }

    for (size_t i = 0; i < ctx->watchdogs->items; i++) {
      Job*      job       = (Job*) ocutils_list_dequeue(ctx->jobs);
      WatchdogContext* watchdog  = (WatchdogContext*)    ocutils_list_get(ctx->watchdogs, i);
      watchdog->myworker->next_job = job;
    }
    unlock_mutex:
    pthread_mutex_unlock(&ctx->lock);
  }
}

/***********************/
/* Interface functions */
/***********************/

ocworkerStatus
ocworker_start(int worker_amt, schedulerContext** ctx)
{
  schedulerContext* myctx = calloc(1,sizeof(schedulerContext));

  myctx->jobs     = ocutils_list_create();
  myctx->watchdogs= ocutils_list_create();
  myctx->jobsDone = ocutils_list_create();
  myctx->memfd    = ocmemfd_create_context("/loadedfile", 1024);

  for (size_t i = 0; i < worker_amt; i++) {
    WatchdogContext* wdctx     = calloc(1, sizeof(WatchdogContext));
    workerContext*    newworker = calloc(1,sizeof(workerContext));
    wdctx->ctx      = myctx;
    wdctx->myworker = newworker;
    ocutils_list_add(myctx->watchdogs, wdctx);
    pthread_create(&wdctx->watchdog, NULL,
      watchdog_main, wdctx);
  }

  pthread_mutex_init(&myctx->lock, NULL);

  pthread_create(&myctx->scheduler, NULL, ocworker_schedule_worker, myctx);

  *ctx = myctx;

  return OCWORKER_OK;
}

ocworkerStatus
ocworker_schedule_job(schedulerContext*  ctx, ocdataFile* file,
  ocdataCodec* codec, int64_t* jobid)
{
  Job* newjob = calloc(1, sizeof(Job));

  newjob->result          = ocdata_new_result(NULL, file, 0, 0, 0);
  newjob->result->comp_id = ocdata_new_comp(-1, codec, ocutils_list_create());
  newjob->jobid           = ++(ctx->lastjobid);
  ocutils_list_add(newjob->result->comp_id->options,
    ocdata_new_comp_option(newjob->result->comp_id, "default", "yes"));
  pthread_mutex_lock(&ctx->lock);
  ocutils_list_enqueue(ctx->jobs, newjob);
  pthread_mutex_unlock(&ctx->lock);

  *jobid = newjob->jobid;

  return OCWORKER_SCHEDULED;
}

ocworkerStatus
ocworker_schedule_jobs(schedulerContext* ctx, ocdataFile* file,
  List* codecs, List** jobids)
{
  List* jobs = ocutils_list_create();
  ocutils_list_foreach_f(codecs, codec, ocdataCodec*)
  {
    int64_t jobid;
    ocworker_schedule_job(ctx, file, codec, &jobid);
    ocutils_list_add(jobs, (void*) jobid);
  }

  *jobids = jobs;

  return OCWORKER_SCHEDULED;
}

ocworkerStatus
ocworker_retrieve_job(schedulerContext* ctx, int64_t jobid, Job** job)
{
  return OCWORKER_OK;
}

ocworkerStatus
ocworker_retrieve_jobs(schedulerContext* ctx, List* jobids, List** jobs)
{
  return OCWORKER_OK;
}

ocworkerStatus
ocworker_unref_job(schedulerContext* ctx, Job** job)
{
  return OCWORKER_OK;
}

ocworkerStatus
ocworker_is_running(schedulerContext* ctx)
{
  int ret = OCWORKER_NO_JOBS;
  pthread_mutex_lock(&ctx->lock);
  if(ctx->jobs->items > 0)
    ret = OCWORKER_IS_RUNNING;
  ocutils_list_foreach_f(ctx->watchdogs, watchdog, WatchdogContext*)
  {
    if(watchdog->myworker->cur_job != NULL)
      ret = OCWORKER_IS_RUNNING;
  }
  pthread_mutex_unlock(&ctx->lock);
  return ret;
}

ocworkerStatus
ocworker_kill(schedulerContext*  ctx)
{
  log_info("Shutting down worker");
  pthread_mutex_lock(&ctx->lock);
  ocutils_list_foreach_f(ctx->watchdogs, watchdog, WatchdogContext*)
  {
    watchdog->myworker->status = WORKER_KILL_SIG;

    pthread_join(watchdog->watchdog, NULL);
  }
  pthread_cancel(ctx->scheduler);

  ocutils_list_destroy(ctx->watchdogs);
  ctx->watchdogs = NULL;

  pthread_mutex_unlock(&ctx->lock);

  return OCWORKER_OK;
}

ocworkerStatus
ocworker_force_kill(schedulerContext* ctx)
{
  ocutils_list_foreach_f(ctx->watchdogs, watchdog, WatchdogContext*)
  {
    kill(watchdog->myworker->ctx->pid, SIGKILL);
  }
}
