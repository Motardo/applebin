
typedef struct Arguments {
  char *outFileName;
  char *file1;
  char *file2;
  signed char verbose;
  signed char noOutput;
} Arguments;


Arguments* commandArgs(int argc, char **argv);
