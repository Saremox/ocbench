#include <stdlib.h>
#include <time.h>
#include "ocworker.h"
#include "debug.h"
#include "ocsched/ocsched.h"
#include "ocutils/list.h"

void ocworker_worker_process_loop(ocschedProcessContext* ctx, void* data)
{
  char*  name_fmt = "worker[%d]";
  size_t namesize = snprintf(NULL,0,name_fmt,ctx->pid)+1;
  char*  name     = malloc(namesize);
                    snprintf(name, namesize, name_fmt, ctx->pid);

  ocMemfdContext* compressed = ocmemfd_create_context(name,1024);

  while (true) {
    struct timespec sleeptimer = {0,250000};
    nanosleep(&sleeptimer,NULL);
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

  pthread_setname_np(pthread_self(),name);
  while (true) {
    struct timespec sleeptimer = {0,250000};
    nanosleep(&sleeptimer,NULL);
  }
}

void* ocworker_schedule_worker(void* data)
{
  ocworkerContext* ctx = (ocworkerContext*) data;\

  pthread_setname_np(pthread_self(),"Job Scheduler");

  while (true) {
    struct timespec sleeptimer = {0,250000};
    nanosleep(&sleeptimer,NULL);
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
      ocsched_fork_process(ocworker_worker_process_loop,"worker",NULL);
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
  ocutils_list_add(ctx->jobs, newjob);
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
