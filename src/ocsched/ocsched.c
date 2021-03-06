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

#include "ocsched.h"
#include "debug.h"
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/prctl.h>

#define PIPE_READ 0
#define PIPE_WRITE 1

#define NAME_MAX 128

mqd_t global_job_queue = 0;

mqd_t
ocsched_get_msgq()
{
  char msgQname[NAME_MAX];
  if(global_job_queue > 0)
    return global_job_queue;

  sprintf(msgQname,"/ocbench-work-queue");
  #if defined(O_CLOEXEC)
    #define MQ_FLAGS_USED O_CREAT | O_RDWR | O_CLOEXEC
  #else
    #define MQ_FLAGS_USED O_CREAT | O_RDWR
  #endif
  check(global_job_queue = mq_open(msgQname,
    MQ_FLAGS_USED,
    S_IRUSR | S_IWUSR, NULL),"Couldn't create work queue");

  return global_job_queue;
error:
  return -1;
}

ocschedProcessContext * \
ocsched_fork_process(ocschedFunction work_function, char* childname, void* data)
{
  ocschedProcessContext * ctx = malloc(sizeof(ocschedProcessContext));
  check((ctx->workQueue = ocsched_get_msgq()) > 0,"Couldn't get work queue");
  ctx->name = malloc(strlen(childname)+1);
  strcpy(ctx->name,childname);

  int master[2];
  int child[2];

  check(pipe(master) == 0,"failed to create pipe");
  check(pipe(child)  == 0,"failed to create pipe");

  if((ctx->pid = fork()) > 0)
  {
    // Closing not needed pipe fd's
    close(master[PIPE_WRITE]);
    close(child[PIPE_READ]);

    ctx->comm.pipe_in = master[PIPE_READ];
    ctx->comm.pipe_out = child[PIPE_WRITE];

    return ctx;
  }
  else if(ctx->pid == 0)
  {
    // Closing not needed pipe fd's
    close(master[PIPE_READ]);
    close(child[PIPE_WRITE]);

    ctx->comm.pipe_in = child[PIPE_READ];
    ctx->comm.pipe_out = master[PIPE_WRITE];

    check(prctl(PR_SET_NAME,childname) ==0,"failed to set child name");

    work_function(ctx,data);

    // Cleaning open pipes up and context up
    close(master[PIPE_WRITE]);
    close(child[PIPE_READ]);
    free(ctx->name);
    free(ctx);

    exit(EXIT_SUCCESS);
  }
  else
  {
error:
    log_err("failed to fork process");
    return NULL;
  }
}

size_t
ocsched_sendto(ocschedProcessContext * ctx, char * buf, size_t n)
{
  int written;
  check((written = write(ctx->comm.pipe_out,buf,n)) >= 0," write failed");
  return written;
error:
  return OCSCHED_FAILURE;
}

ocschedStatus
ocsched_printf(ocschedProcessContext * ctx,char * fmtstr,...)
{
  va_list ap;
  va_start(ap,fmtstr);
  vdprintf(ctx->comm.pipe_out,fmtstr,ap);
  va_end(ap);
  return OCSCHED_SUCCESS;
}

size_t
ocsched_recvfrom(ocschedProcessContext * ctx, char * buf, size_t n)
{
  int readed = 0;
  struct pollfd pipe = {ctx->comm.pipe_in,POLLIN,0};
  poll(&pipe,1,0);
  if(pipe.revents & POLLIN)
    check((readed = read(ctx->comm.pipe_in,buf,n)) >= 0, " read failed");
  return readed;
error:
  return OCSCHED_FAILURE;
}

ocschedStatus
ocsched_destroy_context(ocschedProcessContext ** ctx)
{
  close((*ctx)->comm.pipe_in);
  close((*ctx)->comm.pipe_in);
  free((*ctx)->name);
  free((*ctx));
  (*ctx) = 0;
  return OCSCHED_SUCCESS;
}

ocschedStatus
ocsched_destroy_global_mqueue()
{
  char msgQname[NAME_MAX];
  sprintf(msgQname,"/ocbench-work-queue-%d",getpid());
  mq_unlink(msgQname);
  return OCSCHED_SUCCESS;
}
