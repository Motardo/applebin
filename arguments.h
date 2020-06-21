
typedef struct Arguments {
  char *outFileName;
  char *file1;
  char *file2;
} Arguments;


Arguments* commandArgs(int argc, char **argv);
