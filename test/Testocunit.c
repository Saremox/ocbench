#include "ocunit.h"

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
