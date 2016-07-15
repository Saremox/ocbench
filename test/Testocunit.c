#include "ocunit.h"

int TestOcunitAssert()
{
  oc_assert(1 == 1, "test is true but assert fires");

  return EXIT_SUCCESS;
}

int TestOcunitAssertEqual()
{
  oc_assert_equal(1,1);

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
  oc_run_test(TestOcunitAssertEqual);
  oc_run_test(TestOcunitAssertStrEqual);

  return EXIT_SUCCESS;
}

RUN_TESTS(RunAllTests);
