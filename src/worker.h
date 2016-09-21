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

#include "ocsched/ocsched.h"
#include "job.h"

#ifndef WORKER_H
#define WORKER_H

struct __workerContext;

typedef struct __workerContext workerContext;

struct __workerContext{
  ocschedProcessContext*  ctx;
  Job*                    next_job;
  Job*                    cur_job;
  Job*                    last_job;
  int64_t                 status;
};

typedef enum __workerStatus{
  WORKER_FAILURE = -1,
  WORKER_OK
} workerStatus;



#define WORKER_KILL_SIG 42
#define MINIMUM_RUN_TIME 1000*1000

void worker_main(ocschedProcessContext* ctx, void* data);

#endif
