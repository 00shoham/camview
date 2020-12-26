#include "base.h"

extern int glob_argc;
extern char** glob_argv;
extern _CONFIG* glob_conf;
extern int inCGI;

void SendSpecificImage( char* nickName, char* fileName, int maxWidth )
  {
  int err = 0;

  if( EMPTY( fileName ) )
    {
    CGIHeader( NULL, 0, NULL, 0, NULL, 0, NULL);
    Error("Cannot send empty file for camera %s", nickName );
    }

  if( FileExists( fileName )!=0 )
    {
    CGIHeader( NULL, 0, NULL, 0, NULL, 0, NULL);
    Error("File %s does not exist in camera %s", fileName, nickName );
    }

  long n = FileSize( fileName );;
  if( n<=0 )
    {
    CGIHeader( NULL, 0, NULL, 0, NULL, 0, NULL);
    Error("File %s has invalid size (%ld) in camera %s", fileName, n, nickName );
    }

  if( maxWidth<=0 )
    {
    DumpJPEGToBrowser( nickName, n, fileName );
    }
  else
    {
    unsigned char* imageFile;
    long nBytes = FileRead( fileName, &imageFile );
    if( nBytes!=n )
      {
      CGIHeader( NULL, 0, NULL, 0, NULL, 0, NULL);
      Error( "Inconsistent file size (%ld / %ld)", n, nBytes );
      }

    _IMAGE* input = NewImage( nickName, fileName, 0, 0, 0 );
    err = DecompressJPEGFromBytes( imageFile, nBytes, input );
    FREE( imageFile );
    if( err!=0 || input->data==NULL )
      {
      Error( "Could not parse image %s/%s", nickName, fileName );
      }

    int sWidth, sHeight;
    err = CalculateScaledHeight( input->width, input->height, maxWidth,
                                 &sWidth, &sHeight ) ;
    if( err )
      Error("Failed to calculate new image dimensions (%d)", err );
    if( input->width==sWidth && input->height==sHeight )
      {
      FreeImage( &input );
      DumpJPEGToBrowser( nickName, n, fileName );
      }
    else
      {
      /* scale it first */
      _IMAGE* output = NewImage( nickName, fileName, sWidth, sHeight, input->bpp );
      err = ScaleImage( input, output );
      if( err )
        {
        Error( "Failed to scale image to new dimensions (%d x %d) - %d",
               output->width, output->height, err );
        }
      else
        {
        /* Notice("Scaled image to %d x %d", sWidth, sHeight ); */
        }

      CGIHeader( "image/jpeg", -1, NULL, 0, NULL, 0, NULL );
      err = CompressJPEG( output, "-", JPEG_WRITE_IMAGEQUALITY );
      if( err )
        {
        Error( "Failed to compress scaled image (%d x %d) - %d",
               sWidth, sHeight, err );
        }
      FreeImage( &output );
      FreeImage( &input );
      }

    }
  
  /* do we need this? */
  fputs( "\r\n", stdout );
  fclose( stdout );
  }

char* MostRecentFilename( char* nickName )
  {
  DIR* d = opendir( "." );
  if( d==NULL )
    {
    CGIHeader( NULL, 0, NULL, 0, NULL, 0, NULL);
    Error( "Cannot open folder for camera %s.", nickName );
    }

  struct dirent * de;
  struct stat sbuf;
  time_t when = 0;
  char filename[BUFLEN] = { 0 };
  while( (de=readdir( d ))!=NULL )
    {
    if( NOTEMPTY( de->d_name ) )
      {
      if( StringEndsWith( de->d_name, ".jpg", 0 )==0
          && stat( de->d_name, &sbuf )==0 )
        {
        if( (sbuf.st_mode & S_IFMT)==S_IFREG
            && sbuf.st_mtime > when )
          {
          when = sbuf.st_mtime;
          strncpy( filename, de->d_name, sizeof(filename)-1 );
          }
        }
      }
    }

  closedir( d );

  if( when==0 )
    {
    CGIHeader( NULL, 0, NULL, 0, NULL, 0, NULL);
    Error( "No images found for camera %s.", nickName );
    }

  return strdup( filename );
  }

void SendMostRecentImage( char* nickName, int maxWidth )
  {
  char* fileName = MostRecentFilename( nickName );
  SendSpecificImage( nickName, fileName, maxWidth );
  free( fileName );
  }

void NameMostRecentImage( char* nickName )
  {
  char* fileName = MostRecentFilename( nickName );
  CGIHeader( "text/plain; charset=us-ascii", 0, NULL, 0, NULL, 0, NULL);
  fputs( fileName, stdout );
  fputs( "\r\n\r\n", stdout );
  fflush( stdout );
  free( fileName );
  }

/* assumes cameraID is valid - caller should check this!! */
/* assumes current working dir belongs to the camera */
void GetInfoForSelectedCamera( char* cameraID )
  {
  /* { "s" : "Hello", "i" : 1234 } */
  _TAG_VALUE* list = NULL;

  CGIHeader( "application/json; charset=us-ascii", 0, NULL, 0, NULL, 0, NULL);

  list = NewTagValue( "camera", cameraID, list, 0 );
  time_t earliest = 0;
  time_t latest = 0;
  int n = CountFilesInFolder( NULL, NULL, ".jpg", &earliest, &latest );
  list = NewTagValueInt( "images", n, list, 0 );
  time_t tnow = time(NULL);
  if( n>0 && earliest!=0 )
    {
    list = NewTagValueInt( "earliestFileAge", (int)((long)tnow - (long)earliest), list, 0 );
    }
  if( n>0 && latest!=0 )
    {
    list = NewTagValueInt( "latestFileAge", (int)((long)tnow - (long)latest), list, 0 );
    }

  char buf[BUFLEN];
  if( ListToJSON( list, buf, sizeof(buf)-1 )==0 )
    {
    fputs( buf, stdout );
    }
  FreeTagValue( list );
  }

void GoBackForward( char* cameraID, char* fileName, int goBack, int goForward )
  {
  /* QQQ */
  int allocatedFilename = 0;
  if( EMPTY( fileName ) )
    {
    fileName = MostRecentFilename( cameraID );
    if( fileName!=NULL )
      {
      allocatedFilename = 1;
      }
    }

  int n = CountFilesInFolder( NULL, NULL, ".jpg", NULL, NULL );
  char** files = SafeCalloc( n+20, sizeof(char*), "filename array" );

  DIR* d = opendir( "." );
  if( d==NULL )
    {
    CGIHeader( NULL, 0, NULL, 0, NULL, 0, NULL);
    Error( "Cannot open folder for camera %s.", cameraID );
    }

  int fileNum = 0;
  struct dirent * de;
  while( (de=readdir( d ))!=NULL )
    {
    if( NOTEMPTY( de->d_name )
        && StringEndsWith( de->d_name, ".jpg", 0 )==0
        && fileNum < n+20 )
      {
      files[fileNum] = strdup( de->d_name );
      ++fileNum;
      }
    }
  closedir( d );

  qsort( files, fileNum, sizeof( char* ), CompareStrings );

  int anchor = -1;
  for( int i=0; i<fileNum; ++i )
    {
    char* s = files[i];
    if( NOTEMPTY( s ) && strcmp( s, fileName )==0 )
      {
      anchor = i;
      break;
      }
    }

  _TAG_VALUE* container = NULL;

  if( anchor==-1 )
    {
    _TAG_VALUE* list = NewTagValue( "filename", fileName, NULL, 0 );
    list = NewTagValueInt( "index", 0, NULL, 0 );
    container = NewTagValueList( "list", list, container, 0 );
    }
  else
    {
    int start = anchor - goBack;
    if( start<0 ) start = 0;
    int finish = anchor + goForward;
    if( finish>=fileNum ) finish = fileNum-1;
    for( int i=finish; i>=start; --i )
      {
      _TAG_VALUE* list = NewTagValue( "filename", files[i], NULL, 0 );
      list = NewTagValueInt( "index", i-start, list, 0 );
      container = NewTagValueList( "file", list, container, 0 );
      }
    }

  char buf[BUFLEN];
  if( NestedListToJSON( "images", container, buf, sizeof(buf)-1 )==0 )
    {
    CGIHeader( "application/json; charset=us-ascii", 0, NULL, 0, NULL, 0, NULL);
    fputs( buf, stdout );
    }
  else
    {
    CGIHeader( NULL, 0, NULL, 0, NULL, 0, NULL);
    Error( "Cannot convert %d files to a JSON.", fileNum );
    }

  for( int i=0; i<fileNum; ++i )
    {
    FreeIfAllocated( files+i );
    }
  free( files );

  if( allocatedFilename )
    {
    free( fileName );
    }
  }

void CGIBody()
  {
  char* cameraID = NULL;
  char* fileName = NULL;
  int goBack = 0;
  int goForward = 0;
  int getInfo = 0;
  int nameOnly = 0;
  int maxWidth = 0;

  char* query = getenv( "QUERY_STRING" );
  if( EMPTY( query ) )
    {
    CGIHeader( NULL, 0, NULL, 0, NULL, 0, NULL);
    Error("You must specify a query string.  ?cam=CAMID[&file=FILENAME]");
    }
  else
    {
    /* Notice("query string = %s", query ); */
    }
  char* term = NULL;
  char* internal = NULL;
  for( term = strtok_r( query, "&", &internal );
       term!=NULL;
       term = strtok_r( NULL, "&", &internal ) )
    {
    char* variable = NULL;
    char* value = NULL;
    char* iptr = NULL;
    variable = strtok_r( term, "=", &iptr );
    if( EMPTY( variable ) )
      {
      CGIHeader( NULL, 0, NULL, 0, NULL, 0, NULL);
      Error( "Missing variable name in CGI argument" );
      }
    else
      {
      value = strtok_r( NULL, "=", &iptr );
      if( NOTEMPTY( value ) && strcasecmp( variable, "cam" )==0 )
        {
        cameraID = value;
        }
      else if( NOTEMPTY( value ) && strcasecmp( variable, "file" )==0 )
        {
        fileName = value;
        }
      else if( NOTEMPTY( value ) && strcasecmp( variable, "time" )==0 )
        {
        /* don't care - just used to muck with client-side caching */
        }
      else if( strcasecmp( variable, "nameonly" )==0 )
        {
        nameOnly = 1;
        }
      else if( strcasecmp( variable, "info" )==0 )
        {
        getInfo = 1;
        }
      else if( strcasecmp( variable, "maxwidth" )==0 && atoi( value )>0 )
        {
        maxWidth = atoi( value );
        }
      else if( strcasecmp( variable, "back" )==0 && atoi( value )>0 )
        {
        goBack = atoi( value );
        }
      else if( strcasecmp( variable, "forward" )==0 && atoi( value )>0 )
        {
        goForward = atoi( value );
        }
      else
        {
        CGIHeader( NULL, 0, NULL, 0, NULL, 0, NULL);
        Error( "Invalid argument: %s", variable );
        }
      }
    }

  if( EMPTY( cameraID ) )
    {
    CGIHeader( NULL, 0, NULL, 0, NULL, 0, NULL);
    Error("A camera ID is required.");
    }

  if( ValidCamera( glob_conf, cameraID )!=0 )
    {
    CGIHeader( NULL, 0, NULL, 0, NULL, 0, NULL);
    Error("Invalid camera ID (no such camera in config file).");
    }

  _CAMERA* cam = FindCamera( glob_conf, cameraID );
  if( cam==NULL )
    {
    CGIHeader( NULL, 0, NULL, 0, NULL, 0, NULL);
    Error("No camera. Aborting.");
    }

  CameraBackupFolder( glob_conf, cam );
  if( EMPTY( cam->backupFolderPath ) )
    {
    CGIHeader( NULL, 0, NULL, 0, NULL, 0, NULL);
    Error("Camera %s has no backup folder", cameraID );
    }

  if( chdir( cam->backupFolderPath )!=0 )
    {
    CGIHeader( NULL, 0, NULL, 0, NULL, 0, NULL);
    Error("Cannot change directory to %s", cam->backupFolderPath  );
    }

  /* status calls */
  if( getInfo )
    {
    GetInfoForSelectedCamera( cameraID );
    return;
    }

  if( goBack || goForward )
    {
    if( goBack<0 || goBack>MAX_BACK )
      {
      Error( "Cannot go back by %d", goBack );
      }
    if( goForward<0 || goForward>MAX_FORWARD )
      {
      Error( "Cannot go forward by %d", goForward );
      }
    GoBackForward( cameraID, fileName, goBack, goForward );
    return;
    }

  if( nameOnly )
    {
    if( EMPTY( fileName ) )
      {
      NameMostRecentImage( cameraID );
      }
    else
      {
      CGIHeader( NULL, 0, NULL, 0, NULL, 0, NULL);
      Error("nameonly not compatible with specified filename" );
      }
    }
  else
    { /* return the actual image */
    if( EMPTY( fileName ) )
      {
      SendMostRecentImage( cameraID, maxWidth );
      }
    else
      {
      SendSpecificImage( cameraID, fileName, maxWidth );
      }
    }

  }

int main( int argc, char** argv )
  {
  glob_argc = argc;
  glob_argv = argv;

  _CONFIG* config = NULL;
  config = (_CONFIG*)calloc(1, sizeof(_CONFIG) );
  glob_conf = config;

  inCGI = 1;

  char* confName = CONFIGNAME;
  if( FileExists( confName )!=0 )
    {
    confName = ALTCONFIGNAME;
    }
  if( FileExists( confName )!=0 )
    {
    CGIHeader( NULL, 0, NULL, 0, NULL, 0, NULL);

    Error( "Cannot locate %s or %s", CONFIGNAME, ALTCONFIGNAME );
    }
  SetDefaults( config );
  ReadConfig( config, confName );

  if( EMPTY( config->cgiLogFile ) )
    {
    CGIHeader( NULL, 0, NULL, 0, NULL, 0, NULL);

    Error( "You must specify CGILOGFILE in the configuration file %s",
           CONFIGNAME );
    }

  config->logFileHandle = fopen( config->cgiLogFile, "a" );
  if( config->logFileHandle==NULL )
    {
    CGIHeader( NULL, 0, NULL, 0, NULL, 0, NULL);
    Error( "Failed to open %s", config->cgiLogFile );
    }

  CGIBody();

  fclose( config->logFileHandle );

  return 0;
  }
