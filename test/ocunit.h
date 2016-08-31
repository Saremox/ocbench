/**
 * This File is part of openCompressBench
 * Copyright (C) 2016 Michael Strassberger <saremox@linux.com>
 *
 * openCompressBench is free software: you can redistribute
 * it and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation,
 * version 2 of the License.
 *
 * Some open source application is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @license GPL-2.0 <https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html>
 */
 
#ifndef OCUNITH
#define OCUNITH

#include "debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define OCUNITFAILURE -1

#define oc_unit_output(msg, ...) {\
  printf("\n│  │  └─ Line: %d " msg,__LINE__,##__VA_ARGS__);}
#define oc_assert(test, msg) if(!(test)) {\
  oc_unit_output(msg);\
  return OCUNITFAILURE;}
#define oc_assert_equal_8bit(expected, got) if(expected != got) {\
  oc_unit_output("expected: <0x%hhx> but got : <0x%hhx>",\
    (uint8_t) expected,(uint8_t) got)\
  return OCUNITFAILURE;}
#define oc_assert_equal_16bit(expected, got) if(expected != got) {\
  oc_unit_output("expected: <0x%hx> but got : <0x%hx>",\
    (uint16_t) expected,(uint16_t) got)\
  return OCUNITFAILURE;}
#define oc_assert_equal_32bit(expected, got) if(expected != got) {\
  oc_unit_output("expected: <0x%x> but got : <0x%x>",\
    (uint32_t) expected,(uint32_t) got)\
  return OCUNITFAILURE;}
#define oc_assert_equal_64bit(expected, got) if(expected != got) {\
  oc_unit_output("expected: <0x%lx> but got : <0x%lx>",\
    (uint64_t) expected,(uint64_t) got)\
  return OCUNITFAILURE;}
#define oc_assert_equal_128bit(expected, got) if(expected != got) {\
  oc_unit_output("expected: <0x%llx> but got : <0x%llx>",\
    (uint128_t) expected,(uint128_t) got)\
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
