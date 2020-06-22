#include <stdio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <errno.h>
#include <libgen.h>
#include <string.h>
#include "applebin.h"

ADInfo gADInfo;

void printEntriesList(EntrySpec *entries, int numEntries) {
  printf("Num entries: %d\n", numEntries);
  for (int i = 0; i < numEntries; i++) {
    int type = ntohl(entries[i].type);
    int offset = ntohl(entries[i].offset);
    int length = ntohl(entries[i].length);
    printf("Type: %08x\tOffset: %08x\tLength: %08x\n", type, offset, length);
  }
}

void printVerbose(ADInfo *adi) {
  printEntriesList(adi->entries, adi->numEntries);
}

/* Read the entries list beginning at offset 26 */
void readEntriesList(ADInfo *adi) {
  if (fread(adi->entries, sizeof(EntrySpec), adi->numEntries, adi->headerFile) != adi->numEntries)
    bail("Couldn't read entries");
}

void registerEntries(char *header, ADInfo *adi) {
  EntrySpec* entries = adi->entries;
  adi->rFork = kNotFound;
  for (int i = 0; i < adi->numEntries; i++) {
    int type = ntohl(entries[i].type);
    if (type == FINFO) readFInfo(adi->headerFile, header, entries[i].offset, entries[i].length);
    else if (type == RFORK) {
      adi->rFork = i;
      memcpy(&header[kRForkLength], (char *)&(entries[i].length), 4);
    } // TODO else if (type == DFORK)
  }
}

/** Read the first 26 bytes of the file and return the number of Entries or 0 if
    it is not an AppleSingle or AppleDouble header file. */
ADHeader* readHeader(FILE *fp) {
  ADHeader *adh;

  if ((adh = malloc(sizeof(ADHeader))) == NULL) bail("readHeader: can't malloc 26 bytes");
  if (fseek(fp, 0L, SEEK_SET) != 0) bail("readHeader: Couldn't seek file");
  if (fread(adh->raw, 1, 26, fp) != 26) bail("readHeader: Couldn't read file");

  int magic = ntohl(adh->magic);
  if (magic == MAGIC1 || magic == MAGIC2) adh->magic = magic;
  else return NULL;

  int version = ntohl(adh->version);
  if (version != VERSION2) bail("readHeader: Unknown version");

  adh->numEntries = ntohs(adh->numEntries);
  return adh;
}

/*
  Inputs:     file1
              file2 which may be NULL

  Outputs:    open FILE* positioned at EntriesList
              numEntries
              dataFile name which may be NULL
              basName
              single/double
*/
ADInfo* readHeaderInfo(char *file1, char *file2) {
  char *baseName;
  ADInfo *info = &gADInfo;
  FILE *fp = openFile(file1);
  ADHeader *adh = readHeader(fp);
  if (!adh) {
    if (file2) {
      fclose(fp);
      fp = openFile(file2);
      adh = readHeader(fp);
      if (!adh) bail("neither file is an AppleDouble file");
      else {
        if (adh->magic == MAGIC1) bail("second file argument is AppleSingle file");
        else {
          info->dataFile = file1;
          baseName = basename(file1);
          info->single = 0;
        }
      }
    } else bail("not an AppleSingle/Double file");
  } else {
    if (adh->magic == MAGIC1) {
      if (file2) bail("extra argument after AppleSingle file");
      else {
        baseName = basename(file1);
        info->single = 1;
      }
    } else {
      if (!file2) bail("missing argument after AppleDouble file");
      else {
        info->dataFile = file2;
        baseName = basename(file2);
        info->single = 0;
      }
    }
  }
  strncpy(info->baseName, baseName, 64);
  info->numEntries = adh->numEntries;
  info->headerFile = fp;
  return info;
}
