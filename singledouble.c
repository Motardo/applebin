#include <stdio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <errno.h>
#include <libgen.h>
#include <string.h>
#include "applebin.h"

typedef enum { Missing,
               Unknown,
               Neither,
               Single,
               Double
} InputFileType;

static ADInfo checkInputFiles(char *file1, char *file2);
static InputFileType checkFileType(char *filename);
static void printEntriesList(EntrySpec *entries, int numEntries);
static void readFInfo(FILE *fp, char *header, u_int32_t offset, u_int32_t length);

static void printEntriesList(EntrySpec *entries, int numEntries) {
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
  EntrySpec *entries = malloc(sizeof(EntrySpec) * adi->numEntries);
  if (!entries) bail("Couldn't malloc entries list");
  adi->entries = entries;
  if (fread(adi->entries, sizeof(EntrySpec), adi->numEntries, adi->headerFile) != adi->numEntries)
    bail("Couldn't read entries");
}

static void readFInfo(FILE *fp, char *header, u_int32_t offset, u_int32_t length) {
  if (length > 8) {
    // copy type and creator to header
    if (fseek(fp, ntohl(offset), SEEK_SET) != 0) bail("Couldn't seek file");
    if (fread(&header[kFileType], 1, 8, fp) != 8) bail("Couldn't read file");
  }
}

void registerEntries(char *binHeader, ADInfo *adi) {
  EntrySpec* entries = adi->entries;
  for (unsigned int i = 0; i < adi->numEntries; i++) {
    int type = ntohl(entries[i].type);
    if (type == FINFO) readFInfo(adi->headerFile, binHeader, entries[i].offset, entries[i].length);
    else if (type == RFORK) {
      adi->rsrcOffset = ntohl(entries[i].offset);
      adi->rsrcLength = ntohl(entries[i].length);
      memcpy(&binHeader[kRForkLength], (char *)&(entries[i].length), 4); // cast bc. binHeader not aligned
    } // TODO else if (type == DFORK)
  }
}

/** Get the number of entries in the given AppleSingle/Double header file. Read
    the first 26 bytes of the file and return ADHeader with the number of
    entries, or NULL if it is not an AppleSingle or AppleDouble header file. */
ADHeader* readHeader(FILE *fp) {
  static ADHeader adh;

  if (fseek(fp, 0L, SEEK_SET) != 0) bail("readHeader: Couldn't seek file");
  if (fread(adh.raw, 1, 26, fp) != 26) bail("readHeader: Couldn't read file");

  int magic = ntohl(adh.magic);
  if (magic == MAGIC1 || magic == MAGIC2) adh.magic = magic;
  else return NULL;

  int version = ntohl(adh.version);
  if (version != VERSION2) bail("readHeader: Unknown version");

  adh.numEntries = ntohs(adh.numEntries);
  return &adh;
}

static InputFileType checkFileType(char *filename) {
  if (!filename) return Missing;
  FILE *fp = openFile(filename);
  ADHeader *adh = readHeader(fp);
  InputFileType type;
  if (!adh) type = Neither;
  else if (adh->magic == MAGIC1) type = Single;
  else if (adh->magic == MAGIC2) type = Double;
  else type = Unknown;

  fclose(fp);
  return type;
}

/*
  TODO corner case where data fork happens to be a valid ADF?

  file1 can be: S D N
  file2 can be: S D N M
  12 possibilities

  valid states: SM DN ND

*/
static ADInfo checkInputFiles(char *file1, char *file2) {
  InputFileType type1, type2;
  type1 = checkFileType(file1);
  type2 = checkFileType(file2);

  if (type1 == Single && type2 != Missing) Perror(1, "extra argument: %s after AppleSingle file\n", file2);
  if (type1 == Double && type2 == Single) Perror(2, "AppleDouble and AppleSingle files given\n");
  if (type1 == Double && type2 == Double) Perror(3, "both files are AppleDouble header files\n");
  if (type1 == Double && type2 == Missing) Perror(4, "no second file given after AppleDouble file\n");
  if (type1 == Neither && type2 == Missing) Perror(5, "%s is not an AppleSingle/Double file\n", file1);
  if (type1 == Neither && type2 == Single) Perror(6, "extra argument: %s before AppleSingle file\n", file1);
  if (type1 == Neither && type2 == Neither) Perror(7, "neither argument is an AppleSingle/Double file\n");

  if (type1 == Double && type2 == Neither) return (ADInfo){.header=file1, .dataFile=file2};
  if (type1 == Neither && type2 == Double) return (ADInfo){.header=file2, .dataFile=file1};
  if (type1 == Single && type2 == Missing) return (ADInfo){.header=file1, .single=1};
  Perror(9, "something went wrong\n");
}

/** Reads the AppleSingle/Double header file.
    .header, .dataFile, .headerFile, .numEntries
*/
ADInfo readHeaderInfo(char *file1, char *file2) {
  ADInfo info = checkInputFiles(file1, file2);
  char *baseName = (info.single) ? basename(info.header) : basename(info.dataFile);
  FILE *fp = openFile(info.header);
  ADHeader *adh = readHeader(fp);
  strncpy(info.baseName, baseName, 64);
  info.numEntries = adh->numEntries;
  info.headerFile = fp;
  return info;
}
