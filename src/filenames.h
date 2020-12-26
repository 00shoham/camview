#ifndef _INCLUDE_FILENAMES
#define _INCLUDE_FILENAMES

typedef struct _filename
  {
  char* name;
  struct _filename *next;
  } _FILENAME;

_FILENAME* NewFilename( char* name, _FILENAME* list );
void FreeFilenames( _FILENAME* list );
void BackupFiles( char* parentFolder, _FILENAME* list, char* cmd );

#endif
