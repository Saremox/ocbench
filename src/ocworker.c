#include <stdlib.h>
#include <time.h>
#include "ocworker.h"
#include "debug.h"
#include "ocsched/ocsched.h"
#include "ocutils/list.h"

char* ocworker_serialize_job(ocworkerJob* job)
{
  char* fmt_str  = "BEGIN;%ld;%ld;%s;%ld;%ld;END";
  size_t strsize = snprintf(NULL,0,fmt_str,job->jobid,job->file->size,job->codec->name,
    job->compressed,job->time_needed)+1;
  char* ret = malloc(strsize);
              snprintf(ret, strsize, fmt_str,job->jobid,job->file->size,job->codec->name,
                job->compressed,job->time_needed);
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
  if(job->file == NULL)
  {
    job->file = malloc(sizeof(ocdataFile));
    job->file->size = file_size;
  }

  char* codecname = strtok(NULL, ";");
  if(job->codec == NULL)
  {
    job->codec = malloc(sizeof(ocdataCodec));
    job->codec->name = malloc(strlen(codecname)+1);
                       strcpy(job->codec->name, codecname);
  }

  job->compressed   = atoi(strtok(NULL, ";"));
  job->time_needed  = atoi(strtok(NULL, ";"));

  curPos = strtok(NULL, ";");
  if(strcmp("END", curPos) != 0)
  {
    log_warn("invalid serialized string \"%s\"",curPos);
    return OCWORKER_FAILURE;
  }

  return OCWORKER_OK;
}

void ocworker_worker_process_loop(ocschedProcessContext* ctx, void* data)
{
  char*  name_fmt = "/worker[%d]";
  size_t namesize = snprintf(NULL,0,name_fmt,ctx->pid)+1;
  char*  name     = malloc(namesize);
                    snprintf(name, namesize, name_fmt, ctx->pid);

  ocMemfdContext* decompressed = (ocMemfdContext *) data;
  ocMemfdContext* compressed   = ocmemfd_create_context(name,1024);

  char   recvBuf[4096];

  while (true) {
    struct timespec sleeptimer = {0,25000000};
    nanosleep(&sleeptimer,NULL);

    memset(recvBuf, 0, 4096);
    int readBytes = ocsched_recvfrom(ctx, recvBuf, 4096);
    if(readBytes > 0)
    {
      ocworkerJob* recvjob = calloc(1, sizeof(ocworkerJob));
      debug("recieved \"%s\"",recvBuf);
      ocworker_deserialize_job(recvjob, recvBuf);
      // resize shared memfd buffer to file size
      int oldsize = decompressed->size;
      decompressed->size = recvjob->file->size;
      ocmemfd_remap_buffer(decompressed,oldsize);

      SquashCodec* codec = squash_get_codec(recvjob->codec->name);
      check(codec != NULL,
        "squash cannot find \"%s\" codec",recvjob->codec->name);

      ocmemfd_resize(compressed,
        squash_codec_get_max_compressed_size(codec,decompressed->size));
      recvjob->compressed = compressed->size;

      int ret = squash_codec_compress(codec,
                                      &recvjob->compressed,
                                      compressed->buf,
                                      decompressed->size,
                                      decompressed->buf,
                                      NULL);
      check(ret == SQUASH_OK,"failed to compress data [%d] : %s",
        ret,squash_status_to_string(ret));
      char* sendbuf = ocworker_serialize_job(recvjob);
      ocsched_printf(ctx, sendbuf);
      debug("Send \"%s\" from %d",sendbuf,ctx->pid);
      free(sendbuf);
      ocdata_free_codec(recvjob->codec);
    }
    error:
      continue;
  }
}

void* ocworker_worker_watchdog_loop(void* data)
{
  ocworkerWatchdog* wdctx  = (ocworkerWatchdog*) data;
  ocworker*         worker = wdctx->myworker;

  char*  name_fmt = "Watchdog[%d]";
  size_t namesize = snprintf(NULL,0,name_fmt,worker->ctx->pid)+1;
  char*  name     = malloc(namesize);
                    snprintf(name, namesize, name_fmt, worker->ctx->pid);

  char   recvBuf[4096];

  pthread_setname_np(pthread_self(),name);
  while (true) {
    struct timespec sleeptimer = {0,25000000};
    nanosleep(&sleeptimer,NULL);
    if (worker->next_job != NULL && worker->cur_job == NULL &&
        wdctx->ctx->loadedfile == worker->next_job->file) {
      debug("%s: Next Job is File \"%s\" with \"%s\" as codec",
        name,
        worker->next_job->file->path,
        worker->next_job->codec->name);
      worker->cur_job = worker->next_job;
      worker->next_job = NULL;
      char* sendbuf = ocworker_serialize_job(worker->cur_job);
      ocsched_printf(worker->ctx, sendbuf);
      debug("Send \"%s\" to %d",sendbuf,worker->ctx->pid);
      free(sendbuf);
    }

    if(worker->cur_job != NULL)
    {
      memset(recvBuf, 0, 4096);
      if(ocsched_recvfrom(worker->ctx, recvBuf, 4096) > 0)
      {
        debug("recieved \"%s\"",recvBuf);
        ocworker_deserialize_job(worker->cur_job, recvBuf);
        worker->last_job = worker->cur_job;
        worker->cur_job = NULL;
      }
    }
  }
}

void* ocworker_schedule_worker(void* data)
{
  ocworkerContext* ctx = (ocworkerContext*) data;\

  pthread_setname_np(pthread_self(),"Job Scheduler");

  while (true) {
    struct timespec sleeptimer = {0,25000000};
    nanosleep(&sleeptimer,NULL);

    int running_jobs = 0;
    ocutils_list_foreach_f(ctx->worker, curworker)
    {
      if (((ocworker*)curworker->value)->cur_job != NULL)
        running_jobs++;
    }
    if(running_jobs != 0)
      continue;

    pthread_mutex_lock(&ctx->lock);
    ListNode* job =  ocutils_list_head(ctx->jobs);
    if (job != NULL) {
      // Check if we need to load a new file into our memfd
      if (((ocworkerJob*)job->value)->file != ctx->loadedfile) {
        ocmemfd_load_file(ctx->memfd, ((ocworkerJob*)job->value)->file->path);
        ctx->loadedfile = ((ocworkerJob*)job->value)->file;
      }
    }
    else
    {
      // No jobs are in queue continue waiting
      pthread_mutex_unlock(&ctx->lock);
      continue;
    }

    for (size_t i = 0; i < ctx->worker->items; i++) {
      ocworkerJob* job = (ocworkerJob*) ocutils_list_dequeue(ctx->jobs);
      ocworker* worker = (ocworker*)    ocutils_list_get(ctx->worker, i);
      worker->next_job = job;
    }
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

  myctx->jobs   = ocutils_list_create();
  myctx->worker = ocutils_list_create();
  myctx->memfd  = ocmemfd_create_context("/loadedfile", 1024);

  for (size_t i = 0; i < worker_amt; i++) {
    ocworkerWatchdog* wdctx     = calloc(1, sizeof(ocworkerWatchdog));
    ocworker*         newworker = calloc(1,sizeof(ocworker));
    wdctx->ctx      = myctx;
    wdctx->myworker = newworker;
    newworker->ctx  =
      ocsched_fork_process(ocworker_worker_process_loop,"worker",myctx->memfd);
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
  newjob->codec     = codec;
  newjob->file      = file;
  newjob->jobid     = ++(ctx->lastjobid);
  newjob->refcount++;
  pthread_mutex_init(&newjob->lock, NULL);

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
  ocutils_list_foreach_f(codecs, codec)
  {
    int64_t jobid;
    ocdataCodec* curcodec = (ocdataCodec*) codec->value;
    ocworker_schedule_job(ctx, file, curcodec, &jobid);
    ocutils_list_add(jobs, (void*) jobid);
  }

  *jobids = jobs;

  return OCWORKER_SCHEDULED;
}

ocworkerStatus
ocworker_retrieve_job(ocworkerContext* ctx, int64_t jobid, ocworkerJob** job)
{

}

ocworkerStatus
ocworker_retrieve_jobs(ocworkerContext* ctx, List* jobids, List** jobs)
{

}

ocworkerStatus
ocworker_unref_job(ocworkerContext* ctx, ocworkerJob** job)
{

}

ocworkerStatus
ocworker_kill(ocworkerContext*  ctx)
{

}

void __child_process(ocschedProcessContext* parent, void* data)
{
  int read_tries = 0;
  char recvbuf[1024];
  size_t recvBytes = 0;
  size_t compressed_length = 0;
  SquashCodec* codec = squash_get_codec("lzma");
  ocMemfdContext * decompressed = (ocMemfdContext *) data;

  check(codec > 0,"failed to get squash plugin lzma");

  while (read_tries <= 100) {
    size_t tmp = 0;
    tmp = ocsched_recvfrom(parent,&recvbuf[recvBytes],1);
    check(tmp >= 0,"failed to read from parent");
    if(recvbuf[recvBytes] == '\n')
    {
      recvbuf[recvBytes] == 0x00;
      break;
    }
    else
      recvBytes += tmp;
    struct timespec sleeptimer = {0,100000};
    nanosleep(&sleeptimer,NULL);
    read_tries++;
  }

  int oldsize = decompressed->size;
  int newsize = atoi(recvbuf);

  debug("child got size of shm %d bytes",newsize);

  decompressed->size = newsize;
  ocmemfd_remap_buffer(decompressed,oldsize);

  ocMemfdContext * compressed =
    ocmemfd_create_context("/compressed",
      squash_codec_get_max_compressed_size(codec,decompressed->size));
  compressed_length = compressed->size;
  debug("child compressing data");
  int ret = squash_codec_compress(codec,
                                  &compressed_length,
                                  compressed->buf,
                                  decompressed->size,
                                  decompressed->buf,
                                  NULL);
  check(ret == SQUASH_OK,"failed to compress data [%d] : %s",
    ret,squash_status_to_string(ret));
  debug("compression done. from %d to %d",
    (int32_t) decompressed->size, (int32_t) compressed_length);
  ocsched_printf(parent,"compressed from %d bytes to %d bytes with lzma",
    (int32_t) decompressed->size, (int32_t) compressed_length);
error:
  ocmemfd_destroy_context(&compressed);
  ocmemfd_destroy_context(&decompressed);

  return;
}
