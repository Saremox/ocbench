#include "ocmemfd.h"
#include "debug.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

ocMemfdContext * ocmemfd_create_context(char* path, size_t size)
{
  ocMemfdContext * ctx = malloc(sizeof(ocMemfdContext));
  ctx->fd = memfd_create(path,MFD_ALLOW_SEALING);
  check(ctx->fd > 0, "failed to create memfd at %s with size of %s bytes",path,size)
  ctx->size = size;
  ctx->buf = NULL;
  ocmemfd_resize(ctx, size);
  return ctx;
error:
  free(ctx);
  return (ocMemfdContext *) -1;
}

ocMemfdStatus ocmemfd_map_buffer(ocMemfdContext * ctx)
{
  check(ctx->fd > 0 || ctx->size <= 0, "not a valid context")
  if(ctx->buf > 0)
  {
    log_warn("The given context 0x%x is allready memory maped",ctx);
    return OCMEMFD_ALLREDY_MMAPED;
  }
  ctx->buf = mmap(NULL, ctx->size, PROT_READ | PROT_WRITE,
                  MAP_SHARED, ctx->fd,0);
  check(ctx->buf, "failed to memory map memfd");
  return OCMEMFD_SUCCESS;
error:
  return OCMEMFD_MMAP_FAILURE;
}

ocMemfdStatus ocmemfd_remap_buffer(ocMemfdContext * ctx, size_t oldsize)
{
  if(ctx->buf > 0)
  {
    check(munmap(ctx->buf,oldsize) == 0, "failed to memory unmap memfd");
    ctx->buf = 0; // Reset pointer
  }
  check(ocmemfd_map_buffer(ctx) == OCMEMFD_SUCCESS, "failed to memory map memfd");
  return OCMEMFD_SUCCESS;
error:
  return OCMEMFD_REMMAP_FAILURE;
}

ocMemfdStatus ocmemfd_resize(ocMemfdContext * ctx, size_t newsize)
{
  check(ctx->fd > 0 || ctx->size <= 0, "not a valid context");
  check(newsize > 0, "newsize must be greater than 0");
  size_t oldsize = ctx->size;
  if(newsize > ctx->size)
    if ( ! memfd_is_growable(ctx->fd))
      return OCMEMFD_NOT_GROWABLE;
  else
    if( ! memfd_is_shrinkable(ctx->fd))
      return OCMEMFD_NOT_SHRINKABLE;
  check(ftruncate(ctx->fd,newsize) == 0, "cannot set new size");
  ctx->size = newsize;
  check(ocmemfd_remap_buffer(ctx,oldsize) == OCMEMFD_SUCCESS, "resizing memory map failed");
  return OCMEMFD_SUCCESS;
error:
  return OCMEMFD_RESIZE_FAILURE;
}

ocMemfdStatus
ocmemfd_load_file(ocMemfdContext * ctx, char* path)
{
  int file;
  off_t filesize = 0;
  struct stat st;
  check(ctx->fd > 0 || ctx->size <= 0, "not a valid context");
  if(stat(path,&st) ==0)
    filesize = st.st_size;
  else
    return OCMEMFD_CANNOT_GET_FILE_SIZE;

  check(ocmemfd_resize(ctx,filesize) == OCMEMFD_SUCCESS,
    "cannot resize memfd to %d bytes",filesize);

  if((file = open(path,0, "r")))
  {
    int readBytes = 0;
    while(readBytes < filesize)
    {
      debug("allready read %d bytes from %d",readBytes,filesize);
      int ret = read(file,ctx->buf+readBytes,4096);
      if(ret > 0)
      {
        readBytes += ret;
        continue;
      }
      check(ret == 0,"cannot read from file. %d bytes have been read",readBytes);
    }
    check(readBytes == filesize,
      "only read %d from %d bytes",readBytes,filesize);
  }
  else
    return OCMEMFD_CANNOT_READ;

  close(file);
  return OCMEMFD_SUCCESS;
error:
  return OCMEMFD_LOADFILE_FAILURE;
}

ocMemfdStatus
ocmemfd_destroy_context(ocMemfdContext ** ctx)
{
  if((*ctx)->buf > 0)
  {
    check(munmap((*ctx)->buf,(*ctx)->size) == 0, "failed to memory unmap memfd");
    (*ctx)->buf = 0; // Reset pointer
  }
  close((*ctx)->fd);
  free((*ctx));
  (*ctx) = 0;
  return OCMEMFD_SUCCESS;
error:
  return OCMEMFD_FATAL_FAILURE;
}
