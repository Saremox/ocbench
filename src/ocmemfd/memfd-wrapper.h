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

#include <unistd.h>
#include <linux/memfd.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include "debug.h"

#ifndef MEMFD_WRAPPER_INC
#define MEMFD_WRAPPER_INC

#define F_LINUX_SPECIFIC_BASE 1024
#define F_ADD_SEALS (F_LINUX_SPECIFIC_BASE + 9)
#define F_GET_SEALS (F_LINUX_SPECIFIC_BASE + 10)

#define F_SEAL_SEAL     1<<1
#define F_SEAL_SHRINK   1<<2
#define F_SEAL_GROW     1<<3
#define F_SEAL_WRITE    1<<4

static inline long memfd_create(char* name, int flags)
{
  long fd = syscall(__NR_memfd_create,name,flags);
  check(fd > 0, "failed to create %s memfd file descriptor with 0x%x flags",
        name, flags);
  return fd;
error:
  return -1;
}

static inline int memfd_is_flag_set(long int fd, int flag)
{
  check(fd != 0, "invalid file discriptor");
  check(flag & 0b1111, "unknown flag 0x%x", flag);
  return fcntl(fd,F_GET_SEALS,NULL) & flag;
error:
  return -1;
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
