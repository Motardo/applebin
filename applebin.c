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

ADHeader gADHeader;
char gBuffer[kBufferSize];
char gZeros[128] = {0};

void bail(int result, char *message) {
  perror(message);
  fprintf(stderr, "%s: %d: %d\n", message, result, errno);
  exit(result);
}

int readHeader(FILE *fp) {
  int result, numEntries, magic, version;
  u_int32_t magic1 = 0x00051600;
  u_int32_t magic2 = 0x00051607;
  u_int32_t versionOk = 0x00020000;
  ADHeader *adh = &gADHeader;

  if ((result = fseek(fp, 0L, SEEK_SET)) != 0) bail(result, "Couldn't seek file");
  if ((result = fread(adh->raw, 1, 26, fp)) != 26) bail(result, "Couldn't read file");
  magic = ntohl(adh->magic);
  version = ntohl(adh->version);
  if (magic != magic1 && magic != magic2) bail(BADMAGIC, "Not an AppleDouble file, bad magic number");
  if (version != versionOk) bail(BADVERSION, "AppleDouble file with unknown version");

  numEntries = ntohs(adh->numEntries);

  return numEntries;
}

/* Read the entries list beginning at offset 26 */
EntrySpec* readEntriesList(FILE *fp, int numEntries) {
  EntrySpec *entries;
  int result, bufferSize;

  bufferSize = sizeof(EntrySpec) * numEntries;
  if ((entries = malloc(bufferSize)) == NULL) bail(MALLOC, "Couldn't allocate memory for entries list");
  if ((result = fread(entries, 1, bufferSize, fp)) != bufferSize) bail(result, "Couldn't read entries");

  return entries;
}

FILE* openFile(const char *filename) {
  FILE *fp;

  fp = fopen(filename, "r");
  if (fp == NULL) bail(CANTOPEN, "Couldn't open file");
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
  int result;

  // copy type and creator to header
  if ((result = fseek(fp, ntohl(offset), SEEK_SET)) != 0) bail(result, "Couldn't seek file: FInfo");
  if ((result = fread(&header[kFileType], 1, 8, fp)) != 8) bail(result, "Couldn't read file: FInfo");
}

void setFilename(char *header, const char *filename) {
  int len;

  // TODO drop the marker characters at the beginning of the filename
  len = min(kMaxFileName, strlen(filename));
  header[kFileNameLen] = (char)len;
  memcpy(&header[kFileName], filename, len);
}

int makeBinHeader(char *header, FILE *fp, EntrySpec *entries, int numEntries, const char *filename) {
  int rForkEntry = kNotFound;

  setFilename(header, filename);
  for (int i = 0; i < numEntries; i++) {
    switch (ntohl(entries[i].type)) {
    case FINFO:
      readFInfo(fp, header, entries[i].offset, entries[i].length);
      break;
    case RFORK:
      rForkEntry = i;
      memcpy(&header[kRForkLength], (char *)&(entries[i].length), 4);
      break;
    default:
      break;
    }
  }
  return rForkEntry;
}

int registerDataFork(char *header, const char *filename) {
  struct stat statbuf;
  int dataSize;

  if (stat(filename, &statbuf) != 0) bail(CANTSTAT, "Couldn't stat data fork file");
  dataSize = htonl(statbuf.st_size);
  memcpy(&header[kDForkLength], (char *)&dataSize, 4);

  return statbuf.st_size;
}

FILE* writeHeader(const char *header) {
  FILE *fp;
  char filename[kMaxFileName + 5]; // .bin\0
  int nameLength;

  nameLength = header[kFileNameLen];
  memcpy(filename, &header[kFileName], nameLength);
  memcpy(&filename[nameLength], kSuffix, kSuffixLen);
  filename[nameLength + kSuffixLen] = '\0';
  fp = fopen(filename, "w");
  if (fp == NULL) bail(CANTOPEN, "writeHeader: Couldn't open bin file");
  if (fwrite(header, 1, 128, fp) != 128) bail(CANTWRITE, "writeHeader: Couldn't write header");

  return fp;
}

void copyFile(FILE *dest, FILE *source, int length) {
  int chunks, remainder;

  chunks = length / kBufferSize;
  for (int i = 0; i < chunks; i++) {
    if (fread(gBuffer, 1, kBufferSize, source) != kBufferSize) bail(CANTREAD, "copyFile: Read error");
    if (fwrite(gBuffer, 1, kBufferSize, dest) != kBufferSize) bail(CANTWRITE, "copyFile: Write error");
  }
  remainder = length % kBufferSize;
  if (fread(gBuffer, 1, remainder, source) != remainder) bail(CANTREAD, "copyFile: Read error");
  if (fwrite(gBuffer, 1, remainder, dest) != remainder) bail(CANTWRITE, "copyFile: Write error");
  fclose(source);
}

void writeRFork(FILE *binFile, FILE *aDF, EntrySpec entry) {
  int offset, length, padding;

  offset = ntohl(entry.offset);
  length = ntohl(entry.length);
  if (fseek(aDF, offset, SEEK_SET) != 0) bail(CANTSEEK, "writeRFork: Couldn't seek resource fork entry");
  copyFile(binFile, aDF, length);
  padding = (128 - (length & 0x0000007F) % 128);
  if (fwrite(gZeros, 1, padding, binFile) != padding) bail(CANTWRITE, "writRFork: Write padding zeros");
}

void writeDFork(FILE *binFile, const char *filename, int length) {
  int padding;
  FILE *fp;

  fp = openFile(filename);
  copyFile(binFile, fp, length);
  padding = (128 - (length & 0x0000007F) % 128);
  if (fwrite(gZeros, 1, padding, binFile) != padding) bail(CANTWRITE, "writDFork: Write padding zeros");
}

/*
  argv[1] is name of AppleDoubleFile
  argv[2] is optional name of DataFork file
*/
int main(int argc, char *argv[]) {
  FILE *aDF, *binFile;
  int numEntries, rForkEntry, dataForkLen;
  EntrySpec *entries;
  char header[128] = {0};

  dataForkLen = 0;

  aDF = openFile(argv[1]);
  numEntries = readHeader(aDF);
  entries = readEntriesList(aDF, numEntries);

  printf("Num entries: %d\n", numEntries);
  printEntriesList(entries, numEntries);

  rForkEntry = makeBinHeader(header, aDF, entries, numEntries, argv[1]);
  if (argc >= 3) dataForkLen = registerDataFork(header, argv[2]);

  binFile = writeHeader(header);
  if (argc >= 3) writeDFork(binFile, argv[2], dataForkLen);
  writeRFork(binFile, aDF, entries[rForkEntry]);
  fclose(binFile);
  return 0;
}
