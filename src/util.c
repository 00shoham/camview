#include "base.h"
#include <uuid/uuid.h>

char generatedIdentifierChars[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
char validIdentifierChars[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ-_";
char hexDigits[] = "0123456789abcdef";
#define MAX_ENCODE 2000

char* MakeFullPath( const char* folder, const char* file )
  {
  if( EMPTY( file ) )
    return NULL;

  if( EMPTY( folder ) )
    return strdup( file );

  int lFolder = strlen( folder );
  int lFile = strlen( file );

  int addSlash = 0;
  if( folder[lFolder-1]!='/' )
    addSlash = 1;

  int totalLength = lFolder + lFile + addSlash;
  char* buf = (char*)malloc( totalLength+1 );
  char* ptr = buf;
  memcpy( ptr, folder, lFolder );
  ptr += lFolder;
  if( addSlash )
    *(ptr++) = '/';
  memcpy( ptr, file, lFile );
  ptr += lFile;
  *ptr = 0;

  return buf;
  }

/* Check if a file exists and can be opened.
 * Return file size if true; 0 otherwise.
 */
int FileExists( const char* path )
  {
  FILE* f = fopen( path, "r" );
  if( f==NULL )
    {
    return -1;
    }
  fclose( f );

  return 0;
  }

int FileExists2( const char* folder, const char* fileName )
  {
  char* fullPath = MakeFullPath( folder, fileName );
  int err = FileExists( fullPath );
  FREEIFNOTNULL( fullPath );
  return err;
  }

int FileUnlink2( const char* folder, const char* fileName )
  {
  char* fullPath = MakeFullPath( folder, fileName );
  int err = unlink( fullPath );
  FREEIFNOTNULL( fullPath );
  return err;
  }

int FileRename2( const char* folder, const char* oldName, const char* newName )
  {
  char* oldPath = MakeFullPath( folder, oldName );
  char* newPath = MakeFullPath( folder, newName );
  int err = rename( oldName, newName );
  FREEIFNOTNULL( newPath );
  FREEIFNOTNULL( oldPath );
  return err;
  }

time_t FileDate( const char* path )
  {
  if( FileExists( path )!=0 )
    {
    return -1;
    }

  struct stat sbuf;
  if( stat( path, &sbuf )<0 )
    {
    return -2;
    }

  return sbuf.st_ctime;
  }

time_t FileDate2( const char* folder, const char* fileName )
  {
  char* fullPath = MakeFullPath( folder, fileName );
  time_t t = FileDate( fullPath );
  FREEIFNOTNULL( fullPath );
  return t;
  }

long FileSize( const char* path )
  {
  if( FileExists( path )!=0 )
    {
    return -1;
    }

  struct stat sbuf;
  if( stat( path, &sbuf )<0 )
    {
    return -2;
    }

  return sbuf.st_size;
  }

long FileSize2( const char* folder, const char* fileName )
  {
  char* fullPath = MakeFullPath( folder, fileName );
  long l = FileSize( fullPath );
  FREEIFNOTNULL( fullPath );
  return l;
  }

/* Check if a directory exists and can be opened.
 * Return 1 if true; 0 otherwise.
 */
int DirExists( const char* path )
  {
  DIR* d = opendir( path );
  if( d==NULL )
    {
    return -1;
    }
  closedir( d );
  return 0;
  }

int DirExists2( const char* folder, const char* fileName )
  {
  char* fullPath = MakeFullPath( folder, fileName );
  int err = DirExists( fullPath );
  FREEIFNOTNULL( fullPath );
  return err;
  }

int CompareStrings( const void* vA, const void* vB)
  {
  const char** pa = (const char**)vA;
  const char** pb = (const char**)vB;
  const char* a = *pa;
  const char* b = *pb;

  return strcmp( a, b );
  }

/* path is optional - assume current dir */
int GetOrderedDirectoryEntries( const char* path,
                                const char* prefix,
                                const char* suffix,
                                char*** entriesP,
                                int sort )
  {
  if( EMPTY( path ) )
    path = ".";

  if( entriesP==NULL )
    Error("Must specify a char*** arg entriesP to GetOrderedDirectoryEntries");

  DIR* d = opendir( path );
  if( d==NULL )
    {
    return -1;
    }

  int nEntries = 0;
  struct dirent * de;
  while( (de=readdir( d ))!=NULL )
    {
    if( NOTEMPTY( de->d_name )
        && ( suffix==NULL || StringEndsWith( de->d_name, suffix, 0 )==0 )
        && ( prefix==NULL || StringStartsWith( de->d_name, prefix, 0 )==0 ) )
      {
      ++nEntries;
      }
    }

  if( nEntries==0 )
    {
    *entriesP = NULL;
    closedir( d );
    return 0;
    }

  char** entries = NULL;
  entries = (char**)calloc( nEntries+1, sizeof( char* ) );

  rewinddir( d );
  int n = 0;
  while( (de=readdir( d ))!=NULL && n<nEntries )
    {
    if( NOTEMPTY( de->d_name )
        && ( suffix==NULL || StringEndsWith( de->d_name, suffix, 0 )==0 )
        && ( prefix==NULL || StringStartsWith( de->d_name, prefix, 0 )==0 ) )
      {
      entries[ n ] = strdup( de->d_name );
      ++n;
      }
    }
  entries[n] = NULL;

  closedir( d );

  /* race condition - n could have changed since previous list */
  nEntries = n;
  if( sort )
    {
    qsort( entries, nEntries, sizeof( char* ), CompareStrings );
    }

  *entriesP = entries;
  return nEntries;
  }

int GetOrderedDirectoryEntries2( const char* parentFolder,
                                 const char* childFolder,
                                 const char* prefix,
                                 const char* suffix,
                                 char*** entriesP,
                                 int sort )
  {
  char* fullPath = MakeFullPath( parentFolder, childFolder );
  int err = GetOrderedDirectoryEntries( fullPath,
                                        prefix, suffix,
                                        entriesP, sort );
  FREEIFNOTNULL( fullPath );
  return err;
  }

void FreeArrayOfStrings( char** array, int len )
  {
  if( array==NULL )
    return;

  for( int i=0; i<len; ++i )
    {
    char* p = array[i];
    if( p!=NULL )
      {
      FREE( p );
      p = NULL;
      }
    }

  FREE( array );
  }

/* Extract the path name, without the filename, from a full path.
 * Return NULL on failure conditions.
 * Note that the path argument is not modified.
 */
char* GetFolderFromPath( char* path, char* folder, int folderSize )
  {
  char* src = NULL;
  char* dst = NULL;
  char* lastSlash = NULL;

  if( EMPTY( path ) ) Error( "Cannot extract folder from empty path" );
  if( folder == NULL ) Error( "Cannot write folder to a NULL buffer" );
  dst = folder;
  *dst = 0;

  for( src=path, dst=folder; *src!=0 && (dst-folder)<folderSize-1; ++src, ++dst )
    {
    *dst = *src;
    if( *dst == '/' )
      {
      lastSlash = dst;
      }
    }
  *dst = 0;
  if( lastSlash!=NULL )
    {
    *lastSlash = 0;
    }
  return folder;
  }

/* Extract the filename name, without the folder, from a full path.
 * Return NULL on failure conditions.
 */
char* GetFilenameFromPath( char* path )
  {
  if( EMPTY( path ) )
    {
    return NULL;
    }

  while( path!=NULL && *path!=0 )
    {
    char* ptr = strchr( path, '/' );
    if( ptr==NULL )
      {
      ptr = strchr( path, '\\' );
      }
    if( ptr==NULL )
      {
      return path;
      }
    else
      {
      path = ptr+1;
      }
    }

  return NULL;
  }

char* GetBaseFilenameFromPath( char* path, char* buf, int bufLen )
  {
  if( EMPTY( path ) )
    {
    return NULL;
    }

  if( buf==NULL || bufLen-1 < strlen(path) )
    {
    return NULL;
    }

  strncpy( buf, path, bufLen-1 );
  for( char* ptr=buf+strlen(buf)-1; ptr>=buf; ptr-- )
    {
    if( *ptr=='.' )
      {
      *ptr = 0;
      break;
      }
    }

  return buf;
  }

/* Return a pointer to the first non-whitespace character in a string.
 */
char* TrimHead( char* buf )
  {
  if( EMPTY( buf ) )
    return buf;

  /* printf("TrimHead( %s )\n", buf ); */

  char* ptr = buf;
  while( *ptr!=0 )
    {
    if( *ptr==' ' || *ptr=='\t' || *ptr=='\r' || *ptr=='\n' )
      {
      ++ptr;
      }
    else
      {
      break;
      }
    }

  /* printf("TrimHead --> ( %s )\n", ptr ); */

  return ptr;
  }

/* Remove trailing LF and CR characters from a string.
 * Note that the input argument is modified - all LF or CR
 * in a sequence that appears at the end of the string are
 * replaced with '\0'.
 */
void TrimTail( char* ptr )
  {
  if( EMPTY( ptr ) )
    return;

  for( char* eolc = ptr+strlen(ptr)-1; eolc>=ptr; --eolc )
    {
    if( *eolc == '\r' || *eolc == '\n' || *eolc==' ' || *eolc=='\t' )
      {
      *eolc = 0;
      }
    else
      {
      break;
      }
    }
  }

int randSeeded = 0;

/* Generate a random identifier of N letters and digits in a provided buffer */
void GenerateIdentifier( char* buf, int nChars )
  {
  if( randSeeded==0 )
    {
    srand48( time( NULL ) );
    randSeeded = 1;
    }

  int l = strlen( generatedIdentifierChars );
  for( int i=0; i<nChars; ++i )
    {
    int c = lrand48() % l;
    *(buf++) = generatedIdentifierChars[c];
    }
  *buf = 0;
  }

int StringIsAnIdentifier( char* str )
  {
  if( EMPTY( str ) )
    {
    return -1;
    }
  else
    {
    while( *str!=0 )
      {
      int c = *str;
      char* ptr = strchr( validIdentifierChars, c );
      if( ptr==NULL )
        {
        return -2;
        }
      ++str;
      }
    }

  return 0;
  }

int IsFolderWritable( char* path )
  {
  if( EMPTY( path ) )
    {
    return -1; /* no path provided */
    }

  char pathWithFile[BUFLEN];
  char* ptr = pathWithFile;
  char* end = pathWithFile + sizeof( pathWithFile ) - 1;

  strncpy( ptr, path, end-ptr );
  ptr += strlen( pathWithFile );
  if( *(ptr-1) != '/' )
    {
    *ptr = '/';
    ++ptr;
    *ptr = 0;
    }

  if( end-ptr<10 )
    {
    return -2; /* not enough room in buffer */
    }

  GenerateIdentifier( ptr, 8 );

  FILE* f = fopen( ptr, "w" );
  if( f==NULL )
    {
    return -3; /* not writable! */
    }

  fclose( f );
  unlink( ptr );

  return 0;
  }

/* Modify a string such that any trailing " marks are replaced with
 * '\0' and return a pointer to the first non-" characters in it.
 * Effectively the return string is a version of the input string
 * but without quotes.  Note that the input string is modified.
 */
char* StripQuotes( char* buf )
  {
  if( EMPTY( buf ) )
    {
    return buf;
    }

  if( *buf == '"' )
    {
    ++buf;
    }

  if( buf[strlen(buf)-1]=='"' )
    {
    buf[strlen(buf)-1] = 0;
    }

  return buf;
  }

/* Remove leading and trailing CR and LF chars from a string.
 * Return a pointer to within the original string, which has been
 * modified (at the tail end).
 */
char* StripEOL( char* buf )
  {
  if( EMPTY( buf ) )
    {
    return buf;
    }

  while( *buf == '\r' || *buf=='\n' )
    {
    ++buf;
    }

  if( buf[0]!=0 )
    {
    char* eos = buf + strlen(buf) - 1;
    while( (*eos=='\r' || *eos=='\n') && eos>=buf )
      {
      *eos = 0;
      --eos;
      }
    }

  return buf;
  }

/* SanitizeFilename()
 * Strip unacceptable chars from an input filename, to create a filename
 * that is "safe."  Return an allocated string with the new filename.
 * Don't forget to free the returned filename.
 */
char* SanitizeFilename( const char* path, const char* prefix, const char* fileName, int removeSlash )
  {
  char buf[BUFLEN];
  const char* srcPtr;
  char* dstPtr;
  char* endPtr = buf+BUFLEN-2;

  strncpy( buf, path, sizeof(buf)-2 );
  dstPtr = buf + strlen(buf);

  if( dstPtr > buf )
    {
    if( *(dstPtr-1)!='/' )
      {
      *dstPtr = '/';
      ++dstPtr;
      *dstPtr = 0;
      }
    }

  if( NOTEMPTY( prefix ) )
    {
    strncpy( dstPtr, prefix, endPtr-dstPtr-2 );
    dstPtr += strlen( dstPtr );
    *(dstPtr++) = '-';
    *dstPtr = 0;
    }

  srcPtr = fileName;

  for( ; *srcPtr!=0 && dstPtr<=endPtr; ++srcPtr )
    {
    int c = *srcPtr;
    if( strchr( VALIDFILECHARS, c )!=NULL )
      {
      *dstPtr = c;
      ++dstPtr;
      *dstPtr = 0;
      }
    else
      {
      if( dstPtr>buf && *(dstPtr-1)==REPLACEMENTCHAR )
        {
        /* no need for sequences of replacement chars */
        }
      else
        {
        *dstPtr = REPLACEMENTCHAR;
        ++dstPtr;
        *dstPtr = 0;
        }
      }
    }

  /* strip any / chars */
  dstPtr = buf;
  if( removeSlash )
    {
    char* p;
    while( (p=strchr( dstPtr, '/' ))!=NULL )
      {
      dstPtr = p+1;
      }
    }

  return strdup( dstPtr );
  }

char* GetNameFromPath( char* path )
  {
  if( EMPTY( path ) )
    {
    return NULL;
    }

  for( char* ptr=path+strlen(path)-1; ptr>=path; ptr-- )
    {
    if( *ptr=='/' )
      {
      return ptr+1;
      }
    }

  return path;
  }

int StringEndsWith( const char* string, const char* suffix, int caseSensitive )
  {
  if( EMPTY( string ) || EMPTY( suffix ) )
    {
    return -1;
    }

  int ls = strlen( string );
  int lu = strlen( suffix );

  if( lu>ls )
    {
    return -2;
    }

  if( caseSensitive )
    {
    return strcasecmp( string + ls - lu, suffix );
    }

  return strcmp( string + ls - lu, suffix );
  }

int StringStartsWith( const char* string, const char* prefix, int caseSensitive )
  {
  if( EMPTY( string ) || EMPTY( prefix ) )
    {
    return -1;
    }

  int ls = strlen( string );
  int lp = strlen( prefix );

  if( lp>ls )
    {
    return -2;
    }

  if( caseSensitive )
    {
    return strncasecmp( string, prefix, strlen(prefix) );
    }

  return strncmp( string, prefix, strlen(prefix) );
  }

int CountInString( const char* string, const int c )
  {
  if( EMPTY( string ) )
    {
    return 0;
    }
  int n=0;
  while( *string!=0 )
    {
    if( *string ==c )
      {
      ++n;
      }
    ++string;
    }

  return n;
  }

char** libraryExtensions = NULL;
int nExtensions = 0;

int FilterDirEnt( const struct dirent *de )
  {
  if( de==NULL )
    return 0;

  const char* fileName = de->d_name;
  if( EMPTY( fileName ) )
    {
    return 0;
    }
  else
    {
    for( int i=0; i<nExtensions; ++i )
      {
      char* ext = libraryExtensions[i];
      if( StringEndsWith( fileName, ext, 0 )==0 )
        {
        return 1;
        }
      }
    }

  return 0;
  }

void FreeDirentList( struct dirent **de, int n )
  {
  for( int i=0; i<n; ++i )
    {
    FREEIFNOTNULL( de[i] );
    }
  }

/* NewTagValue()
 * Allocates and initializes a _TAG_VALUE data structure.
 * This is a linked list of tag/value pairs with an option to have a 'child' linked
 * list as well (tag/value/sub-list).
 * Memory management: Don't forget to clean up with FreeTagValue when done.
 */
_TAG_VALUE* NewTagValue( char* tag, char* value, _TAG_VALUE* list, int replaceDup )
  {
  if( replaceDup )
    {
    /* do we already have this tag?  if so, replace the value */
    for( _TAG_VALUE* t=list; t!=NULL; t=t->next )
      {
      if( NOTEMPTY( t->tag ) && strcmp( t->tag,tag )==0 )
        {
        FreeIfAllocated( &(t->value) );
        if( value==NULL ) t->value = NULL;
        else t->value = strdup( value );
        t->type = VT_STR;
        return list;
        }
      }
    }

  /* nope - it's new.  create a new item in the list */
  _TAG_VALUE* n = (_TAG_VALUE*)SafeCalloc(1, sizeof( _TAG_VALUE ), "_TAG_VALUE" );
  if( tag==NULL )
    n->tag = NULL;
  else
    n->tag = strdup( tag );

  n->type = VT_STR;
  if( value==NULL ) n->value = NULL;
  else n->value = strdup( value );
  n->subHeaders = NULL;
  n->next = list;
  return n;
  }

int AllDigits( char* str )
  {
  while( *str!=0 )
    {
    int c = *str;
    if( ! isdigit( c ) )
      return 0;
    ++str;
    }

  return 1;
  }

int AllDigitsSingleDot( char* str )
  {
  int nDots = 0;
  while( (*str)!=0 )
    {
    int c = *str;
    if( ! isdigit( c ) )
      {
      if( c == '.' )
        {
        ++nDots;
        }
      else
        {
        return 0;
        }

      if( nDots>1 )
        {
        return 0;
        }
      }

    ++str;
    }

  if( nDots==1 )
    {
    return 1;
    }

  return 0;
  }

enum valueType GuessType( char* str )
  {
  if( EMPTY( str ) )
    return VT_INVALID;

  if( AllDigits( str ) )
    return VT_INT;

  if( AllDigitsSingleDot( str ) )
    return VT_DOUBLE;

  return VT_STR;
  }

_TAG_VALUE* NewTagValueGuessType( char* tag, char* value, _TAG_VALUE* list, int replaceDup )
  {
  if( replaceDup )
    {
    /* do we already have this tag?  if so, replace the value */
    for( _TAG_VALUE* t=list; t!=NULL; t=t->next )
      {
      if( ( NOTEMPTY( t->tag ) && NOTEMPTY( tag ) && strcmp( t->tag,tag )==0 )
          || ( EMPTY( t->tag ) && EMPTY( tag ) ) )
        {
        FreeIfAllocated( &(t->value) );
        t->type = GuessType( value );
        switch( t->type )
          {
          case VT_INVALID:
            break;
          case VT_STR:
            t->value = strdup( value );
            break;
          case VT_INT:
            t->iValue = atoi( value );
            break;
          case VT_DOUBLE:
            t->dValue = atof( value );
            break;
          case VT_LIST:
            Error( "We do not guess the type of a list value in TV" );
            break;
          case VT_NULL:
            /* not a thing */
            break;
          }
        return list;
        }
      }
    }

  /* nope - it's new.  create a new item in the list */
  _TAG_VALUE* n = (_TAG_VALUE*)SafeCalloc(1, sizeof( _TAG_VALUE ), "_TAG_VALUE" );
  if( EMPTY( tag ) )
    n->tag = NULL;
  else
    n->tag = strdup( tag );
  n->type = GuessType( value );
  switch( n->type )
    {
    case VT_INVALID:
      break;
    case VT_STR:
      n->value = strdup( value );
      break;
    case VT_INT:
      n->iValue = atoi( value );
      break;
    case VT_DOUBLE:
      n->dValue = atof( value );
      break;
    case VT_LIST:
      Error( "TV type guessing does not return LIST" );
      break;
    case VT_NULL:
      /* not a thing */
      break;
    }

  n->next = list;
  return n;
  }

_TAG_VALUE* NewTagValueNull( char* tag, _TAG_VALUE* list, int replaceDup )
  {
  if( replaceDup )
    {
    /* do we already have this tag?  if so, replace the value */
    for( _TAG_VALUE* t=list; t!=NULL; t=t->next )
      {
      if( ( NOTEMPTY( t->tag ) && NOTEMPTY( tag ) && strcmp( t->tag,tag )==0 )
          || ( EMPTY( t->tag ) && EMPTY( tag ) ) )
        {
        FreeIfAllocated( &(t->value) );
        t->type = VT_NULL;
        t->value = NULL;
        return list;
        }
      }
    }

  /* nope - it's new.  create a new item in the list */
  _TAG_VALUE* n = (_TAG_VALUE*)SafeCalloc(1, sizeof( _TAG_VALUE ), "_TAG_VALUE" );
  if( EMPTY( tag ) )
    n->tag = NULL;
  else
    n->tag = strdup( tag );
  n->type = VT_NULL;

  n->next = list;
  return n;
  }

_TAG_VALUE* NewTagValueInt( char* tag, int value, _TAG_VALUE* list, int replaceDup )
  {
  if( replaceDup )
    {
    /* do we already have this tag?  if so, replace the value */
    for( _TAG_VALUE* t=list; t!=NULL; t=t->next )
      {
      if( NOTEMPTY( t->tag ) && strcmp( t->tag,tag )==0 )
        {
        t->iValue = value;
        t->type = VT_INT;
        return list;
        }
      }
    }

  /* nope - it's new.  create a new item in the list */
  _TAG_VALUE* n = (_TAG_VALUE*)SafeCalloc(1, sizeof( _TAG_VALUE ), "_TAG_VALUE" );
  n->tag = strdup( tag );
  n->iValue = value;
  n->type = VT_INT;
  n->subHeaders = NULL;
  n->next = list;
  return n;
  }

_TAG_VALUE* NewTagValueDouble( char* tag, double value, _TAG_VALUE* list, int replaceDup )
  {
  if( replaceDup )
    {
    /* do we already have this tag?  if so, replace the value */
    for( _TAG_VALUE* t=list; t!=NULL; t=t->next )
      {
      if( NOTEMPTY( t->tag ) && strcmp( t->tag,tag )==0 )
        {
        t->dValue = value;
        t->type = VT_DOUBLE;
        return list;
        }
      }
    }

  /* nope - it's new.  create a new item in the list */
  _TAG_VALUE* n = (_TAG_VALUE*)SafeCalloc(1, sizeof( _TAG_VALUE ), "_TAG_VALUE" );
  n->tag = strdup( tag );
  n->dValue = value;
  n->type = VT_DOUBLE;
  n->subHeaders = NULL;
  n->next = list;
  return n;
  }

_TAG_VALUE* NewTagValueList( char* tag, _TAG_VALUE* value, _TAG_VALUE* list, int replaceDup )
  {
  if( replaceDup )
    {
    /* do we already have this tag?  if so, replace the value */
    for( _TAG_VALUE* t=list; t!=NULL; t=t->next )
      {
      if( NOTEMPTY( t->tag ) && strcmp( t->tag,tag )==0 )
        {
        if( t->subHeaders!=NULL )
          {
          FreeTagValue( t->subHeaders );
          }
        t->subHeaders = value;
        t->type = VT_LIST;
        return list;
        }
      }
    }

  /* nope - it's new.  create a new item in the list */
  _TAG_VALUE* n = (_TAG_VALUE*)SafeCalloc(1, sizeof( _TAG_VALUE ), "_TAG_VALUE" );
  if( EMPTY( tag ) )
    n->tag = NULL;
  else
    n->tag = strdup( tag );
  n->subHeaders = value;
  n->type = VT_LIST;
  n->next = list;
  return n;
  }

_TAG_VALUE* DeleteTagValue( _TAG_VALUE* list, char* tag )
  {
  if( EMPTY( tag ) || list==NULL )
    return list;

  _TAG_VALUE** ptr = &list;
  while( ptr!=NULL && (*ptr)!=NULL )
    {
    _TAG_VALUE* current = *ptr;
    if( current!=NULL
        && NOTEMPTY( current->tag )
        && strcmp( current->tag, tag )==0 )
      {
      *ptr = current->next;
      FreeIfAllocated( &(current->tag) );
      FreeIfAllocated( &(current->value) );
      FreeTagValue( current->subHeaders );
      FREE( current );
      /* keep going as there may be other tag/value pairs with the same tag */
      }
    else
      {
      if( current==NULL )
        break;
      ptr = & (current->next);
      }
    }

  return list;
  }

char* GetTagValue( _TAG_VALUE* list, char* tagName )
  {
  char* retVal = NULL;
  if( EMPTY( tagName ) )
    {
    return NULL;
    }

  while( list!=NULL )
    {
    if( NOTEMPTY( list->tag)
        && strcasecmp(list->tag,tagName)==0
        && list->type==VT_STR )
      {
      retVal = list->value;
      break;
      }
    list = list->next;
    }

  return retVal;
  }

_TAG_VALUE* FindTagValue( _TAG_VALUE* list, char* tagName )
  {
  _TAG_VALUE* retVal = NULL;
  if( EMPTY( tagName ) )
    {
    return NULL;
    }

  while( list!=NULL )
    {
    if( NOTEMPTY( list->tag)
        && strcmp(list->tag,tagName)==0 )
      {
      retVal = list;
      break;
      }
    list = list->next;
    }

  return retVal;
  }

int GetTagValueInt( _TAG_VALUE* list, char* tagName )
  {
  int retVal = INVALID_INT;
  if( EMPTY( tagName ) )
    {
    return retVal;
    }

  while( list!=NULL )
    {
    if( NOTEMPTY( list->tag )
        && strcasecmp(list->tag,tagName)==0
        && list->type==VT_INT )
      {
      retVal = list->iValue;
      break;
      }
    list = list->next;
    }

  return retVal;
  }

double GetTagValueDouble( _TAG_VALUE* list, char* tagName )
  {
  double retVal = INVALID_DOUBLE;
  if( EMPTY( tagName ) )
    {
    return retVal;
    }

  while( list!=NULL )
    {
    if( NOTEMPTY( list->tag )
        && strcasecmp(list->tag,tagName)==0 )
      {
      if( list->type==VT_DOUBLE )
        {
        retVal = list->dValue;
        break;
        }
      else if( list->type==VT_INT )
        {
        retVal = (double)list->iValue;
        break;
        }
      }
    list = list->next;
    }

  return retVal;
  }

_TAG_VALUE* GetTagValueList( _TAG_VALUE* list, char* tagName )
  {
  if( EMPTY( tagName ) || list==NULL )
    return NULL;

  _TAG_VALUE* retVal = NULL;
  for( ; list!=NULL; list=list->next )
    {
    if( NOTEMPTY( list->tag )
        && strcasecmp( list->tag, tagName )==0 )
      {
      retVal = list->subHeaders;
      break;
      }
    }

  return retVal;
  }

int CompareTagValueList( _TAG_VALUE* a, _TAG_VALUE* b )
  {
  if( a==b )
    return 0;

  if( a!=NULL && b==NULL )
    return -1;

  if( a==NULL && b!=NULL )
    return -2;

  for( _TAG_VALUE* tv=a; tv!=NULL; tv=tv->next )
    {
    if( EMPTY( tv->tag ) )
      continue;

    switch( tv->type )
      {
      case VT_LIST:
        {
        _TAG_VALUE* cmp = GetTagValueList( b, tv->tag );
        if( cmp==NULL )
          return -100;
        int err = CompareTagValueList( tv->subHeaders, cmp );
        if( err!=0 )
          return -100 + err;
        }
        break;

      case VT_INVALID:
        if( tv->subHeaders!=NULL )
          {}
        else
          Warning( "Comparing tags - [%s] has invalid type", NULLPROTECT( tv->tag ) );
        break;

      case VT_STR:
        if( EMPTY( tv->value ) )
          {
          printf( "%s has empty string\n", NULLPROTECT( tv->tag ) );
          return -3;
          }

        char* str = GetTagValue( b, tv->tag );
        if( EMPTY( str ) )
          return -4;

        int c = strcmp( tv->value, str );
        if( c )
          return -5;
        break;

      case VT_INT:
        {
        int i = GetTagValueInt( b, tv->tag );
        if( i!=tv->iValue )
          return -6;
        }
        break;

      case VT_DOUBLE:
        {
        double d = GetTagValueDouble( b, tv->tag );
        if( d!=tv->dValue )
          return -7;
        }
        break;

      case VT_NULL:
        {
        _TAG_VALUE* sub = FindTagValue( b, tv->tag );
        if( sub==NULL || sub->type!=VT_NULL )
          return -8;
        }
        break;
      }

    if( tv->type != VT_LIST
        && tv->subHeaders!=NULL )
      { /* not really a list but there are sub-headers anyways? */
      _TAG_VALUE* sub = GetTagValueList( b, tv->tag );
      if( sub==NULL )
        return -200;
      int err = CompareTagValueList( tv->subHeaders, sub );
      if( err!=0 )
        return -200 + err;
      }

    }

  return 0;
  }

int CompareTagValueListBidirectional( _TAG_VALUE* a, _TAG_VALUE* b )
  {
  int c1 = CompareTagValueList( a, b );
  int c2 = CompareTagValueList( b, a );
  return c1 | c2;
  }

void PrintTagValue( int indent, _TAG_VALUE* list )
  {
  if( indent<0 || indent>100 )
    return;
  if( list==NULL )
    return;

  char indentBuf[200];
  memset( indentBuf, ' ', indent );
  indentBuf[indent] = 0;

  char printLine[BUFLEN];
  char* ptr = printLine;
  char* end = printLine + sizeof(printLine) - 1;

  strncpy( ptr, indentBuf, end-ptr );
    ptr += strlen( ptr );
  if( list->tag!=NULL )
    {
    strncpy( ptr, "\"", end-ptr );
      ptr += strlen( ptr );
    strncpy( ptr, NULLPROTECT( list->tag ), end-ptr );
      ptr += strlen( ptr );
    strncpy( ptr, "\"", end-ptr );
      ptr += strlen( ptr );
    strncpy( ptr, ": ", end-ptr );
      ptr += strlen( ptr );
    }
  switch( list->type )
    {
    case VT_INVALID:
      if( list->subHeaders!=NULL )
        {}
      else
        Warning( "Printing tags - [%s] has invalid type", NULLPROTECT( list->tag ) );
      break;
    case VT_LIST:
      if( list->subHeaders==NULL )
        {
        strncpy( ptr, "[]", end-ptr );
          ptr += strlen( ptr );
        }
      break;
    case VT_NULL:
      strncpy( ptr, "null", end-ptr );
      ptr += strlen( ptr );
      break;
    case VT_STR:
      if( list->value!=NULL )
        {
        strncpy( ptr, "\"", end-ptr );
          ptr += strlen( ptr );
        strncpy( ptr, list->value, end-ptr );
          ptr += strlen( ptr );
        strncpy( ptr, "\"", end-ptr );
          ptr += strlen( ptr );
        }
      break;
    case VT_INT:
      snprintf( ptr, (int)(end-ptr), "(int) %d", list->iValue );
      break;
    case VT_DOUBLE:
      snprintf( ptr, (int)(end-ptr), "(double) %lf", list->dValue );
      break;
    }
  /*
  strncpy( ptr,end-ptr, "\n" );
    ptr += strlen( ptr );
  */
  Notice( printLine );
  if( list->subHeaders!=NULL )
    {
    PrintTagValue( indent+2, list->subHeaders );
    }
  if( list->next!=NULL )
    {
    PrintTagValue( indent, list->next );
    }
  }

char* GetTagValueSafe( _TAG_VALUE* list, char* tagName, char* expr )
  {
  char* val = NULL;
  val = GetTagValue( list, tagName );

  if( EMPTY( expr ) )
    {
    return val;
    }

  if( StringMatchesRegex( expr, val )==0 )
    {
    return val;
    }

  /* no match - return empty string as original is dangerous */
  return NULL;
  }

_TAG_VALUE* CopyTagValueList( _TAG_VALUE* list )
  {
  if( list==NULL )
    return NULL;

  _TAG_VALUE* newList = NULL;
  for( ; list!=NULL; list=list->next )
    {
    switch( list->type )
      {
      case VT_INVALID:
        Error( "Cannot copy a _TAG_VALUE list with an invalid-type entry" );

      case VT_STR:
        newList = NewTagValue( list->tag, list->value, newList, 0 );
        break;

      case VT_INT:
        newList = NewTagValueInt( list->tag, list->iValue, newList, 0 );
        break;

      case VT_DOUBLE:
        newList = NewTagValueDouble( list->tag, list->dValue, newList, 0 );
        break;

      case VT_LIST:
        newList = NewTagValueList( list->tag, CopyTagValueList( list->subHeaders ), newList, 0 );
        break;

      case VT_NULL:
        newList = NewTagValueNull( list->tag, newList, 0 );
        break;
      }
    }

  return newList;
  }

/* FreeTagValue()
 * Recurively free up memory in a _TAG_VALUE nested linked list.
 * Recommended to set the pointer to the list to NULL after calling this.
 */
void FreeTagValue( _TAG_VALUE* list )
  {
  if( list==NULL )
    {
    return;
    }
  if( list->subHeaders!=NULL )
    {
    FreeTagValue( list->subHeaders );
    list->subHeaders = NULL;
    }
  if( list->next!=NULL )
    {
    FreeTagValue( list->next );
    list->next = NULL;
    }
  if( list->tag!=NULL )
    {
    FREE( list->tag );
    }
  if( list->value!=NULL )
    {
    FREE( list->value );
    }
  FREE( list );
  }

/* ExpandMacros
 * Inputs: src string, _TAG_VALUE linked list of search/replace pairs
 * Outputs: dst buffer of dstLen size
 * Returns: negative=error, else # of string replacements.
 * %MACRO% or $MACRO[^a-z] replaced as per search/replace data.
 */
int ExpandMacros( char* src, char* dst, int dstLen, _TAG_VALUE* patterns )
  {
  if( src==NULL || dst==NULL )
    {
    return -1;
    }

  /*
  Notice("ExpandMacros( %s )", NULLPROTECT( src ) );
  for( _TAG_VALUE* t=patterns; t!=NULL; t=t->next )
    {
    Notice("... %s : %s", NULLPROTECT(t->tag), NULLPROTECT(t->value) );
    }
  */

  char* endp = dst + dstLen;
  char* ends = src + strlen(src);
  int c;
  int nReplacements = 0;
  while( (*src)!=0 )
    {
    c = *src;
    if( c=='%' )
      {
      int nextC = *(src+1);
      if( nextC=='%' )
        {
        *dst = '%';
        ++dst;
        src+=2;
        }
      else
        {
        int gotReplacement = 0;
        for( _TAG_VALUE* t = patterns; t!=NULL; t=t->next )
          {
          if( NOTEMPTY( t->tag ) )
            {
            int l = strlen(t->tag);
            int m = strlen(NULLPROTECTE(t->value));
            /*   %MACRO% */
            if( t->value!=NULL 
                && dst+m+1<endp
                && src+l+1<ends
                && strncmp( src+1, t->tag, l )==0 )
              {
              int trailP = *(src+l+1);
              if( trailP=='%' )
                {
                strcpy( dst, NULLPROTECTE( t->value ) );
                dst += m;
                src += l+2;
                ++gotReplacement;
                ++nReplacements;
                break;
                }
              else
                {
                /* no trailing % - not this macro anyways */
                }
              }
            else
              {
              /* empty search string or not enough space or no match */
              }
            }
          else
            {
            /* empty search string */
            }
          } /* for loop of tag/value pairs */

        if( gotReplacement==0 ) /* no match - just a % sign */
          {
          *(dst++) = *(src++);
          }
        } /* char after % is not another % */
      } /* char was '%' */
    else if( c=='$' )
      {
      int nextC = *(src+1);
      if( nextC=='$' )
        {
        *dst = '$';
        ++dst;
        src+=2;
        }
      else
        {
        int gotReplacement = 0;
        for( _TAG_VALUE* t = patterns; t!=NULL; t=t->next )
          {
          if( NOTEMPTY( t->tag ) )
            {
            int l = strlen(t->tag);
            int m = strlen(NULLPROTECTE(t->value));
            /* $MACRO */
            if( t->value!=NULL 
                && dst+m+1<endp
                && src+l<ends
                && strncmp( src+1, t->tag, l )==0 )
              {
              int trailP = *(src+l+1);
              if( !isalpha( trailP ) )
                {
                strcpy( dst, NULLPROTECTE( t->value ) );
                dst += m;
                *(dst++) = trailP;
                src += l+2;
                ++gotReplacement;
                ++nReplacements;
                break;
                }
              else
                {
                /* no trailing (non-alpha) chars - not this macro anyways */
                }
              }
            else
              {
              /* empty search string or not enough space or no match */
              }
            }
          else
            {
            /* empty search string */
            }
          } /* for loop of tag/value pairs */

        if( gotReplacement==0 ) /* no match - just a % sign */
          {
          *(dst++) = *(src++);
          }
        } /* char after $ is not another $ */
      } /* char was '$' */
    else
      {
      *(dst++) = *(src++);
      }
    }

  *dst = 0;

  return nReplacements;
  }

int ExpandMacrosVA( char* src, char* dst, int dstLen, ... )
  {
  _TAG_VALUE* patterns = NULL;

  va_list argptr;
  va_start( argptr, dstLen );
  for(;;)
    {
    char* tag = NULL;
    char* value = NULL;

    tag = va_arg( argptr, char* );
    if( tag==NULL )
      {
      break;
      }

    value = va_arg( argptr, char* );
    if( value!=NULL )
      {
      patterns = NewTagValue( tag, value, patterns, 1 );
      }
    }

  va_end( argptr );

  int n = ExpandMacros( src, dst, dstLen, patterns );
  FreeTagValue( patterns );

  return n;
  }

_TAG_VALUE* TagIntList( char* placeHolderPtr, ... )
  {
  _TAG_VALUE* list = NULL;

  va_list argptr;
  va_start( argptr, placeHolderPtr );
  for(;;)
    {
    char* tag = NULL;

    tag = va_arg( argptr, char* );
    if( tag==NULL )
      {
      break;
      }

    int value = va_arg( argptr, int );
    list = NewTagValueInt( tag, value, list, 1 );
    }

  va_end( argptr );

  return list;
  }

void LowerCase( char* dst, int dstSize, char* src )
  {
  if( dst==NULL )
    return;

  if( dst!=src )
    *dst = 0;

  if( EMPTY( src ) )
    return;

  char* endp = dst + dstSize - 1;
  while( (*src)!=0 && dst<endp )
    {
    int c = *src;
    c = tolower( c );
    *dst = c;
    ++dst;
    ++src;
    }

  *dst = 0;
  }

void *SafeCalloc( size_t nmemb, size_t size, char* msg )
  {
  void* ptr = calloc( nmemb, size );
  if( ptr==NULL )
    {
    Error("Failed to allocate %d x %d (%s)", nmemb, size, msg );
    }
  return ptr;
  }

void FreeIfAllocated( char** ptr )
  {
  if( (*ptr)==NULL )
    return;
  FREE( *ptr );
  *ptr = NULL;
  }

int GetAvailableSpaceOnVolume( char* path, char* buf, int bufLen )
  {
  if( EMPTY( path ) )
    {
    Warning("GetAvailableSpaceOnVolume must be given a path");
    return -1;
    }

  if( buf==NULL || bufLen<10 )
    {
    Warning("No buffer passed to GetAvailableSpaceOnVolume");
    return -1;
    }

  buf[0] = 0;

  char cmd[BUFLEN];
  int err = ExpandMacrosVA( DF_COMMAND,
                          cmd, sizeof(cmd)-1,
                          "PATH", path,
                          NULL, NULL );
  if( err!=1 )
    {
    Warning( "Should have seen one expansion in [%s] - got %d",
             DF_COMMAND, err );
    return -2;
    }

  FILE* dfH = popen( cmd, "r" );
  if( dfH==NULL )
    {
    Warning( "Failed to popen(%s)", cmd );
    return -3;
    }

  size_t n = fread( buf, sizeof(char), bufLen, dfH );
  fclose( dfH );
  if( n>0 && n<bufLen-1 )
    {
    buf[n] = 0;
    TrimTail( buf );
    return 0;
    }

  Warning( "Reading from DF command returned invalid result - %d", n );
  return -4;
  }

long GetAvailableSpaceOnVolumeBytes( char* path )
  {
  char buf[BUFLEN];
  int n = GetAvailableSpaceOnVolume( path, buf, sizeof( buf ) );
  if( n<0 )
    {
    return n;
    }

  double space = 0;
  if( sscanf( buf, "%lf", &space )==1 )
    {
    return (long)space * (long)1024;
    }
  else
    {
    Warning("DF results [%s] do not look like %%lfG", buf );
    return -10;
    }
  }

/* Regular expression test.
 * 0 = match.
 */
int StringMatchesRegex( char* expr, char* str )
  {
  if( EMPTY( expr ) )
    {
    return 0;
    }

  if( EMPTY( str ) )
    {
    return 0;
    }

  regex_t re;
  int err = regcomp( &re, expr, REG_EXTENDED|REG_NOSUB );
  if( err!=0 )
    {
    Warning("Cannot compile e-mail RegExp: [%s] (%d)", expr, err);
    return -100 + err;
    }

  int status = regexec( &re, str, 0, NULL, 0 );
  regfree( &re );

  return status;
  }

char* ExtractRegexFromString( char* expr, char* str )
  {
  if( EMPTY( expr ) )
    {
    return NULL;
    }

  if( EMPTY( str ) )
    {
    return NULL;
    }

  regex_t re;
  int err = regcomp( &re, expr, REG_EXTENDED );
  if( err!=0 )
    {
    Warning("Cannot compile e-mail RegExp: [%s] (%d)", expr, err);
    return NULL;
    }

  regmatch_t matches[100];
  int status = regexec( &re, str, 1, matches, 0 );
  regfree( &re );
  if( status!=0 )
    {
    return NULL;
    }

  /* printf( "Match found at %d ~ %d\n",
             (int)(matches[0].rm_so), (int)(matches[0].rm_eo) );
  */
  int l = matches[0].rm_eo - matches[0].rm_so;
  char* buf = calloc( l+1, sizeof(char) );
  strncpy( buf, str+matches[0].rm_so, l );
  return buf;
  }

int StringToIp( IPADDR* dst, char* src )
  {
  if( EMPTY( src ) || dst==NULL )
    {
    return -1;
    }

  memset( dst, 0, sizeof(IPADDR) );

  int a=0;
  int b=0;
  int c=0;
  int d=0;
  int bits=0;

  if( strchr( src, '/' )==NULL )
    {
    int n = sscanf( src, "%d.%d.%d.%d", &a, &b, &c, &d );
    if( n!=4 )
      {
      return -2;
      }
    if( !ISBYTE( a ) || !ISBYTE( b ) || !ISBYTE( c ) || !ISBYTE( d ) )
      {
      return -3;
      }
    dst->bytes[0] = a;
    dst->bytes[1] = b;
    dst->bytes[2] = c;
    dst->bytes[3] = d;
    dst->bits = 0;

    return 0;
    }

  int n = sscanf( src, "%d.%d.%d.%d/%d", &a, &b, &c, &d, &bits );
  if( n!=5 )
    {
    return -4;
    }

  if( !ISBYTE( a ) || !ISBYTE( b ) || !ISBYTE( c ) || !ISBYTE( d ) )
    {
    return -3;
    }

  if( bits<8 || bits>32 )
    {
    return -5;
    }

  dst->bytes[0] = a;
  dst->bytes[1] = b;
  dst->bytes[2] = c;
  dst->bytes[3] = d;
  dst->bits = bits;

  return 0;
  }

void ExpandIP( int* array, IPADDR* addr )
  {
  array[0] =  (addr->bytes[0] & 0x80) ? 1 : 0;
  array[1] =  (addr->bytes[0] & 0x40) ? 1 : 0;
  array[2] =  (addr->bytes[0] & 0x20) ? 1 : 0;
  array[3] =  (addr->bytes[0] & 0x10) ? 1 : 0;
  array[4] =  (addr->bytes[0] & 0x08) ? 1 : 0;
  array[5] =  (addr->bytes[0] & 0x04) ? 1 : 0;
  array[6] =  (addr->bytes[0] & 0x02) ? 1 : 0;
  array[7] =  (addr->bytes[0] & 0x01) ? 1 : 0;

  array[8] =  (addr->bytes[1] & 0x80) ? 1 : 0;
  array[9] =  (addr->bytes[1] & 0x40) ? 1 : 0;
  array[10] = (addr->bytes[1] & 0x20) ? 1 : 0;
  array[11] = (addr->bytes[1] & 0x10) ? 1 : 0;
  array[12] = (addr->bytes[1] & 0x08) ? 1 : 0;
  array[13] = (addr->bytes[1] & 0x04) ? 1 : 0;
  array[14] = (addr->bytes[1] & 0x02) ? 1 : 0;
  array[15] = (addr->bytes[1] & 0x01) ? 1 : 0;

  array[16] = (addr->bytes[2] & 0x80) ? 1 : 0;
  array[17] = (addr->bytes[2] & 0x40) ? 1 : 0;
  array[18] = (addr->bytes[2] & 0x20) ? 1 : 0;
  array[19] = (addr->bytes[2] & 0x10) ? 1 : 0;
  array[20] = (addr->bytes[2] & 0x08) ? 1 : 0;
  array[21] = (addr->bytes[2] & 0x04) ? 1 : 0;
  array[22] = (addr->bytes[2] & 0x02) ? 1 : 0;
  array[23] = (addr->bytes[2] & 0x01) ? 1 : 0;

  array[24] = (addr->bytes[3] & 0x80) ? 1 : 0;
  array[25] = (addr->bytes[3] & 0x40) ? 1 : 0;
  array[26] = (addr->bytes[3] & 0x20) ? 1 : 0;
  array[27] = (addr->bytes[3] & 0x10) ? 1 : 0;
  array[28] = (addr->bytes[3] & 0x08) ? 1 : 0;
  array[29] = (addr->bytes[3] & 0x04) ? 1 : 0;
  array[30] = (addr->bytes[3] & 0x02) ? 1 : 0;
  array[31] = (addr->bytes[3] & 0x01) ? 1 : 0;
  }

int IPinSubnet( IPADDR* ip, IPADDR* subnet )
  {
  int bigIP[32];
  int bigSUBNET[32];

  ExpandIP( bigIP, ip );
  ExpandIP( bigSUBNET, subnet );

  for( int i=0; i<subnet->bits; ++i )
    {
    if( bigIP[i] != bigSUBNET[i] )
      {
      return -1;
      }
    }

  return 0;
  }

char* Encode( int nBytes, unsigned char* bytes )
  {
  if( nBytes<1 || nBytes>MAX_ENCODE || bytes==NULL )
    {
    Error("Cannot encode %d bytes at %p", nBytes, NULLPROTECT( bytes ) );
    }

  char* buf = (char*)calloc( 2*nBytes+1, sizeof(char) );
  char* ptr = buf;
  for( int i=0; i<nBytes; ++i )
    {
    int c = bytes[i];
    int high = (c & 0xf0)>>4;
    int low = (c & 0x0f);
    *(ptr++) = hexDigits[ high ];
    *(ptr++) = hexDigits[ low ];
    }
  *ptr = 0;
  return buf;
  }

unsigned char* Decode( char* string )
  {
  if( EMPTY( string ) )
    {
    return NULL;
    }

  int n = strlen( string );
  unsigned char* buf = (unsigned char*)calloc( n/2+1, sizeof(char*) );
  unsigned char* dst = buf;
  char* src = NULL;
  char* ptr = string;
  while( *ptr )
    {
    int high = *ptr;
    src = strchr( hexDigits, high );
    if( src==NULL )
      {
      Error("Cannot decode string with [%c] char (%s).", high, string );
      }
    high = (src - hexDigits);
    high = high << 4;

    ++ptr;

    int low = *ptr;
    src = strchr( hexDigits, low );
    if( src==NULL )
      {
      Error("Cannot decode string with [%c] char (%s).", low, string );
      }
    low = (src - hexDigits);

    ++ptr;

    int c = high | low;
    *(dst++) = c;
    }

  *dst = 0;

  return buf;
  }

int GetArgumentFromQueryString( char** bufPtr, char* keyword, char* regExp )
  {
  if( bufPtr==NULL )
    {
    return -1;
    }

  char* query = getenv("QUERY_STRING");
  if( EMPTY( query ) )
    {
    return -2;
    }

  char* ptr = strstr( query, keyword );
  if( EMPTY( ptr ) )
    {
    return -3;
    }

  ptr += strlen( keyword );
  if( EMPTY( ptr ) )
    {
    return -4;
    }

  if( *ptr!='=' )
    {
    return -5;
    }
  ++ptr;

  char* p = NULL;
  for( p = ptr; *p!=0 && *p!='&' && *p!='#'; ++p )
    {
    }
  int length = (p-ptr);
  if( length<1 )
    {
    return -6;
    }

  *bufPtr = calloc( length+10, sizeof(char) );
  strncpy( *bufPtr, ptr, length );

  if( NOTEMPTY( regExp ) )
    {
    if( StringMatchesRegex( regExp, *bufPtr )==0 )
      {
      return 0;
      }
    else
      {
      FREE( *bufPtr );
      *bufPtr = NULL;
      return -7;
      }
   }

  return 0;
  }

int ProcessExistsAndIsMine( pid_t p )
  {
  if( getpgid(p) < 0 )
    {
    /* often this is normal */
    Warning("getpgid(%ld) error -- %d:%s", (long)p, errno, strerror( errno ) );
    return -1; /* either does not exist or not my child */
    }

  if( kill( p, 0 ) != 0 )
    {
    Warning("kill(%ld) error -- %d:%s", (long)p, errno, strerror( errno ) );
    return -2; /* p does not seem to exist */
    }

  return 0;
  }

int FileCopy( const char* src, const char* dst )
  {
  if( EMPTY( src ) )
    {
    Warning("Cannot copy unspecified source file");
    return -1;
    }

  if( EMPTY( dst ) )
    {
    Warning("Cannot copy to unnamed destination file");
    return -2;
    }

  FILE* s = fopen( src, "r" );
  if( s==NULL )
    {
    Warning("Cannot read from [%s] for copy operation (%d:%s)", src, errno, strerror( errno )  );
    return -3;
    }

  FILE* d = fopen( dst, "w" );
  if( d==NULL )
    {
    Warning("Cannot write to [%s] for copy operation (%d:%s)", dst, errno, strerror( errno ) );
    fclose( s );
    return -4;
    }

  char data[BIGBUF];
  size_t n = 0;
  do
    {
    n = fread( data, sizeof(char), sizeof(data)-1, s );
    if( n>0 )
      {
      (void)fwrite( data, sizeof(char), n, d );
      }
    } while( n>0 );

  fclose( d );
  fclose( s );

  return 0;
  }

int FileCopy2( const char* srcFolder, const char* srcFile,
               const char* dstFolder, const char* dstFile )
  {
  char* srcPath = MakeFullPath( srcFolder, srcFile );
  char* dstPath = MakeFullPath( dstFolder, dstFile );
  int err = FileCopy( srcPath, dstPath );
  FREEIFNOTNULL( dstPath );
  FREEIFNOTNULL( srcPath );
  return err;
  }

long FileRead( const char* filename, unsigned char** data )
  {
  long size = FileSize( filename );

  if( size<0 )
    {
    return size;
    }

  if( data==NULL )
    {
    Warning("FileRead requires a target data pointer");
    return -3;
    }

  *data = (unsigned char*)malloc( size+1 );
  if( *data==NULL )
    {
    Warning("Cannot allocate %ld bytes in FileRead", size );
    return -4;
    }

  FILE* s = fopen( filename, "r" );
  if( s==NULL )
    {
    Warning("Cannot read from %s (%d:%s)", filename, errno, strerror( errno ) );
    return -5;
    }

  unsigned char* ptr = *data;
  long remaining = size;
  size_t n = 0;
  do
    {
    n = fread( ptr, sizeof(char), remaining, s );
    if( n>0 )
      {
      ptr += n;
      }
    } while( n>0 );

  fclose( s );

  if( ptr - (*data) != size )
    {
    Warning("Read %ld bytes but expected %ld bytes", ptr - (*data), size );
    return -6;
    }

  ptr = *data;
  *(ptr+size) = 0;

  return size;
  }

long FileRead2( const char* folder, const char* fileName,
                unsigned char** data )
  {
  char* fullPath = MakeFullPath( folder, fileName );
  long l = FileRead( fullPath, data );
  FREEIFNOTNULL( fullPath );
  return l;
  }

void EnsureDirExists( char* path )
  {
  if( EMPTY( path ) )
    {
    Error( "Cannot ensure that empty folder exists" );
    }

  if( DirExists( path )!=0 )
    {
    int err = mkdir( path, 0755 );
    if( err!=0 )
      {
      Error( "Failed to create folder [%s]", path );
      }
    }
  }

char* DateStr( time_t t, char* buf, int bufLen )
  {
  if( buf==NULL || bufLen<11 )
    Error( "DateStr called with too-small buffer" );
  struct tm *tmp = localtime( &t );
  snprintf( buf, bufLen, "%04d-%02d-%02d",
            tmp->tm_year+1900, tmp->tm_mon+1, tmp->tm_mday );
  return buf;
  }

char* DateNow( char* buf, int bufLen )
  {
  time_t now = time(NULL);
  return DateStr( now, buf, bufLen );
  }

char* TimeStr( time_t t, char* buf, int bufLen, int showSeconds )
  {
  if( buf==NULL || bufLen<9 )
    Error( "TimeStr called with too-small buffer" );
  struct tm *tmp = localtime( &t );
  if( showSeconds )
    {
    snprintf( buf, bufLen, "%02d:%02d:%02d",
              tmp->tm_hour, tmp->tm_min, tmp->tm_sec );
    }
  else
    {
    snprintf( buf, bufLen, "%02d:%02d",
              tmp->tm_hour, tmp->tm_min );
    }
  return buf;
  }

char* TimeNow( char* buf, int bufLen, int showSeconds )
  {
  time_t now = time(NULL);
  return TimeStr( now, buf, bufLen, showSeconds );
  }

char* DateTimeNow( char* buf, int bufLen, int showSeconds )
  {
  if( buf==NULL || bufLen<21 )
    Error( "DateTimeNow called with too-small buffer" );
  (void)DateNow( buf, 11 );
  buf[10] = ' ';
  (void)TimeNow( buf+11, 9, showSeconds );
  return buf;
  }

char* DateTimeStr( char* buf, int bufLen, int showSeconds, time_t t )
  {
  if( buf==NULL || bufLen<21 )
    Error( "DateTimeNow called with too-small buffer" );
  (void)DateStr( t, buf, 11 );
  buf[10] = ' ';
  (void)TimeStr( t, buf+11, 9, showSeconds );
  return buf;
  }

int ValidDate( char* when )
  {
  int cy, m, d;
  if( sscanf( when, "%d-%d-%d", &cy, &m, &d )!=3 )
    return -1;
  if( cy<1900 || cy>2040 )
    return -2;
  if( m<1 || m>12 )
    return -3;
  if( d<1 || d>31 )
    return -4;
  return 0;
  }

int ValidTime( char* when )
  {
  int h=0, m=0, s=0;
  if( sscanf( when, "%d:%d:%d", &h, &m, &s )!=3
      && sscanf( when, "%d:%d", &h, &m )!=2 )
    return -1;
  if( h<0 || h>23 )
    return -2;
  if( m<0 || m>59 )
    return -3;
  if( s<0 || s>59 )
    return -4;
  return 0;
  }

int CountFilesInFolder( char* folder, char* prefix, char* suffix,
                        time_t *earliest, time_t* latest )
  {
  int nEntries = 0;
  char* realFolder = NULL;

  if( EMPTY( folder ) )
    {
    realFolder = ".";
    }
  else
    {
    realFolder = folder;
    }

  DIR* d = opendir( realFolder );
  if( d==NULL )
    {
    Error( "Cannot open directory %s (%d:%s)", folder, errno, strerror( errno ) );
    }

  struct dirent * de;
  while( (de=readdir( d ))!=NULL )
    {
    if( NOTEMPTY( de->d_name )
        && ( suffix==NULL || StringEndsWith( de->d_name, suffix, 0 )==0 )
        && ( prefix==NULL || StringStartsWith( de->d_name, prefix, 0 )==0 ) )
      {
      ++nEntries;

      if( earliest!=NULL || latest!=NULL )
        {
        char fullPath[BUFLEN];
        char* realPath = NULL;
        if( folder==NULL )
          {
          realPath = de->d_name;
          }
        else
          {
          snprintf( fullPath, sizeof(fullPath)-1, "%s/%s", folder, de->d_name );
          realPath = fullPath;
          }

        struct stat sbuf;
        if( stat( realPath, &sbuf )==0 )
          {
          time_t t = sbuf.st_mtime;
          if( earliest!=NULL )
            {
            if( *earliest==0 || t<*earliest )
              {
              *earliest = t;
              }
            }
          if( latest!=NULL )
            {
            if( *latest==0 || t>*latest )
              {
              *latest = t;
              }
            }
          }
        }
      }
    }

  closedir( d );

  return nEntries;
  }

int ListToJSON( _TAG_VALUE* list, char* buf, int bufLen )
  {
  if( buf==NULL || bufLen<10 )
    {
    return -1;
    }

  char* ptr = buf;
  char* end = buf+bufLen-1;
  int inArray = 0;
  for( _TAG_VALUE* tv=list; tv!=NULL; tv=tv->next )
    {
    if( EMPTY( tv->tag ) )
      {
      inArray = 1;
      break;
      }
    }

  if( inArray )
    strcpy( ptr, "[ " );
  else
    strcpy( ptr, "{ " );

  ptr += strlen( ptr );

  int itemNum = 0;
  for( ; list!=NULL; list=list->next )
    {
    if( itemNum )
      {
      strncpy( ptr, ", ", end-ptr );
      ptr += strlen( ptr );
      }

    if( NOTEMPTY( list->tag ) )
      {
      snprintf( ptr, end-ptr, "\"%s\": ", list->tag );
      ptr += strlen( ptr );
      }

    if( list->subHeaders==NULL )
      { /* scalar value or empty list */
      switch( list->type )
        {
        case VT_INVALID:
          Warning( "Composing tags into JSON - [%s] has invalid type", NULLPROTECT( list->tag ) );
          break;

        case VT_STR:
          if( NOTEMPTY( list->value ) )
            {
            snprintf( ptr, end-ptr, "\"%s\"", list->value );
            ptr += strlen( ptr );
            }
          else
            {
            snprintf( ptr, end-ptr, "\"\"" );
            ptr += strlen( ptr );
            }
          break;

        case VT_INT:
          snprintf( ptr, end-ptr, "%d", list->iValue );
          ptr += strlen( ptr );
          break;

        case VT_DOUBLE:
          snprintf( ptr, end-ptr, "%lf", list->dValue );
          ptr += strlen( ptr );
          break;

        case VT_LIST:
          snprintf( ptr, end-ptr, "[ ]" );
          ptr += strlen( ptr );
          break;

        case VT_NULL:
          snprintf( ptr, end-ptr, "null" );
          ptr += strlen( ptr );
          break;
        }
      }
    else
      { /* vector value in subheaders */
      strncpy( ptr, "[", end-ptr );
      int err = ListToJSON( list->subHeaders, ptr, end-ptr );
      ptr += strlen( ptr );
      strncpy( ptr, "]", end-ptr );
      if( err )
        return err;
      }

    ++itemNum;
    }

  if( inArray )
    strncpy( ptr, " ]", end-ptr );
  else
    strncpy( ptr, " }", end-ptr );

  ptr += strlen( ptr );

  return 0;
  }

int NestedListToJSON( char* arrayName, _TAG_VALUE* list, char* buf, int bufLen )
  {
  if( buf==NULL || bufLen<10 )
    {
    return -1;
    }

  char* ptr = buf;
  char* end = buf+bufLen-1;

  snprintf( ptr, end-ptr, "\{\"%s\": [", NULLPROTECT(arrayName) );
  ptr += strlen( ptr );

  int itemNum = 0;
  for( ; list!=NULL; list=list->next )
    {
    if( itemNum )
      {
      strncpy( ptr, ", ", end-ptr );
      ptr += strlen( ptr );
      }

    (void)ListToJSON( list->subHeaders, ptr, end-ptr-1 );
    ptr += strlen( ptr );
    ++itemNum;
    }

  strcpy( ptr, " ]}" );
  ptr += strlen( ptr );

  return 0;
  }

int JSONToHTTPPost( char* relURL, char* json, char* buf, int bufLen )
  {
  if( buf==NULL || bufLen<10 )
    {
    Warning( "JSONToHTTPPost - NULL or short output buffer (aborting)" );
    return -1;
    }

  if( EMPTY( json ) )
    {
    Warning( "JSONToHTTPPost - NULL or empty JSON (continuing with empty string)" );
    buf[0] = 0;
    }

  char* ptr = buf;
  snprintf( ptr, bufLen, "POST %s HTTP/1.1\r\n", relURL );
  bufLen -= strlen( ptr );
  ptr += strlen( ptr );

  int l = 0;
  if( NOTEMPTY( json ) )
    l = strlen( json );

  snprintf( ptr, bufLen, "Content-Length: %d\r\n", l );
  bufLen -= strlen( ptr );
  ptr += strlen( ptr );

  snprintf( ptr, bufLen, "Content-Type: application/json\r\n\r\n" );
  bufLen -= strlen( ptr );
  ptr += strlen( ptr );

  snprintf( ptr, bufLen, "%s\r\n", NULLPROTECT(json) );

  return 0;
  }

long long TimeInMicroSeconds()
  {
  struct timeval te; 
  gettimeofday(&te, NULL); // get current time
  long long us = te.tv_sec*1000000LL + te.tv_usec;
  return us;
  }

char* TypeName( enum valueType t )
  {
  switch( t )
    {
    case VT_STR: return "STR";
    case VT_INT: return "INT";
    case VT_DOUBLE: return "DOUBLE";
    default: return "UNDEFINED";
    }
  }

int POpenAndRead( const char *cmd, int* readPtr, pid_t* childPtr )
  {
  int fd[2];
  int readFD = -1;
  int writeFD = -1;
  int pid = -1;

  /* Create a pipe to talk to the child */
  int err = pipe( fd );
  if( err<0 )
    Error( "Failed to pipe() - %d:%s", errno, strerror( errno ) );

  readFD = fd[0];
  writeFD = fd[1];

  /* Fork so we have parent/child processes */
  pid = fork();
  if( pid<0 )
    Error( "Failed to fork() - %d:%s", errno, strerror( errno ) );

  if( pid == 0 ) /* child */
    {
    NARGV* args = nargv_parse( cmd );
    if( args==NULL )
      Error( "Failed to parse cmd line [%s]", cmd );
    fflush( stdout );
    close( readFD );
    dup2( writeFD, 1 );
    close( writeFD );
    (void)execv( args->argv[0], args->argv );
    /* end of code */
    }
  else /* parent */
    {
    *childPtr = pid;
    close( writeFD );
    *readPtr = readFD;
    fcntl( readFD, F_SETFL, O_NONBLOCK );
    }

  return 0;
  }

int POpenAndSearch( const char *cmd, char* subString, char** result )
  {
  if( EMPTY( cmd ) || EMPTY( subString ) )
    return -1;

  int fileDesc = -1;
  pid_t child = -1;
  int err = POpenAndRead( cmd, &fileDesc, &child );
  if( err ) Error( "Cannot popen child [%s].", cmd );

  int flags = fcntl( fileDesc, F_GETFL);
  flags &= ~O_NONBLOCK;
  fcntl( fileDesc, F_SETFL, flags);

  FILE* f = fdopen( fileDesc, "r" );
  char buf[BUFLEN];
  while( fgets( buf, sizeof(buf)-1, f )==buf )
    {
    if( strstr( buf, subString )!=NULL )
      {
      fclose( f );
      if( result!=NULL )
        *result = strdup( buf );
      return 0;
      }
    }

  fclose( f );
  return -2;
  }

int POpenAndReadWrite( const char* cmd, int* readFD, int* writeFD, pid_t* child )
  {
  int err = 0;
  int input[2];
  int inputRead = -1;
  int inputWrite = -1;
  int output[2];
  int outputRead = -1;
  int outputWrite = -1;
  int pid = -1;

  /* Create pipes to talk to the child */
  err = pipe( input );
  if( err<0 )
    Error( "Failed to pipe() - %d:%s", errno, strerror( errno ) );

  inputRead = input[0];
  inputWrite = input[1];

  err = pipe( output );
  if( err<0 )
    Error( "Failed to pipe() - %d:%s", errno, strerror( errno ) );

  outputRead = output[0];
  outputWrite = output[1];

  /* Fork so we have parent/child processes */
  pid = fork();
  if( pid<0 )
    Error( "Failed to fork() - %d:%s", errno, strerror( errno ) );

  if( pid == 0 ) /* child */
    {
    NARGV* args = nargv_parse( cmd );
    if( args==NULL )
      Error( "Failed to parse cmd line [%s]", cmd );
    close( inputWrite );
    close( outputRead );
    dup2( inputRead, 0 );
    close( inputRead );
    dup2( outputWrite, 1 );
    dup2( outputWrite, 2 );
    close( outputWrite );
    (void)execv( args->argv[0], args->argv );
    /* end of code */
    }
  else /* parent */
    {
    *child = pid;
    close( inputRead );
    close( outputWrite );
    *readFD = outputRead;
    *writeFD = inputWrite;
    fcntl( outputRead, F_SETFL, O_NONBLOCK );
    }

  return 0;
  }

int POpenAndWrite( const char *cmd, int* writePtr, pid_t* childPtr )
  {
  int fd[2];
  int readFD = -1;
  int writeFD = -1;
  int pid = -1;

  /* Create a pipe to talk to the child */
  int err = pipe( fd );
  if( err<0 )
    Error( "Failed to pipe() - %d:%s", errno, strerror( errno ) );

  readFD = fd[0];
  writeFD = fd[1];

  /* Fork so we have parent/child processes */
  pid = fork();
  if( pid<0 )
    Error( "Failed to fork() - %d:%s", errno, strerror( errno ) );

  if( pid == 0 ) /* child */
    {
    NARGV* args = nargv_parse( cmd );
    if( args==NULL )
      Error( "Failed to parse cmd line [%s]", cmd );
    fflush( stdout );
    close( writeFD );
    dup2( readFD, 0 );
    close( readFD );
    close( 2 );
    (void)execv( args->argv[0], args->argv );
    /* end of code */
    }
  else /* parent */
    {
    *childPtr = pid;
    close( readFD );
    *writePtr = writeFD;
    fcntl( writeFD, F_SETFL, O_NONBLOCK );
    }

  return 0;
  }

int AsyncReadFromChildProcess( char* cmd,
                               int sleepSeconds,
                               void* params,
                               void (*GotLineCallback)( void*, char* ),
                               void (*TimeoutCallback)( void* )
                               )
  {
  int fileDesc = -1;
  pid_t child = -1;
  int err = POpenAndRead( cmd, &fileDesc, &child );
  if( err ) Error( "Cannot popen child." );

  char buf[BUFLEN];
  char* ptr = NULL;
  char* endPtr = buf + sizeof(buf) - 2;

  int retVal = 0;
  int exited = 0;

  for(;;)
    {
    int nBytesRead = 0;
    ptr = buf;
    *ptr = 0;

    for( ptr=buf; ptr<endPtr; ++ptr )
      {
      nBytesRead = read( fileDesc, ptr, 1 );
      if( nBytesRead<=0 )
        {
        *ptr = 0;
        break;
        }
      if( *ptr=='\n' )
        {
        ++ptr;
        *ptr = 0;
        break;
        }
      }

    if( nBytesRead>0 && buf[0]!=0 )
      {
      (*GotLineCallback)( params, buf );
      buf[0] = 0;
      }
/*
    else
      {
      printf("Read %d bytes starting with a %d byte\n", (int)(ptr-buf), (int)buf[0] );
      }
*/

    retVal = 0;
    int wstatus;
    if( waitpid( child, &wstatus, WNOHANG )==-1 )
      {
      Notice( "waitpid returned -1.\n");
      retVal = -1;
      break;
      }
    if( WIFEXITED( wstatus ) )
      {
      exited = 1;
      Notice( "child exited.\n");
      retVal = 0;
      break;
      }

    if( nBytesRead<=0 )
      {
      sleep( sleepSeconds );
      (*TimeoutCallback)( params );
      }
    }

  close( fileDesc );
  if( ! exited )
    {
    kill( child, SIGHUP );
    }

  return retVal;
  }

int ReadLineFromCommand( char* cmd, char* buf, int bufSize, int timeoutSeconds, int maxtimeSeconds )
  {
  int fileDesc = -1;
  pid_t child = -1;

  int err = POpenAndRead( cmd, &fileDesc, &child );
  if( err ) Error( "Cannot popen child to run [%s].", cmd );

  char* ptr = buf;
  char* endPtr = buf + bufSize - 2;

  int retVal = 0;
  time_t tStart = time(NULL);
  int exited = 0;

  for(;;)
    {
    if( (int)(time(NULL) - tStart) >= maxtimeSeconds )
      {
      retVal = -3;
      break;
      }

    fd_set readSet;
    fd_set exceptionSet;
    struct timeval timeout;

    FD_ZERO( &readSet );
    FD_SET( fileDesc, &readSet );
    FD_ZERO( &exceptionSet );
    FD_SET( fileDesc, &exceptionSet );
    timeout.tv_sec = timeoutSeconds;
    timeout.tv_usec = 0;
    
    int result = select( fileDesc+1, &readSet, NULL, &exceptionSet, &timeout );
    if( result>0 )
      {
      int nBytes = read( fileDesc, ptr, endPtr-ptr );
      if( nBytes>0 )
        {
        ptr += nBytes;
        *ptr = 0;
        if( strchr( buf, '\n' )!=NULL )
          {
          break;
          }
        }
      }

    int wStatus;
    if( waitpid( child, &wStatus, WNOHANG )==-1 )
      {
      Notice( "waitpid returned -1 (error.  errno=%d/%s).\n", errno, strerror( errno ));
      retVal = -1;
      break;
      }

    if( WIFEXITED( wStatus ) )
      {
      exited = 1;
      Notice( "child exited.\n");
      retVal = 0;
      break;
      }
    }

  close( fileDesc );

  if( ! exited )
    {
    kill( child, SIGHUP );
    }

  return retVal;
  }

int ReadLinesFromCommand( char* cmd, char** bufs, int nBufs, int bufSize, int timeoutSeconds, int maxtimeSeconds )
  {
  int fileDesc = -1;
  pid_t child = -1;

  for( int i=0; i<nBufs; ++i )
    *(bufs[i]) = 0;

  int err = POpenAndRead( cmd, &fileDesc, &child );
  if( err ) Error( "Cannot popen child to run [%s].", cmd );

  int lineNo = 0;
  char* ptr = bufs[lineNo];
  char* endPtr = ptr + bufSize - 2;

  int retVal = 0;
  time_t tStart = time(NULL);

  int exited = 0;

  while( fileDesc > 0 )
    {
    if( (int)(time(NULL) - tStart) >= maxtimeSeconds )
      {
      retVal = -3;
      break;
      }

    fd_set readSet;
    fd_set exceptionSet;
    struct timeval timeout;

    FD_ZERO( &readSet );
    FD_SET( fileDesc, &readSet );
    FD_ZERO( &exceptionSet );
    FD_SET( fileDesc, &exceptionSet );
    timeout.tv_sec = timeoutSeconds;
    timeout.tv_usec = 0;
    
    int result = select( fileDesc+1, &readSet, NULL, &exceptionSet, &timeout );
    if( result>0 )
      {
      char tinyBuf[2];
      tinyBuf[0] = 0;
      int n = 0;
      while( ptr < endPtr && (n=read( fileDesc, tinyBuf, 1 ))==1 )
        {
        int c = tinyBuf[0];
        if( c=='\n' )
          {
          ++lineNo;
          if( lineNo >= nBufs )
            {
            close( fileDesc );
            fileDesc = -1;
            break;
            }
          ptr = bufs[lineNo];
          endPtr = ptr + bufSize - 2;
          }
        else
          {
          *ptr = c;
          ++ptr;
          *ptr = 0;
          }
        }
      }

    int wStatus;
    if( waitpid( child, &wStatus, WNOHANG )==-1 )
      {
      Notice( "waitpid returned -1 (error.  errno=%d/%s).\n", errno, strerror( errno ));
      retVal = -1;
      break;
      }

    if( WIFEXITED( wStatus ) )
      {
      exited = 1;
      Notice( "child exited.\n");
      retVal = 0;
      break;
      }
    }

  if( fileDesc>0 )
    {
    close( fileDesc );
    }

  if( exited==0 )
    {
    kill( child, SIGHUP );
    }

  return retVal;
  }

int WriteReadLineToFromCommand( char* cmd, char* stdinLine, char* buf, int bufSize, int timeoutSeconds, int maxtimeSeconds )
  {
  int readFD = -1;
  int writeFD = -1;
  pid_t child = -1;

  int err = POpenAndReadWrite( cmd, &readFD, &writeFD, &child );
  if( err ) Error( "Cannot popen child to run [%s].", cmd );

  int l = strlen(stdinLine);
  int nBytes = write( writeFD, stdinLine, l );
  if( nBytes>0 )
    {
    if( nBytes!=l )
      Warning( "Tried to write %d bytes but only managed %d", l, nBytes );
    }
  close( writeFD );

  char* ptr = buf;
  char* endPtr = buf + bufSize - 2;

  int retVal = 0;
  time_t tStart = time(NULL);
  int exited = 0;

  for(;;)
    {
    if( (int)(time(NULL) - tStart) >= maxtimeSeconds )
      {
      retVal = -3;
      break;
      }

    fd_set readSet;
    fd_set exceptionSet;
    struct timeval timeout;

    FD_ZERO( &readSet );
    FD_SET( readFD, &readSet );
    FD_ZERO( &exceptionSet );
    FD_SET( readFD, &exceptionSet );
    timeout.tv_sec = timeoutSeconds;
    timeout.tv_usec = 0;
    
    int result = select( readFD+1, &readSet, NULL, &exceptionSet, &timeout );
    if( result>0 )
      {
      int nBytes = read( readFD, ptr, endPtr-ptr );
      if( nBytes>0 )
        {
        ptr += nBytes;
        *ptr = 0;
        if( strchr( buf, '\n' )!=NULL )
          {
          break;
          }
        }
      }

    int wStatus;
    if( waitpid( child, &wStatus, WNOHANG )==-1 )
      {
      Notice( "waitpid returned -1 (error.  errno=%d/%s).\n", errno, strerror( errno ));
      retVal = -1;
      break;
      }

    if( WIFEXITED( wStatus ) )
      {
      exited = 1;
      Notice( "child exited.\n");
      retVal = 0;
      break;
      }
    }

  close( readFD );
  if( ! exited )
    {
    kill( child, SIGHUP );
    }

  return retVal;
  }

int WriteLineToCommand( char* cmd, char* stdinLine, int timeoutSeconds, int maxtimeSeconds )
  {
  int fileDesc = -1;
  pid_t child = -1;

  int err = POpenAndWrite( cmd, &fileDesc, &child );
  if( err ) Error( "Cannot popen child to run [%s].", cmd );

  int retVal = 0;
  time_t tStart = time(NULL);
  while( *stdinLine != 0 )
    {
    if( (int)(time(NULL) - tStart) >= maxtimeSeconds )
      {
      retVal = -3;
      break;
      }

    fd_set writeSet;
    fd_set exceptionSet;
    struct timeval timeout;

    FD_ZERO( &writeSet );
    FD_SET( fileDesc, &writeSet );
    FD_ZERO( &exceptionSet );
    FD_SET( fileDesc, &exceptionSet );
    timeout.tv_sec = timeoutSeconds;
    timeout.tv_usec = 0;
    
    int result = select( fileDesc+1, NULL, &writeSet, &exceptionSet, &timeout );
    if( result>0 )
      {
      int l = strlen( stdinLine );
      int nBytes = write( fileDesc, stdinLine, l );
      if( nBytes>0 )
        {
        if( nBytes!=l )
          Warning( "Tried to write %d bytes but only managed %d", l, nBytes );
        stdinLine += nBytes;
        }
      }
    else
      {
      Warning( "Timeout waiting for child - %d:%d:%s", err, errno, strerror( errno ) );
      }
    }

  close( fileDesc );

  int wStatus;
  if( waitpid( child, &wStatus, WNOHANG )==-1 )
    {
    Notice( "waitpid returned -1 (error.  errno=%d/%s).\n", errno, strerror( errno ));
    retVal = -1;
    }

  if( WIFEXITED( wStatus ) )
    {
    Notice( "child exited.\n");
    retVal = 0;
    }

  if( retVal != 0 )
    {
    kill( child, SIGHUP );
    }

  return retVal;
  }

/* 0 return only indicates that child forked successfully */
int AsyncRunCommandNoIO( char* cmd )
  {
  if( EMPTY( cmd ) )
    return -1;

  NARGV* args = nargv_parse( cmd );
  if( args==NULL )
    Error( "Failed to parse cmd line [%s]", cmd );

  pid_t pid = fork();
  if( pid<0 )
    Error( "Failed to fork() - %d:%s", errno, strerror( errno ) );

  if( pid == 0 ) /* child */
    {
    close( 0 );
    close( 1 );
    close( 2 );
    (void)execv( args->argv[0], args->argv );
    /* end of code */
    }

  nargv_free( args );

  return 0;
  }

void SignalHandler( int signo )
  {
  if( signo == SIGHUP )
    {
    syslog( LOG_NOTICE, "Received SIGHUP - exiting" );
    exit( 0 );
    }
  else if( signo == SIGUSR1 )
    {
    syslog( LOG_NOTICE, "Received SIGUSR1 - closing handle" );
    void EmergencyCloseHandles();
    }
  }

void GrabEndOfFile( FILE* input, char* output, int outputLen )
  {
  char* endPtr = output + outputLen - 1;
  char* ptr = output;
  char buf[BUFLEN];
  while( fgets( buf, sizeof(buf)-1, input )==buf )
    {
    strncpy( ptr, buf, endPtr-ptr );
    ptr += strlen( buf );
    *ptr = 0;
    }
  }

void TailFile( FILE* input, int nLines, char* output, int outputLen )
  {
  if( input==NULL || nLines<1 || output==NULL || outputLen<2 )
    Error( "Abuse of TailFile" );

  fseek( input, 0, SEEK_END );
  off_t pos = ftell( input );
  int gotLines = 0;
  int c = -1;

  while( pos>=0 )
    {
    if( pos==0 )
      {
      ++gotLines;
      break;
      }

    fseek( input, pos, SEEK_SET );
    c = fgetc( input );

    if( c=='\n' )
      ++gotLines;

    if( gotLines>nLines )
      break;

    --pos;
    }

  if( pos<=0 )
    {
    pos = 0;
    fseek( input, pos, SEEK_SET );
    }

  GrabEndOfFile( input, output, outputLen );
  }

int IsRecent( time_t when, int maxAge )
  {
  int nSeconds = (int)(time(NULL) - when);
  nSeconds = abs( nSeconds );
  if( nSeconds < maxAge )
    return 1;
  return 0;
  }

char* GenerateAllocateGUID()
  {
  char buf[100];

  uuid_t x;
  uuid_generate( x );
  uuid_unparse_lower( x, buf );

  return strdup( buf );
  }

/*
  Time strings converted to time_t.
  Either relative to "now" or absolute time/date.
  Formats as follows:
  +20m
  +2h15m
  2021-07-02 19:00:00
*/
time_t ParseTimeString( char* str )
  {
  if( EMPTY( str ) )
    return 0;

  time_t tNow = time(NULL);

  int Y=0;
  int M=0;
  int D=0;
  int h=0;
  int m=0;
  int s=0;

  if( str[0]=='+' )
    {
    if( StringMatchesRegex( "[0-9]+h[0-9]+m", str+1 )==0
        && sscanf( str+1, "%dh%dm", &h, &m )==2 )
      return tNow + h*60*60 + m*60;
    else if( StringMatchesRegex( "[0-9]+h", str+1 )==0
        && sscanf( str+1, "%dh", &h )==1 )
      return tNow + h*60*60;
    else if( StringMatchesRegex( "[0-9]+m", str+1 )==0
        && sscanf( str+1, "%dm", &m )==1 )
      return tNow + m*60;
    else if( StringMatchesRegex( "[0-9]+s", str+1 )==0
        && sscanf( str+1, "%ds", &s )==1 )
      return tNow + s;
    else if( StringMatchesRegex( "[0-9]+m[0-9]+s", str+1 )==0
        && sscanf( str+1, "%dm%ds", &m, &s )==2 )
      return tNow + m*60 + s;
    else if( StringMatchesRegex( "[0-9]+h[0-9]+m[0-9]+s", str+1 )==0
        && sscanf( str+1, "%dh%dm%ds", &h, &m, &s )==3 )
      return tNow + h*60*60 + m*60 + s;
    else
      return -1;
    }

  char* sp = strchr( str, ' ' );
  if( sp==NULL )
    return -1;

  if( ValidDate( str )==0
      && ValidTime( sp+1 )==0
      && sscanf( str, "%d-%d-%d", &Y, &M, &D )==3
      && ( sscanf( sp+1, "%d:%d:%d", &h, &m, &s )==3
           || sscanf( sp+1, "%d:%d", &h, &m )==2 ) )
    {
    struct tm *tmPtr = localtime( &tNow );
    struct tm tStr;
    memset( &tStr, 0, sizeof( tStr ) );
    tStr.tm_year = Y - 1900;
    tStr.tm_mon = M - 1;
    tStr.tm_mday = D;
    tStr.tm_hour = h;
    tStr.tm_min = m;
    tStr.tm_sec = s;
    tStr.tm_isdst = tmPtr->tm_isdst;
    return mktime( &tStr );
    }

  return -1;
  }

int LockFile( char* fileName )
  {
  int fd = open( fileName, O_CREAT | O_WRONLY, S_IRWXU );

  struct flock fl;
  fl.l_type   = F_WRLCK;  /* F_RDLCK, F_WRLCK, F_UNLCK    */
  fl.l_whence = SEEK_SET; /* SEEK_SET, SEEK_CUR, SEEK_END */
  fl.l_start  = 0;        /* Offset from l_whence         */
  fl.l_len    = 1;        /* length, 0 = to EOF           */
  fl.l_pid    = getpid(); /* our PID                      */

  int err = fcntl(fd, F_SETLKW, &fl);  /* set the lock, waiting if necessary */

  if( err )
    {
    printf( "Lockf error - %d / %d / %s\n", err, errno, strerror( errno ) );fflush(stdout);
    close( fd );
    return -1;
    }

  return fd;
  }

int UnLockFile( int fd )
  {
  if( fd<=0 )
    return -1;

  struct flock fl;
  fl.l_type   = F_UNLCK;
  fl.l_whence = SEEK_SET;
  fl.l_start  = 0;
  fl.l_len    = 1;
  fl.l_pid    = getpid();

  int err1 = fcntl(fd, F_SETLKW, &fl);

  int err2 = close( fd );

  if( err1 )
    return err1;

  if( err2 )
    return err2;

  return 0;
  }

int Touch( char* path )
  {
  if( EMPTY( path ) )
    Error( "Cannot Touch(NULL)" );

  FILE* f = fopen( path, "a" );
  if( f!=NULL )
    {
    fclose( f );
    return 0;
    }
  else
    Warning( "Failed to fopen(%s,a)", path );

  return -1;
  }

uid_t GetUID( const char* logName )
  {
  if( EMPTY( logName ) )
    Error( "GetUID(NULL)" );
  struct passwd* p = getpwnam( logName );
  if( p==NULL )
    Error( "GetUID(%s) -> invalid user", logName );
  return p->pw_uid;
  }

gid_t GetGID( const char* groupName )
  {
  if( EMPTY( groupName ) )
    Error( "GetjID(NULL)" );
  struct group* g = getgrnam( groupName );
  if( g==NULL )
    Error( "GetGID(%s) -> invalid group", groupName );
  return g->gr_gid;
  }

int TouchEx( const char* folder, const char* fileName, const char* user, const char* group, mode_t mode )
  {
  if( EMPTY( folder )
      || EMPTY( fileName )
      || EMPTY( user )
      || EMPTY( group )
      || mode==0 )
    Error( "TouchEx(...NULL...)" );

  int err = 0;
  char* fullPath = MakeFullPath( folder, fileName );
  err = FileExists( fullPath );
  if( err!=0 )
    { /* create it */
    err = Touch( fullPath );
    if( err!=0 )
      { /* failed */
      return err;
      }
    }

  /* should exist now */
  uid_t userNum = GetUID( user );
  gid_t groupNum = GetGID( group );
  err = chown( fullPath, userNum, groupNum );
  if( err )
    {
    Warning( "chown( %s, %d, %d ) -> %d:%d:%s",
           fullPath, (int)userNum, (int)groupNum, err, errno, strerror( errno ) );
    return err;
    }

  err = chmod( fullPath, mode );
  if( err )
    {
    Warning( "chmod( %s, %d ) -> %d:%d:%s",
           fullPath, (int)mode, err, errno, strerror( errno ) );
    return err;
    }

  return 0;
  }

char* AggregateMessages( _TAG_VALUE* messages )
  {
  int length = 0;
  int n = 0;
  for( _TAG_VALUE* tv = messages; tv!=NULL; tv=tv->next )
    {
    if( NOTEMPTY( tv->value ) )
      {
      ++n;
      length += strlen( tv->value );
      }
    }

  if( n==0 || length==0 )
    return NULL;

  char* buf = (char*)calloc( length + n*5, sizeof(char) );
  char* ptr = buf;

  for( _TAG_VALUE* tv = messages; tv!=NULL; tv=tv->next )
    {
    if( NOTEMPTY( tv->value ) )
      {
      strcpy( ptr, tv->value );
      ptr += strlen( tv->value );
      strcpy( ptr, ". " );
      ptr += strlen( tv->value );
      }
    }

  return buf;
  }

#define GETAPACHEUSER\
  "/bin/bash -c 'cat /etc/apache2/* 2>/dev/null | grep APACHE_RUN_USER= | sed \"s/.*=//\"'"

char* GetWebUser()
  {
  char buf[BUFLEN];
  int err = ReadLineFromCommand( GETAPACHEUSER, buf, sizeof(buf)-1, 1, 5 );
  if( err )
    Error( "Failed to find apache2 username from cmd [%s] -- %d:%d:%s",
           GETAPACHEUSER, err, errno, strerror( errno ) );
  return strdup( strtok( buf, "\r\n" ) );
  }

#define GETAPACHEGROUP\
  "/bin/bash -c 'cat /etc/apache2/* 2>/dev/null | grep APACHE_RUN_GROUP= | sed \"s/.*=//\"'"

char* GetWebGroup()
  {
  char buf[BUFLEN];
  int err = ReadLineFromCommand( GETAPACHEGROUP, buf, sizeof(buf)-1, 1, 5 );
  if( err )
    Error( "Failed to find apache2 groupname from cmd [%s] -- %d:%d:%s",
           GETAPACHEGROUP, err, errno, strerror( errno ) );
  return strdup( strtok( buf, "\r\n" ) );
  }

int RotateFile( char* path )
  {
  char friendlyTime[BUFLEN];
  (void)DateTimeStr( friendlyTime, sizeof( friendlyTime )-1, 0, time(NULL) );
  int l = strlen( path ) + strlen( friendlyTime ) + 10;
  char* newName = (char*)SafeCalloc( l, sizeof(char), "path for file rotation" );
  strcpy( newName, path );
  strcat( newName, "-" );
  strcat( newName, friendlyTime );
  int err = rename( path, newName );
  if( err )
    Warning( "Failed to rename [%s] to [%s] - %d:%d:%s",
             path, newName, err, errno, strerror( errno ) );
  FREE( newName );
  return err;
  }

void KillExistingCommandInstances( char* commandLine )
  {
  if( EMPTY( commandLine ) )
    return;

  Notice( "KillExistingCommandInstances( %s )", commandLine );
  char* procLine = NULL;
  int nTries = 10;
  while( POpenAndSearch( "/bin/ps -efww", commandLine, &procLine )==0
         && nTries-- )
    {
    Notice( "Command [%s] already running - will try to stop it", commandLine );
    if( NOTEMPTY( procLine ) )
      {
      char* ptr = NULL;
      char* userID = strtok_r( procLine, " \t\r\n", &ptr );
      char* processID = strtok_r( NULL, " \t\r\n", &ptr );
      long pidNum = -1;
      if( NOTEMPTY( processID ) )
        pidNum = atol( processID );

      Notice( "Killing process %ld which belongs to %s", pidNum, userID );
      if( pidNum>0 )
        {
        int err = kill( (pid_t)pidNum, SIGHUP );
        if( err )
          {
          Warning( "Failed to send HUP signal to process %d: %d:%d:%s",
                   pidNum, err, errno, strerror( errno ) );
          break;
          }
        else
          sleep(1); /* might take a while to stop it */
        }
      FREE( procLine );
      }
    }
  }
