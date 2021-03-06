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
#include "ocutils/list.h"

int verbosityLevel = OCDEBUG_ERROR;

List* testlist;

int initTestlist()
{
  testlist = ocutils_list_create();

  return EXIT_SUCCESS;
}

int destroyTestlist()
{
  ocutils_list_destroy(testlist);

  return EXIT_SUCCESS;
}

int TestOcutilsListNewIsEmpty()
{
  initTestlist();
  oc_assert_equal_64bit(0, ocutils_list_size(testlist));
  destroyTestlist();

  return EXIT_SUCCESS;
}

int TestOcutilsListAdd()
{
  initTestlist();

  char* testStr = "I'm a test String";
  ocutils_list_add(testlist,testStr);
  oc_assert_equal_64bit(1, testlist->items);
  oc_assert_str_equal(testStr, (char*) ocutils_list_head(testlist)->value);

  destroyTestlist();
  return EXIT_SUCCESS;
}

int TestOcutilsListAddOrder()
{
  initTestlist();

  char* testStr1 = "I'm a test String";
  char* testStr2 = "I'm a test String too";

  ocutils_list_add(testlist,testStr1);
  ocutils_list_add(testlist,testStr2);

  oc_assert_equal_64bit(0, ocutils_list_getpos(testlist,testStr1));
  oc_assert_equal_64bit(1, ocutils_list_getpos(testlist,testStr2));

  destroyTestlist();
  return EXIT_SUCCESS;
}

int TestOcutilsListQueueEnqueueOrder()
{
  initTestlist();

  char* testStr1 = "I'm a test String";
  char* testStr2 = "I'm a test String too";

  ocutils_list_enqueue(testlist,testStr1);
  ocutils_list_enqueue(testlist,testStr2);

  oc_assert_equal_64bit(0, ocutils_list_getpos(testlist,testStr1));
  oc_assert_equal_64bit(1, ocutils_list_getpos(testlist,testStr2));

  destroyTestlist();
  return EXIT_SUCCESS;
}

int TestOcutilsListQueueDequeueOrder()
{
  initTestlist();

  char* testStr1 = "I'm a test String";
  char* testStr2 = "I'm a test String too";

  ocutils_list_enqueue(testlist,testStr1);
  ocutils_list_enqueue(testlist,testStr2);

  oc_assert_str_equal(testStr1, (char*) ocutils_list_dequeue(testlist));
  oc_assert_str_equal(testStr2, (char*) ocutils_list_dequeue(testlist));

  destroyTestlist();
  return EXIT_SUCCESS;
}

int TestOcutilsListDequeueMoreThanEnqueued()
{
  initTestlist();

  char* testStr1 = "I'm a test String";
  char* testStr2 = "I'm a test String too";

  ocutils_list_enqueue(testlist,testStr1);
  ocutils_list_enqueue(testlist,testStr2);

  oc_assert_str_equal(testStr1, (char*) ocutils_list_dequeue(testlist));
  oc_assert_str_equal(testStr2, (char*) ocutils_list_dequeue(testlist));
  oc_assert_equal_64bit(NULL, ocutils_list_dequeue(testlist));

  destroyTestlist();
  return EXIT_SUCCESS;
}

int TestOcutilsListStackPushOrder()
{
  initTestlist();

  char* testStr1 = "I'm a test String";
  char* testStr2 = "I'm a test String too";

  ocutils_list_push(testlist,testStr1);
  ocutils_list_push(testlist,testStr2);

  oc_assert_equal_64bit(1, ocutils_list_getpos(testlist,testStr1));
  oc_assert_equal_64bit(0, ocutils_list_getpos(testlist,testStr2));

  destroyTestlist();
  return EXIT_SUCCESS;
}

int TestOcutilsListStackPopOrder()
{
  initTestlist();

  char* testStr1 = "I'm a test String";
  char* testStr2 = "I'm a test String too";

  ocutils_list_push(testlist,testStr1);
  ocutils_list_push(testlist,testStr2);

  oc_assert_str_equal(testStr2, (char*) ocutils_list_pop(testlist));
  oc_assert_str_equal(testStr1, (char*) ocutils_list_pop(testlist));

  destroyTestlist();
  return EXIT_SUCCESS;
}

int TestOcutilsListStackPopMoreThanPushed()
{
  initTestlist();

  char* testStr1 = "I'm a test String";
  char* testStr2 = "I'm a test String too";

  ocutils_list_push(testlist,testStr1);
  ocutils_list_push(testlist,testStr2);

  oc_assert_str_equal(testStr2, (char*) ocutils_list_pop(testlist));
  oc_assert_str_equal(testStr1, (char*) ocutils_list_pop(testlist));
  oc_assert_equal_64bit(NULL, ocutils_list_pop(testlist));

  destroyTestlist();
  return EXIT_SUCCESS;
}

int RunAllTests()
{
  oc_run_test(TestOcutilsListNewIsEmpty);
  oc_run_test(TestOcutilsListAdd);
  oc_run_test(TestOcutilsListAddOrder);
  oc_run_test(TestOcutilsListQueueEnqueueOrder);
  oc_run_test(TestOcutilsListQueueDequeueOrder);
  oc_run_test(TestOcutilsListDequeueMoreThanEnqueued);
  oc_run_test(TestOcutilsListStackPushOrder);
  oc_run_test(TestOcutilsListStackPopOrder);
  oc_run_test(TestOcutilsListStackPopMoreThanPushed);

  return EXIT_SUCCESS;
}

RUN_TESTS(RunAllTests);
