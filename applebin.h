// AppleDouble constants
#define RFORK 2
#define FINFO 9

#define MAGIC1 0x00051600
#define MAGIC2 0x00051607
#define VERSION2 0x00020000

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

// File copy chunk size
// FIXME fread chokes on large chunks
#define kBufferSize 4096

// Prototypes
FILE* openFile(const char *filename);
void printEntriesList(EntrySpec *entries, int numEntries);
void bail(char *message);
void printEntriesList(EntrySpec *entries, int numEntries);
void readEntriesList(FILE *fp, EntrySpec *entries, int numEntries);
int readHeader(FILE *fp);
EntrySpec* registerEntries(char *header, FILE *fp, EntrySpec *entries, int numEntries);
void readRFork(FILE *fp, char *header, u_int32_t offset, u_int32_t length);
void readFInfo(FILE *fp, char *header, u_int32_t offset, u_int32_t length);
void registerRFork(char *header, u_int32_t offset, u_int32_t length);
void setFilename(char *header, const char *filename);
int registerDataFork(char *header, const char *filename);
FILE* writeHeader(const char *header);
void writeRFork(FILE *binFile, FILE *aDF, EntrySpec *rForkEntry);
void writeDFork(FILE *binFile, const char *filename, int length);
void copyFile(FILE *dest, FILE *source, int length);
