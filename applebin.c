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

void readFInfo(FILE *fp, char *header, u_int32_t offset, u_int32_t length) {
  if (length > 8) {
    // copy type and creator to header
    if (fseek(fp, ntohl(offset), SEEK_SET) != 0) bail("Couldn't seek file");
    if (fread(&header[kFileType], 1, 8, fp) != 8) bail("Couldn't read file");
  }
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

int registerDataFork(char *header, const char *filename) {
  struct stat statbuf;
  if (stat(filename, &statbuf) != 0) bail("Couldn't stat file");

  int dataSize = htonl(statbuf.st_size);
  memcpy(&header[kDForkLength], (char *)&dataSize, 4);

  return statbuf.st_size;
}

/** Create the MacBinary file with a .bin extension
    Use the out-filename option if given, else use the internal basename from the header
*/
FILE* writeHeader(const char *header, const char *outFileName) {
  char filename[kMaxFileName + kSuffixLen + 1]; // .bin\0
  int length;
  const char *baseName;

  if (outFileName) {
    length = strlen(outFileName);
    baseName = outFileName;
  } else {
    length = header[kFileNameLen];
    baseName = &header[kFileName];
  }
  memcpy(filename, baseName, length);
  memcpy(&filename[length], kSuffix, kSuffixLen);
  filename[length + kSuffixLen] = '\0';

  FILE *fp = fopen(filename, "w");
  if (fp == NULL) bail("Couldn't open bin file");
  if (fwrite(header, 1, 128, fp) != 128) bail("Couldn't write header");

  return fp;
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

void writeRFork(FILE *binFile, ADInfo *adi) {
  int offset = ntohl(adi->entries[adi->rFork].offset);
  int length = ntohl(adi->entries[adi->rFork].length);
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
  ADInfo *adi = readHeaderInfo(args->file1, args->file2);
  int numEntries = adi->numEntries;
  EntrySpec entries[numEntries];
  adi->entries = entries;
  readEntriesList(adi);

  if (args->verbose || args->noCreate) printVerbose(adi);
  if (args->noCreate) return(EXIT_SUCCESS);

  char header[128] = {0};
  setFilename(header, adi->baseName);
  registerEntries(header, adi);

  int dataForkLen;
  char *dataFile = adi->dataFile;
  if (dataFile) dataForkLen = registerDataFork(header, dataFile);

  FILE *binFile = writeHeader(header, args->outFileName);
  if (dataFile) writeDFork(binFile, dataFile, dataForkLen);
  if (adi->rFork != kNotFound) writeRFork(binFile, adi);
  fclose(binFile);
  return (EXIT_SUCCESS);
}
