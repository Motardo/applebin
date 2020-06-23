#include <stdio.h>
#include <stdlib.h>    /* exit */
#include <string.h>    /* strdup */
#include <getopt.h>
#include "arguments.h"

static void printHelp(void);
static void printVersion(void);

static void printHelp() {
  printf("Help\n");
  exit(EXIT_SUCCESS);
}
static void printVersion() {
  printf("Version\n");
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
       {"version",    no_argument,       0, 'V'},
       {"verbose",    no_argument,       0, 'v'},
       {"no-create",  no_argument,       0, 'n'},
       {"out-file",   required_argument, 0, 'o'},
       {0,            0,                 0,  0 }
      };

    int option_index = 0;
    int c = getopt_long(argc, argv, "hVvno:",
                    long_options, &option_index);
    if (c == -1) break;
    else if (c == 'h') printHelp();
    else if (c == 'V') printVersion();
    else if (c == 'v') args.verbose = 1;
    else if (c == 'n') args.noCreate = 1;
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
