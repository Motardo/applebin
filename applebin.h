// AppleDouble constants
#define RFORK 2
#define FINFO 9

#define MAGIC1 0x00051600
#define MAGIC2 0x00051607
#define VERSION2 0x00020000

typedef union EntrySpec {
  char raw[12];
  struct {
    u_int32_t type;
    u_int32_t offset;
    u_int32_t length;
  };
} EntrySpec;

typedef union  ADHeader {
  char raw[26];
  struct {
    u_int32_t magic;
    u_int32_t version;
    char filler[16];
    u_int16_t numEntries;
  };
} ADHeader;

typedef struct ADInfo {
  char *header;
  char *dataFile;
  char baseName[65];
  unsigned int  numEntries;
  signed char single; // 1 = singe, 0 = double
  FILE *headerFile;
  EntrySpec *entries;
  u_int32_t rsrcOffset;
  u_int32_t rsrcLength;
  u_int32_t dataOffset;
  u_int32_t dataLength;
} ADInfo;

#define Perror(result, ...) {                   \
    fprintf(stderr, __VA_ARGS__);               \
    exit(result);                               \
  }

// singleDouble.c
ADHeader* readHeader(FILE *fp);
ADInfo readHeaderInfo(char *file1, char *file2);
void readEntriesList(ADInfo *adi);
void registerEntries(char *header, ADInfo *adi);
void printVerbose(ADInfo *adi);

// MacBinary header constants
#define kFileNameLen 1
#define kFileName 2
#define kFileType 65
#define kFileCreator 69
#define kDForkLength 83
#define kRForkLength 87

#define kSuffix ".bin"
#define kSuffixLen 4
#define kMaxFileName 59

// Prototypes
FILE* openFile(const char *filename);
void bail(char *message);
void readRFork(FILE *fp, char *header, u_int32_t offset, u_int32_t length);
void registerRFork(char *header, ADInfo *adi);
void setFilename(char *header, const char *filename);
void copyFile(FILE *dest, FILE *source, int length);
void padFile(FILE *fp, int length);
