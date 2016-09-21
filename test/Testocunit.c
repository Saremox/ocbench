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

int verbosityLevel = OCDEBUG_DEBUG;

int TestOcunitAssert()
{
  oc_assert(1 == 1, "test is true but assert fires");

  return EXIT_SUCCESS;
}

int TestOcunitAssertEqual32Bit()
{
  uint32_t i = 1;
  oc_assert_equal_32bit(i,i);

  return EXIT_SUCCESS;
}

int TestOcunitAssertStrEqual()
{
  oc_assert_str_equal("Hello","Hello");

  return EXIT_SUCCESS;
}

int RunAllTests()
{
  oc_run_test(TestOcunitAssert);
  oc_run_test(TestOcunitAssertEqual32Bit);
  oc_run_test(TestOcunitAssertStrEqual);

  return EXIT_SUCCESS;
}

RUN_TESTS(RunAllTests);
