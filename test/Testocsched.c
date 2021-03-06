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

#include "ocunit.h"
#include "ocsched/ocsched.h"
#include <unistd.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/ioctl.h>

int verbosityLevel = OCDEBUG_DEBUG;

#define EXIT_TEST_OK 42
#define TESTSTRING "Hello world!"

int TestProcessCreationChild(ocschedProcessContext* ctx, void* data) {
  oc_assert(ctx->pid==0, "pid should be zero in child");
  exit(EXIT_TEST_OK);
}

int TestProcessCreation()
{
  int ret;
  ocschedProcessContext * ctx =
    ocsched_fork_process((ocschedFunction) TestProcessCreationChild,
      "test",NULL);

  oc_assert(ctx->pid>0, "pid of child should be greater than 0");

  waitpid(ctx->pid,&ret,0);
  oc_assert_equal_32bit(EXIT_TEST_OK, WEXITSTATUS(ret));

  return EXIT_SUCCESS;
}

int TestCommMasterToChildChild(ocschedProcessContext* ctx, void* data) {
  int count = 0, tries = 0;
  while (count < strlen(TESTSTRING) - 1 && tries <= 100) {
    ioctl(ctx->comm.pipe_in, FIONREAD, &count);
    struct timespec sleeptimer = {0,100000};
    nanosleep(&sleeptimer,NULL);
    tries++;
  }
  oc_assert(tries < 100, " child timeout on reading pipe");
  char buf[count+1];
  oc_assert(read(ctx->comm.pipe_in,&buf,count) > 0,"failed to read");
  buf[count] = 0x00;
  oc_assert_str_equal(TESTSTRING,buf)

  exit(EXIT_TEST_OK);
}

int TestCommMasterToChild()
{
  int ret;
  ocschedProcessContext * ctx =
    ocsched_fork_process((ocschedFunction) TestCommMasterToChildChild,
      "test",NULL);

  oc_assert(write(ctx->comm.pipe_out,TESTSTRING,strlen(TESTSTRING)) > 0,
    "failed to write");

  waitpid(ctx->pid,&ret,0);
  oc_assert_equal_32bit(EXIT_TEST_OK, WEXITSTATUS(ret));

  return EXIT_SUCCESS;
}

int TestCommChildToMasterChild(ocschedProcessContext* ctx, void* data) {
  oc_assert(write(ctx->comm.pipe_out,TESTSTRING,strlen(TESTSTRING)),
    "failed to write");

  exit(EXIT_TEST_OK);
}

int TestCommChildToMaster()
{
  int ret, count = 0, tries = 0;
  ocschedProcessContext * ctx =
    ocsched_fork_process((ocschedFunction) TestCommChildToMasterChild,
      "test",NULL);

  while (count < strlen(TESTSTRING) - 1 && tries <= 100) {
    ioctl(ctx->comm.pipe_in, FIONREAD, &count);
    struct timespec sleeptimer = {0,100000};
    nanosleep(&sleeptimer,NULL);
    tries++;
  }
  oc_assert(tries < 100, " child timeout on reading pipe");
  char buf[count+1];
  oc_assert(read(ctx->comm.pipe_in,&buf,count),"failed to read");
  buf[count] = 0x00;
  oc_assert_str_equal(TESTSTRING,buf)

  waitpid(ctx->pid,&ret,0);
  oc_assert_equal_32bit(EXIT_TEST_OK, WEXITSTATUS(ret));

  return EXIT_SUCCESS;
}

int RunAllTests()
{
  oc_run_test(TestProcessCreation);
  oc_run_test(TestCommMasterToChild);
  oc_run_test(TestCommChildToMaster);

  return EXIT_SUCCESS;
}

RUN_TESTS(RunAllTests);
