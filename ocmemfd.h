#include "memfd-wrapper.h"

#include <sys/mman.h>

#ifndef OCMENFDH
#define OCMENFDH

typedef struct {
  long int  fd;   /*!< file descriptor which is holding the memfd */
  size_t    size; /*!< current size of memfd */
  char*     buf;  /*!< current address of memory maping of the memfd*/
} ocMemfdContext;

typedef enum {
  OCMEMFD_SUCCESS,
  OCMEMFD_NOT_WRITEABLE,
  OCMEMFD_NOT_GROWABLE,
  OCMEMFD_NOT_SHRINKABLE,
  OCMEMFD_ALLREDY_MMAPED,
  OCMEMFD_RESIZE_FAILURE,
  OCMEMFD_MMAP_FAILURE,
  OCMEMFD_REMMAP_FAILURE}
ocMemfdStatus;

ocMemfdContext * ocmemfd_create_context(
  char* path, /**< [in] the path in the kernel memfd file system */
  size_t size /**< [in] the desired size of the memfd*/
);
ocMemfdStatus    ocmemfd_map_buffer(
  ocMemfdContext * ctx  /**< [in,out] the context */
);
ocMemfdStatus    ocmemfd_resize(
  ocMemfdContext * ctx, /**< [in,out] the context */
  size_t newsize /**< [in] the new desired size of the memfd */
);

#endif
