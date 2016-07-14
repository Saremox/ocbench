#include "ocmemfd/ocmemfd.h"
#include "ocunit.h"

#define SIZEOFMEMFD 512

ocMemfdContext * fd;

int Testocmemfd_create_context()
{
  oc_assert((fd= ocmemfd_create_context("/testmemfd",SIZEOFMEMFD)) > 0,
    "create context should not return null pointer");
  return EXIT_SUCCESS;
}

int TestMemoryMap()
{
  oc_assert(fd->buf > 0,"memory map pointer is not valid")
  sprintf(fd->buf , "Test");
  oc_assert_str_equal("Test", fd->buf);

  return EXIT_SUCCESS;
}

int TestGetSeals()
{
  int ret = memfd_is_flag_set(fd->fd,F_SEAL_GROW);

  return EXIT_SUCCESS;
}

int TestResizingMemfdGrow()
{
  int newsize = 1024;
  int oldsize = SIZEOFMEMFD;

  oc_assert_equal(oldsize, fd->size);
  oc_assert(ocmemfd_resize(fd,newsize) == OCMEMFD_SUCCESS,"resizing was not successful");
  oc_assert_equal(newsize, fd->size);

  return EXIT_SUCCESS;
}

int TestResizingMemfdShrink()
{
  int newsize = SIZEOFMEMFD;
  int oldsize = 1024;

  oc_assert_equal(oldsize, fd->size);
  oc_assert(ocmemfd_resize(fd,newsize) == OCMEMFD_SUCCESS,"resizing was not successful");
  oc_assert_equal(newsize, fd->size);

  return EXIT_SUCCESS;
}

int TestAfterResizeDataStillExists()
{
  int newsize = 768;
  int oldsize = SIZEOFMEMFD;

  ocmemfd_resize(fd,newsize);
  oc_assert_str_equal("Test", fd->buf);

  return EXIT_SUCCESS;
}

int RunAllTests()
{
  oc_run_test(Testocmemfd_create_context);
  oc_run_test(TestGetSeals);
  oc_run_test(TestMemoryMap);
  oc_run_test(TestResizingMemfdGrow);
  oc_run_test(TestResizingMemfdShrink);
  oc_run_test(TestAfterResizeDataStillExists);


  return EXIT_SUCCESS;
}

RUN_TESTS(RunAllTests);
