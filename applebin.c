/*
  Given the filename of an AppleDouble header file on the command line followed
  by the data fork file if it exists, converts the file(s) into a MacBinary
  format file
*/

#include <arpa/inet.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <libgen.h>
#include "applebin.h"
#include "arguments.h"

// Padding zeros for copying to file
char gZeros[128] = {0};

static char* makeBinFileName(ADInfo *adi);
static FILE* writeHeader(const char *header, const char *binName);
static void writeRFork(FILE *binFile, const ADInfo *adi);
static void writeDFork(FILE *binFile, const char *filename, int length);
static void registerDataFork(char *header, ADInfo *adi);

void bail(char *message) {
  perror(message);
  fprintf(stderr, "%s:%d: %s: %d\n", __FILE__, __LINE__, message, errno);
  exit(errno);
}

FILE* openFile(const char *filename) {
  FILE *fp = fopen(filename, "r");
  if (fp == NULL) bail("Couldn't open file");
  return fp;
}

/** The internal filename to use if the AppleSingle/Double file does not specify one
    Set to the basename of the AppleSingle/Double file
*/
void setFilename(char *header, const char *filename) {
  if (filename[0] == '.' && filename[1] == '_') filename = filename + 2;
  int len = strlen(filename);

  if (len > kMaxFileName) len = kMaxFileName;
  header[kFileNameLen] = len;
  memcpy(&header[kFileName], filename, len);
}

static void registerDataFork(char *header, ADInfo *adi) {
  struct stat statbuf;
  if (stat(adi->dataFile, &statbuf) != 0) bail("Couldn't stat file");
  adi->dataLength = statbuf.st_size;
  int dataSize = htonl(statbuf.st_size);
  memcpy(&header[kDForkLength], (char *)&dataSize, 4);
}

static FILE* writeHeader(const char *header, const char *binName) {
  FILE *fp = fopen(binName, "w");

  if (fp == NULL) bail("Couldn't open bin file");
  if (fwrite(header, 1, 128, fp) != 128) bail("Couldn't write header");

  return fp;
}

/** Create the MacBinary file and write it's header. Use the out-filename option
    if given, else use name determined from input file and .bin extension. */
static char* makeBinFileName(ADInfo *adi) {
  char *baseName = (adi->dataFile) ? strdup(adi->dataFile) : strdup(adi->header);
  size_t baseLength = strlen(baseName);
  size_t suffixLength = sizeof(kSuffix);
  char *binName = malloc(baseLength + suffixLength + 1);
  memcpy(binName, baseName, baseLength);
  free(baseName);
  memcpy(&binName[baseLength], kSuffix, suffixLength);
  binName[baseLength + suffixLength] = '\0';
  return binName;
}

void copyFile(FILE *dest, FILE *source, int length) {
  const size_t kBufferSize = (64 * 1024);
  char buffer[kBufferSize];

  int chunks = length / kBufferSize;
  for (int i = 0; i < chunks; i++) {
    if (fread(buffer, kBufferSize, 1, source) != 1) bail("Couldn't read file");
    if (fwrite(buffer, kBufferSize, 1, dest) != 1) bail("Couldn't write file");
  }
  size_t remainder = length % kBufferSize;
  if (fread(buffer, 1, remainder, source) != remainder) bail("Couldn't read file");
  if (fwrite(buffer, 1, remainder, dest) != remainder) bail("Couldn't write file");
  fclose(source);
}

/** Write zeros to file to pad the length to a multiple of 128 bytes.
    \param \c fp pointer to a file open for writing with the position at the end
    \param \c length the length of data previously written
*/
void padFile(FILE *fp, int length) {
  size_t padding = 128 - (length % 128);
  if (padding == 128) return; // no padding needed
  if (fwrite(gZeros, 1, padding, fp) != padding) bail("Couldn't write padding zeros");
}

static void writeRFork(FILE *binFile, const ADInfo *adi) {
  int offset = adi->rsrcOffset;
  int length = adi->rsrcLength;
  if (fseek(adi->headerFile, offset, SEEK_SET) != 0) bail("Couldn't seek file");
  copyFile(binFile, adi->headerFile, length);
  padFile(binFile, length);
}

void writeDFork(FILE *binFile, const char *filename, int length) {
  FILE *fp = openFile(filename);
  copyFile(binFile, fp, length);
  padFile(binFile, length);
}

int main(int argc, char *argv[]) {
  Arguments *args = commandArgs(argc, argv);
  ADInfo adi = readHeaderInfo(args->file1, args->file2);
  readEntriesList(&adi);

  if (args->verbose || args->noCreate) printVerbose(&adi);
  if (args->noCreate) return(EXIT_SUCCESS);

  char binHeader[128] = {0};
  char *baseName = (adi.single) ? basename(adi.header) : basename(adi.dataFile);
  strncpy(adi.baseName, baseName, 64);
  setFilename(binHeader, adi.baseName);
  registerEntries(binHeader, &adi);
  free(adi.entries);

  if (adi.dataFile) registerDataFork(binHeader, &adi);

  char* binName = (args->outFileName) ? args->outFileName : makeBinFileName(&adi);
  FILE *binFile = writeHeader(binHeader, binName);
  free(binName);

  if (adi.dataFile) writeDFork(binFile, adi.dataFile, adi.dataLength);
  if (adi.rsrcLength) writeRFork(binFile, &adi);
  fclose(binFile);
  return (EXIT_SUCCESS);
}
