/**
 * This File is part of openCompressBench
 * Copyright (C) 2016 Michael Strassberger <saremox@linux.com>
 *
 * openCompressBench is free software: you can redistribute
 * it and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation,
 * version 2 of the License.
 *
 * openCompressBench is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with openCompressBench.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @license GPL-2.0 <https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html>
 */

#include <setjmp.h>
#include <signal.h>
#include <squash/squash.h>
#include <stdbool.h>
#include <stdlib.h>
#include "debug.h"
#include "ocmemfd/ocmemfd.h"
#include "ocutils/timer.h"
#include "worker.h"

int32_t got_killed = 0;
jmp_buf buf;

static void signal_handler(int signl)
{
  switch (signl) {
    case SIGINT:

      break;
    case WORKER_KILL_SIG:
      log_info("Worker[%d] shutting down",getpid());
      got_killed = 1;
      break;
    case SIGSEGV:
      log_warn("Worker[%d] got segfault",getpid());
      longjmp(buf, 1);
      break;
  }
}

workerStatus worker_compression(Job* myJob,
                                SquashCodec* myCodec,
                                ocMemfdContext* original,
                                ocMemfdContext* compressed)
{
  struct timespec begin,end;
  int iterations = 0;
  myJob->result->compressed_size = compressed->size;
  myJob->result->compressed_time = 0;
  Timer* myTimer = ocutils_timer_create();

  for ( ; myJob->result->compressed_time < MINIMUM_RUN_TIME; iterations++) {
    ocutils_timer_start(myTimer);
    int ret = squash_codec_compress(myCodec,
                                    &myJob->result->compressed_size,
                                    compressed->buf,
                                    original->size,
                                    original->buf,
                                    NULL);
    ocutils_timer_stop(myTimer);
    myJob->result->compressed_time += ocutils_timer_get_usecs(myTimer);

    check(ret == SQUASH_OK,"failed to compress data with %s [%d] : %s",
      myJob->result->comp_id->codec_id->name,
      ret,squash_status_to_string(ret));
  }

  myJob->result->compressed_time /= iterations;
  return WORKER_OK;
error:
  return WORKER_FAILURE;
}

workerStatus worker_decompression(Job* myJob,
                                  SquashCodec* myCodec,
                                  ocMemfdContext* compressed,
                                  ocMemfdContext* decompressed)
{
  struct timespec begin,end;
  int iterations = 0;
  size_t decompressedsize;
  myJob->result->decompressed_time = 0;
  Timer* myTimer = ocutils_timer_create();

  for ( ; myJob->result->decompressed_time < MINIMUM_RUN_TIME; iterations++) {
    decompressedsize = decompressed->size;
    ocutils_timer_start(myTimer);
    int ret = squash_codec_decompress(myCodec,
                                      &decompressedsize,
                                      decompressed->buf,
                                      myJob->result->compressed_size,
                                      compressed->buf,
                                      NULL);
    ocutils_timer_stop(myTimer);
    myJob->result->decompressed_time += ocutils_timer_get_usecs(myTimer);

    check(ret == SQUASH_OK,"failed to decompress data with %s [%d] : %s",
      myJob->result->comp_id->codec_id->name,
      ret,squash_status_to_string(ret));
  }

  myJob->result->decompressed_time /= iterations;
  return WORKER_OK;
error:
  return WORKER_FAILURE;
}

void worker_main(ocschedProcessContext* ctx, void* data)
{
  char*  name_fmt = "/worker[%d]-compressed";
  size_t namesize = snprintf(NULL,0,name_fmt,getpid())+1;
  char*  namecp   = malloc(namesize);
                    snprintf(namecp, namesize, name_fmt, getpid());

         name_fmt = "/worker[%d]-decompressed";
         namesize = snprintf(NULL,0,name_fmt,getpid())+1;
  char*  namedcp  = malloc(namesize);
                    snprintf(namedcp, namesize, name_fmt, getpid());

  signal(SIGSEGV, signal_handler);
  signal(SIGINT, signal_handler);
  signal(WORKER_KILL_SIG, signal_handler);

  ocMemfdContext* original = (ocMemfdContext *) data;
  ocMemfdContext* compressed   = ocmemfd_create_context(namecp,4096);
  ocMemfdContext* decompressed = ocmemfd_create_context(namedcp,4096);

  char   recvBuf[4096];

  if(setjmp(buf))
  {
    log_info("Try graceful termination of worker[%d] after segfault",getpid());
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGSEGV);
    sigprocmask(SIG_UNBLOCK, &sigset, NULL);
    goto error;
  }

  while (true) {
    if(got_killed == 1)
      break;

    memset(recvBuf, 0, 4096);
    int readBytes = ocsched_recvfrom(ctx, recvBuf, 4096);
    if(readBytes > 0)
    {
      Job* recvjob = calloc(1, sizeof(Job));
      debug("recieved \"%s\"",recvBuf);
      deserialize_job(recvjob, recvBuf);
      // resize shared memfd buffer to file size
      int oldsize = original->size;
      original->size = recvjob->result->file_id->size;
      ocmemfd_remap_buffer(original,oldsize);

      SquashCodec* codec =
        squash_get_codec(recvjob->result->comp_id->codec_id->name);
      check(codec != NULL, "squash cannot find \"%s\" codec",
        recvjob->result->comp_id->codec_id->name);

      ocmemfd_resize(decompressed, original->size);
      ocmemfd_resize(compressed,
        squash_codec_get_max_compressed_size(codec,original->size));

      check(
        worker_compression(recvjob, codec, original, compressed)
        == WORKER_OK, "Failed to compress");

      check(
        worker_decompression(recvjob, codec, compressed, decompressed)
        == WORKER_OK, "Failed to decompress");


      // Early resource freeing
      ocmemfd_resize(compressed, 4096);
      ocmemfd_resize(decompressed, 4096);

      char* sendbuf = serialize_job(recvjob);
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
  }
  ocdata_garbage_collect();
  ocmemfd_destroy_context(&compressed);
  ocmemfd_destroy_context(&decompressed);
  ocmemfd_destroy_context(&original);
  free(namecp);
  free(namedcp);
  exit(EXIT_SUCCESS);
error:
  ocmemfd_destroy_context(&compressed);
  ocmemfd_destroy_context(&decompressed);
  ocmemfd_destroy_context(&original);
  kill(getpid(),SIGKILL);
}
