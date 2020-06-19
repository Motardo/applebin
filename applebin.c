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
#include "applebin.h"

// Padding zeros for copying to file
char gZeros[128] = {0};

void bail(char *message) {
  perror(message);
  fprintf(stderr, "%s: %d\n", message, errno);
  exit(errno);
}

int readHeader(FILE *fp) {
  ADHeader adh;

  if (fseek(fp, 0L, SEEK_SET) != 0) bail("readHeader: Couldn't seek file");
  if (fread(adh.raw, 1, 26, fp) != 26) bail("readHeader: Couldn't read file");
  int magic = ntohl(adh.magic);
  int version = ntohl(adh.version);

  if (magic != MAGIC1 && magic != MAGIC2) bail("readHeader: Unknown magic number");
  if (version != VERSION2) bail("readHeader: Unknown version");

  return ntohs(adh.numEntries);
}

/* Read the entries list beginning at offset 26 */
void readEntriesList(FILE *aDF, EntrySpec *entries, int numEntries) {
  if (fread(entries, sizeof(EntrySpec), numEntries, aDF) != numEntries)
    bail("Couldn't read entries");
}

FILE* openFile(const char *filename) {
  FILE *fp = fopen(filename, "r");
  if (fp == NULL) bail("Couldn't open file");
  return fp;
}

void printEntriesList(EntrySpec *entries, int numEntries) {
  int type, offset, length;

  for (int i = 0; i < numEntries; i++) {
    type = ntohl(entries[i].type);
    offset = ntohl(entries[i].offset);
    length = ntohl(entries[i].length);
    printf("Type: %08x\tOffset: %08x\tLength: %08x\n", type, offset, length);
  }
}

void readFInfo(FILE *fp, char *header, u_int32_t offset, u_int32_t length) {
  // copy type and creator to header
  if (fseek(fp, ntohl(offset), SEEK_SET) != 0) bail("readFInfo: Couldn't seek file");
  if (fread(&header[kFileType], 1, 8, fp) != 8) bail("readFInfo: Couldn't read file");
}

void setFilename(char *header, const char *filename) {
  // FIXME drop the marker characters at the beginning of the filename
  if (filename[0] == '.' && filename[1] == '_') filename = filename + 2;
  int len = strlen(filename);

  if (len > kMaxFileName) len = kMaxFileName;
  header[kFileNameLen] = len;
  memcpy(&header[kFileName], filename, len);
}

EntrySpec* registerEntries(char *header, FILE *fp, EntrySpec *entries, int numEntries) {
  EntrySpec* rForkEntry = NULL;
  for (int i = 0; i < numEntries; i++) {
    int type = ntohl(entries[i].type);
    if (type == FINFO) readFInfo(fp, header, entries[i].offset, entries[i].length);
    else if (type == RFORK) {
      rForkEntry = &entries[i];
      memcpy(&header[kRForkLength], (char *)&(entries[i].length), 4);
    }
  }
  return rForkEntry;
}

int registerDataFork(char *header, const char *filename) {
  struct stat statbuf;
  if (stat(filename, &statbuf) != 0) bail("registerDataFork: couldn't stat file");

  int dataSize = htonl(statbuf.st_size);
  memcpy(&header[kDForkLength], (char *)&dataSize, 4);

  return statbuf.st_size;
}

FILE* writeHeader(const char *header) {
  char filename[kMaxFileName + kSuffixLen + 1]; // .bin\0
  int length = header[kFileNameLen];

  memcpy(filename, &header[kFileName], length);
  memcpy(&filename[length], kSuffix, kSuffixLen);
  filename[length + kSuffixLen] = '\0';

  FILE *fp = fopen(filename, "w");
  if (fp == NULL) bail("writeHeader: Couldn't open bin file");
  if (fwrite(header, 1, 128, fp) != 128) bail("writeHeader: Couldn't write header");

  return fp;
}

void copyFile(FILE *dest, FILE *source, int length) {
  char buffer[kBufferSize];

  int chunks = length / kBufferSize;
  for (int i = 0; i < chunks; i++) {
    if (fread(buffer, 1, kBufferSize, source) != kBufferSize) bail("copyFile: read error");
    if (fwrite(buffer, 1, kBufferSize, dest) != kBufferSize) bail("copyFile: write error");
  }
  int remainder = length % kBufferSize;
  if (fread(buffer, 1, remainder, source) != remainder) bail("copyFile: small read error");
  if (fwrite(buffer, 1, remainder, dest) != remainder) bail("copyFile: small write error");
  fclose(source);
}

void writeRFork(FILE *binFile, FILE *aDF, EntrySpec *rForkEntry) {
  int offset = ntohl(rForkEntry->offset);
  int length = ntohl(rForkEntry->length);
  if (fseek(aDF, offset, SEEK_SET) != 0) bail("writeRFork: Couldn't seek resource fork entry");
  copyFile(binFile, aDF, length);
  int padding = (128 - (length & 0x0000007F) % 128);
  if (fwrite(gZeros, 1, padding, binFile) != padding) bail("writRFork: Write padding zeros");
}

void writeDFork(FILE *binFile, const char *filename, int length) {
  FILE *fp = openFile(filename);
  copyFile(binFile, fp, length);
  int padding = 128 - (length % 128);
  if (padding == 128) return; // no padding needed
  if (fwrite(gZeros, 1, padding, binFile) != padding) bail("writDFork: Write padding zeros");
}

/*
  argv[1] is name of AppleDoubleFile
  argv[2] is optional name of DataFork file
*/
int main(int argc, char *argv[]) {
  char header[128] = {0};
  int dataForkLen = 0;

  FILE *aDF = openFile(argv[1]);
  int numEntries = readHeader(aDF);

  EntrySpec entries[numEntries];
  readEntriesList(aDF, entries, numEntries);

  printf("Num entries: %d\n", numEntries);
  printEntriesList(entries, numEntries);

  setFilename(header, argv[1]);
  EntrySpec *rForkEntry = registerEntries(header, aDF, entries, numEntries);
  if (argc >= 3) dataForkLen = registerDataFork(header, argv[2]);

  FILE *binFile = writeHeader(header);
  if (argc >= 3) writeDFork(binFile, argv[2], dataForkLen);
  if (rForkEntry != NULL) writeRFork(binFile, aDF, rForkEntry);
  fclose(binFile);
}
