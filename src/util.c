#include "base.h"

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
  /* redundant - stat will fail.
  if( FileExists( path )!=0 )
    {
    return -1;
    }
  */

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
        t->value = strdup( value );
        return list;
        }
      }
    }

  /* nope - it's new.  create a new item in the list */
  _TAG_VALUE* n = (_TAG_VALUE*)SafeCalloc(1, sizeof( _TAG_VALUE ), "_TAG_VALUE" );
  n->tag = strdup( tag );
  n->value = strdup( value );
  n->subHeaders = NULL;
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
        return list;
        }
      }
    }

  /* nope - it's new.  create a new item in the list */
  _TAG_VALUE* n = (_TAG_VALUE*)SafeCalloc(1, sizeof( _TAG_VALUE ), "_TAG_VALUE" );
  n->tag = strdup( tag );
  n->iValue = value;
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
        return list;
        }
      }
    }

  /* nope - it's new.  create a new item in the list */
  _TAG_VALUE* n = (_TAG_VALUE*)SafeCalloc(1, sizeof( _TAG_VALUE ), "_TAG_VALUE" );
  n->tag = strdup( tag );
  n->subHeaders = value;
  n->next = list;
  return n;
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
    if( list->tag!=NULL && *list->tag!=0 && strcmp(list->tag,tagName)==0 )
      {
      retVal = list->value;
      }
    list = list->next;
    }

  return retVal;
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
    list->tag = NULL;
    }
  if( list->value!=NULL )
    {
    FREE( list->value );
    list->value = NULL;
    }
  FREE( list );
  }

/* PrintTagValue()
 * Used for debugging - see what's in the linked-list struct.
 */
void PrintTagValue( char* prefix, _TAG_VALUE* list )
  {
  for( int i=0; list!=NULL; list=list->next, ++i )
    {
    printf( "%s  %02d. tag=%s, value=%s\n",
            prefix, i, NULLPROTECT( list->tag ), NULLPROTECT( list->value) );
    PrintTagValue( "  ", list->subHeaders );
    }
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

void LowerCase( char* dst, int dstSize, char* src )
  {
  if( dst==NULL )
    return;

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
  (void)pclose( dfH );
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

char* Encode( int nBytes, char* bytes )
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

char* Decode( char* string )
  {
  if( EMPTY( string ) )
    {
    return NULL;
    }

  int n = strlen( string );
  char* buf = (char*)calloc( n/2+1, sizeof(char*) );
  char* dst = buf;
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
  buf[10] = 0;
  (void)TimeNow( buf+11, 9, showSeconds );
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
  if( sscanf( when, "%d-%d-%d", &h, &m, &s )!=3
      && sscanf( when, "%d-%d", &h, &m )!=2 )
    return -1;
  if( h<0 || h>23 )
    return -2;
  if( m<0 || m>59 )
    return -3;
  if( s<0 || s>59 )
    return -4;
  return 0;
  }

char* GUID()
  {
  FILE* h = popen( "/usr/bin/uuidgen", "r" );
  if( h==NULL )
    {
    Error( "Cannot launch /usr/bin/uuidgen (%d:%s)", errno, strerror( errno ) );
    }
  char buf[BUFLEN];
  if( fgets( buf, sizeof(buf)-1, h )!=buf )
    {
    Error( "Cannot read from /usr/bin/uuidgen (%d:%s)", errno, strerror( errno ) );
    }
  (void)pclose( h );
  return strdup( StripEOL( buf ) );
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
    if( NOTEMPTY( list->tag )
        && NOTEMPTY( list->value ) )
      {
      snprintf( ptr, end-ptr, "\"%s\": \"%s\"", list->tag, list->value );
      ptr += strlen( ptr );
      ++itemNum;
      }
    else if( NOTEMPTY( list->tag ) )
      {
      snprintf( ptr, end-ptr, "\"%s\": %d", list->tag, list->iValue );
      ptr += strlen( ptr );
      ++itemNum;
      }
    }

  strcpy( ptr, " }" );
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

long long TimeInMicroSeconds()
  {
  struct timeval te; 
  gettimeofday(&te, NULL); // get current time
  long long us = te.tv_sec*1000000LL + te.tv_usec;
  return us;
  }

/* be sure we are in the correct directory first! */
void SwapInProcess( int debugStderr, int debugStdout, char* nick, char* commandLine )
  {
  if( EMPTY( commandLine ) )
    {
    Error( "Cannot run empty command line for camera %s", NULLPROTECT(nick) );
    }

  NARGV* args = nargv_parse( commandLine );
  if( args==NULL )
    {
    Error( "Failed to parse cmd line [%s] for [%s]",
           commandLine, NULLPROTECT(nick) );
    }

  /* printf("Trying to exec [%s] with %d args\n", args->argv[0], args->argc ); */
  fclose( stdin );
  if( freopen( debugStderr ? "stderr.log" : "/dev/null", "w", stderr )==NULL )
    {
    Warning("Failed to redirect stderr");
    }
  if( freopen( debugStdout ? "stdout.log" : "/dev/null", "w", stdout )==NULL )
    {
    Warning("Failed to redirect stdout");
    }
  (void)execv( args->argv[0], args->argv );
  Error( "execv on [%s] returned error %d - %s",
         commandLine, errno, strerror( errno ) );
  }

int RunCommand( int debugStderr, int debugStdout, char* commandLine )
  {
  pid_t child = fork();

  if( child<0 )
    {
    Warning("Failed to fork a child process" );
    return -1;
    }

  if( child==0 ) /* in child */
    {
    SwapInProcess( debugStderr, debugStdout, NULL, commandLine );

    /* should never reach this */
    }

  int status;
  for(;;)
    {
    if( waitpid( child, &status, 0)==-1 )
      {
      Warning( "waitpid errored out" );
      return -1;
      }
    if( WIFEXITED( status ) )
      {
      return WEXITSTATUS(status);
      }
    }
  }

#define READ_END 0
#define WRITE_END 1

/* either works or exits process with error msg
   - does not return with an error code */
void RunCommandWithManyFilesOnStdin( char* commandLine, char* pathWithFilenames )
  {
  if( EMPTY( commandLine ) )
    {
    Error( "Cannot run empty command line with multi-file stdin" );
    }

  if( EMPTY( pathWithFilenames ) )
    {
    Error( "Cannot run command without specifying file specifying inputs" );
    }

  if( FileExists( pathWithFilenames )!=0 )
    {
    Error( "File %s does not exist - cannot run command", pathWithFilenames );
    }

  int fd[2];
  if( pipe(fd)!=0 )
    {
    Error( "Failed to create a pipe - %d:%s", errno, strerror( errno ) );
    }

  pid_t childProcess = fork();
  if( childProcess<0 )
    {
    Error( "Failed to fork - %d:%s", errno, strerror( errno ) );
    }

  if( childProcess==0 )
    { /* child */
    dup2( fd[READ_END], STDIN_FILENO );
    close( fd[READ_END] ); /* this handle is redundant - use STDIN */
    close( fd[WRITE_END] ); /* only the parent needs this side */

    NARGV* args = nargv_parse( commandLine );
    if( args==NULL )
      {
      Error( "Failed to parse cmd line [%s]", commandLine );
      }

    (void)execv( args->argv[0], args->argv );
    Error( "execv on [%s] returned error %d - %s",
           commandLine, errno, strerror( errno ) );
    }
  else
    { /* parent */
    close( fd[READ_END] ); /* only the child needs this side */

    /* read a list of filenames from the master file: */
    FILE* listFile = fopen( pathWithFilenames, "r" );
    if( listFile==NULL )
      {
      Error( "Cannot open %s for reading", pathWithFilenames );
      }

    char buf[BUFLEN];
    while( fgets( buf, sizeof(buf)-1, listFile )==buf )
      {
      /* for each file, read its data into memory: */
      char* dataFileName = StripEOL( buf );
      unsigned char* dataInFile = NULL;
      long nBytes = FileRead( dataFileName, &dataInFile );

      /* write the data we just read into the pipe */
      unsigned char* ptr = dataInFile;
      while( nBytes>0 )
        {
        long n = write( fd[WRITE_END], ptr, nBytes );
        nBytes -= n;
        ptr += n;
        }
      free( dataInFile );
      nBytes = 0;
      }
    fclose( listFile );

    /* all done feeding data to the child - now wait for it to exit*/
    close( fd[WRITE_END] );
    for(;;)
      {
      int childExitStatus = 0;
      waitpid( childProcess, &childExitStatus, 0);
      if( WIFEXITED(childExitStatus) )
        {
        break;
        }
      }
    }
  }

