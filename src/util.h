#ifndef _INCLUDE_UTIL
#define _INCLUDE_UTIL

#define EMPTY( XX ) ( XX==NULL || *(XX)==0 )
#define NOTEMPTY( XX ) ( XX!=NULL && *(XX)!=0 )
#define NULLPROTECT( XX ) XX==NULL ? "NULL" : XX
#define NULLPROTECTE( XX ) XX==NULL ? "" : XX

#define ERROR_QUARANTINE -10

typedef struct _tag_value
  {
  char* tag;
  char* value;
  int iValue;
  struct _tag_value *subHeaders;
  struct _tag_value *next;
  } _TAG_VALUE;

_TAG_VALUE* NewTagValue( char* tag, char* value, _TAG_VALUE* list, int replaceDup );
_TAG_VALUE* NewTagValueInt( char* tag, int value, _TAG_VALUE* list, int replaceDup );
_TAG_VALUE* NewTagValueList( char* tag, _TAG_VALUE* subList, _TAG_VALUE* list, int replaceDup );
char* GetTagValue( _TAG_VALUE* list, char* tagName );
char* GetTagValueSafe( _TAG_VALUE* list, char* tagName, char* expr );
void FreeTagValue( _TAG_VALUE* list );

typedef struct ipaddr
  {
  unsigned char bytes[4];
  int bits;
  } IPADDR;

#define ISBYTE(I) ((I)>=0 && (I)<256)

#define FREE(X) {if( (X)==NULL ) {Error("Trying to free NULL (%s:%d)", __FILE__, __LINE__ );} else { free(X); X=NULL; }}
#define FREEIFNOTNULL(X) {if( (X)!=NULL ) { free(X); X=NULL; }}
#define SAFESTRDUP(X) ((X)==NULL?NULL:strdup(X))

struct _config;
typedef struct _config _CONFIG;

char* MakeFullPath( const char* folder, const char* file );
char* TrimHead( char* ptr );
void TrimTail( char* ptr );
int FileExists( const char* path );
int FileExists2( const char* folder, const char* fileName );
int FileUnlink2( const char* folder, const char* fileName );
int FileRename2( const char* folder, const char* oldName, const char* newName );
long FileSize( const char* path );
long FileSize2( const char* folder, const char* fileName );
time_t FileDate( const char* path );
time_t FileDate2( const char* folder, const char* fileName );
int FileCopy( const char* src, const char* dst );
int FileCopy2( const char* srcFolder, const char* srcFile,
               const char* dstFolder, const char* dstFile );
long FileRead( const char* filename, unsigned char** data );
long FileRead2( const char* folder, const char* filename,
                unsigned char** data );
char* GetFolderFromPath( char* path, char* folder, int folderSize );
char* GetFilenameFromPath( char* path );
char* GetBaseFilenameFromPath( char* path, char* buf, int bufLen );
int DirExists( const char* path );
int DirExists2( const char* folder, const char* fileName );
void GenerateIdentifier( char* buf, int nChars );
int IsFolderWritable( char* path );
char* StripQuotes( char* buf );
char* StripEOL( char* buf );
char* SanitizeFilename( const char* path, const char* prefix, const char* fileName, int removeSlash );
void ChompEOL( FILE* stream );
char* GetNameFromPath( char* path );
int StringStartsWith( const char* string, const char* prefix, int caseSensitive );
int StringEndsWith( const char* string, const char* suffix, int caseSensitive );
int CountInString( const char* string, const int c );
int FilterDirEnt( const struct dirent *de );
void FreeDirentList( struct dirent **de, int n );
int StringIsAnIdentifier( char* str );
int ExpandMacros( char* src, char* dst, int dstLen, _TAG_VALUE* patterns );
int ExpandMacrosVA( char* src, char* dst, int dstLen, ... );
void LowerCase( char* dst, int dstSize, char* src );
void *SafeCalloc( size_t nmemb, size_t size, char* msg);
void FreeIfAllocated( char** ptr );
int GetAvailableSpaceOnVolume( char* path, char* buf, int bufLen );
long GetAvailableSpaceOnVolumeBytes( char* path );
int StringMatchesRegex( char* expr, char* str );
char* ExtractRegexFromString( char* expr, char* str );
int StringToIp( IPADDR* dst, char* src );
int IPinSubnet( IPADDR* ip, IPADDR* subnet );
char* Encode( int nBytes, char* bytes );
char* Decode( char* string );
int GetArgumentFromQueryString( char** bufPtr, char* keyword, char* regExp );
int ProcessExistsAndIsMine( pid_t p );
void FreeArrayOfStrings( char** array, int len );
int GetOrderedDirectoryEntries( const char* path,
                                const char* prefix,
                                const char* suffix,
                                char*** entriesP,
                                int sort );
int GetOrderedDirectoryEntries2( const char* parentFolder,
                                 const char* childFolder,
                                 const char* prefix,
                                 const char* suffix,
                                 char*** entriesP,
                                 int sort );
void EnsureDirExists( char* path );
char* DateStr( time_t t, char* buf, int bufLen );
char* DateNow( char* buf, int bufLen );
char* TimeStr( time_t t, char* buf, int bufLen, int showSeconds );
char* TimeNow( char* buf, int bufLen, int showSeconds );
char* DateTimeNow( char* buf, int bufLen, int showSeconds );
char* DateNow( char* buf, int bufLen );
int ValidTime( char* when );
int ValidDate( char* when );
int CompareStrings( const void* vA, const void* vB);
char* GUID();
int CountFilesInFolder( char* folder, char* prefix, char* suffix,
                        time_t *earliest, time_t* latest );
int ListToJSON( _TAG_VALUE* list, char* buf, int bufLen );
int NestedListToJSON( char* arrayName, _TAG_VALUE* list, char* buf, int bufLen );
long long TimeInMicroSeconds();

#endif
