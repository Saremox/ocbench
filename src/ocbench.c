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

char*             transactionlog  = NULL;
char*             databasePath    = "ocbench.sqlite";
char*             directoryPath   = "./";
char*             codecs          = "bzip2:bzip2,lzma:xz,zlib:gzip";
int               worker          = 1;
int               verbosityLevel  = OCDEBUG_WARN;

// Globals
List*             files;
List*             codecList;
List*             jobs;
ocworkerContext*  workerctx = NULL;
ocdataContext*    myctx     = NULL;
int               shutdown_request = 0;

void print_help(char* programname)
{
  printf(
"Usage: %s [OPTION]... --directory=./\n"
"\n"
"Options:\n"
"  -D, --database       PATH to sqlite3 database file\n"
"                       Default: ./results.sqlite3\n"
"  -d, --directory      PATH to directory which will be analyzed\n"
"                       Default: current working directory\n"
"  -t, --transactionlog PATH to a transactionlog file\n"
"                       if the program get's terminated before finished this\n"
"                       file will be used to resume execution there it left\n"
"  -c, --codecs         STRING comma seperated list of codecs which will\n"
"                       be used. Format: \"plugin:codec;codec,plugin:...\"\n"
"                       Default: \"bzip2:bzip2,lzma:xz,zlib:gzip\"\n"
"  -w, --Worker         INT ammount of worker processes.\n"
"                       Default: 1\n\n"
"  -v, --verbose        more verbosity equals --log-level=2\n"
"  -q  --quiet          log only errors equals --log-level=0\n"
"  -l, --log-level      INT 0 LOG_ERR\n"
"                           1 LOG_WARN DEFAULT\n"
"                           2 LOG_INFO\n"
"                           3 LOG_DEBUG\n"
"  -L, --license        \n"
"  -W, --warranty       Print warranty details\n"
"  -C, --conditions     Print redistribution conditions\n"
"  -h, --help           Print this page\n"
"  -u, --Usage          Print this page\n"
"  -V, --version        Print version of %s\n\n"
,programname,programname);
  exit(EXIT_SUCCESS);
}

void print_version(char* programname)
{
printf(
"%s openCompressBench Version %d.%d.%d+%s@%s\n"
"Copyright (C) 2016 Michael Strassberger "
"<saremox@linux.com>\n"
"License GPLv2: GNU GPL Version 2\n"
"<https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html>\n"
"This is free software: you are free to change and redistribute it.\n"
"There is NO WARRANTY, to the extent permitted by law.\n",
programname,OCBENCH_VERSION_MAJOR,OCBENCH_VERSION_MINOR,OCBENCH_VERSION_PATCH,
GIT_BRANCH,GIT_COMMIT_HASH);
exit(EXIT_SUCCESS);
}

void print_license()
{
printf(
"openCompressBench\n\n"
"Copyright (C) 2016 Michael Strassberger saremox@linux.com\n\n"
"This program is free software; you can redistribute it and/or\n"
"modify it under the terms of the GNU General Public License\n"
"as published by the Free Software Foundation; version 2 of \n"
"the License.\n\n"
"This program is distributed in the hope that it will be useful,\n"
"but WITHOUT ANY WARRANTY; without even the implied warranty of \n"
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the \n"
"GNU General Public License for more details.\n\n"
"You should have received a copy of the GNU General Public License\n"
"along with this program; if not, write to the Free Software \n"
"Foundation, Inc.,""51 Franklin Street, Fifth Floor, Boston, \n"
"MA 02110-1301, USA.\n");
exit(EXIT_SUCCESS);
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

  while (1) {
    int option_index = 0;
    static struct option long_options[] = {
        {"codecs",          required_argument, 0, 'c'},
        {"database",        required_argument, 0, 'D'},
        {"worker",          required_argument, 0, 'w'},
        {"directory",       required_argument, 0, 'd'},
        {"transactionlog",  required_argument, 0, 't'},
        {"log-level",       required_argument, 0, 'l'},
        {"verbose",         no_argument,       0, 'v'},
        {"quiet",           no_argument,       0, 'q'},
        {"license",         no_argument,       0, 'L'},
        {"help",            no_argument,       0, 'h'},
        {"usage",           no_argument,       0, 'u'},
        {"version",         no_argument,       0, 'V'},
        {0,                 0,                 0,  0 }
    };

    c = getopt_long(argc, argv, "c:D:w:d:t:l:vqLhuV",
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
    case 'd':
      debug("Setting directory for analysis to: \"%s\"",optarg);
      directoryPath = optarg;
      break;
    case 't':
      debug("Setting transactionlog to: \"%s\"",optarg);
      transactionlog = optarg;
      break;
    case 'l':
      tmp = atoi(optarg);
      if(tmp >= 0 && tmp <= OCDEBUG_DEBUG)
        verbosityLevel = tmp;
      else
        log_err("Invalid log level: \"%s\"",optarg);
      break;
    case 'v':
      debug("Turn on Verbosity");
      verbosityLevel = OCDEBUG_INFO;
      break;
    case 'q':
      debug("Turn on quiet");
      verbosityLevel = OCDEBUG_ERROR;
      break;
    case 'L':
      print_license();
    case 'h':
    case 'u':
      print_help(argv[0]);
    case 'V':
      print_version(argv[0]);
    }
  }
  return;
  error:
    print_help(argv[0]);
}

void cleanup()
{
  if(myctx)
    ocdata_destroy_context(&myctx);
  ocdata_garbage_collect();
  if(codecList)
    ocutils_list_destroy(codecList);
  if(files)
    ocutils_list_destroy(files);
}

void shutdown()
{
  ocworker_kill(workerctx);

  cleanup();

  exit(EXIT_SUCCESS);
}

void signal_handler(int sig)
{
  switch (sig) {
    case SIGTERM:
      shutdown();
    case SIGINT:
      if(shutdown_request == 0)
      {
        log_info("Got ctrl + C terminating gracefully");
        shutdown_request = SIGKILL;
      }
      else
      {
        log_info("Got ctrl + C second time FORCE QUIT");
        ocworker_force_kill(workerctx);
        exit(EXIT_FAILURE);
      }
  }
}

int main (int argc, char *argv[])
{
  parse_arguments(argc,argv);

  ocworker_start(worker,&workerctx);

  // Register Signal handler
  signal(SIGTERM, signal_handler);
  signal(SIGINT, signal_handler);

  files     = ocutils_list_create();
  codecList = ocutils_list_create();

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

  while (true) {
    struct timespec sleeptimer = {1,0};
    nanosleep(&sleeptimer,NULL);
    if(ocworker_is_running(workerctx) != OCWORKER_IS_RUNNING)
      break;
    if(shutdown_request == SIGKILL)
      shutdown();
  }

  ocworker_kill(workerctx);

  jobs   = workerctx->jobsDone;

  ocdata_create_context(&myctx,databasePath,0);

  ocutils_list_foreach_f(jobs, curjob,ocworkerJob*)
  {
    ocdata_add_result(myctx, curjob->result);
  }

  cleanup();
  return EXIT_SUCCESS;
  error:
  return EXIT_FAILURE;
}
