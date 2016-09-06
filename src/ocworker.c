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

#include <stdlib.h>
#include <time.h>
#include <wait.h>
#include "ocworker.h"
#include "debug.h"
#include "ocsched/ocsched.h"
#include "ocutils/list.h"

#define MINIMUM_RUN_TIME 1000*1000

int got_killed = 0; // Used by worker processes

char* ocworker_serialize_job(ocworkerJob* job)
{
  char* fmt_str  = "BEGIN;%ld;%ld;%s;%ld;%ld;%ld;END";
  size_t strsize = snprintf(NULL,0,fmt_str,
    job->jobid,
    job->result->file_id->size,
    job->result->comp_id->codec_id->name,
    job->result->compressed_size,
    job->result->compressed_time,
    job->result->decompressed_time)+1;
  char* ret = malloc(strsize);
              snprintf(ret, strsize, fmt_str,
                job->jobid,
                job->result->file_id->size,
                job->result->comp_id->codec_id->name,
                job->result->compressed_size,
                job->result->compressed_time,
                job->result->decompressed_time);
  return ret;
}

ocworkerStatus ocworker_deserialize_job(ocworkerJob* job, char* serialized_str)
{
  char* copy = malloc(strlen(serialized_str)+1);
  strcpy(copy, serialized_str);
  char* curPos = strtok(copy, ";");
  if(strcmp("BEGIN", curPos) != 0)
  {
    log_warn("invalid serialized string \"%s\"",curPos);
    return OCWORKER_FAILURE;
  }

  job->jobid = atoi(strtok(NULL, ";"));

  int file_size = atoi(strtok(NULL, ";"));
  if(job->result == NULL)
    job->result = ocdata_new_result(NULL, NULL, 0, 0, 0);

  if(job->result->file_id == NULL)
    job->result->file_id  = ocdata_new_file(-1, NULL, file_size);

  char* codecname = strtok(NULL, ";");

  if(job->result->comp_id == NULL)
    job->result->comp_id = ocdata_new_comp(-1, NULL, NULL);

  if(job->result->comp_id->codec_id == NULL)
    job->result->comp_id->codec_id = ocdata_new_codec(-1, NULL, codecname);

  job->result->compressed_size   = atoi(strtok(NULL, ";"));
  job->result->compressed_time   = atoi(strtok(NULL, ";"));
  job->result->decompressed_time = atoi(strtok(NULL, ";"));

  curPos = strtok(NULL, ";");
  if(strcmp("END", curPos) != 0)
  {
    log_warn("invalid serialized string \"%s\"",curPos);
    return OCWORKER_FAILURE;
  }

  free(copy);

  return OCWORKER_OK;
}

void ocworker_worker_process_signalhandler(int signl)
{
  switch (signl) {
    case SIGINT:

      break;
    case OCWORKER_KILL_SIG:
      log_info("Worker[%d] shutting down",getpid());
      got_killed = 1;
      break;
    case SIGSEGV:
      log_warn("Worker[%d] got segfault",getpid());

      break;
  }
}

void ocworker_worker_process_loop(ocschedProcessContext* ctx, void* data)
{
  char*  name_fmt = "/worker[%d]-compressed";
  size_t namesize = snprintf(NULL,0,name_fmt,getpid())+1;
  char*  namecp   = malloc(namesize);
                    snprintf(namecp, namesize, name_fmt, getpid());

         name_fmt = "/worker[%d]-decompressed";
         namesize = snprintf(NULL,0,name_fmt,getpid())+1;
  char*  namedcp  = malloc(namesize);
                    snprintf(namedcp, namesize, name_fmt, getpid());

  signal(SIGSEGV, ocworker_worker_process_signalhandler);
  signal(SIGINT, ocworker_worker_process_signalhandler);
  signal(OCWORKER_KILL_SIG, ocworker_worker_process_signalhandler);

  ocMemfdContext* orignal = (ocMemfdContext *) data;
  ocMemfdContext* compressed   = ocmemfd_create_context(namecp,4096);
  ocMemfdContext* decompressed = ocmemfd_create_context(namedcp,orignal->size);

  char   recvBuf[4096];

  while (true) {
    if(got_killed == 1)
      break;

    memset(recvBuf, 0, 4096);
    int readBytes = ocsched_recvfrom(ctx, recvBuf, 4096);
    if(readBytes > 0)
    {
      ocworkerJob* recvjob = calloc(1, sizeof(ocworkerJob));
      debug("recieved \"%s\"",recvBuf);
      ocworker_deserialize_job(recvjob, recvBuf);
      // resize shared memfd buffer to file size
      int oldsize = orignal->size;
      orignal->size = recvjob->result->file_id->size;
      ocmemfd_remap_buffer(orignal,oldsize);

      SquashCodec* codec =
        squash_get_codec(recvjob->result->comp_id->codec_id->name);
      check(codec != NULL, "squash cannot find \"%s\" codec",
        recvjob->result->comp_id->codec_id->name);

      ocmemfd_resize(decompressed, orignal->size);
      ocmemfd_resize(compressed,
        squash_codec_get_max_compressed_size(codec,orignal->size));
      recvjob->result->compressed_size = compressed->size;

      recvjob->result->compressed_time = 0;
      int iterations = 0;

      for ( ; recvjob->result->compressed_time < MINIMUM_RUN_TIME; iterations++) {
        struct timespec begin,end;

        clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&begin);
        int ret = squash_codec_compress(codec,
                                        &recvjob->result->compressed_size,
                                        compressed->buf,
                                        orignal->size,
                                        orignal->buf,
                                        NULL);
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&end);

        int64_t secs = end.tv_sec - begin.tv_sec;
        int64_t usecs = end.tv_nsec - begin.tv_nsec;

        recvjob->result->compressed_time += secs*1000*1000 + usecs/1000;

        check(ret == SQUASH_OK,"failed to compress data [%d] : %s",
          ret,squash_status_to_string(ret));
      }

      recvjob->result->compressed_time /= iterations;
      iterations = 0;

      for ( ; recvjob->result->decompressed_time < MINIMUM_RUN_TIME; iterations++) {
        struct timespec begin,end;

        clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&begin);
        int ret = squash_codec_decompress(codec,
                                        &decompressed->size,
                                        decompressed->buf,
                                        compressed->size,
                                        compressed->buf,
                                        NULL);
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&end);

        int64_t secs = end.tv_sec - begin.tv_sec;
        int64_t usecs = end.tv_nsec - begin.tv_nsec;

        recvjob->result->decompressed_time += secs*1000*1000 + usecs/1000;

        check(ret == SQUASH_OK,"failed to compress data [%d] : %s",
          ret,squash_status_to_string(ret));
      }

      recvjob->result->decompressed_time /= iterations;

      // Early resource freeing
      ocmemfd_resize(compressed, 4096);
      ocmemfd_resize(decompressed, 4096);

      char* sendbuf = ocworker_serialize_job(recvjob);
      ocsched_printf(ctx, sendbuf);
      debug("Send \"%s\" from %d",sendbuf,ctx->pid);
      free(sendbuf);
      free(recvjob);

      ocdata_garbage_collect();
    }
    else
    {
      struct timespec sleeptimer = {0,25000000};
      nanosleep(&sleeptimer,NULL);
    }
    error:
      continue;
  }
  ocdata_garbage_collect();
  ocmemfd_destroy_context(&compressed);
  ocmemfd_destroy_context(&orignal);
  free(namecp);
  free(namedcp);
  exit(EXIT_SUCCESS);
}

void* ocworker_worker_watchdog_loop(void* data)
{
  ocworkerWatchdog* wdctx  = (ocworkerWatchdog*) data;
  ocworker*         worker = wdctx->myworker;

  worker->ctx =
    ocsched_fork_process( ocworker_worker_process_loop,"worker",
                          wdctx->ctx->memfd);

  char*  name_fmt = "Watchdog[%d]";
  size_t namesize = snprintf(NULL,0,name_fmt,worker->ctx->pid)+1;
  char*  name     = malloc(namesize);
                    snprintf(name, namesize, name_fmt, worker->ctx->pid);

  char   recvBuf[4096];

  pthread_setname_np(pthread_self(),name);
  while (true) {
    // Watchdog
    int child_status = 0;
    waitpid(worker->ctx->pid,&child_status,WNOHANG);

    if(worker->status == OCWORKER_KILL_SIG)
    {
      log_info("Watchdog[%d] got kill signal",worker->ctx->pid);
      kill(worker->ctx->pid,OCWORKER_KILL_SIG);

      waitpid(worker->ctx->pid, &child_status, 0);
      goto killWatchdog;
    }

    if(WIFSIGNALED(child_status))
    {
      log_warn("Watchdog[%d] CHILD HUNG: %d CODEC: %s JOBID: %ld",
        worker->ctx->pid,
        WTERMSIG(child_status),
        worker->cur_job->result->comp_id->codec_id->name,
        worker->cur_job->jobid);
      // Try to reanimate child
      worker->next_job = worker->cur_job;
      worker->cur_job  = NULL;
      worker->ctx =
        ocsched_fork_process( ocworker_worker_process_loop,"worker",
                              wdctx->ctx->memfd);
    }
    // Job transfer
    if (worker->cur_job == NULL && worker->next_job != NULL &&
        wdctx->ctx->loadedfile == worker->next_job->result->file_id) {
      debug("%s: Next Job is File \"%s\" with \"%s\" as codec",
        name,
        worker->next_job->result->file_id->path,
        worker->next_job->result->comp_id->codec_id->name);
      worker->cur_job = worker->next_job;
      worker->next_job = NULL;
      char* sendbuf = ocworker_serialize_job(worker->cur_job);
      ocsched_printf(worker->ctx, sendbuf);
      debug("Send \"%s\" to %d",sendbuf,worker->ctx->pid);
      free(sendbuf);
    }
    else if(worker->cur_job != NULL)
    {
      memset(recvBuf, 0, 4096);
      if(ocsched_recvfrom(worker->ctx, recvBuf, 4096) > 0)
      {
        debug("recieved \"%s\"",recvBuf);
        ocworker_deserialize_job(worker->cur_job, recvBuf);
        worker->last_job = worker->cur_job;
        worker->cur_job = NULL;
      }
      else
      {
        struct timespec sleeptimer = {0,25000000};
        nanosleep(&sleeptimer,NULL);
      }
    }
    else if(worker->next_job == NULL)
    {
      struct timespec sleeptimer = {0,25000000};
      nanosleep(&sleeptimer,NULL);
    }
  }
killWatchdog:
  ocsched_destroy_context(&worker->ctx);
  free(wdctx);
  free(name);
  pthread_exit(EXIT_SUCCESS);
}


void* ocworker_schedule_worker(void* data)
{
  ocworkerContext* ctx = (ocworkerContext*) data;\

  pthread_setname_np(pthread_self(),"Job Scheduler");

  while (true) {
    struct timespec sleeptimer = {0,25000000};
    nanosleep(&sleeptimer,NULL);

    int running_jobs = 0;
    pthread_mutex_lock(&ctx->lock);
    ocutils_list_foreach_f(ctx->worker, curworker, ocworker*)
    {
      if (curworker->cur_job != NULL)
        running_jobs++;
    }
    if(running_jobs != 0)
      goto unlock_mutex;


    for (size_t i = 0; i < ctx->worker->items; i++)
    {
      ocworker* worker = (ocworker*)    ocutils_list_get(ctx->worker, i);
      if(worker->last_job != NULL)
      {
        ocutils_list_add(ctx->jobsDone, worker->last_job);
        worker->last_job = NULL;
      }
    }

    ListNode* job =  ocutils_list_head(ctx->jobs);
    if (job != NULL) {
      // Check if we need to load a new file into our memfd
      if (((ocworkerJob*)job->value)->result->file_id != ctx->loadedfile) {
        ocmemfd_load_file(ctx->memfd, ((ocworkerJob*)job->value)->result->file_id->path);
        ctx->loadedfile = ((ocworkerJob*)job->value)->result->file_id;
      }
    }
    else
    {
      // No jobs are in queue continue waiting
      goto unlock_mutex;
    }

    for (size_t i = 0; i < ctx->worker->items; i++) {
      ocworkerJob* job = (ocworkerJob*) ocutils_list_dequeue(ctx->jobs);
      ocworker* worker = (ocworker*)    ocutils_list_get(ctx->worker, i);
      worker->next_job = job;
    }
    unlock_mutex:
    pthread_mutex_unlock(&ctx->lock);
  }
}

/***********************/
/* Interface functions */
/***********************/

ocworkerStatus
ocworker_start(int worker_amt, ocworkerContext** ctx)
{
  ocworkerContext* myctx = calloc(1,sizeof(ocworkerContext));

  myctx->jobs     = ocutils_list_create();
  myctx->worker   = ocutils_list_create();
  myctx->jobsDone = ocutils_list_create();
  myctx->memfd    = ocmemfd_create_context("/loadedfile", 1024);

  for (size_t i = 0; i < worker_amt; i++) {
    ocworkerWatchdog* wdctx     = calloc(1, sizeof(ocworkerWatchdog));
    ocworker*         newworker = calloc(1,sizeof(ocworker));
    wdctx->ctx      = myctx;
    wdctx->myworker = newworker;
    ocutils_list_add(myctx->worker, newworker);
    pthread_create(&newworker->watchdog, NULL,
      ocworker_worker_watchdog_loop, wdctx);
  }

  pthread_mutex_init(&myctx->lock, NULL);

  pthread_create(&myctx->scheduler, NULL, ocworker_schedule_worker, myctx);

  *ctx = myctx;

  return OCWORKER_OK;
}

ocworkerStatus
ocworker_schedule_job(ocworkerContext*  ctx, ocdataFile* file,
  ocdataCodec* codec, int64_t* jobid)
{
  ocworkerJob* newjob = calloc(1, sizeof(ocworkerJob));

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
ocworker_schedule_jobs(ocworkerContext* ctx, ocdataFile* file,
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
ocworker_retrieve_job(ocworkerContext* ctx, int64_t jobid, ocworkerJob** job)
{
  return OCWORKER_OK;
}

ocworkerStatus
ocworker_retrieve_jobs(ocworkerContext* ctx, List* jobids, List** jobs)
{
  return OCWORKER_OK;
}

ocworkerStatus
ocworker_unref_job(ocworkerContext* ctx, ocworkerJob** job)
{
  return OCWORKER_OK;
}

ocworkerStatus
ocworker_is_running(ocworkerContext* ctx)
{
  int ret = OCWORKER_NO_JOBS;
  pthread_mutex_lock(&ctx->lock);
  if(ctx->jobs->items > 0)
    ret = OCWORKER_IS_RUNNING;
  ocutils_list_foreach_f(ctx->worker, worker, ocworker*)
  {
    if(worker->cur_job != NULL)
      ret = OCWORKER_IS_RUNNING;
  }
  pthread_mutex_unlock(&ctx->lock);
  return ret;
}

ocworkerStatus
ocworker_kill(ocworkerContext*  ctx)
{
  log_info("Shutting down worker");
  ocutils_list_foreach_f(ctx->worker, worker, ocworker*)
  {
    worker->status = OCWORKER_KILL_SIG;

    pthread_join(worker->watchdog, NULL);
  }
  pthread_cancel(ctx->scheduler);

  ocutils_list_freefp(ctx->worker, free);
  ocutils_list_destroy(ctx->worker);

  return OCWORKER_OK;
}
