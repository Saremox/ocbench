#include <stdio.h>
#include <stdlib.h>
#include <wait.h>
#include <getopt.h>
#include <time.h>
#include "debug.h"
#include "ocbenchConfig.h"
#include "ocmemfd/ocmemfd.h"
#include "ocsched/ocsched.h"
#include "ocdata/ocdata.h"
#include "ocutils/list.h"
#include <squash/squash.h>

char* databasePath    = "ocbench.sqlite";
char* directoryPath   = "./";
char* codecs          = "bzip2:bzip2,lzma:xz,zlib:gzip";
int   worker          = 1;
int   verbosityLevel  = OCDEBUG_ERROR;

#define MAX_MEMFD_BUF 1024

void print_help(char* programname)
{
  printf(
"Usage: %s [OPTION]... --directory=./\n"
"\n"
"Options:\n"
"  -D, --database   PATH to sqlite3 database file\n"
"                   Default: ./results.sqlite3\n"
"  -d, --directory  PATH to directory which will be analyzed\n"
"                   Default: current working directory\n"
"  -c, --codecs     STRING comma seperated list of codecs which will\n"
"                   be used. Format: \"plugin:codec;codec,plugin:...\"\n"
"                   Default: \"bzip2:bzip2,lzma:xz,zlib:gzip\"\n"
"  -w, --Worker     INT ammount of worker processes.\n"
"                   Default: 1\n\n"
"  -v, --verbose    more verbosity equals --log-level=2\n"
"  -l, --log-level  INT 0 LOG_ERR\n"
"                       1 LOG_WARN\n"
"                       2 LOG_INFO\n"
"                       3 LOG_DEBUG\n"
"  -h, --help       Print this page\n"
"  -u, --Usage      Print this page\n"
"  -V, --version    Print version of %s\n\n"
,programname,programname);
  exit(EXIT_SUCCESS);
}

void print_version(char* programname)
{
printf(
"%s openCompressBench Version %d.%d\n"
"Copyright (C) 2016 Michael Strassberger "
"<saremox@linux.com>\n"
"openCompressBench comes with ABSOLUTELY NO WARRANTY.\n"
"This is free software, and you are welcome\n"
"to redistribute it under certain conditions;\n",
programname,OCBENCH_VERSION_MAJOR,OCBENCH_VERSION_MINOR);
}

off_t file_size(const char *filename) {
  struct stat st;

  if (stat(filename, &st) == 0)
    return st.st_size;

  return -1;
}

bool parse_dir(char* path, List* files)
{
  DIR* curdir;
  struct dirent* currentfile;

  if(path[strlen(path)-1] != '/')
  {
    char* newpath = malloc(snprintf(0, 0, "%s%s",path,"/")+1);
    sprintf(newpath, "%s%s\0", path, "/");
    path = newpath;
  }

  debug("Open Dir: %s",path);
  curdir = opendir(path);
  if(curdir == NULL && errno != ENOTDIR)
  {
    log_warn("Cannot open path \"%s\" as directory",path);
    return false;
  }
  else if (curdir == NULL && errno == ENOTDIR)
  {
    return false;
  }
  debug("Reading Dir: %s",path);
  while ((currentfile = readdir(curdir)) != 0) \
  {
    // Skip cur and parent directory fd's
    if( strcmp(currentfile->d_name, ".")  == 0 ||
        strcmp(currentfile->d_name, "..") == 0)
        continue;
    debug("Testing File: %s",currentfile->d_name);
    char* filepath = malloc(snprintf(0, 0, "%s%s",path,
                                     currentfile->d_name)+1);
    sprintf(filepath,"%s%s\0", path, currentfile->d_name);
    char* dircheck = malloc(snprintf(0, 0, "%s%s",filepath,"/")+1);
    sprintf(dircheck, "%s%s\0", filepath, "/");
    if (parse_dir(dircheck, files)) {
      free(filepath);
      free(dircheck);
      continue;
    }
    free(dircheck);
    ocdataFile* file = malloc(sizeof(ocdataFile));
    file->path    = filepath;
    file->size    = file_size(filepath);
    file->file_id = -1;
    ocutils_list_add(files, file);
    debug("Add: %s with a size of %ld bytes",file->path,file->size);
  }
  closedir(curdir);
  free(currentfile);
  errno=0;
  return true;
}

void childprocess(ocschedProcessContext* parent, void* data)
{
  int read_tries = 0;
  char recvbuf[1024];
  size_t recvBytes = 0;
  size_t compressed_length = 0;
  SquashCodec* codec = squash_get_codec("lzma");
  ocMemfdContext * decompressed = (ocMemfdContext *) data;

  check(codec > 0,"failed to get squash plugin lzma");

  while (read_tries <= 100) {
    size_t tmp = 0;
    tmp = ocsched_recvfrom(parent,&recvbuf[recvBytes],1);
    check(tmp >= 0,"failed to read from parent");
    if(recvbuf[recvBytes] == '\n')
    {
      recvbuf[recvBytes] == 0x00;
      break;
    }
    else
      recvBytes += tmp;
    struct timespec sleeptimer = {0,100000};
    nanosleep(&sleeptimer,NULL);
    read_tries++;
  }

  int oldsize = decompressed->size;
  int newsize = atoi(recvbuf);

  debug("child got size of shm %d bytes",newsize);

  decompressed->size = newsize;
  ocmemfd_remap_buffer(decompressed,oldsize);

  ocMemfdContext * compressed =
    ocmemfd_create_context("/compressed",
      squash_codec_get_max_compressed_size(codec,decompressed->size));
  compressed_length = compressed->size;
  debug("child compressing data");
  int ret = squash_codec_compress(codec,
                                  &compressed_length,
                                  compressed->buf,
                                  decompressed->size,
                                  decompressed->buf,
                                  NULL);
  check(ret == SQUASH_OK,"failed to compress data [%d] : %s",
    ret,squash_status_to_string(ret));
  debug("compression done. from %d to %d",
    (int32_t) decompressed->size, (int32_t) compressed_length);
  ocsched_printf(parent,"compressed from %d bytes to %d bytes with lzma",
    (int32_t) decompressed->size, (int32_t) compressed_length);
error:
  ocmemfd_destroy_context(&compressed);
  ocmemfd_destroy_context(&decompressed);

  return;
}

int compressionTest(char * file)
{
  char buf[1024];
  int childreturn = 0;
  ocMemfdContext * fd = ocmemfd_create_context("/cache",MAX_MEMFD_BUF);

  debug("forking child");
  ocschedProcessContext * child =
    ocsched_fork_process(childprocess,"child",fd);
  debug("loading file");
  ocmemfd_load_file(fd,file);
  debug("send filesize %d bytes to child",(int32_t) fd->size);
  ocsched_printf(child,"%d\n",fd->size);
  waitpid(child->pid,&childreturn,0);
  if(WIFSIGNALED(childreturn))
  {
    errno=WTERMSIG(childreturn);
    log_err("child failed");
  }
  int bytes_recv = ocsched_recvfrom(child,buf,1024);
  buf[bytes_recv] = 0x00;

  printf("Recv from child: %s\n",buf);

  ocmemfd_destroy_context(&fd);
  ocsched_destroy_context(&child);
  ocsched_destroy_global_mqueue();

  return 0;
}

void parse_arguments(int argc, char *argv[])
{
  int c,tmp;
  int optind = 0;

  while (1) {
    int this_option_optind = optind ? optind : 1;
    int option_index = 0;
    static struct option long_options[] = {
        {"codecs",    required_argument, 0, 'c'},
        {"database",  required_argument, 0, 'D'},
        {"worker",    required_argument, 0, 'w'},
        {"directory", required_argument, 0, 'd'},
        {"verbose",   no_argument,       0, 'v'},
        {"log-level", required_argument, 0, 'l'},
        {"help",      no_argument,       0, 'h'},
        {"usage",     no_argument,       0, 'u'},
        {"version",   no_argument,       0, 'V'},
        {0,           0,                 0,  0 }
    };

    c = getopt_long(argc, argv, "c:D:w:vd:huVl:",
             long_options, &option_index);
    if (c == -1)
      break;

    switch (c) {
    case 'c':
      debug("Setting codecs to: \"%s\"",optarg);
      codecs = optarg;
      break;
    case 'D':
      debug("Setting Database file to: \"%s\"",optarg);
      databasePath = optarg;
      break;
    case 'w':
      debug("Setting Worker count to: \"%s\"",optarg);
      worker = atoi(optarg);
      check(worker > 0, "Worker count must be greater than 0");
      break;
    case 'v':
      debug("Turn on Verbosity");
      verbosityLevel = OCDEBUG_INFO;
      break;
    case 'l':
      tmp = atoi(optarg);
      if(tmp >= 0 && tmp <= OCDEBUG_DEBUG)
        verbosityLevel = tmp;
      else
        log_err("Invalid log level: \"%s\"",optarg);
      break;
    case 'd':
      debug("Setting directory for analysis to: \"%s\"",optarg);
      directoryPath = optarg;
      break;
    case 'h':
    case 'u':
      print_help(argv[0]);
      break;
    case 'V':
      print_version(argv[0]);
      exit(EXIT_SUCCESS);
    }
  }
  return;
  error:
    print_help(argv[0]);
}

int main (int argc, char *argv[])
{
  parse_arguments(argc,argv);
  ocdataContext* myctx;
  ocdataFile testfile   = {-1,"/src/ocBench",5000};
  ocdataPlugin bzip2p   = {-1,"bzip2"};
  ocdataCodec bzip2     = {-1,&bzip2p ,"bzip2"};
  ocdataOption level    = {-1,"Level"};
  ocdataOption dictsize = {-1,"Dict Size"};
  ocdataCompresion comp = {-1,&bzip2,ocutils_list_create()};
  ocdataCompressionOption levelbzip2 = {&comp,&level,"7"};
  ocdataCompressionOption dictsizebzip2 = {&comp,&dictsize,"128"};
  ocdataResult            res = {&comp,&testfile,2500,2500};
  ocutils_list_push(comp.options,&levelbzip2);
  ocutils_list_push(comp.options,&dictsizebzip2);

  ocdata_create_context(&myctx,"testdb.sqlite",0);

  ocdata_add_result(myctx,&res);

  compressionTest(argv[1]);
  ocdata_destroy_context(&myctx);
  return 0;
}
