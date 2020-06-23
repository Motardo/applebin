
typedef struct Arguments {
  char *outFileName;    ///< \c --out-file option
  char *file1;          ///< the first non-option argument
  char *file2;          ///< the second non-option argument
  signed char verbose;  ///< \c --verbose option
  signed char noCreate; ///< \c --no-create option
} Arguments;


Arguments* commandArgs(int argc, char **argv);
