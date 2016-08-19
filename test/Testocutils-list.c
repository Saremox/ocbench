#include "ocunit.h"
#include "ocutils/list.h"

int verbosityLevel = OCDEBUG_DEBUG;

List* testlist;

int initTestlist()
{
  testlist = ocutils_list_create();
}

int destroyTestlist()
{
  ocutils_list_destroy(testlist);
}

int TestOcutilsListNewIsEmpty()
{
  initTestlist();
  oc_assert_equal_64bit(0, ocutils_list_size(testlist));
  destroyTestlist();
}

int TestOcutilsListAdd()
{
  initTestlist();

  char* testStr = "I'm a test String";
  ocutils_list_add(testlist,testStr);
  oc_assert_equal_64bit(1, testlist->items);
  oc_assert_str_equal(testStr, ocutils_list_head(testlist)->value);

  destroyTestlist();
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
}

int TestOcutilsListQueueEnqueueOrder()
{
  initTestlist();

  char* testStr1 = "I'm a test String";
  char* testStr2 = "I'm a test String too";

  ocutils_list_add(testlist,testStr1);
  ocutils_list_add(testlist,testStr2);

  oc_assert_equal_64bit(0, ocutils_list_getpos(testlist,testStr1));
  oc_assert_equal_64bit(1, ocutils_list_getpos(testlist,testStr2));

  destroyTestlist();
}

int TestOcutilsListQueueDequeueOrder()
{
  initTestlist();

  char* testStr1 = "I'm a test String";
  char* testStr2 = "I'm a test String too";

  ocutils_list_add(testlist,testStr1);
  ocutils_list_add(testlist,testStr2);

  oc_assert_str_equal(testStr1, ocutils_list_dequeue(testlist));
  oc_assert_str_equal(testStr2, ocutils_list_dequeue(testlist));

  destroyTestlist();
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
}

int TestOcutilsListStackPopOrder()
{
  initTestlist();

  char* testStr1 = "I'm a test String";
  char* testStr2 = "I'm a test String too";

  ocutils_list_push(testlist,testStr1);
  ocutils_list_push(testlist,testStr2);

  oc_assert_str_equal(testStr2, ocutils_list_pop(testlist));
  oc_assert_str_equal(testStr1, ocutils_list_pop(testlist));

  destroyTestlist();
}

int TestOcutilsListStackPopMoreThanPushed

int RunAllTests()
{
  oc_run_test(TestOcutilsListNewIsEmpty);
  oc_run_test(TestOcutilsListAdd);
  oc_run_test(TestOcutilsListAddOrder);
  oc_run_test(TestOcutilsListQueueEnqueueOrder);
  oc_run_test(TestOcutilsListQueueDequeueOrder);
  oc_run_test(TestOcutilsListStackPushOrder);
  oc_run_test(TestOcutilsListStackPopOrder);

  return EXIT_SUCCESS;
}

RUN_TESTS(RunAllTests);
