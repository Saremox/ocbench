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

#include "job.h"
#include "debug.h"
#include "ocdata/ocdata.h"
#include <stdlib.h>

char* serialize_job(Job* job)
{
  char* fmt_str  = "BEGIN;%ld;%ld;%s;%ld;%ld;%ld;END";
  size_t strsize = snprintf(NULL,0,fmt_str,
    job->jobid,
    job->result->file_id->size,
    job->result->comp_id->codec_id->name,
    job->result->compressed_size,
    job->result->compressed_time,
    job->result->decompressed_time)+1;
  char* ret = malloc(strsize);
              snprintf(ret, strsize, fmt_str,
                job->jobid,
                job->result->file_id->size,
                job->result->comp_id->codec_id->name,
                job->result->compressed_size,
                job->result->compressed_time,
                job->result->decompressed_time);
  return ret;
}

int32_t deserialize_job(Job* job, char* serialized_str)
{
  char* copy = malloc(strlen(serialized_str)+1);
  strcpy(copy, serialized_str);
  char* curPos = strtok(copy, ";");
  if(strcmp("BEGIN", curPos) != 0)
  {
    log_warn("invalid serialized string \"%s\"",curPos);
    return EXIT_FAILURE;
  }

  job->jobid = atoi(strtok(NULL, ";"));

  int file_size = atoi(strtok(NULL, ";"));
  if(job->result == NULL)
    job->result = ocdata_new_result(NULL, NULL, 0, 0, 0);

  if(job->result->file_id == NULL)
    job->result->file_id  = ocdata_new_file(-1, NULL, file_size);

  char* codecname = strtok(NULL, ";");

  if(job->result->comp_id == NULL)
    job->result->comp_id = ocdata_new_comp(-1, NULL, NULL);

  if(job->result->comp_id->codec_id == NULL)
    job->result->comp_id->codec_id = ocdata_new_codec(-1, NULL, codecname);

  job->result->compressed_size   = atoi(strtok(NULL, ";"));
  job->result->compressed_time   = atoi(strtok(NULL, ";"));
  job->result->decompressed_time = atoi(strtok(NULL, ";"));

  curPos = strtok(NULL, ";");
  if(strcmp("END", curPos) != 0)
  {
    log_warn("invalid serialized string \"%s\"",curPos);
    return EXIT_FAILURE;
  }

  free(copy);

  return EXIT_SUCCESS;
}
