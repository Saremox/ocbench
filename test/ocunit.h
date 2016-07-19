#ifndef OCUNITH
#define OCUNITH

#include "debug.h"
#include <stdio.h>
#include <stdlib.h>

#define OCUNITFAILURE -1

#define oc_unit_output(msg, ...) {\
  printf("\n│  │  └─ Line: %d " msg,__LINE__,##__VA_ARGS__);}
#define oc_assert(test, msg) if(!(test)) {\
  oc_unit_output(msg);\
  return OCUNITFAILURE;}
#define oc_assert_equal(expected, got) if(expected != got) {\
  oc_unit_output("expected: <0x%x> but got : <0x%x>",expected,got)\
  return OCUNITFAILURE;}
#define oc_assert_str_equal(expected, got) if(strcmp(expected,got) != 0) {\
  oc_unit_output("expected \"%s\" but got \"%s\"",expected,got );\
  return OCUNITFAILURE;}
#define oc_assert_mem_equal(expected, got, size) if(memcmp(expected,got,size) != 0) {\
  oc_unit_output("expected \"%s\" but got \"%s\"",expected,got );\
  return OCUNITFAILURE;}
#define oc_run_test(test) printf("\n│  ├─ Case #%04d%-40s:",ocunit_run_tests, " " #test); \
  fflush(stdout);\
  ocunit_last_return = test();\
  ocunit_run_tests++;\
  if(ocunit_last_return != 0)\
    return -1;\
  printf(" PASSED");\
  ocunit_passed_tests++;\

#define RUN_TESTS(name) int main(int argc, char *argv[]) {\
    argc = 1; \
    printf("├─ RUNNING: %s", argv[0]);\
        name();\
    printf("\n│  └─ Tests passed : %d out of %d\n",\
           ocunit_passed_tests, ocunit_run_tests);\
        exit(0);\
}

int ocunit_passed_tests;
int ocunit_run_tests;
int ocunit_last_return;
#endif
