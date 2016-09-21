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

#ifndef WATCHDOG_H
#define WATCHDOG_H

#include "scheduler.h"
#include "worker.h"

typedef struct __WatchdogContext{
  schedulerContext* ctx;
  workerContext*    myworker;
  pthread_t         watchdog;
} WatchdogContext;

void* watchdog_main(void* data);

#endif
