#include "base.h"

/*
#define DEBUG 1
*/

FILE* debugOutput = NULL;

extern int inCGI;
int gotHeader = 0;

void CGIHeader( char* contentType,
                long contentLength,
                char* pageTitle,
                int nCSS, char** css,
                int nJS, char** js
                )
  {
  if( gotHeader )
    return;
  gotHeader = 1;

  inCGI = 1;

  if( NOTEMPTY( contentType ) )
    {
    printf( "Content-Type: %s\r\n", contentType );
    if( contentLength>0 )
      {
      printf( "Content-Length: %ld\r\n", contentLength );
      }
    fputs( "\r\n", stdout );

    /* Not HTML.  The stuff below is HTML specific. */
    return;
    }

  fputs( "Content-Type: text/html; Charset=US-ASCII\r\n\r\n", stdout );

  fputs( "<!doctype html>\n", stdout);
  fputs( "<html lang=\"en\">\n", stdout);
  fputs( "  <head>\n", stdout);
  if( NOTEMPTY( pageTitle ) )
    {
    printf( "    <title>%s</title>\n", pageTitle );
    }

  if( css!=NULL && nCSS>0 && nCSS<100 )
    {
    for( int i=0; i<nCSS; i++ )
      {
      char* styleSheet = css[i];
      if( NOTEMPTY( styleSheet ) )
        {
        printf( "    <link rel=\"stylesheet\" href=\"%s\"/>\n", styleSheet );
        }
      }
    }

  if( js!=NULL && nJS>0 && nJS<100 )
    {
    for( int i=0; i<nJS; i++ )
      {
      char* jsFile = js[i];
      if( NOTEMPTY( jsFile ) )
        {
        printf( "    <script src=\"%s\"></script>\n", jsFile );
        }
      }
    }

  fputs( "  </head>\n", stdout);
  fputs( "  <body>\n", stdout);
  }

void CGIFooter()
  {
  fputs( "  </body>\n", stdout);
  fputs( "</html>\n", stdout);
  }

void DumpJPEGToBrowser( char* nickName, long nBytes, char* fileName )
  {
  FILE* f = fopen( fileName, "r" );
  if( f==NULL )
    {
    CGIHeader( NULL, 0, NULL, 0, NULL, 0, NULL);
    Error("Cannot open file %s in camera %s", fileName, nickName );
    }
  CGIHeader( "image/jpeg", nBytes, NULL, 0, NULL, 0, NULL );
  int c = -1;
  int nChars = 0;
  while( (c=fgetc(f))!=EOF )
    {
    fputc( c, stdout );
    ++nChars;
    }
  fclose( f );
  if( nBytes != nChars )
    {
    Warning( "Expected to send %d bytes (%s/%s) - but actually sent %d",
             nBytes, nickName, fileName, nChars );
    }
  }

#ifdef DEBUG
/* myfgets()
 * wrapper to fgets used for debugging.  do not use in production code.
 */
char* myfgets( char* buf, int buflen, FILE* f )
  {
  char* s = fgets( buf, buflen, f );
  if( s==NULL )
    {
    /* Notice("fgets returned NULL"); */
    }
  else
    {
    if( debugOutput!=NULL )
      {
      fputs( buf, debugOutput );
      }
    }

  return s;
  }

/* myfgetc()
 * wrapper to fgetc used for debugging.  do not use in production code.
 */
int myfgetc( FILE* f )
  {
  int c = fgetc( f );
  if( debugOutput!=NULL )
    {
    fputc( c, debugOutput );
    }

  return c;
  }

size_t myfread( void *ptr, size_t size, size_t nmemb, FILE *stream)
  {
  size_t n = fread( ptr, size, nmemb, stream );
  if( n>0 && debugOutput!=NULL )
    {
    (void)fwrite( ptr, size, n, debugOutput );
    }
  /* LogMessage("fread() --> %d items", n); */

  return n;
  }

#else
  #define myfgets fgets
  #define myfgetc fgetc
  #define myfread fread
#endif

/* ChompEOL()
 * Assumes that we've read a separator but not the EOL char(s).
 * reads the \r and \n chars until it hits some other char and
 * calls 'ungetc' to push the first non-EOL char back into the
 * stream.
 */
void ChompEOL( FILE* stream )
  {
  int c;
  int n=0;
  for(;;)
    {
    c = myfgetc( stream );
    if( c=='\r' || c=='\n' )
      {
      ++n;
      }
    else
      {
      if( c!=EOF )
        {
        ungetc( c, stream );
        }
      break;
      }
    }
  /* fprintf(debugOutput, "{ChompEOL()->%d}",n); */
  }

int ReadSeparator( char* buf, int l, FILE* stream )
  {
  int i=0;
  for( i=0; i<l; ++i )
    {
    int c = myfgetc( stream );
    if( c=='-' || c=='_' || isalnum(c) )
      {
      buf[i] = c;
      }
    else
      {
      ungetc( c, stream );
      break;
      }
    }
  buf[i] = 0;

  /* fprintf( debugOutput, "{read possible separator (%s)}", buf ); */
  return i;
  }


/* CopySingleVariable()
 * Move a _TAG_VALUE list, which represents the current CGI header variable
 * (which could be multiple lines, with sub-arguments), into the headers
 * list of a _CGI_HEADER struct.
 * Recommended to set the pointer to NULL after calling this.
 */
void CopySingleVariable( _CGI_HEADER* header, _TAG_VALUE* workingHeaders )
  {
  if( workingHeaders==NULL )
    return;

  char* tag = NULL;
  char* value = NULL;

  for( _TAG_VALUE* ptr=workingHeaders; ptr!=NULL; ptr=ptr->next )
    {
    if( ptr->tag!=NULL && *ptr->tag!=0 && strcmp(ptr->tag,"value")==0
        && ptr->value!=NULL && *ptr->value!=0 )
      {
      value = ptr->value;
      }
    }

  for( _TAG_VALUE* ptr=workingHeaders; ptr!=NULL; ptr=ptr->next )
    {
    if( ptr->tag!=NULL && *ptr->tag!=0 && strcmp(ptr->tag,"Content-Disposition")==0
        && ptr->value!=NULL && *ptr->value!=0 && strcmp(ptr->value,"form-data")==0 )
      {
      for( _TAG_VALUE* sptr=ptr->subHeaders; sptr!=NULL; sptr=sptr->next )
        {
        if( sptr->tag!=NULL && *sptr->tag!=0 && strcmp(sptr->tag,"name")==0
            && sptr->value!=NULL && *sptr->value!=0 )
          {
          tag = sptr->value;
          }
        }
      }
    }

  if( tag!=NULL && value!=NULL )
    {
    header->headers = NewTagValue( tag, value, header->headers, 0 );
    }
  }

/* ParsePostData()
 * Read stdin from a CGI POST and (a) populate a data structure with
 * form variables.  What we read is dropped into a _CGI_HEADER
 * struct whose address is passed in as an argument.
 * Don't forget to free the linked lists in there when done.
 */
int ParsePostData( _CONFIG* conf,
                    FILE* stream,
                    _CGI_HEADER *header,
                    int (*funcPtr)( _CONFIG* , _CGI_HEADER * ) )
  {
#ifdef DEBUG
  debugOutput = conf->logFileHandle;
  if( debugOutput!=NULL )
    {
    fprintf( debugOutput, "----start----\n");
    fflush( debugOutput );
    }
#endif

  enum postState state = ps_FIRSTHEADER;

  header->separatorString = NULL;
  header->files = NULL;
  header->headers = NULL;

  _TAG_VALUE* workingHeaders = NULL;

  while( !feof( stream ) )
    {
    /* shared among different states */
    char buf[BUFLEN];

    if( state==ps_FIRSTHEADER )
      {
      /* LogMessage("ps_FIRSTHEADER"); */
      if( myfgets(buf, BUFLEN, stream)!=buf )
        {
        /* this is normal if nothing was posted.. */
        /* Warning("Failed to load line of text from CGI stream (1)\n"); */
        return -1;
        }
      header->separatorString = strdup( StripEOL( buf ));

      if( *(header->separatorString) != '='
          && *(header->separatorString) != '-'
          && strstr( header->separatorString, "=" ) != NULL )
        {
#ifdef DEBUG
        Notice( "Perhaps we have var=value assignment in [%s]?",
                header->separatorString );
        Notice( "header->headers is initially %p", header->headers  );
#endif
        header->headers = ParseQueryString( header->headers , header->separatorString );
#ifdef DEBUG
        Notice( "header->headers is now %p", header->headers  );
#endif
        }
      else
        {
        state=ps_HEADERLINE;
        }
      }

    else if( state==ps_HEADERLINE )
      {
      /* LogMessage("ps_HEADERLINE"); */
      if( myfgets(buf, BUFLEN, stream)!=buf )
        {
        if( ! feof( stream ) )
          {
          Warning("Failed to load line of text from CGI stream (2)\n");
          }
        break;
        }

      if( buf[0]==CR && buf[1]==LF && buf[2]==0 )
        {
        /* next line is contents or start of data */
        if( HeadersContainTagAndSubTag( workingHeaders, "Content-Disposition", "filename" ) )
          { /* data stream */
          state = ps_DATA;
          }
        else
          { /* variable value */
          state = ps_VARIABLE;
          }
        }
      else
        {
        if( buf[0]>='A' && buf[0]<='Z' )
          {
          workingHeaders = ParseHeaderLine( workingHeaders, buf );
          }
        else
          {
          if( buf[0]=='-' && buf[1]=='-' && buf[2]=='\r' && buf[3]=='\n' )
            {
            /* end of MIME segments */
            break;
            }
          else
            {
            CGIHeader( NULL, 0, NULL, 0, NULL, 0, NULL);
            Error("Header line should start with [A-Z]: [%s]", buf );
            }
          }
        }
      }

    else if( state==ps_VARIABLE )
      {
      /* LogMessage("ps_VARIABLE"); */
      if( myfgets(buf, BUFLEN, stream)!=buf )
        {
        CGIHeader( NULL, 0, NULL, 0, NULL, 0, NULL);
        Error("Failed to load line of text from CGI stream (2)\n");
        }

      workingHeaders = ParseValue( buf, workingHeaders );
      /* printf("WorkingHeaders:\n"); */
      /* PrintTagValue("", workingHeaders); */
      CopySingleVariable( header, workingHeaders );
      FreeTagValue( workingHeaders );
      workingHeaders = NULL;

      state = ps_SEPARATOR;
      }

    else if( state==ps_DATA )
      {
      CGIHeader( NULL, 0, NULL, 0, NULL, 0, NULL);
      Error("Uploading files not supported by this CGI.");
#if 0
      /* LogMessage("ps_DATA"); */

      if( funcPtr( conf, header ) )
        {
        Warning( "Upload aborted." );
        fclose(stdin);
        FreeTagValue( workingHeaders );
        workingHeaders = NULL;
        break;
        }
      else
        {
        int uploadResults = UploadFile( conf, writePath, stream, workingHeaders, header );
        /* LogMessage("UploadFile returned %d", uploadResults); */
        if( uploadResults==0 )
          {
          FreeTagValue( workingHeaders );
          workingHeaders = NULL;
          state = ps_HEADERLINE;
          }
        else /* likely an empty upload - get the 'value' from empty line */
          {
          if( uploadResults<0 )
            {
            /* return -10; */
            }
          state = ps_VARIABLE;
          }
        }
#endif
      }

    else if( state==ps_SEPARATOR )
      {
      /* LogMessage("ps_SEPARATOR"); */

      if( EMPTY( header->separatorString ) )
        {
        CGIHeader( NULL, 0, NULL, 0, NULL, 0, NULL);
        Error("Parse error: no defined separator");
        }

      if( myfgets(buf, BUFLEN, stream)!=buf )
        {
        CGIHeader( NULL, 0, NULL, 0, NULL, 0, NULL);
        Error("Failed to load line of text from CGI stream (5)\n");
        }

      if( strncmp( buf, header->separatorString, strlen(header->separatorString ) )==0 )
        {
        FreeTagValue( workingHeaders );
        workingHeaders = NULL;
        state=ps_HEADERLINE;
        }
      else
        {
        /* perhaps we have a multi-line form input (<textarea>)? */
        if( header!=NULL && header->headers!=NULL && header->headers->value!=NULL )
          {
          /*
          Notice( "Appending it to: [%s] (old value = [%s])",
                  NULLPROTECT( header->headers->tag ),
                  NULLPROTECT( header->headers->value ) );
          */
          char** v = &(header->headers->value);

          int l = strlen( *v ) + strlen( buf ) + 5;
          char* tmp = calloc( l, sizeof( char ) );
          strcpy( tmp, *v );
          if( strchr( tmp, '\r' )==0 )
            {
            strcat( tmp, "\n" );
            }
          strcat( tmp, buf );
          free( *v );
          *v = tmp;
          }
        else
          { /* or not? */
          Warning("Parse problem: expected separator but got [%s]", buf );
          workingHeaders = AppendValue( buf, workingHeaders );
          }
        }
      }
    }

#ifdef DEBUG
  Notice("Broke out of ParsePost - separatorString=%s", NULLPROTECT( header->separatorString) );
#endif

  return 0;
  }

/* examples of headers we have to parse:
 * Content-Disposition: form-data; name="filename"; filename="auto-cert.txt"
 * Content-Type: text/plain
 */

/* ParseHeaderSubVariables()
 * Parses a string which represents header "sub-variables" in a CGI POST, as shown
 * in the example above, and populate _TAG_VALUE structs with what we found.
 * You pass in a buffer which contains the already-read string to process.
 * You also pass in a pointer to a list of already known sub-variables and
 * the function will return an expanded linked list (returns a new head).
 */
_TAG_VALUE* ParseHeaderSubVariables( _TAG_VALUE* list, char* buf )
  {
  char* firstEquals = NULL;
  char* firstSemi = NULL;
  char* ptr;
  _TAG_VALUE* myList = list;

  if( *buf==0 || *buf==LF || *buf==CR )
    {
    return list;
    }

  for( ptr=buf; *ptr!=0; ++ptr )
    {
    int c = *ptr;
    if( firstEquals==NULL && c=='=' )
      {
      firstEquals = ptr;
      }
    if( firstSemi==NULL && (c==';'||c==CR) )
      {
      firstSemi = ptr;
      }
    if( firstEquals!=NULL && firstSemi!=NULL )
      {
      break;
      }
    }

  if( firstEquals==NULL || firstSemi==NULL || (firstEquals+1)>=firstSemi
      || ( (*(firstEquals+1))!='"' && !isalpha(*(firstEquals+1)) ) )
    {
    Warning("HTTP header extra data should look like name=\"value\"; but is [%s]", buf );
    return list;
    }

  *firstEquals = 0;
  *firstSemi = 0;

  myList = NewTagValue( buf+1, StripQuotes(firstEquals+1), list, 0 );

  /* recurse as we may have more sub-variables in the input buffer */
  if( *(firstSemi+1)!=0 )
    {
    myList = ParseHeaderSubVariables( myList, firstSemi+1 );
    }

  return myList;
  }

char* RemoveURLEncoding( char* src, char* dst )
  {
  if( EMPTY( src ) )
    {
    return src;
    }

  char* sptr = NULL;
  char* dptr = dst;
  for( sptr=src; (*sptr)!=0; ++sptr )
    {
    if( (*sptr)=='%'
        && strchr( HEXDIGITS, *(sptr+1) )!=NULL
        && strchr( HEXDIGITS, *(sptr+2) )!=NULL )
      {
      int n = 0;
      char buf[3];
      buf[0] = *(sptr+1);
      buf[1] = *(sptr+2);
      buf[2] = 0;
      if( sscanf( buf, "%x", &n )==1 )
        {
        *dptr = n;
        ++sptr;
        ++sptr;
        }
      else
        {
        *dptr = *sptr;
        }
      }
    else
      {
      *dptr = *sptr;
      }
    ++dptr;
    }
  *dptr = 0;

  return dst;
  }

_TAG_VALUE* ParseQueryString( _TAG_VALUE* list, char* string )
  {
#ifdef DEBUG
  Notice("ParseQueryString( %p, %s )", list, string );
#endif

  /* no assignment */
  if( strstr( string, "=" )==NULL )
    return list;

  char* ptr = NULL;
  char* nextString = NULL;
  for( ptr=string; *ptr!=0; ++ptr )
    {
    if( *ptr=='&' )
      {
      *ptr = 0;
      nextString = ptr+1;
      break;
      }
    }

  char* value = NULL;
  char* tag = string;
  for( ptr=string; *ptr!=0; ++ptr )
    {
    if( *ptr=='=' )
      {
      *ptr = 0;
      value = ptr+1;
      break;
      }
    }

  if( tag!=NULL && value!=NULL )
    {
    if( strstr( value, "%" )!=NULL )
      { /* potentially remove URL encoding */
      char* buf = malloc( strlen(value)+2 );
      char* dst = RemoveURLEncoding( value, buf );
      if( dst!=NULL )
        {
        list = NewTagValue( tag, dst, list, 0 );
        }
      free( buf );
      }
    else
      { /* no URL encoding involved */
      list = NewTagValue( tag, value, list, 0 );
      }
    }

  if( nextString!=NULL )
    {
    return ParseQueryString( list, nextString );
    }
  else
    {
    return list;
    }
  }

/* examples of headers we have to parse:
 * Content-Disposition: form-data; name="filename"; filename="auto-cert.txt"
 * Content-Type: text/plain
 */

/* ParseHeaderLine()
 * Parses a string which represents a CGI POST header line such as above.
 * Populate _TAG_VALUE structs with what we found.
 * You pass in a buffer which contains the already-read string to process.
 * You also pass in a pointer to a list of already known variables and
 * the function will return an expanded linked list (returns a new head).
 */
_TAG_VALUE* ParseHeaderLine( _TAG_VALUE* workingHeaders, char* buf )
  {
  char* firstColon = NULL;
  char* firstSemi = NULL;
  char* ptr;

  for( ptr=buf; *ptr!=0; ++ptr )
    {
    int c = *ptr;
    if( firstColon==NULL && c==':' )
      {
      firstColon = ptr;
      }
    if( firstSemi==NULL && (c==';'||c==CR) )
      {
      firstSemi = ptr;
      }
    if( firstColon!=NULL && firstSemi!=NULL )
      {
      break;
      }
    }

  if( firstColon==NULL || firstSemi==NULL || (firstColon+2)>=firstSemi )
    {
    CGIHeader( NULL, 0, NULL, 0, NULL, 0, NULL);
    Error("HTTP header should have at least [a: b;] but is only [%s]", buf );
    }

  *firstColon = 0;
  *firstSemi = 0;
  workingHeaders = NewTagValue( buf, firstColon+2, workingHeaders, 0 );
  if( *(firstSemi+1)!=0 )
    { /* there is stuff after the semicolon or CR */
    workingHeaders->subHeaders = ParseHeaderSubVariables( NULL, firstSemi+1 );
    }

  return workingHeaders;
  }

/* HeadersContainTag()
 * return non-NULL if a linked list of tag-value pairs contains a given tag
 */
_TAG_VALUE* HeadersContainTag( _TAG_VALUE* list, char* tag )
  {
  while( list!=NULL )
    {
    if( list->tag!=NULL && strcmp( list->tag, tag )==0 )
      {
      return list;
      }
    list=list->next;
    }

  return NULL;
  }

/* return 0 if list contains a tag and sub-tag */
int HeadersContainTagAndSubTag( _TAG_VALUE* list, char* tag, char* subTag )
  {
  _TAG_VALUE* parent = NULL;
  parent = HeadersContainTag( list, tag );
  if( parent!=NULL )
    {
    _TAG_VALUE* child = NULL;
    child = HeadersContainTag( parent->subHeaders, subTag );
    if( child!=NULL )
      {
      return 0;
      }
    }

  return -1;
  }

/* ParseValue()
 * Extract any trailing CR chars and stick a value=[%s] entry in a
 * linked list.  Used to process the value line of an HTTP form variable.
 * Note that old linked list head is passed in, new head is returned.
 */
_TAG_VALUE* ParseValue( char* buf, _TAG_VALUE* workingHeaders )
  {
  for( char *ptr=buf; *ptr!=0; ++ptr )
    {
    if( *ptr == CR )
      {
      *ptr = 0;
      break;
      }
    }

  workingHeaders = NewTagValue( "value", buf, workingHeaders, 0 );

  return workingHeaders;
  }

_TAG_VALUE* AppendValue( char* buf, _TAG_VALUE* workingHeaders )
  {
  for( char *ptr=buf; *ptr!=0; ++ptr )
    {
    if( *ptr == CR )
      {
      *ptr = 0;
      break;
      }
    }

  if( workingHeaders->tag!=NULL
      && strcmp( workingHeaders->tag,"value")==0
      && workingHeaders->value!=NULL )
    {
    int l = strlen( workingHeaders->value ) + strlen( buf ) + 5;
    char* tmp = calloc( l, sizeof( char ) );
    strcpy( tmp, workingHeaders->value );
    strcat( tmp, "\n" );
    strcat( tmp, buf );
    free( workingHeaders->value );
    workingHeaders->value = tmp;
    }

  return workingHeaders;
  } 

/* size is for http header; path is full path to the file (not
   just the folder where the file exists);
   filename is what the client should name the file */
void DownloadFile( long filesize, char* path, char* fileName )
  {
  FILE* f = fopen( path, "r" );
  if( f==NULL )
    {
    CGIHeader( NULL, 0, NULL, 0, NULL, 0, NULL);
    Error("Cannot open [%s] for download", NULLPROTECT( path ) );
    }

  printf( "Content-Type: application/octet-stream\r\n");
  printf( "Content-Disposition: attachment; filename=\"%s\"\r\n",
          fileName==NULL ? "no-name-file" : fileName );
  printf( "Content-Length: %ld\r\n", filesize );
  printf( "X-Pad: avoid browser bug\r\n");
  printf( "\r\n" );

  char buf[BUFLEN];
  int n = 0;
  unsigned long total = 0;
  while( (n=fread( buf, sizeof(char), sizeof(buf), f ))>0 )
    {
    int m = fwrite( buf, sizeof(char), n, stdout );
    if( m!=n )
      {
      Warning("Failed to write file @ %'lu", total);
      break;
      }
    total += m;
    }

  }
