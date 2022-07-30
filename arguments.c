#include <stdio.h>
#include <stdlib.h>    /* exit */
#include <string.h>    /* strdup */
#include <getopt.h>
#include "arguments.h"

static void printHelp(void);
static void printVersion(void);

static void printHelp() {
  char* help =
"Usage: applebin [OPTION]... FILE [FILE]\n"
"Convert an AppleSingle or AppleDouble FILE to MacBinary format.\n"
"\n"
"Mandatory arguments to long options are mandatory for short options too.\n"
"  -o, --out-file=FILE   output converted MacBinary file to FILE\n"
"  -l, --list-only       display a summary of the file(s) but do not convert\n"
"  -d, --debug           display the type, length, and offset of header entries\n"
"  -h, --help            display this help and exit\n"
"  -v, --version         output version information and exit\n"
;
  printf("%s\n", help);
  exit(EXIT_SUCCESS);
}
static void printVersion() {
  printf("Version 1.0\n");
  exit(EXIT_SUCCESS);
}

/** Parse command line options and arguments.
    \return a pointer to an Arguments struct
 */
Arguments* commandArgs(int argc, char **argv) {
  static Arguments args;

  while (1) {
    static struct option long_options[] =
      {
       {"help",       no_argument,       0, 'h'},
       {"version",    no_argument,       0, 'v'},
       {"debug",      no_argument,       0, 'd'},
       {"list-only",  no_argument,       0, 'l'},
       {"out-file",   required_argument, 0, 'o'},
       {0,            0,                 0,  0 }
      };

    int option_index = 0;
    int c = getopt_long(argc, argv, "hvdlo:",
                    long_options, &option_index);
    if (c == -1) break;
    else if (c == 'h') printHelp();
    else if (c == 'v') printVersion();
    else if (c == 'd') args.verbose = 1;
    else if (c == 'l') args.noCreate = 1;
    else if (c == 'o') args.outFileName = strdup(optarg);
    else {
      fprintf(stderr, "%s: unknown option '%c'\n", argv[0], c);
      exit(EXIT_FAILURE);
    }
  }

  if (optind >= argc) {
    fprintf(stderr, "%s: missing file argument\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  if (optind < argc) args.file1 = strdup(argv[optind++]);
  if (optind < argc) args.file2 = strdup(argv[optind++]);
  if (optind < argc) {
    fprintf(stderr, "%s: too many arguments\n", argv[0]);
    printHelp();
  }

  return &args;
}
