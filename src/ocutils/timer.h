/**
 * This File is part of openCompressBench
 * Copyright (C) 2016 Michael Strassberger <saremox@linux.com>
 *
 * openCompressBench is free software: you can redistribute
 * it and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation,
 * version 2 of the License.
 *
 * Some open source application is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @license GPL-2.0 <https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html>
 */

#include <stdlib.h>
#include <time.h>

#ifndef OCUTILS_TIMER_H
#define OCUTILS_TIMER_H

struct __Timer;

typedef struct __Timer Timer;

struct __Timer {
  struct timespec begin;
  struct timespec end;
};

#define ocutils_timer_create() \
  calloc(sizeof(Timer),1)
#define ocutils_timer_start(TIMER) \
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&TIMER->begin)
#define ocutils_timer_stop(TIMER) \
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&TIMER->end)
#define ocutils_timer_get_usecs(TIMER) \
  ((TIMER->end.tv_sec - TIMER->begin.tv_sec)*1000000 + \
  (TIMER->end.tv_nsec - TIMER->begin.tv_nsec)/1000)
#endif
