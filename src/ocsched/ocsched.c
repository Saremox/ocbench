#include "ocsched.h"
#include "debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/prctl.h>

#define PIPE_READ 0
#define PIPE_WRITE 1

ocschedProcessContext * \
ocsched_fork_process(ocschedFunction work_function, char* childname, void* data)
{
  ocschedProcessContext * ctx = malloc(sizeof(ocschedProcessContext));
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

size_t
ocsched_recvfrom(ocschedProcessContext * ctx, char * buf, size_t n)
{
  int readed;
  check((readed = read(ctx->comm.pipe_in,buf,n)) >= 0, " read failed");
  return readed;
error:
  return OCSCHED_FAILURE;
}
