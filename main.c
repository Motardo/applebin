#include <endian.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

enum {
      BADMAGIC,
      BADVERSION,
      CANTOPEN,
      MALLOC
};


typedef union  AppleDoubleHeader {
  char raw[26];
  struct {
    u_int32_t magic;
    u_int32_t version;
    char filler[16];
    u_int16_t numEntries;
  };
} ADHeader;

typedef union AppleDoubleEntrySpec {
  char raw[12];
  struct {
    u_int32_t type;
    u_int32_t offset;
    u_int32_t length;
  };
} EntrySpec;

ADHeader gADHeader;

FILE* openFile(char *filename);
void printEntriesList(EntrySpec *entries, int numEntries);
void bail(int result, char *message);
void printEntriesList(EntrySpec *entries, int numEntries);
EntrySpec* readEntriesList(FILE *fp, int numEntries);
FILE* openFile(char *filename);
int readHeader(FILE *fp);

void bail(int result, char *message) {
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
  magic = be32toh(adh->magic);
  version = be32toh(adh->version);
  if (magic != magic1 && magic != magic2) bail(BADMAGIC, "Not an AppleDouble file, bad magic number");
  if (version != versionOk) bail(BADVERSION, "AppleDouble file with unknown version");

  numEntries = be16toh(adh->numEntries);

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

FILE* openFile(char *filename) {
  FILE *fp;
  
  fp = fopen(filename, "r");
  if (fp == NULL) bail(CANTOPEN, "Couldn't open file");
  return fp;
}

void printEntriesList(EntrySpec *entries, int numEntries) {
  int type, offset, length;

  for (int i = 0; i < numEntries; i++) {
    type = be32toh(entries[i].type);
    offset = be32toh(entries[i].offset);
    length = be32toh(entries[i].length);
    printf("Type: %08x\tOffset: %08x\tLength: %08x\n", type, offset, length);
  }
}

int main(int argc, char *argv[]) {
  FILE *fp;
  int numEntries;
  char *filename;
  EntrySpec *entries;

  filename = argv[1];
  fp = openFile(filename);
  numEntries = readHeader(fp);
  entries = readEntriesList(fp, numEntries);

  printf("Num entries: %d\n", numEntries);
  printEntriesList(entries, numEntries);
  
  fclose(fp);
  return 0;
}
