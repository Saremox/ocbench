#include "ocmemfd.h"
#include "debug.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

ocMemfdContext * ocmemfd_create_context(char* path, size_t size)
{
  ocMemfdContext * ctx = malloc(sizeof(ocMemfdContext));
  #if defined(HAVE_LINUX_MEMFD_H) && !defined(WITHOUT_MEMFD)
    ctx->fd = memfd_create(path,MFD_ALLOW_SEALING);
    check(ctx->fd > 0,
      "failed to create memfd at %s with size of %d bytes", path, (int32_t) size);
  #elif defined(HAVE_SHM_OPEN) && !defined(WITHOUT_SHM)
    ctx->fd = shm_open(path,O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    check(ctx->fd > 0,
      "failed to create shm at %s with size of %d bytes", path, (int32_t) size);
  #else
    ctx->fd = fileno(tmpfile());
    check(ctx->fd > 0,
      "failed to create tmpfile with size of %d bytes", (int32_t) size);
  #endif
  ctx->path = malloc(strlen(path));
  strcpy(ctx->path,path);
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
    log_warn("The given context 0x%lx is allready memory maped",(int64_t) ctx);
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
  #if defined(HAVE_LINUX_MEMFD_H) && !defined(WITHOUT_MEMFD)
  if(newsize > ctx->size)
    if ( ! memfd_is_growable(ctx->fd))
      return OCMEMFD_NOT_GROWABLE;
  else
    if( ! memfd_is_shrinkable(ctx->fd))
      return OCMEMFD_NOT_SHRINKABLE;
  #endif
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
    "cannot resize memfd to %d bytes",(int32_t) filesize);

  if((file = open(path,0, "r+b")))
  {
    int readBytes = 0;
    while(readBytes < filesize)
    {
      int ret = read(file,ctx->buf+readBytes,filesize);
      readBytes += ret;
      check(ret >= 0,"cannot read from file. %d bytes have been read",readBytes);
    }
    check(readBytes == filesize,
      "only read %d from %d bytes",readBytes, (int32_t) filesize);
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
  #if defined(HAVE_SHM_OPEN) && !defined(WITHOUT_SHM) && defined(WITHOUT_MEMFD)
    shm_unlink((*ctx)->path);
  #endif
  close((*ctx)->fd);
  if((*ctx)->path >0)
    free((*ctx)->path);
  free((*ctx));
  (*ctx) = 0;
  return OCMEMFD_SUCCESS;
error:
  return OCMEMFD_FATAL_FAILURE;
}
