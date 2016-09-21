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

#include <stdbool.h>
#include <stdlib.h>
#include <wait.h>
#include "debug.h"
#include "watchdog.h"
#include "worker.h"

void* watchdog_main(void* data)
{
  WatchdogContext* wdctx  = (WatchdogContext*) data;
  workerContext*    worker = wdctx->myworker;

  worker->ctx =
    ocsched_fork_process( worker_main,"worker",
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

    if(worker->status == WORKER_KILL_SIG)
    {
      log_info("WatchdogContext[%d] got kill signal",worker->ctx->pid);
      kill(worker->ctx->pid,WORKER_KILL_SIG);

      waitpid(worker->ctx->pid, &child_status, 0);
      goto killWatchdogContext;
    }

    if(WIFSIGNALED(child_status))
    {
      if(worker->cur_job == NULL)
      {
        log_err("WatchdogContext[%d] FATAL: CHILD HUNG WITH %d WITH NO JOB ASSIGNED",
          worker->ctx->pid,
          WTERMSIG(child_status));
        log_err("WatchdogContext[%d] CAN NOT RECOVER WORKER STATE. EXITING",
          worker->ctx->pid);
        goto killWatchdogContext;
      }
      else
      {
        log_warn("WatchdogContext[%d] CHILD HUNG: %d CODEC: %s JOBID: %ld",
          worker->ctx->pid,
          WTERMSIG(child_status),
          worker->cur_job->result->comp_id->codec_id->name,
          worker->cur_job->jobid);
        // Try to reanimate child
        worker->next_job = worker->cur_job;
        worker->cur_job  = NULL;

        // Destroy old context with open fd
        ocsched_destroy_context(&worker->ctx);

        worker->ctx =
          ocsched_fork_process( worker_main,"worker",
                                wdctx->ctx->memfd);
      }
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
      char* sendbuf = serialize_job(worker->cur_job);
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
        deserialize_job(worker->cur_job, recvBuf);
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
killWatchdogContext:
  ocsched_destroy_context(&worker->ctx);
  free(wdctx);
  free(name);
  pthread_exit(EXIT_SUCCESS);
}
