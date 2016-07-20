#include <stdio.h>
#include <stdlib.h>
#include <wait.h>
#include "debug.h"
#include "ocbenchConfig.h"
#include "ocmemfd/ocmemfd.h"
#include "ocsched/ocsched.h"
#include <squash/squash.h>

#define MAX_MEMFD_BUF 1024

void childprocess(ocschedProcessContext* parent, void* data)
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
    usleep(100);
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

int compressionTest(char * file)
{
  char buf[1024];
  int childreturn = 0;
  ocMemfdContext * fd = ocmemfd_create_context("/cache",MAX_MEMFD_BUF);

  debug("forking child");
  ocschedProcessContext * child =
    ocsched_fork_process(childprocess,"child",fd);
  debug("loading file");
  ocmemfd_load_file(fd,file);
  debug("send filesize %d bytes to child",(int32_t) fd->size);
  ocsched_printf(child,"%d\n",fd->size);
  waitpid(child->pid,&childreturn,0);
  if(WIFSIGNALED(childreturn))
  {
    errno=WTERMSIG(childreturn);
    log_err("child failed");
  }
  int bytes_recv = ocsched_recvfrom(child,buf,1024);
  buf[bytes_recv] = 0x00;

  printf("Recv from child: %s\n",buf);

  ocmemfd_destroy_context(&fd);
  ocsched_destroy_context(&child);
  ocsched_destroy_global_mqueue();

  return 0;
}

int main (int argc, char *argv[])
{
  if (argc < 2)
  {
    fprintf(stdout,"%s openCompressBench Version %d.%d\n"
            "Copyright (C) 2016 Michael Strassberger "
            "<saremox@linux.com>\n"
            "openCompressBench comes with ABSOLUTELY NO WARRANTY; for details\n"
            "type `show w'.  This is free software, and you are welcome\n"
            "to redistribute it under certain conditions; type `show c'\n"
            "for details.\n\n",
            argv[0],
            OCBENCH_VERSION_MAJOR,
            OCBENCH_VERSION_MINOR);
    fprintf(stdout,"Usage: %s FILE_PATH\n",argv[0]);
    return 1;
  }
  compressionTest(argv[1]);
  return 0;
}
