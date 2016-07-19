#include <stdio.h>
#include <stdlib.h>
#include <wait.h>
#include "ocbenchConfig.h"
#include "ocmemfd/ocmemfd.h"
#include "ocsched/ocsched.h"
#include <squash/squash.h>

#define MAX_MEMFD_BUF 1024

void childprocess(ocschedProcessContext* parent, void* data)
{
  size_t compressed_length = 0;
  SquashCodec* codec = squash_get_codec("lzma");
  ocMemfdContext * decompressed = (ocMemfdContext *) data;
  ocMemfdContext * compressed =
    ocmemfd_create_context("/compressed",
      squash_codec_get_max_compressed_size(codec,decompressed->size));
  compressed_length = compressed->size;
  int ret = squash_codec_compress(codec,
                                  &compressed_length,
                                  compressed->buf,
                                  decompressed->size,
                                  decompressed->buf,
                                  NULL);
  check(ret == SQUASH_OK,"failed to compress data [%d] : %s",
    ret,squash_status_to_string(ret));
  ocsched_printf(parent,"compressed from %d bytes to %d bytes with lzma",
    decompressed->size,compressed_length);
error:
  return;
}

int compressionTest(char * file)
{
  char buf[1024];
  int childreturn = 0;
  ocMemfdContext * fd = ocmemfd_create_context("/cache",MAX_MEMFD_BUF);
  ocmemfd_load_file(fd,file);

  ocschedProcessContext * child =
    ocsched_fork_process(childprocess,"child",fd);

  waitpid(child->pid,&childreturn,0);
  int bytes_recv = ocsched_recvfrom(child,buf,1024);
  buf[bytes_recv] = 0x00;

  printf("Recv from child: %s\n",buf);

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
