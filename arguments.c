#include <stdio.h>     /* for printf */
#include <stdlib.h>    /* for exit */
#include <getopt.h>
#include <string.h>    /* for strdup */
#include "arguments.h"


Arguments gArgs;


Arguments* commandArgs(int argc, char **argv) {
  int c;
  Arguments *args = &gArgs;

  while (1) {
    int option_index = 0;
    static struct option long_options[] =
      {
       {"help",    no_argument,       0, 'h'},
       {"version", no_argument,       0, 'v'},
       {"output",  required_argument, 0, 'o'},
       {0,         0,                 0,  0 }
      };

    c = getopt_long(argc, argv, "hvo:",
                    long_options, &option_index);
    if (c == -1)
      break;

    switch (c) {
    case 0:
      printf("option %s", long_options[option_index].name);
      if (optarg)
        printf(" with arg %s", optarg);
      printf("\n");
      break;

    case 'h':
      printf("option h\n");
      break;

    case 'v':
      printf("option v\n");
      break;

    case 'o':
      printf("option o with value '%s'\n", optarg);
      if (args->outFileName) exit(13); //FIXME
      args->outFileName = strdup(optarg);
      break;

    case '?':
    case ':':
      exit(EXIT_FAILURE);
      break;

    default:
      fprintf(stderr, "?? getopt returned character code 0%o ??\n", c);
      exit(EXIT_FAILURE);
    }
  }

  if (optind >= argc) {
    fprintf(stderr, "%s: missing file argument\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  if (optind < argc) args->file1 = strdup(argv[optind++]);
  if (optind < argc) args->file2 = strdup(argv[optind++]);
  if (optind < argc) {
    fprintf(stderr, "%s: too many arguments\n", argv[0]);
    //TODO print usage
    exit(EXIT_FAILURE);
  }
  printf("file1: %s\n", args->file1);
  printf("file2: %s\n", args->file2);

  return args;
}
#ifdef WithMain
int main(int argc, char **argv) {
  Arguments *args = commandArgs(argc, argv);

  exit(EXIT_SUCCESS);
}
#endif
