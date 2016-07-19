#include "ocmemfd/ocmemfd.h"
#include "ocunit.h"
#include <fcntl.h>
#include <sys/stat.h>

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

  oc_assert(ocmemfd_resize(fd,newsize) == OCMEMFD_SUCCESS,"resizing was not successful");
  oc_assert_equal(newsize, fd->size);
  // Test if we can set every byte in memfd
  memset(fd->buf,0x00,fd->size);

  return EXIT_SUCCESS;
}

int TestResizingMemfdGrowBig()
{
  int newsize = 16 * 1024 * 1024;

  oc_assert(ocmemfd_resize(fd,newsize) == OCMEMFD_SUCCESS,"resizing was not successful");
  oc_assert_equal(newsize, fd->size);
  // Test if we can set every byte in memfd
  memset(fd->buf,0x00,fd->size);

  return EXIT_SUCCESS;
}

int TestResizingMemfdShrink()
{
  int newsize = SIZEOFMEMFD;

  oc_assert(ocmemfd_resize(fd,newsize) == OCMEMFD_SUCCESS,"resizing was not successful");
  oc_assert_equal(newsize, fd->size);
  // Test if we can set every byte in memfd
  memset(fd->buf,0x00,fd->size);

  return EXIT_SUCCESS;
}

int TestResizingMemfdShrinkTiny()
{
  int newsize = 16;

  oc_assert(ocmemfd_resize(fd,newsize) == OCMEMFD_SUCCESS,"resizing was not successful");
  oc_assert_equal(newsize, fd->size);
  // Test if we can set every byte in memfd
  memset(fd->buf,0x00,fd->size);

  return EXIT_SUCCESS;
}

int TestAfterResizeDataStillExists()
{
  int newsize = SIZEOFMEMFD + 256;

  sprintf(fd->buf , "Test");
  ocmemfd_resize(fd,newsize);
  oc_assert_str_equal("Test", fd->buf);

  return EXIT_SUCCESS;
}

int TestLoadFileIntoMemfd()
{
  int file;
  struct stat st;
  off_t filesize = 0;
  char * path = "./Testocmemfd";
  char * memorymap;

  oc_unit_output("test %s\n",path);
  if(stat(path,&st) ==0)
    filesize = st.st_size;
  else
    log_err("cannot stat %s for testing",path);

  oc_unit_output("Open %s in memfd\n",path);
  oc_assert(ocmemfd_load_file(fd,path) == OCMEMFD_SUCCESS, "failed to load file");
  oc_unit_output("Open %s and memory map it",path);
  file = open(path,0,'r');
  memorymap = mmap(NULL, filesize, PROT_READ,MAP_PRIVATE, file,0);

  oc_assert(memorymap != MAP_FAILED, "failed to memorymap test file");
  oc_unit_output("Beginning to compare memory of size %d",filesize);
  oc_unit_output("fd->buf points to 0x%x",fd->buf);
  oc_unit_output("memorymap points to 0x%x",memorymap);
  sleep(2);
  for (size_t i = 0; i < filesize; i++) {
    oc_unit_output("test memorymap[%d] and fd->buf[%d] are equal\n",i,i);
    oc_unit_output("memorymap[%d] = 0x%x\n",i,memorymap[i]);
    oc_unit_output("fd->buf[%d] = 0x%x\n",i,fd->buf[i]);
    oc_assert_equal(memorymap[i],fd->buf[i]);
  }
  oc_unit_output("Done memory test\n");
  oc_assert_mem_equal(memorymap,fd->buf,filesize-1);

  munmap(memorymap,filesize);
  close(file);\

  return EXIT_SUCCESS;
}

int TestDetroyContext()
{
  oc_assert(ocmemfd_destroy_context(&fd) == OCMEMFD_SUCCESS,
    "failed to destroy context");

  return EXIT_SUCCESS;
}

int RunAllTests()
{
  oc_run_test(Testocmemfd_create_context);
  oc_run_test(TestGetSeals);
  oc_run_test(TestMemoryMap);
  oc_run_test(TestResizingMemfdGrow);
  oc_run_test(TestResizingMemfdGrowBig);
  oc_run_test(TestResizingMemfdShrink);
  oc_run_test(TestResizingMemfdShrinkTiny);
  oc_run_test(TestAfterResizeDataStillExists);
  oc_run_test(TestLoadFileIntoMemfd);
  oc_run_test(TestDetroyContext);


  return EXIT_SUCCESS;
}

RUN_TESTS(RunAllTests);
