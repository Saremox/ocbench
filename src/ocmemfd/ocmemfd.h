#include "memfd-wrapper.h"
#include "ocbenchConfig.h"

#include <sys/mman.h>
#include <stdint.h>

#ifndef OCMENFDH
#define OCMENFDH

typedef struct {
  long int  fd;   /*!< file descriptor which is holding the memfd */
  size_t    size; /*!< current size of memfd */
  uint8_t*  buf;  /*!< current address of memory maping of the memfd*/
  char*     path; /*!< holds the path of the memfd or shm object */
} ocMemfdContext;

typedef enum {
  OCMEMFD_SUCCESS,
  OCMEMFD_FATAL_FAILURE,
  OCMEMFD_NOT_WRITEABLE,
  OCMEMFD_NOT_GROWABLE,
  OCMEMFD_NOT_SHRINKABLE,
  OCMEMFD_ALLREDY_MMAPED,
  OCMEMFD_RESIZE_FAILURE,
  OCMEMFD_MMAP_FAILURE,
  OCMEMFD_REMMAP_FAILURE,
  OCMEMFD_LOADFILE_FAILURE,
  OCMEMFD_CANNOT_READ,
  OCMEMFD_CANNOT_GET_FILE_SIZE
}
ocMemfdStatus;

/** Creates a new context and manages systemcall for new memfd pointer
  * and memory maps it into virtual address space. See ocMemfdContext
  */
ocMemfdContext * /**< [out] valid context for usage in ocmemfd functions */
ocmemfd_create_context(
  char* path, /**< [in] the path in the kernel memfd file system */
  size_t size /**< [in] the desired size of the memfd*/
);

/** Resizes the given memfd in memfdcontext to newsize
  * set memory map accordingly
  */
ocMemfdStatus /**< [out] on success OCMEMFD_SUCCESS */
ocmemfd_resize(
  ocMemfdContext * ctx, /**< [in,out] the context */
  size_t newsize /**< [in] the new desired size of the memfd */
);

ocMemfdStatus
ocmemfd_remap_buffer(
  ocMemfdContext * ctx, /**< [in,out] the context */
  size_t oldsize /**< [in] the old size of the memfd */
);

ocMemfdStatus /**< [out] on success OCMEMFD_SUCCESS */
ocmemfd_load_file(
  ocMemfdContext * ctx, /**< [in,out] the context */
  char* path /**< [in] path of the file that gets loaded into the memfd */
);

ocMemfdStatus
ocmemfd_destroy_context(
  ocMemfdContext ** ctx /**< [in,out] the context */
);

#endif
