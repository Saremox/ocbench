#include <stdio.h>
#include <stdlib.h>
#include "ocbenchConfig.h"

int main (int argc, char *argv[])
{
  if (argc < 2)
    {
    fprintf(stdout,"%s openCompressBench Version %d.%d\n"
            "Copyright (C) 2016 Michael Strassberger "
            "<saremox@linux.com>\n"
            "openCompressBench comes with ABSOLUTELY NO WARRANTY; for details\n"
            "type `show w'.  This is free software, and you are welcome"
            "to redistribute it under certain conditions; type `show c'"
            "for details.",
            argv[0],
            OCBENCH_VERSION_MAJOR,
            OCBENCH_VERSION_MINOR);
    fprintf(stdout,"Usage: %s\n",argv[0]);
    return 1;
    }
  return 0;
}
