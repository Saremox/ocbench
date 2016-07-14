#ifndef OCUNITH
#define OCUNITH

#include "debug.h"
#include <stdio.h>
#include <stdlib.h>

#define oc_assert(test, msg) if(!(test)) { log_err(msg); return msg; }
#define oc_assert_equal(expected, got) if(expected != got) {\
  log_err("expected: <0x%x> but got : <0x%x>",expected,got)}
#define oc_run_test(test) printf("\n│  ├─ Case #%04d %s:",tests_run, " " #test); \
  lastmsg = test();\
  tests_run++;\
  if(lastmsg != 0)\
    return lastmsg;\
  printf(" PASSED");\
  passed_test++;\

#define RUN_TESTS(name) int main(int argc, char *argv[]) {\
    argc = 1; \
    printf("├─ RUNNING: %s", argv[0]);\
        char *result = name();\
        if (result != 0) {\
            printf(" FAILED: %s\n", result);\
        }\
    printf("\n│  └─ Tests passed : %d out of %d\n", passed_test, tests_run);\
        exit(0);\
}

int passed_test;
int tests_run;
char *lastmsg;
#endif
