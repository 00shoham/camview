#include "base.h"

extern int glob_argc;
extern char** glob_argv;
extern _CONFIG* glob_conf;
extern int inCGI;

enum downloadFormat
  {
  df_count,
  df_mp4,
  df_tar
  };

char* GenerateDownloadFilename( int nActiveCams, char* firstCamName,
                                char* fromDate, char* fromTime,
                                char* toDate, char* toTime,
                                enum downloadFormat df )
  {
  char buf[BUFLEN];
  char* nptr = buf;
  char* nend = buf + sizeof(buf) - 1;

  strncpy( nptr, glob_conf->downloadDir, nend-nptr );
  nptr += strlen( nptr );
  strncpy( nptr, "/images-", nend-nptr );
  nptr += strlen( nptr );

  if( nActiveCams==1 )
    {
    strncpy( nptr, firstCamName, nend-nptr  );
    nptr += strlen( nptr );
    }
  else
    {
    snprintf( nptr, nend-nptr, "%d-cams", nActiveCams );
    nptr += strlen( nptr );
    }

  strncpy( nptr, "-", nend-nptr );
  nptr += strlen( nptr );

  strncpy( nptr, fromDate, nend-nptr );
  nptr += strlen( nptr );
  strncpy( nptr, "-", nend-nptr );
  nptr += strlen( nptr );

  strncpy( nptr, fromTime, nend-nptr );
  nptr += strlen( nptr );
  strncpy( nptr, "-", nend-nptr );
  nptr += strlen( nptr );

  strncpy( nptr, toDate, nend-nptr );
  nptr += strlen( nptr );
  strncpy( nptr, "-", nend-nptr );
  nptr += strlen( nptr );

  strncpy( nptr, toTime, nend-nptr );
  nptr += strlen( nptr );

  /* sanitize the filename */
  for( char* cptr=buf; cptr<nptr; ++cptr )
    {
    int c = *cptr;
    if( strchr( VALIDPATHCHARS, c )==NULL )
      {
      *cptr = '_';
      }
    }

  if( df==df_tar )
    {
    strncpy( nptr, ".tar", nend-nptr );
    nptr += strlen( nptr );
    }
  else if( df==df_mp4 )
    {
    strncpy( nptr, ".mp4", nend-nptr );
    nptr += strlen( nptr );
    }
  else
    {
    Error( "Unexpected file format" );
    }

  *nptr = 0;

  return strdup( buf );
  }


/* pass in a path to a file we want to download.  Return a unique ID for that file */
char* LogDownload( char* filePath )
  {
  if( glob_conf==NULL || EMPTY( glob_conf->downloadDir ) )
    {
    Error( "Cannot log a download - no config file or download dir" );
    }

  char* guid = GenerateAllocateGUID();

  time_t t = time( NULL );
  t += DOWNLOAD_EXPIRY_SECONDS;
  char expiryStamp[100];
  snprintf( expiryStamp, sizeof(expiryStamp)-1, "%lx", (long)t );

  char entry[BUFLEN];
  snprintf( entry, sizeof(entry)-1, "%s %s %s\n",
            expiryStamp, guid, filePath );

  char ledger[BUFLEN];
  snprintf( ledger, sizeof(ledger)-1, "%s/%s",
            glob_conf->downloadDir, FILENAME_DOWNLOADS );
  FILE* f = fopen( ledger, "a+" );
  if( f==NULL )
    {
    Error( "Failed to open %s", ledger );
    }
  fputs( entry, f );
  fclose( f );

  return guid;
  }

char* ListFileName( char* buf, int bufLen )
  {
  if( buf==NULL || bufLen<100 )
    {
    Error( "Need a buffer into which a list file path will be written" );
    }

  if( glob_conf==NULL )
    {
    Error( "Cannot generate a list file without a config file" );
    }

  if( EMPTY( glob_conf->downloadDir ) )
    {
    Error( "Cannot generate a list file without a download directory defined" );
    }

  char* ptr = buf;
  char* end = buf + bufLen - 1;
  strncpy( ptr, glob_conf->downloadDir, end-ptr );
  ptr += strlen( ptr );
  if( *(ptr-1)!='/' )
    {
    *ptr = '/';
    ++ptr;
    *ptr = 0;
    }

  if( end-ptr<10 )
    {
    Error( "No room left in buffer for list filename" );
    }

  GenerateIdentifier( ptr, 8 );

  return buf;
  }

char* ExecuteDownload( int* cameras,
                       char* fromDate, char* fromTime,
                       char* toDate, char* toTime,
                       enum downloadFormat df,
                       char** guidPtr,
                       char** fileNamePtr )
  {
  char status[BIGBUF];
  char* ptr = status;
  char* end = status + sizeof(status)-1;
  *ptr = 0;

  EnsureDirExists( glob_conf->downloadDir );

  char listFileName[ BUFLEN/2 ];
  listFileName[0] = 0;
  (void)ListFileName( listFileName, sizeof( listFileName ) );

  FILE* listFile = NULL;
  if( df!=df_count )
    {
    listFile = fopen( listFileName, "w" );
    if( listFile==NULL )
      {
      CGIHeader( NULL, 0, NULL, 0, NULL, 0, NULL);
      Error( "Cannot create list file" );
      }
    }

  int camNum = 0;
  int nActiveCams = 0;
  char* firstCamName = NULL;
  for( _CAMERA* cam = glob_conf->cameras; cam!=NULL; cam=cam->next )
    {
    if( cameras[camNum] )
      {
      ++nActiveCams;
      if( firstCamName==NULL )
        {
        firstCamName = cam->nickName;
        }

      if( EMPTY( cam->nickName ) )
        {
        Error( "Camera %d has empty nickname", camNum );
        }

      CameraBackupFolder( glob_conf, cam );

      int nImages = 0;
      char** images = NULL;
      nImages = FindMatchingImages( cam, fromDate, fromTime, toDate, toTime, &images );

      /* regardless of format - show how many images match per camera */
      snprintf( ptr, end-ptr, "%s: %d<br/>\n", cam->nickName, nImages );
      ptr += strlen( ptr );

      if( df!=df_count )
        {
        for( int i=0; i<nImages; ++i )
          {
          if( NOTEMPTY( images[i] ) )
            {
            fputs( cam->nickName, listFile );
            fputs( "/", listFile );
            fputs( images[i], listFile );
            fputs( "\n", listFile );
            }
          }
        }

      if( images!=NULL )
        {
        for( int i=0; i<nImages; ++i )
          {
          if( images[i]!=NULL )
            {
            free( images[i] );
            images[i] = NULL;
            }
          }
        free( images );
        images = NULL;
        }
      }
    else
      {
      /* Notice("Camera %d (%s) not selected", camNum, cam->nickName ); */
      }

    ++camNum;
    }

  if( listFile!=NULL )
    {
    fclose( listFile );
    listFile = NULL;
    }

  int err = chdir( glob_conf->backupDir );
  if( err )
    {
    CGIHeader( NULL, 0, NULL, 0, NULL, 0, NULL);
    Error( "Failed to chdir to %s (%d:%s)", glob_conf->backupDir,
           err, strerror( err ) );
    }
  else
    {
    Notice( "Changed folder to %s", glob_conf->backupDir );
    }

  /* work out the output filename: */
  char* archiveFile = NULL;
  if( df!=df_count )
    {
    archiveFile = GenerateDownloadFilename(
                    nActiveCams, firstCamName,
                    fromDate, fromTime,
                    toDate, toTime,
                    df );
    } /* we've got our output filename now */

  /* generate the download and send it over */
  if( df==df_tar )
    {
    char cmd[BUFLEN];

    snprintf( cmd, sizeof(cmd)-1,
              "/bin/tar cf '%s' --files-from=%s",
              archiveFile,
              listFileName );
    Notice( "Running - %s", cmd );
    err = SyncRunCommandNoIO( cmd );
    if( err==-1 && errno==ECHILD )
      { /* child exited, no return code */
      err = 0;
      }

    if( err!=0 )
      {
      snprintf( ptr, end-ptr, "Failed to run tar: %d/%s",
                err, strerror( err ) );
      ptr += strlen( ptr );
      Warning( "Command returned error %d - %d - %s", err, errno, strerror(errno) );
      }
    else
      {
      Notice( "Run was successful" );
      }
    }
  else if( df==df_mp4 )
    {
    char cmd[BUFLEN];

    /*
    snprintf( cmd, sizeof(cmd)-1,
              "cat `cat %s`"
              " | /usr/bin/ffmpeg -y"
              " -framerate 1 "
              " -f image2pipe -i -"
              " -pix_fmt yuv420p"
              " '%s'",
              listFileName,
              archiveFile );
    */
    snprintf( cmd, sizeof(cmd)-1,
              "/usr/bin/ffmpeg -y"
              " -framerate 1 "
              " -f image2pipe -i -"
              " -pix_fmt yuv420p"
              " '%s'",
              archiveFile );

    Notice("Running - %s", cmd );
    /*
    err = SyncRunShellNoIO( cmd );
    */
    err = SyncRunCommandManyFilesStdin( cmd, listFileName );
    if( err==-1 && errno==ECHILD )
      { /* child exited, no return code */
      err = 0;
      }

    if( err!=0 )
      {
      snprintf( ptr, end-ptr, "Failed to run ffmpeg: %d/%s",
                err, strerror( err ) );
      ptr += strlen( ptr );
      Warning( "Command returned error %d - %d - %s", err, errno, strerror(errno) );
      }
    else
      {
      Notice( "Run was successful" );
      }
    }
  else if( df==df_count )
    {
    /* just need a counter, which we've already calculated and printed */
    }
  else
    { /* this should not be */
    Error( "Unexpected download format" );
    }

  if( archiveFile!=NULL )
    {
    char* guid = LogDownload( archiveFile );
    if( guid!=NULL && guidPtr!=NULL )
      {
      *guidPtr = guid;
      }
    if( fileNamePtr!=NULL )
      {
      *fileNamePtr = strdup( GetFilenameFromPath( archiveFile ) );
      }
    /* long l = FileSize( archiveFile ); */
    /* DownloadFile( l, archiveFile, GetFilenameFromPath( archiveFile ) ); */
    /* unlink( archiveFile ); */

    FREE( archiveFile );
    }

  if( NOTEMPTY( listFileName ) )
    {
    unlink( listFileName );
    }

  return strdup( status );
  }

int ValidateDownloadForm( _CGI_HEADER *h )
  {
  int status = 0;
  char nowDateBuf[20];
  char nowTimeBuf[20];
  char* nowDate = DateNow( nowDateBuf, sizeof( nowDateBuf ) );
  char* nowTime = TimeNow( nowTimeBuf, sizeof( nowTimeBuf ), 1 );

  char* fromDate = GetTagValueSafe( h->headers, "from-date", RE_DATE );
  if( EMPTY( fromDate ) ) { ++status; Warning( "A from-date must be specified"); }
  if( ValidDate( fromDate )!=0 ) { ++status; Warning( "Invalid from-date: %s", fromDate ); }

  char* fromTime = GetTagValueSafe( h->headers, "from-time", RE_TIME );
  if( fromTime==NULL )
    fromTime = GetTagValueSafe( h->headers, "from-time", RE_TIME_NOSEC );
  if( EMPTY( fromTime ) ) { ++status; Warning( "A from-time must be specified"); }
  if( ValidTime( fromTime )!=0 ) { ++status; Warning( "Invalid from-time: %s", fromTime ); }

  char* toDate = GetTagValueSafe( h->headers, "to-date", RE_DATE );
  if( EMPTY( toDate ) ) { ++status; Warning( "A to-date must be specified"); }
  if( ValidDate( toDate )!=0 ) { ++status; Warning( "Invalid to-date: %s", toDate ); }

  char* toTime = GetTagValueSafe( h->headers, "to-time", RE_TIME );
  if( toTime==NULL )
    toTime = GetTagValueSafe( h->headers, "to-time", RE_TIME_NOSEC );
  if( EMPTY( toTime ) ) { ++status; Warning( "A to-time must be specified"); }
  if( ValidTime( toTime )!=0 ) { ++status; Warning( "Invalid to-time: %s", toTime ); }

  if( strcmp( fromDate, nowDate )>0 ) { ++status; Warning( "From-date in the future" ); }
  if( strcmp( fromDate, nowDate )==0 && strcmp( fromTime, nowTime )==0 ) { ++status; Warning( "From-time in the future" ); }
  if( strcmp( fromDate, toDate )>0 ) { ++status; Warning( "From-date later than to-date" ); }
  if( strcmp( fromDate, toDate )==0 && strcmp( fromTime, toTime )>0 ) { ++status; Warning( "From-time later than to-time" ); }

  for( _TAG_VALUE* t=h->headers; t!=NULL; t=t->next )
    {
    if( NOTEMPTY( t->tag ) && strcmp( t->tag, "camera" )==0 )
      {
      if( EMPTY( t->value ) ) Error( "Camera tag has no value" );
      _CAMERA* cam = FindCamera( glob_conf, t->value );
      if( cam==NULL ) Error( "Invalid camera ID" );
      }
    }

  char* format = GetTagValueSafe( h->headers, "format", RE_IDENTIFIER );
  if( EMPTY( format ) ) Error( "Format not specified" );
  if( strcmp( format, "count" )!=0
      && strcmp( format, "tar" )!=0
      && strcmp( format, "mp4" )!=0 ) Error( "Invalid format" );

  return status;
  }

void CGIBody()
  {
  enum downloadFormat df = df_count;

  /*
  char status[BIGBUF];
  char* ptr = status;
  char* end = status + sizeof(status)-1;
  *ptr = 0;
  */
  int* cameras = NULL;
  int nCameras = CountCameras( glob_conf );
  if( nCameras<1 )
    {
    CGIHeader( NULL, 0, NULL, 0, NULL, 0, NULL);
    Error("No cameras defined");
    }

  cameras = (int*)calloc( nCameras, sizeof( int ) );

  time_t tnow = time(NULL);
  time_t tstart = tnow - 60 * 60; /* one hour ago */
  time_t tearliest = tnow - OLDEST_DOWNLOAD * 24 * 60 * 60;
  char fromDateBuf[20];
  char* fromDate = DateStr( tstart, fromDateBuf, sizeof( fromDateBuf ) );
  char fromTimeBuf[20];
  char* fromTime = TimeStr( tstart, fromTimeBuf, sizeof( fromTimeBuf ), 0 );
  char toDateBuf[20];
  char* toDate = DateStr( tnow, toDateBuf, sizeof( toDateBuf ) );
  char latestDateBuf[20];
  char* latestDate = DateStr( tnow, latestDateBuf, sizeof( latestDateBuf ) );
  char toTimeBuf[20];
  char* toTime = TimeStr( tnow, toTimeBuf, sizeof( toTimeBuf ), 0 );
  char earliestDateBuf[20];
  char* earliestDate = DateStr( tearliest, earliestDateBuf, sizeof( earliestDateBuf ) );

  _CGI_HEADER h;
  int err = ParsePostData( stdin, &h, ValidateDownloadForm );

  if( err==0 && h.headers!=NULL )
    { /* successful form post */
    /*
    snprintf( ptr, end-ptr, "Form tags:<br/>\n" );
    ptr += strlen( ptr );
    for( _TAG_VALUE* t=h.headers; t!=NULL; t=t->next )
      {
      snprintf( ptr, end-ptr, "%s=%s<br/>\n", NULLPROTECT( t->tag ), NULLPROTECT( t->value ) );
      ptr += strlen( ptr );
      }
    */

    char* s = NULL;
    s = GetTagValueSafe( h.headers, "from-date", RE_DATE );
    if( NOTEMPTY( s ) ) fromDate = s;
    s = GetTagValueSafe( h.headers, "from-time", RE_TIME );
    if( NOTEMPTY( s ) ) fromTime = s;
    s = GetTagValueSafe( h.headers, "from-time", RE_TIME_NOSEC );
    if( NOTEMPTY( s ) ) fromTime = s;
    s = GetTagValueSafe( h.headers, "to-date", RE_DATE );
    if( NOTEMPTY( s ) ) toDate = s;
    s = GetTagValueSafe( h.headers, "to-time", RE_TIME );
    if( NOTEMPTY( s ) ) toTime = s;
    s = GetTagValueSafe( h.headers, "to-time", RE_TIME_NOSEC );
    if( NOTEMPTY( s ) ) toTime = s;

    int camNum = 0;
    int numCamSelected = 0;
    for( _CAMERA* cam = glob_conf->cameras; cam!=NULL; cam=cam->next )
      {
      if( EMPTY( cam->nickName ) )
        {
        CGIHeader( NULL, 0, NULL, 0, NULL, 0, NULL);
        Error( "Camera %d has empty nickname", camNum );
        }
      for( _TAG_VALUE* t=h.headers; t!=NULL; t=t->next )
        {
        if( NOTEMPTY( t->tag )
            && strcmp( t->tag, "camera" )==0
            && NOTEMPTY( t->value )
            && strcmp( t->value, cam->nickName )==0
            && camNum < nCameras )
          {
          cameras[camNum] = 1;
          ++ numCamSelected;
          }
        }

      ++camNum;
      }

    if( numCamSelected==0 )
      {
      CGIHeader( NULL, 0, NULL, 0, NULL, 0, NULL);
      Error( "You must select at least one camera." );
      }

    s = GetTagValueSafe( h.headers, "format", RE_IDENTIFIER );
    if( NOTEMPTY( s ) && strcmp( s, "mp4" )==0 )
      { df = df_mp4; }
    else if( NOTEMPTY( s ) && strcmp( s, "tar" )==0 )
      { df = df_tar; }
    else if( NOTEMPTY( s ) && strcmp( s, "count" )==0 )
      { df = df_count; }
    else
      {
      CGIHeader( NULL, 0, NULL, 0, NULL, 0, NULL);
      Error( "Invalid download format" );
      }

    }
  else
    { /* no data posted - so we're just displaying our form */
    }

  /* download is a separate thing now - so always show a form */
  CGIHeader( "text/html", 0, DOWNLOAD_TITLE, 0, NULL, 0, NULL);

  char camString[BIGBUF];
  char* camPtr = camString;
  char* camEndPtr = camString + sizeof( camString ) - 1;

  int camNum = 0;
  for( _CAMERA* cam = glob_conf->cameras; cam!=NULL; cam=cam->next )
    {
    int checked = cameras[camNum];
    snprintf( camPtr, camEndPtr-camPtr-1,
              "              <input type=checkbox id=\"cam-%s\" name=\"camera\" value=\"%s\"%s/>\n",
              cam->nickName, cam->nickName, 
              checked ? " checked" : "" );
    camPtr += strlen( camPtr );

    snprintf( camPtr, camEndPtr-camPtr-1,
              "              <label for=\"cam-%s\">%s</label><br/>\n",
              cam->nickName, cam->nickName );
    camPtr += strlen( camPtr );

    ++camNum;
    }
  *camPtr = 0;

  char page[BIGBUF];

  char* fmtCount = "";
  if( df == df_count ) { fmtCount = " checked"; }
  char* fmtTar = "";
  if( df == df_tar ) { fmtTar = " checked"; }
  char* fmtMp4 = "";
  if( df == df_mp4 ) { fmtMp4 = " checked"; }

  char cwd[BUFLEN];
  if( getcwd( cwd, sizeof(cwd)-1 )!=cwd )
    {
    Error( "Cannot figure out initial folder path" );
    }

  char* guid = NULL;
  char* fileName = NULL;
  char* msg = ExecuteDownload( cameras, fromDate, fromTime, toDate, toTime, df, &guid, &fileName );
  if( chdir( cwd )!=0 )
    {
    Error( "Cannot return to folder %s -- %d:%s", cwd, err, strerror( err ) );
    }
  else
    {
    Notice( "Changed folder to %s", cwd );
    }

  char* template = NULL;
  long templateLen = FileRead( DOWNLOAD_TEMPLATE, (unsigned char**)&template );
  if( templateLen<=0 )
    {
    CGIHeader( NULL, 0, NULL, 0, NULL, 0, NULL);
    Error( "Cannot open template page %s", DOWNLOAD_TEMPLATE );
    }

  err = ExpandMacrosVA( template, page, sizeof(page)-1,
                        "STATUS", msg,
                        "GUID", guid==NULL ? "" : guid,
                        "FILENAME", fileName==NULL ? "" : fileName,
                        "FROMDATE", fromDate,
                        "FROMTIME", fromTime,
                        "TODATE", toDate,
                        "TOTIME", toTime,
                        "EARLIESTDATE", earliestDate,
                        "LATESTDATE", latestDate,
                        "CAMERALIST", camString,
                        "FORMAT_COUNT", fmtCount,
                        "FORMAT_TAR", fmtTar,
                        "FORMAT_MP4", fmtMp4,
                        NULL, NULL );

  
  FreeIfAllocated( &guid );
  FreeIfAllocated( &fileName );
  FREE( template );
  FREE( msg );

  if( err<0 )
    {
    CGIHeader( NULL, 0, NULL, 0, NULL, 0, NULL);
    Error( "Failed to expand defaults in %s", DOWNLOAD_TEMPLATE );
    }

  fputs( page, stdout );
  fputs( "\r\n\r\n", stdout );
  fflush( stdout );
  }

void TryToSendFileByGuid( char* guid )
  {
  char ledger[BUFLEN];
  snprintf( ledger, sizeof(ledger)-1, "%s/%s",
            glob_conf->downloadDir, FILENAME_DOWNLOADS );
  FILE* f = fopen( ledger, "r" );
  if( f==NULL )
    {
    Error( "Failed to open %s", ledger );
    }

  long expTime = 0;
  char ledgerGUID[BUFLEN];
  char filePath[BUFLEN];
  while( !feof( f ) )
    {
    if( fscanf( f, "%lx %s %s", &expTime, ledgerGUID, filePath )==3 )
      {
      if( strcmp( guid, ledgerGUID )==0 )
        {
        if( FileExists( filePath )==0 )
          {
          long l = FileSize( filePath );
          DownloadFile( l, filePath, GetFilenameFromPath( filePath ) );
          }
        else
          {
          CGIHeader( NULL, 0, NULL, 0, NULL, 0, NULL);
          Error( "The file [%s] could not be found (expired?).", filePath );
          }
        }
      }
    }

  fclose( f );
  }

int main( int argc, char** argv )
  {
  glob_argc = argc;
  glob_argv = argv;

  _CONFIG* config = NULL;
  config = (_CONFIG*)calloc(1, sizeof(_CONFIG) );
  glob_conf = config;

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

  if( EMPTY( config->backupDir ) )
    {
    CGIHeader( NULL, 0, NULL, 0, NULL, 0, NULL);
    Error( "You must specify BACKUP_DIR in the configuration file %s",
           CONFIGNAME );
    }

  if( EMPTY( config->downloadDir ) )
    {
    CGIHeader( NULL, 0, NULL, 0, NULL, 0, NULL);
    Error( "You must specify DOWNLOAD_DIR in the configuration file %s",
           CONFIGNAME );
    }

  logFileHandle = fopen( config->cgiLogFile, "a" );
  if( logFileHandle==NULL )
    {
    CGIHeader( NULL, 0, NULL, 0, NULL, 0, NULL);
    Error( "Failed to open %s", config->cgiLogFile );
    }

  /* is it a request to send a particular file? */
  char* query = getenv( "QUERY_STRING" );
  if( NOTEMPTY( query )
      && StringMatchesRegex( RE_IDENTIFIER, query )==0 )
    {
    TryToSendFileByGuid( query );
    }
  else
    {
    CGIBody();
    }

  fclose( logFileHandle );

  return 0;
  }
