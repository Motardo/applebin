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

typedef int uint32;

typedef union  AppleDoubleHeader {
  char raw[26];
  struct {
    uint32 magic;
    uint32 version;
    char filler[16];
    unsigned char numEntries[2];
  };
} ADHeader;

typedef union AppleDoubleEntrySpec {
  char raw[12];
  struct {
    uint32 type;
    uint32 offset;
    uint32 length;
  };
} EntrySpec;

ADHeader gADHeader;

void bail(int result, char *message) {
  fprintf(stderr, "%s: %d: %d\n", message, result, errno);
  exit(result);
}

/* convert big endian short to platform int */
int BEWord(unsigned char word[2]) {
  return (int)word[1] + (256 * (int)word[0]);
}

int readHeader(FILE *fp) {
  int result, numEntries;
  uint32 magic1 = 0x00160500;
  uint32 magic2 = 0x07160500;
  uint32 version = 0x00000200;
  ADHeader *adh = &gADHeader;
  
  if ((result = fseek(fp, 0L, SEEK_SET)) != 0) bail(result, "Couldn't seek file");
  if ((result = fread(adh->raw, 1, 26, fp)) != 26) bail(result, "Couldn't read file");
  //printf("magic: 0x%08x\n", adh->magic);
  //printf("version: 0x%08x\n", adh->version);
  if (adh->magic != magic1 && adh->magic != magic2) bail(BADMAGIC, "Not an AppleDouble file, bad magic number");
  if (adh->version != version) bail(BADVERSION, "AppleDouble file with unknown version");

  numEntries = BEWord(adh->numEntries);

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
  for (int i = 0; i < numEntries; i++) {
    printf("Type: %08x\tOffset: %08x\tLength: %08x\n", entries[i].type, entries[i].offset, entries[i].length);
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
  for (int i = 0; i < numEntries; i++) {
  }

  printf("Num entries: %d\n", numEntries);
  printEntriesList(entries, numEntries);
  
  fclose(fp);
  return 0;
}
