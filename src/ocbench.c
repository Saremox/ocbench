#include <dirent.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <wait.h>
#include <sys/stat.h>
#include "debug.h"
#include "ocbenchConfig.h"
#include "ocmemfd/ocmemfd.h"
#include "ocsched/ocsched.h"
#include "ocdata/ocdata.h"
#include "ocutils/list.h"
#include "ocworker.h"
#include <squash/squash.h>


// options

char* databasePath    = "ocbench.sqlite";
char* directoryPath   = "./";
char* codecs          = "bzip2:bzip2,lzma:xz,zlib:gzip";
int   worker          = 1;
int   verbosityLevel  = OCDEBUG_WARN;

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
"  -q  --quiet      log only errors equals --log-level=0\n"
"  -l, --log-level  INT 0 LOG_ERR\n"
"                       1 LOG_WARN DEFAULT\n"
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
"%s openCompressBench Version %d.%d.%d+%s@%s\n"
"Copyright (C) 2016 Michael Strassberger "
"<saremox@linux.com>\n"
"openCompressBench comes with ABSOLUTELY NO WARRANTY.\n"
"This is free software, and you are welcome\n"
"to redistribute it under certain conditions;\n",
programname,OCBENCH_VERSION_MAJOR,OCBENCH_VERSION_MINOR,OCBENCH_VERSION_PATCH,
GIT_BRANCH,GIT_COMMIT_HASH);
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
    sprintf(newpath, "%s%s", path, "/");
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
    sprintf(filepath,"%s%s", path, currentfile->d_name);
    char* dircheck = malloc(snprintf(0, 0, "%s%s",filepath,"/")+1);
    sprintf(dircheck, "%s%s", filepath, "/");
    if (parse_dir(dircheck, files)) {
      free(filepath);
      free(dircheck);
      continue;
    }
    ocdataFile* file = ocdata_new_file(-1, filepath, file_size(filepath));
    if(file->size == 0)
      ocdata_free_file(file);
    else
      ocutils_list_add(files, file);
    free(filepath);
    free(dircheck);
    debug("Add: %s with a size of %ld bytes",file->path,file->size);
  }
  closedir(curdir);
  free(currentfile);
  errno=0;
  return true;
}

void parse_codecs(char* codecstring, List* codecList)
{
  char*  copycodecs    = malloc(strlen(codecstring)+1);
                         memcpy(copycodecs, codecstring, strlen(codecstring)+1);
  List*  pluginstrings = ocutils_list_create();

  // Reading plugin string
  char* curptr = strtok(copycodecs, ",");
  while (curptr != NULL)
  {
    ocutils_list_add(pluginstrings, curptr);
    curptr = strtok(NULL, ",");
  }

  ocutils_list_foreach_f(pluginstrings, pluginstring, char*)
  {
    char* pluginname = strtok(pluginstring, ":");

    ocdataPlugin* tmpPlugin = ocdata_new_plugin(-1, pluginname);

    SquashPlugin* testplugin = squash_get_plugin(pluginname);
    check(testplugin != NULL,"Squash didn't found plugin name \"%s\"",
      pluginname);

    char* codecsForPlugin = strtok(NULL, ":");
    check(codecsForPlugin != NULL,
      "no codecs specified for plugin %s. Forgot ':'?"
      ,pluginname)

    char* codecName = strtok(codecsForPlugin, ";");
    while (codecName != NULL)
    {
      ocdataCodec* tmpCodec = ocdata_new_codec(-1, tmpPlugin, codecName);
      ocutils_list_add(codecList, tmpCodec);
      SquashCodec* testcodec = squash_get_codec(tmpCodec->name);
      check(testcodec != NULL,
        "Squash didn't found codec \"%s\" in plugin \"%s\"",
        tmpCodec->name,tmpCodec->plugin_id->name);
      codecName = strtok(NULL, ";");
    }
  }

  free(copycodecs);
  ocutils_list_destroy(pluginstrings);
  return;
error:
  exit(EXIT_FAILURE);
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
        {"quiet",     no_argument,       0, 'q'},
        {"log-level", required_argument, 0, 'l'},
        {"help",      no_argument,       0, 'h'},
        {"usage",     no_argument,       0, 'u'},
        {"version",   no_argument,       0, 'V'},
        {0,           0,                 0,  0 }
    };

    c = getopt_long(argc, argv, "c:D:w:vqd:huVl:",
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
    case 'q':
      debug("Turn on quiet");
      verbosityLevel = OCDEBUG_ERROR;
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
  List* files     = ocutils_list_create();
  List* codecList = ocutils_list_create();

  parse_arguments(argc,argv);
  parse_dir(directoryPath, files);
  parse_codecs(codecs,codecList);
  if(verbosityLevel == OCDEBUG_DEBUG)
  {
    ocutils_list_foreach_f(codecList, curCodec, ocdataCodec*)
    {
      debug("Plugin: %8s with Codec: %8s",
        curCodec->plugin_id->name,
        curCodec->name);
    }
  }
  check(files->items > 0, "No Files found at \"%s\"",directoryPath);

  List*            jobs;
  ocworkerContext* workerctx = NULL;
  ocworker_start(worker,&workerctx);
  check(workerctx!=NULL,"worker context should not be NULL");

  ocutils_list_foreach_f(files, curfile, ocdataFile*)
  {
    ocworker_schedule_jobs(workerctx, curfile,
      codecList, &jobs);
    // Destroy jobid list since we don't need it by now
    ocutils_list_destroy(jobs);
  }

  // Workaround to ensure jobs are scheduled for exit check
  struct timespec sleeptimer = {2,0};
  nanosleep(&sleeptimer,NULL);

  while (ocworker_is_running(workerctx) == OCWORKER_IS_RUNNING) {
    struct timespec sleeptimer = {0,250*1000*1000};
    nanosleep(&sleeptimer,NULL);
  }

  ocworker_kill(workerctx);

  ocdataContext* myctx;
  jobs   = workerctx->jobsDone;

  ocdata_create_context(&myctx,databasePath,0);

  ocutils_list_foreach_f(jobs, curjob,ocworkerJob*)
  {
    ocdata_add_result(myctx, curjob->result);
  }

  ocdata_destroy_context(&myctx);

  ocdata_garbage_collect();
  ocutils_list_destroy(codecList);
  ocutils_list_destroy(files);
  return EXIT_SUCCESS;
  error:
  return EXIT_FAILURE;
}
