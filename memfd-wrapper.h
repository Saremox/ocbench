#include <unistd.h>
#include <linux/memfd.h>
#include <sys/syscall.h>
#include <fcntl.h>

#ifndef MEMFD_WRAPPER_INC
#define MEMFD_WRAPPER_INC

#define F_LINUX_SPECIFIC_BASE 1024
#define F_ADD_SEALS (F_LINUX_SPECIFIC_BASE + 9)
#define F_GET_SEALS (F_LINUX_SPECIFIC_BASE + 10)

#define F_SEAL_SEAL     1>>1
#define F_SEAL_SHRINK   1>>2
#define F_SEAL_GROW     1>>3
#define F_SEAL_WRITE    1>>4

static inline long memfd_create(char* name, int flags)
{
  return syscall(__NR_memfd_create,name,flags);
}

static inline int memfd_is_flag_set(long int fd, int flag)
{
  return (fd != 0 && fcntl(fd,F_GET_SEALS,NULL) & flag);
}

static inline int memfd_is_writeable(long int fd)
{
  return !memfd_is_flag_set(fd,F_SEAL_WRITE);
}

static inline int memfd_is_sealed(long int fd)
{
  return !memfd_is_flag_set(fd,F_SEAL_SEAL);
}

static inline int memfd_is_growable(long int fd)
{
  return !memfd_is_flag_set(fd,F_SEAL_GROW);
}

static inline int memfd_is_shrinkable(long int fd)
{
  return !memfd_is_flag_set(fd,F_SEAL_SHRINK);
}

#endif
