#include "ocsched.h"
#include "debug.h"
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

  sprintf(msgQname,"/ocbench-work-queue-%d",getpid());
  check(global_job_queue = mq_open(msgQname,
    O_CREAT | O_RDWR | O_CLOEXEC,
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

  pipe(master);
  pipe(child);

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
}

size_t
ocsched_recvfrom(ocschedProcessContext * ctx, char * buf, size_t n)
{
  int readed;
  check((readed = read(ctx->comm.pipe_in,buf,n)) >= 0, " read failed");
  return readed;
error:
  return OCSCHED_FAILURE;
}
