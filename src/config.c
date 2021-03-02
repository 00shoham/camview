#include "base.h"

void SetDefaults( _CONFIG* config )
  {
  config->configName = NULL;
  config->nCameras = 0;
  config->cameras = NULL;
  config->baseDir = NULL;
  config->backupDir = NULL;
  config->downloadDir = NULL;
  config->logFile = NULL;
  config->cgiLogFile = NULL;
  config->minimumIntervalSeconds = MINIMUM_CAPTURE_INTERVAL;
  config->storePreMotion = STORE_PRE_MOTION;
  config->motionFrames = MOTION_FRAMES;
  config->fileCacheLength = FILE_CACHE_LENGTH;
  config->color_diff_threshold = DEFAULT_COLOR_DIFF_THRESHOLD;
  config->color_dark = DEFAULT_COLOR_DARK;
  config->dark_brightness_boost = DEFAULT_DARK_BRIGHTNESS_BOOST;
  config->despeckle_dark_threshold = DEFAULT_DESPECKLE_DARK_THRESHOLD;
  config->despeckle_nondark_min = DEFAULT_DESPECKLE_NONDARK_MIN;
  config->despeckle_bright_threshold = DEFAULT_DESPECKLE_BRIGHT_THRESHOLD;
  config->despeckle_nonbright_max = DEFAULT_DESPECKLE_NONBRIGHT_MAX;
  config->checkerboard_square_size = DEFAULT_CHECKERBOARD_SQUARE_SIZE;
  config->checkerboard_min_white = DEFAULT_CHECKERBOARD_MIN_WHITE;
  config->checkerboard_num_white = DEFAULT_CHECKERBOARD_NUM_WHITE;
  config->checkerboard_percent = DEFAULT_CHECKERBOARD_PERCENT;
  config->hup_interval = DEFAULT_HUP_INTERVAL;
  }

void SetDefaultsSingleCamera( _CONFIG* config, _CAMERA* cam )
  {
  cam->minimumIntervalSeconds = config->minimumIntervalSeconds;
  cam->storePreMotion = config->storePreMotion;
  cam->motionFrames = config->motionFrames;
  cam->color_diff_threshold = config->color_diff_threshold;
  cam->color_dark = config->color_dark;
  cam->dark_brightness_boost = config->dark_brightness_boost;
  cam->despeckle_dark_threshold = config->despeckle_dark_threshold;
  cam->despeckle_nondark_min = config->despeckle_nondark_min;
  cam->despeckle_bright_threshold = config->despeckle_bright_threshold;
  cam->despeckle_nonbright_max = config->despeckle_nonbright_max;
  cam->checkerboard_square_size = config->checkerboard_square_size;
  cam->checkerboard_min_white = config->checkerboard_min_white;
  cam->checkerboard_num_white = config->checkerboard_num_white;
  cam->checkerboard_percent = config->checkerboard_percent;
  if( pthread_mutex_init( &(cam->lock), NULL )!=0 )
    {
    Error( "Failed to init a mutex for camera %s", NULLPROTECT( cam->nickName ) );
    }
  if( pthread_mutex_init( &(cam->tlock), NULL )!=0 )
    {
    Error( "Failed to init second mutex for camera %s", NULLPROTECT( cam->nickName ) );
    }
  }

void FreeCamera( _CAMERA* cam )
  {
  if( cam==NULL )
    return;

  FreeImage( &(cam->recentImage) );
  FreeIfAllocated( &(cam->nickName) );
  FreeIfAllocated( &(cam->ipAddress) );
  FreeIfAllocated( &(cam->captureCommand) );
  FreeIfAllocated( &(cam->folderPath) );
  FreeIfAllocated( &(cam->backupFolderPath) );
  FreeIfAllocated( &(cam->lastStoredImage) );
  FreeIfAllocated( &(cam->lastImageSourceName) );

  (void)pthread_mutex_destroy( &(cam->lock) );
  (void)pthread_mutex_destroy( &(cam->tlock) );

  FREE( cam );
  }

void FreeConfig( _CONFIG* config )
  {
  _CAMERA* cam = config->cameras;
  while( cam!=NULL )
    {
    _CAMERA* nextCam = cam->next;
    if( nextCam!=NULL )
      {
      FreeCamera( cam );
      --(config->nCameras);
      }
    cam = nextCam;
    }

  if( config->nCameras!=0 )
    {
    Error( "When freeing config, wound up with %d camera%s",
           config->nCameras,
           config->nCameras>0 ? "s" : ""
           );
    }

  if( config->filesToBackup!=NULL )
    {
    FreeFilenames( config->filesToBackup );
    config->filesToBackup = NULL;
    }

  FreeIfAllocated( &(config->baseDir) );
  FreeIfAllocated( &(config->backupDir) );
  FreeIfAllocated( &(config->downloadDir) );
  FreeIfAllocated( &(config->logFile) );
  FreeIfAllocated( &(config->cgiLogFile) );

  if( config->logFileHandle!=NULL )
    {
    fclose( config->logFileHandle );
    config->logFileHandle = NULL;
    }

  if( config->list )
    {
    FreeTagValue( config->list );
    }

  if( config->includes )
    {
    FreeTagValue( config->includes );
    }

  FREE( config );
  }

int ValidCamera( _CONFIG* config, char* name )
  {
  if( EMPTY( name ) )
    return -1;

  for( _CAMERA* cam = config->cameras; cam!=NULL; cam=cam->next )
    {
    if( NOTEMPTY( cam->nickName )
        && strcasecmp( cam->nickName, name )==0 )
      {
      return 0;
      }
    }

  return -2;
  }

void ProcessConfigLine( char* ptr, char* equalsChar, _CONFIG* config )
  {
  *equalsChar = 0;

  char* variable = TrimHead( ptr );
  TrimTail( variable );
  char* value = TrimHead( equalsChar+1 );
  char* originalValue = value;
  TrimTail( value );

  if( NOTEMPTY( variable ) && NOTEMPTY( value ) )
    {
    char valueBuf[BUFLEN];

    /* expand any macros in the value */
    if( strchr( value, '$' )!=NULL )
      {
      int loopMax = 10;
      while( loopMax>0 )
        {
        int n = ExpandMacros( value, valueBuf, sizeof( valueBuf ), config->list );
        if( n>0 )
          {
          /* don't free the original value - that's done elsewhere */
          if( value!=NULL && value!=originalValue )
            {
            free( value );
            }
          value = strdup( valueBuf );
          }
        else
          {
          break;
          }
        --loopMax;
        }
      }

    config->list = NewTagValue( variable, value, config->list, 1 );

    /* printf("ProcessConfigLine( %s, %s )\n", variable, value ); */
    if( strcasecmp( variable, "CAMERA" )==0 )
      {
      _CAMERA* cam = (_CAMERA*)calloc(1,sizeof(_CAMERA));
      SetDefaultsSingleCamera( config, cam  );
      cam->next = config->cameras;
      config->cameras = cam;
      ++(config->nCameras);

      FreeIfAllocated( &( config->cameras->nickName ) );
      config->cameras->nickName = strdup( value );
      }
    else if( strcasecmp( variable, "COMMAND" )==0 )
      {
      if( config->cameras==NULL )
        Error("%s cannot precede CAMERA in config", variable );
      FreeIfAllocated( &( config->cameras->captureCommand ) );
      config->cameras->captureCommand = strdup( value );
      }
    else if( strcasecmp( variable, "FILES_TO_CACHE" )==0
             && atoi( value ) >0 )
      {
      config->fileCacheLength = atoi( value );
      }
    else if( strcasecmp( variable, "BACKUP_COMMAND" )==0 )
      {
      FreeIfAllocated( &( config->backupCommand ) );
      config->backupCommand = strdup( value );
      }
    else if( strcasecmp( variable, "DEFAULT_COLOR_DIFF_THRESHOLD" )==0
             && atoi( value ) >0 )
      {
      config->color_diff_threshold = atoi( value );
      }
    else if( strcasecmp( variable, "COLOR_DIFF_THRESHOLD" )==0
             && atoi( value ) >0 )
      {
      if( config->cameras==NULL )
        Error("%s cannot precede CAMERA in config", variable );
      config->cameras->color_diff_threshold = atoi( value );
      }
    else if( strcasecmp( variable, "DEFAULT_COLOR_DARK" )==0
             && atoi( value ) >0 )
      {
      config->color_dark = atoi( value );
      }
    else if( strcasecmp( variable, "COLOR_DARK" )==0
             && atoi( value ) >0 )
      {
      if( config->cameras==NULL )
        Error("%s cannot precede CAMERA in config", variable );
      config->cameras->color_dark = atoi( value );
      }
    else if( strcasecmp( variable, "DEFAULT_DARK_BRIGHTNESS_BOOST" )==0
             && atof( value ) >1.0 && atof( value )<5.0 )
      {
      config->dark_brightness_boost = atof( value );
      }
    else if( strcasecmp( variable, "DARK_BRIGHTNESS_BOOST" )==0
             && atof( value ) >1.0 && atof( value )<5.0 )
      {
      if( config->cameras==NULL )
        Error("%s cannot precede CAMERA in config", variable );
      config->cameras->dark_brightness_boost = atof( value );
      }
    else if( strcasecmp( variable, "DEFAULT_DESPECKLE_DARK_THRESHOLD" )==0
             && atoi( value ) >0 )
      {
      config->despeckle_dark_threshold = atoi( value );
      }
    else if( strcasecmp( variable, "DESPECKLE_DARK_THRESHOLD" )==0
             && atoi( value ) >0 )
      {
      if( config->cameras==NULL )
        Error("%s cannot precede CAMERA in config", variable );
      config->cameras->despeckle_dark_threshold = atoi( value );
      }
    else if( strcasecmp( variable, "DEFAULT_DESPECKLE_NONDARK_MIN" )==0
             && atoi( value ) >0 )
      {
      config->despeckle_nondark_min = atoi( value );
      }
    else if( strcasecmp( variable, "DESPECKLE_NONDARK_MIN" )==0
             && atoi( value ) >0 )
      {
      if( config->cameras==NULL )
        Error("%s cannot precede CAMERA in config", variable );
      config->cameras->despeckle_nondark_min = atoi( value );
      }
    else if( strcasecmp( variable, "DEFAULT_DESPECKLE_BRIGHT_THRESHOLD" )==0
             && atoi( value ) >0 )
      {
      config->despeckle_bright_threshold = atoi( value );
      }
    else if( strcasecmp( variable, "DESPECKLE_BRIGHT_THRESHOLD" )==0
             && atoi( value ) >0 )
      {
      if( config->cameras==NULL )
        Error("%s cannot precede CAMERA in config", variable );
      config->cameras->despeckle_bright_threshold = atoi( value );
      }
    else if( strcasecmp( variable, "DEFAULT_DESPECKLE_NONBRIGHT_MAX" )==0
             && atoi( value ) >0 )
      {
      config->despeckle_nonbright_max = atoi( value );
      }
    else if( strcasecmp( variable, "DESPECKLE_NONBRIGHT_MAX" )==0
             && atoi( value ) >0 )
      {
      if( config->cameras==NULL )
        Error("%s cannot precede CAMERA in config", variable );
      config->cameras->despeckle_nonbright_max = atoi( value );
      }
    else if( strcasecmp( variable, "DEFAULT_CHECKERBOARD_SQUARE_SIZE" )==0
             && atoi( value ) >0 )
      {
      config->checkerboard_square_size = atoi( value );
      }
    else if( strcasecmp( variable, "CHECKERBOARD_SQUARE_SIZE" )==0
             && atoi( value ) >0 )
      {
      if( config->cameras==NULL )
        Error("%s cannot precede CAMERA in config", variable );
      config->cameras->checkerboard_square_size = atoi( value );
      }
    else if( strcasecmp( variable, "DEFAULT_CHECKERBOARD_MIN_WHITE" )==0
             && atoi( value ) >0 )
      {
      config->checkerboard_min_white = atoi( value );
      }
    else if( strcasecmp( variable, "CHECKERBOARD_MIN_WHITE" )==0
             && atoi( value ) >0 )
      {
      if( config->cameras==NULL )
        Error("%s cannot precede CAMERA in config", variable );
      config->cameras->checkerboard_min_white = atoi( value );
      }
    else if( strcasecmp( variable, "DEFAULT_CHECKERBOARD_NUM_WHITE" )==0
             && atoi( value ) >0 )
      {
      config->checkerboard_num_white = atoi( value );
      }
    else if( strcasecmp( variable, "CHECKERBOARD_NUM_WHITE" )==0
             && atoi( value ) >0 )
      {
      if( config->cameras==NULL )
        Error("%s cannot precede CAMERA in config", variable );
      config->cameras->checkerboard_num_white = atoi( value );
      }
    else if( strcasecmp( variable, "DEFAULT_CHECKERBOARD_PERCENT" )==0
             && atof( value ) >0 )
      {
      config->checkerboard_percent = atof( value );
      }
    else if( strcasecmp( variable, "CHECKERBOARD_PERCENT" )==0
             && atof( value ) >0 )
      {
      if( config->cameras==NULL )
        Error("%s cannot precede CAMERA in config", variable );
      config->cameras->checkerboard_percent = atof( value );
      }
    else if( strcasecmp( variable, "DEBUG" )==0 )
      {
      if( config->cameras==NULL )
        Error("%s cannot precede CAMERA in config", variable );
      if( strcasecmp( value, "true" )==0
          || strcasecmp( value, "1" )==0
          || strcasecmp( value, "yes" )==0 )
        {
        config->cameras->debug = 1;
        }
      }
    else if( strcasecmp( variable, "CONFIG_NAME" )==0 )
      {
      FreeIfAllocated( &( config->configName ) );
      config->configName = strdup( value );
      }
    else if( strcasecmp( variable, "BASE_DIR" )==0 )
      {
      FreeIfAllocated( &( config->baseDir ) );
      config->baseDir = strdup( value );
      }
    else if( strcasecmp( variable, "BACKUP_DIR" )==0 )
      {
      FreeIfAllocated( &( config->backupDir ) );
      config->backupDir = strdup( value );
      }
    else if( strcasecmp( variable, "DOWNLOAD_DIR" )==0 )
      {
      FreeIfAllocated( &( config->downloadDir ) );
      config->downloadDir = strdup( value );
      }
    else if( strcasecmp( variable, "LOG_FILE" )==0 )
      {
      FreeIfAllocated( &( config->logFile ) );
      config->logFile = strdup( value );
      }
    else if( strcasecmp( variable, "CGI_LOG_FILE" )==0 )
      {
      FreeIfAllocated( &( config->cgiLogFile ) );
      config->cgiLogFile = strdup( value );
      }
    else if( strcasecmp( variable, "DEFAULT_MINIMUM_CAPTURE_INTERVAL" )==0
             && atoi(value)>0 )
      {
      config->minimumIntervalSeconds  = atoi( value );
      }
    else if( strcasecmp( variable, "MINIMUM_CAPTURE_INTERVAL" )==0
             && atoi(value)>0 )
      {
      if( config->cameras==NULL )
        Error("%s cannot precede CAMERA in config", variable );
      config->cameras->minimumIntervalSeconds = atoi(value);
      }
    else if( strcasecmp( variable, "DEFAULT_MOTION_FRAMES" )==0
             && atoi(value)>0 )
      {
      config->motionFrames  = atoi( value );
      }
    else if( strcasecmp( variable, "STORE_PRE_MOTION" )==0 )
      {
      if( strcasecmp( value, "true" )==0
          || strcasecmp( value, "1" )==0
          || strcasecmp( value, "yes" )==0 )
        {
        config->storePreMotion  = 1;
        }
      else
        {
        config->storePreMotion  = 0;
        }
      }
    else if( strcasecmp( variable, "MOTION_FRAMES" )==0
             && atoi(value)>0 )
      {
      if( config->cameras==NULL )
        Error("%s cannot precede CAMERA in config", variable );
      config->cameras->motionFrames = atoi(value);
      }
    else if( strcasecmp( variable, "STORE_PRE_MOTION" )==0 )
      {
      if( config->cameras==NULL )
        Error("%s cannot precede CAMERA in config", variable );
      if( strcasecmp( value, "true" )==0
          || strcasecmp( value, "1" )==0
          || strcasecmp( value, "yes" )==0 )
        {
        config->cameras->storePreMotion = 1;
        }
      else
        {
        config->cameras->storePreMotion = 0;
        }
      }
    else if( strcasecmp( variable, "HUP_INTERVAL" )==0
             && atoi(value)>0 )
      {
      config->hup_interval = atoi(value);
      }
    else
      {
      /* append this variable to our linked list, for future expansion */
      /* do this always, so not here for just
         invalid commands:
         config->list = NewTagValue( variable, value, config->list, 1 );
      */
      }
    }
  }

void PrintConfig( FILE* f, _CONFIG* config )
  {
  if( f==NULL )
    {
    Error("Cannot print configuration to NULL file");
    }

  if( NOTEMPTY( config->configName ) )
    {
    fprintf( f, "CONFIG_NAME=%s\n", config->configName );
    }

  if( NOTEMPTY( config->baseDir ) )
    {
    fprintf( f, "BASE_DIR=%s\n", config->baseDir );
    }

  if( NOTEMPTY( config->backupDir ) )
    {
    fprintf( f, "BACKUP_DIR=%s\n", config->backupDir );
    }

  if( NOTEMPTY( config->downloadDir ) )
    {
    fprintf( f, "DOWNLOAD_DIR=%s\n", config->downloadDir );
    }

  if( config->fileCacheLength != FILE_CACHE_LENGTH )
    {
    fprintf( f, "FILES_TO_CACHE=%d\n", config->fileCacheLength );
    }

  if( NOTEMPTY( config->backupCommand ) )
    {
    fprintf( f, "BACKUP_COMMAND=%s\n", config->backupCommand );
    }

  if( NOTEMPTY( config->logFile ) )
    {
    fprintf( f, "LOG_FILE=%s\n", config->logFile );
    }

  if( NOTEMPTY( config->cgiLogFile ) )
    {
    fprintf( f, "CGI_LOG_FILE=%s\n", config->cgiLogFile );
    }

  if( config->color_diff_threshold != DEFAULT_COLOR_DIFF_THRESHOLD  )
    {
    fprintf( f, "DEFAULT_COLOR_DIFF_THRESHOLD=%d\n", config->color_diff_threshold );
    }

  if( config->color_dark != DEFAULT_COLOR_DARK  )
    {
    fprintf( f, "DEFAULT_COLOR_DARK=%d\n", config->color_dark );
    }

  if( config->dark_brightness_boost != DEFAULT_DARK_BRIGHTNESS_BOOST  )
    {
    fprintf( f, "DEFAULT_DARK_BRIGHTNESS_BOOST=%8.2lf\n", config->dark_brightness_boost );
    }

  if( config->despeckle_dark_threshold != DEFAULT_DESPECKLE_DARK_THRESHOLD )
    {
    fprintf( f, "DEFAULT_DESPECKLE_DARK_THRESHOLD=%d\n", config->despeckle_dark_threshold );
    }

  if( config->despeckle_nonbright_max != DEFAULT_DESPECKLE_NONBRIGHT_MAX )
    {
    fprintf( f, "DEFAULT_DESPECKLE_NONBRIGHT_MAX=%d\n", config->despeckle_nonbright_max );
    }

  if( config->despeckle_bright_threshold != DEFAULT_DESPECKLE_BRIGHT_THRESHOLD )
    {
    fprintf( f, "DEFAULT_DESPECKLE_BRIGHT_THRESHOLD=%d\n", config->despeckle_bright_threshold );
    }

  if( config->despeckle_nondark_min != DEFAULT_DESPECKLE_NONDARK_MIN )
    {
    fprintf( f, "DEFAULT_DESPECKLE_NONDARK_MIN=%d\n", config->despeckle_nondark_min );
    }

  if( config->checkerboard_square_size != DEFAULT_CHECKERBOARD_SQUARE_SIZE )
    {
    fprintf( f, "DEFAULT_CHECKERBOARD_SQUARE_SIZE=%d\n", config->checkerboard_square_size );
    }

  if( config->checkerboard_min_white != DEFAULT_CHECKERBOARD_MIN_WHITE )
    {
    fprintf( f, "DEFAULT_CHECKERBOARD_MIN_WHITE=%d\n", config->checkerboard_min_white );
    }

  if( config->checkerboard_num_white != DEFAULT_CHECKERBOARD_NUM_WHITE )
    {
    fprintf( f, "DEFAULT_CHECKERBOARD_NUM_WHITE=%d\n", config->checkerboard_num_white );
    }

  if( config->checkerboard_percent != DEFAULT_CHECKERBOARD_PERCENT )
    {
    fprintf( f, "DEFAULT_CHECKERBOARD_PERCENT=%8.2lf\n", config->checkerboard_percent );
    }

  if( config->minimumIntervalSeconds != MINIMUM_CAPTURE_INTERVAL )
    {
    fprintf( f, "DEFAULT_MINIMUM_CAPTURE_INTERVAL=%d\n", config->minimumIntervalSeconds );
    }

  if( config->motionFrames != MOTION_FRAMES )
    {
    fprintf( f, "DEFAULT_MOTION_FRAMES=%d\n", config->motionFrames );
    }

  if( config->storePreMotion != STORE_PRE_MOTION )
    {
    fprintf( f, "STORE_PRE_MOTION=%s\n",
             config->storePreMotion ? "true" : "false" );
    }

  if( config->hup_interval != DEFAULT_HUP_INTERVAL )
    {
    fprintf( f, "HUP_INTERVAL=%d\n", config->hup_interval );
    }

  fprintf(f, "# %d camera%s\n",
           config->nCameras,
           config->nCameras>1 ? "s" : ""
          );

  int i=0;
  for( _CAMERA* cam = config->cameras; cam!=NULL; cam=cam->next )
    {
    fprintf( f, "\n" );
    fprintf( f, "# Camera %d\n", i );
    fprintf( f, "CAMERA=%s\n", NULLPROTECT(cam->nickName) );
    fprintf( f, "COMMAND=%s\n", NULLPROTECT(cam->captureCommand) );
    if( cam->debug )
      {
      fprintf( f, "DEBUG=true\n" );
      }
    if( cam->minimumIntervalSeconds != config->minimumIntervalSeconds  )
      {
      fprintf( f, "MINIMUM_CAPTURE_INTERVAL=%d\n", cam->minimumIntervalSeconds );
      }
    if( cam->motionFrames != config->motionFrames )
      {
      fprintf( f, "MOTION_FRAMES=%d\n", cam->motionFrames );
      }
    if( cam->storePreMotion != config->storePreMotion )
      {
      fprintf( f, "STORE_PRE_MOTION=%s\n",
               cam->storePreMotion ? "true" : "false" );
      }

    if( config->color_diff_threshold != cam->color_diff_threshold )
      {
      fprintf( f, "COLOR_DIFF_THRESHOLD=%d\n", cam->color_diff_threshold );
      }

    if( config->color_dark != cam->color_dark )
      {
      fprintf( f, "COLOR_DARK=%d\n", cam->color_dark );
      }

    if( config->dark_brightness_boost != cam->dark_brightness_boost )
      {
      fprintf( f, "DARK_BRIGHTNESS_BOOST=%8.2lf\n", cam->dark_brightness_boost );
      }

    if( config->despeckle_dark_threshold != cam->despeckle_dark_threshold )
      {
      fprintf( f, "DESPECKLE_DARK_THRESHOLD=%d\n", cam->despeckle_dark_threshold );
      }

    if( config->despeckle_nonbright_max != cam->despeckle_nonbright_max )
      {
      fprintf( f, "DESPECKLE_NONBRIGHT_MAX=%d\n", cam->despeckle_nonbright_max );
      }

    if( config->despeckle_bright_threshold != cam->despeckle_bright_threshold )
      {
      fprintf( f, "DESPECKLE_BRIGHT_THRESHOLD=%d\n", cam->despeckle_bright_threshold );
      }

    if( config->despeckle_nondark_min != cam->despeckle_nondark_min )
      {
      fprintf( f, "DESPECKLE_NONDARK_MIN=%d\n", cam->despeckle_nondark_min );
      }

    if( config->checkerboard_square_size != cam->checkerboard_square_size )
      {
      fprintf( f, "CHECKERBOARD_SQUARE_SIZE=%d\n", cam->checkerboard_square_size );
      }

    if( config->checkerboard_min_white != cam->checkerboard_min_white )
      {
      fprintf( f, "CHECKERBOARD_MIN_WHITE=%d\n", cam->checkerboard_min_white );
      }

    if( config->checkerboard_num_white != cam->checkerboard_num_white )
      {
      fprintf( f, "CHECKERBOARD_NUM_WHITE=%d\n", cam->checkerboard_num_white );
      }

    if( config->checkerboard_percent != cam->checkerboard_percent )
      {
      fprintf( f, "CHECKERBOARD_PERCENT=%8.2lf\n", cam->checkerboard_percent );
      }

    if( config->minimumIntervalSeconds != cam->minimumIntervalSeconds )
      {
      fprintf( f, "DEFAULT_MINIMUM_CAPTURE_INTERVAL=%d\n", config->minimumIntervalSeconds );
      }

    ++i;
    }
  }

void ReadConfig( _CONFIG* config, char* filePath )
  {
  char folder[BUFLEN];
  folder[0] = 0;
  (void)GetFolderFromPath( filePath, folder, sizeof( folder )-1 );

  if( EMPTY( filePath ) )
    {
    Error( "Cannot read configuration file with empty/NULL name");
    }

  FILE* f = fopen( filePath, "r" );
  if( f==NULL )
    {
    Error( "Failed to open configuration file %s", filePath );
    }
  config->parserLocation = NewTagValue( filePath, "", config->parserLocation, 0 );
  config->parserLocation->iValue = 0;
  ++ ( config->currentlyParsing );

  /* this is wrong if we have #include's
  SetDefaults( config );
  */

  char buf[BUFLEN];
  char* endOfBuf = buf + sizeof(buf)-1;
  while( fgets(buf, sizeof(buf)-1, f )==buf )
    {
    ++(config->parserLocation->iValue);

    char* ptr = TrimHead( buf );
    TrimTail( ptr );

    while( *(ptr + strlen(ptr) - 1)=='\\' )
      {
      char* startingPoint = ptr + strlen(ptr) - 1;
      if( fgets(startingPoint, endOfBuf-startingPoint-1, f )!=startingPoint )
        {
        ++(config->parserLocation->iValue);
        break;
        }
      ++config->parserLocation->iValue;
      TrimTail( startingPoint );
      }

    if( *ptr==0 )
      {
      continue;
      }

    if( *ptr=='#' )
      {
      ++ptr;
      if( strncmp( ptr, "include", 7 )==0 )
        { /* #include */
        ptr += 7;
        while( *ptr!=0 && ( *ptr==' ' || *ptr=='\t' ) )
          {
          ++ptr;
          }
        if( *ptr!='"' )
          {
          Error("#include must be followed by a filename in \" marks.");
          }
        ++ptr;
        char* includeFileName = ptr;
        while( *ptr!=0 && *ptr!='"' )
          {
          ++ptr;
          }
        if( *ptr=='"' )
          {
          *ptr = 0;
          }
        else
          {
          Error("#include must be followed by a filename in \" marks.");
          }

        int redundantInclude = 0;
        for( _TAG_VALUE* i=config->includes; i!=NULL; i=i->next )
          {
          if( NOTEMPTY( i->tag ) && strcmp( i->tag, includeFileName )==0 )
            {
            redundantInclude = 1;
            break;
            }
          }

        if( redundantInclude==0 )
          {
          config->includes = NewTagValue( includeFileName, "included", config->includes, 1 );

          if( config->listIncludes )
            {
            if( config->includeCounter )
              {
              fputs( " ", stdout );
              }
            fputs( includeFileName, stdout );
            ++config->includeCounter;
            }

          char* confPath = SanitizeFilename( CONFIGDIR, NULL, includeFileName, 0 );
          if( FileExists( confPath )==0 )
            {
            ReadConfig( config, confPath );
            }
          else
            {
            confPath = SanitizeFilename( folder, NULL, includeFileName, 0 );
            if( FileExists( confPath )==0 )
              {
              ReadConfig( config, confPath );
              }
            else
              {
              Warning( "Cannot open #include \"%s\" -- skipping.",
                       confPath );
              }
            }
          }
        }
      else if( strncmp( ptr, "print", 5 )==0 )
        { /* #print */
        ptr += 5;
        while( *ptr!=0 && ( *ptr==' ' || *ptr=='\t' ) )
          {
          ++ptr;
          }
        if( *ptr!='"' )
          {
          Error("#include must be followed by a filename in \" marks.");
          }
        ++ptr;
        char* printFileName = ptr;
        while( *ptr!=0 && *ptr!='"' )
          {
          ++ptr;
          }
        if( *ptr=='"' )
          {
          *ptr = 0;
          }
        else
          {
          Error("#print must be followed by a filename in \" marks.");
          }

        FILE* printFile = fopen( printFileName, "w" );
        if( printFile==NULL )
          {
          Error( "Could not open/create %s to print configuration.",
                 printFileName );
          }
        PrintConfig( printFile, config );
        fclose( printFile );
        Notice( "Printed configuration to %s.  Exiting.", printFileName );
        exit(0);
        }

      /* not #include or #include completely read by now */
      continue;
      }

    /* printf("Processing [%s]\n", ptr ); */
    char* equalsChar = NULL;
    for( char* eolc = ptr; *eolc!=0; ++eolc )
      {
      if( equalsChar==NULL && *eolc == '=' )
        {
        equalsChar = eolc;
        }

      if( *eolc == '\r' || *eolc == '\n' )
        {
        *eolc = 0;
        break;
        }
      }

    if( *ptr!=0 && equalsChar!=NULL && equalsChar>ptr )
      {
      ProcessConfigLine( ptr, equalsChar, config );
      }
    }

  /* unroll the stack of config filenames after ReadConfig ended */
  _TAG_VALUE* tmp = config->parserLocation->next;
  if( config->parserLocation->tag!=NULL ) { FREE( config->parserLocation->tag ); }
  if( config->parserLocation->value!=NULL ) { FREE( config->parserLocation->value ); }
  FREE( config->parserLocation );
  config->parserLocation = tmp;

  -- ( config->currentlyParsing );

  fclose( f );

  /*
  This is wrong if we have #include's !
  FreeTagValue( config->list );
  config->list = NULL;
  */
  }

int CountCameras( _CONFIG* conf )
  {
  int n = 0;
  for( _CAMERA* cam=conf->cameras; cam!=NULL; cam=cam->next )
    {
    ++n;
    }
  return n;
  }

void PrintVariable( _CONFIG* config, char* varName )
  {
  if( EMPTY( varName ) )
    return;

  for( _TAG_VALUE* t=config->list; t!=NULL; t=t->next )
    {
    if( NOTEMPTY( t->tag )
        && strcmp( t->tag, varName )==0 )
      {
      if( NOTEMPTY( t->value ) )
        {
        fputs( t->value, stdout );
        }
      else
        {
        printf( "%d", t->iValue );
        }
      }
    }
  }

void ValidateConfig( _CONFIG* config )
  {
  if( config==NULL )
    {
    Error( "Cannot validate a NULL configuration" );
    }

  if( EMPTY( config->baseDir )!=0 )
    {
    Error( "BASE_DIR must be set in config file" );
    }
  if( IsFolderWritable( config->baseDir )!=0 )
    {
    Error( "BASE_DIR %s must be but is not writable", config->baseDir );
    }

  if( EMPTY( config->backupDir )!=0 )
    {
    Error( "BACKUP_DIR must be set in config file" );
    }
  if( IsFolderWritable( config->backupDir )!=0 )
    {
    Error( "BACKUP_DIR %s must be but is not writable", config->backupDir );
    }

  if( EMPTY( config->downloadDir )!=0 )
    {
    Error( "DOWNLOAD_DIR must be set in config file" );
    }
  if( IsFolderWritable( config->downloadDir )!=0 )
    {
    Error( "DOWNLOAD_DIR %s must be but is not writable", config->downloadDir );
    }

  if( config->nCameras<=0 || config->cameras==NULL )
    {
    Error( "Configuration file must define at least one camera." );
    }

  if( EMPTY( config->configName ) )
    {
    Error( "Configuration file must specify a name (CONFIG_NAME)" );
    }

  if( EMPTY( config->logFile ) )
    {
    Error( "Configuration file must specify a log file (LOG_FILE)" );
    }

  if( EMPTY( config->cgiLogFile ) )
    {
    Error( "Configuration file must specify a CGI log file (CGI_LOG_FILE)" );
    }
  }

