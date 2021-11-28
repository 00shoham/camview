#include "base.h"

#undef DEBUG
/* #define DEBUG 1 */

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

  FREE( cam );
  }

void CleanCameraFolder( _CAMERA* cam )
  {
  /* printf("CleanCameraFolder(%s)\n", cam->nickName);*/
  if( EMPTY( cam->folderPath ) )
    {
    Warning( "Cannot clean files from undefined folder for %s",
             NULLPROTECT( cam->nickName ) );
    return;
    }

  /* remove the jpg files */
  char** folder = NULL;
  int nFiles = GetOrderedDirectoryEntries(
                 cam->folderPath, NULL, ".jpg", &folder, 0 );
  for( int i=0; i<nFiles; ++i )
    {
    if( NOTEMPTY( folder[i] ) )
      {
      FileUnlink2( cam->folderPath, folder[i] );
      }
    }
  if( nFiles>0 )
    {
    FreeArrayOfStrings( folder, nFiles );
    }

  /* rename the log files */
  char *nowName = TimeStampFilename( 0 );
  if( nowName!=NULL )
    {
    if( FileExists2( cam->folderPath, "stderr.log" )==0 )
      {
      char newname[BUFLEN];
      snprintf( newname, sizeof(newname)-1, "%s.stderr.log", nowName );
      (void)FileRename2( cam->folderPath, "stderr.log", newname );
      }
    if( FileExists2( cam->folderPath, "stdout.log" )==0 )
      {
      char newname[BUFLEN];
      snprintf( newname, sizeof(newname)-1, "%s.stdout.log", nowName );
      (void)FileRename2( cam->folderPath, "stdout.log", newname );
      }
    free( nowName );
    nowName = NULL;
    }
  }

void KillCameraProcess( _CAMERA* cam )
  {
  /* printf("KillCameraProcess(%s)\n", cam->nickName); */
  Warning("Stopping process %d on camera %s", cam->childProcess, cam->nickName );
  if( cam->childProcess
      && ProcessExistsAndIsMine( cam->childProcess ) == 0 )
    {
    (void)kill( cam->childProcess, SIGHUP );
    }
  else
    Warning("Process %d on camera %s no longer active", cam->childProcess, cam->nickName );

  cam->childProcess = 0;
  cam->launchTime = 0;
  cam->lastImageTime = 0;
  cam->largestFile = 0;
  cam->launchAttempts = 0;
  cam->lastImageCount = 0;

  KillExistingCommandInstances( cam->captureCommand );

  CleanCameraFolder( cam );

  sleep(1);
  }

_CONFIG* glob_conf=NULL;
void StopCaptureProcesses()
  {
  if( glob_conf==NULL )
    return;
  if( glob_conf->cameras==NULL )
    return;
  if( glob_conf->nCameras<=0 )
    return;

  for( _CAMERA* cam=glob_conf->cameras; cam!=NULL; cam=cam->next )
    {
    if( cam->childProcess>0 )
      {
      KillCameraProcess( cam );
      }
    }
  }

void TerminateMonitor( int signo )
  {
  if( glob_conf!=NULL
      && glob_conf->logFileHandle!=NULL )
    {
    Warning("Caught signal %d - terminating.", signo);
    }
  StopCaptureProcesses();

  exit(1);
  }

void PingCameras( int signo )
  {
  Notice("SIGHUP or timeout.  Will try to reactivate non-functional cameras.");
  if( glob_conf==NULL )
    return;
  if( glob_conf->cameras==NULL )
    return;
  if( glob_conf->nCameras<=0 )
    return;

  for( _CAMERA* cam=glob_conf->cameras; cam!=NULL; cam=cam->next )
    {
    if( cam->childProcess<=0 )
      {
      cam->launchAttempts = 0;
      cam->launchTime = 0;
      cam->lastImageCount = 0;
      cam->lastImageTime = 0;
      cam->largestFile = 0;
      cam->motionCountdown = 0;
      }
    }

  int n = LaunchAllCameras( glob_conf );
  Notice("Checked all capture processes.  Launched %d new processes.", n );
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
    Error( "Failed to parse cmd line [%s] for camera %s",
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

void CameraBackupFolder( _CONFIG* config, _CAMERA* cam )
  {
  if( EMPTY( config->backupDir ) )
    {
    Error( "Configuration file must specify BACKUPDIR" );
    }

  EnsureDirExists( config->backupDir );

  char camFolder[BUFLEN];
  snprintf( camFolder, sizeof(camFolder)-1, "%s/%s",
            config->backupDir, cam->nickName );

  EnsureDirExists( camFolder );

  if( cam->backupFolderPath==NULL )
    {
    cam->backupFolderPath = strdup( camFolder );
    }

  }

void CameraFolder( _CONFIG* config, _CAMERA* cam )
  {
  if( EMPTY( config->baseDir ) )
    {
    Error( "Configuration file must specify BASEDIR" );
    }

  EnsureDirExists( config->baseDir );

  char camFolder[BUFLEN];
  snprintf( camFolder, sizeof(camFolder)-1, "%s/%s",
            config->baseDir, cam->nickName );

  EnsureDirExists( camFolder );

  if( cam->folderPath==NULL )
    {
    cam->folderPath = strdup( camFolder );
    }

  if( chdir( camFolder )!=0 )
    {
    Error( "Failed to chdir into %s - %d:%s", camFolder, errno, strerror(errno) );
    }
  }

void WritePIDFile( _CAMERA* cam )
  {
  if( cam==NULL )
    return;

  if( EMPTY( cam->nickName ) || EMPTY( cam->folderPath ) )
    return;

  if( cam->childProcess<=0 )
    return;

  char buf[BUFLEN];
  snprintf( buf, sizeof(buf)-1, "%s/%s.pid",
            cam->folderPath, cam->nickName );
  FILE* f = fopen( buf, "w" );
  if( f==NULL )
    return;

  fprintf( f, "%d\n", cam->childProcess );

  fclose( f );
  }

pid_t LaunchCapture( _CONFIG* config, _CAMERA* cam )
  {
  /* check config first - is what we're trying sufficiently defined? */
  if( config==NULL )
    Error("Cannot launch capture with NULL config");
  if( cam==NULL )
    Error("Cannot launch capture of NULL camera");
  if( EMPTY( config->baseDir ) )
    Error("Cannot launch capture without specifying a working folder");
  if( EMPTY( cam->nickName ) )
    Error("Cannot launch capture of a camera with no name");
  if( EMPTY( cam->captureCommand ) )
    Error("Cannot launch capture of a camera with no capture command");

  Notice( "LaunchCapture on %s", cam->nickName );

  if( cam->childProcess )
    {
    /* do we already have one?  if so, kill it first. */
    if( ProcessExistsAndIsMine( cam->childProcess )==0 )
      {
      Notice("LaunchCapture( %s ) -- process already exists", cam->nickName );
      int err = 0;

      err = kill( cam->childProcess, SIGHUP );
      if( err==0 )
        {
        sleep(1);
        if( ProcessExistsAndIsMine( cam->childProcess )==0 )
          { /* needs a more brutal kill */
          Warning( "LaunchCapture( %s ) -- kill -1 not enough", cam->nickName );
          err = kill( cam->childProcess, SIGKILL );
          sleep(1);
          if( ProcessExistsAndIsMine( cam->childProcess )==0 )
            {
            Warning( "LaunchCapture( %s ) -- kill -9 not enough!!", cam->nickName );
            }
          }
        }
      else
        {
        Warning( "LaunchCapture( %s ) -- failed to kill -HUP existing child", cam->nickName );
        }
      }
    else
      {
      Notice( "LaunchCapture on %s - no legacy process.", cam->nickName );
      }
    }

  cam->childProcess = 0;
  cam->lastImageTime = 0;

  if( EMPTY( cam->ipAddress ) )
    {
    /* printf("Trying to get IP address for %s\n", cam->nickName ); */
    char ipAddr[100];
    int err = IPAddressFromCommand( ipAddr, sizeof(ipAddr), cam->captureCommand );
    if( err==0 && NOTEMPTY( ipAddr ) )
      {
      cam->ipAddress = strdup( ipAddr );
      }
    }

  if( NOTEMPTY( cam->ipAddress ) )
    {
    /* printf("Trying to ping address for %s\n", cam->nickName ); */
    int err = PingAddress( cam->ipAddress );
    if( err!=0 )
      {
      Warning( "Camera %s has IP address %s but cannot be reached by ping.",
               cam->nickName, cam->ipAddress );
      return -1;
      }
    }

  /* printf("Ensuring there is a backup folder for %s\n", cam->nickName ); */
  CameraBackupFolder( config, cam );
  /* printf("Ensuring there is a working folder for %s\n", cam->nickName ); */
  CameraFolder( config, cam );
  /* printf("Cleaning the working folder for %s\n", cam->nickName ); */
  CleanCameraFolder( cam );

  /* printf("forking %s\n", cam->nickName ); */
  Notice("Forking and running %s", cam->captureCommand );

  KillExistingCommandInstances( cam->captureCommand );

  pid_t child = fork();

  if( child<0 )
    {
    Warning("Failed to fork a child to monitor camera %s", cam->nickName);
    return -2;
    }

  if( child==0 ) /* in child */
    {
    SwapInProcess( cam->debug, cam->debug, cam->nickName, cam->captureCommand );

    /* should never reach this */
    }

  Notice("Forked PID %d to monitor camera %s", (int)child, cam->nickName);
  cam->childProcess = child;
  cam->launchTime = time(NULL);
  cam->launchAttempts = 0;
  WritePIDFile( cam );
  if( chdir( config->baseDir )!=0 )
    {
    Warning( "Failed to chdir to %s - %d:%s", config->baseDir,
             errno, strerror( errno ) );
    }

  return child;
  }

int CheckConfigForValidCameras( _CONFIG* config )
  {
  if( config==NULL )
    {
    Warning( "No config - cannot launch cameras");
    return -1;
    }
  if( config->cameras==NULL )
    {
    Warning( "No cameras (NULL) - cannot launch cameras");
    return -2;
    }
  if( config->nCameras<1 )
    {
    Warning( "No cameras (%d) - cannot launch cameras", config->nCameras);
    return -3;
    }
  if( EMPTY( config->baseDir ) )
    {
    Warning( "No baseDir - cannot launch cameras", config->nCameras);
    return -4;
    }

  return 0;
  }

int LaunchAllCameras( _CONFIG* config )
  {
  Notice("LaunchAllCameras()" );
  int err = CheckConfigForValidCameras( config );
  if( err<0 )
    return err;

  int nLaunched = 0;
  for( _CAMERA* cam=config->cameras; cam!=NULL;  )
    {
    /* printf("Trying to launch camera %s\n", cam->nickName ); */
    pid_t p = cam->childProcess;
    if( p>0 && ProcessExistsAndIsMine( cam->childProcess )==0 )
      { /* child already running.  do nothing. */
      cam = cam->next;
      }
    else
      { /* no valid child - try to launch. */
      p = LaunchCapture( config, cam );
      if( p<=0 )
        {
        /* failed.  perhaps try again? */
        Warning( "Did not manage to launch process for camera %s on attempt %d",
                 NULLPROTECT( cam->nickName ),
                 cam->launchAttempts );
        ++ cam->launchAttempts;
        if( cam->launchAttempts >= MAX_LAUNCH_ATTEMPTS )
          { /* too many tries - skip this camera */
          Warning( "Reached maximum attempts to launch capture for %s.",
                   NULLPROTECT( cam->nickName ) );
          cam=cam->next;
          }
        else
          { /* but not immediately ... */
          sleep(1);
          }
        }
      else
        {
        ++nLaunched;
        cam=cam->next;
        }
      }
    }

  return nLaunched;
  }

void StoreImage( int preLocked,
                 _CONFIG* config, _CAMERA* cam, char* fileName, int size, int deltaTime )
  {
  if( ! preLocked )
    pthread_mutex_lock( &(cam->lock) );

  /*
  Notice( "StoreImage( ..., %d, %s, %s, %d, %d )",
          preLocked, cam->nickName, fileName, size, deltaTime );
  */
  char *nowName = TimeStampFilename( deltaTime );
  /* printf("... nowName = [%s]\n", nowName ); */
  if( nowName!=NULL )
    {
    char newName[BUFLEN];
    snprintf( newName, sizeof(newName)-1, "image-%s.jpg", nowName );
    int err = FileCopy2( cam->folderPath, fileName,
                         cam->backupFolderPath, newName );
    if( err )
      {
      Warning("Failed to copy %s/%s to %s/%s", cam->folderPath, fileName,
                                               cam->backupFolderPath, newName );
      }
    /* printf("... copied %s to %s (return=%d)\n", fileName, newName, err ); */
    FreeIfAllocated( &(cam->lastStoredImage) );
    cam->lastStoredImage = strdup( newName );
    cam->lastImageTime = time(NULL);
    if( size > cam->largestFile )
      {
      cam->largestFile = size;
      }

    FREE( nowName );
    char relName[BUFLEN+100]; /* size is just to suppress compiler warning */
    snprintf( relName, sizeof(relName)-1, "%s/%s", cam->nickName, newName );
    if( NOTEMPTY( config->backupCommand ) )
      {
      config->filesToBackup = NewFilename( relName, config->filesToBackup );
      ++ config->nFilesToBackup;
      if( config->nFilesToBackup >= config->fileCacheLength )
        {
        BackupFiles( config->backupDir, config->filesToBackup, config->backupCommand );
        FreeFilenames( config->filesToBackup );
        config->filesToBackup = NULL;
        config->nFilesToBackup = 0;
        }
      }
    }

  if( ! preLocked )
    pthread_mutex_unlock( &(cam->lock) );
  }

void StoreImageIfDifferent( int preLocked,
                            _CONFIG* config,
                            _CAMERA* cam,
                            _IMAGE* imageA,
                            _IMAGE* imageB )
  {
  double motionPercent = 0;

  if( imageA==NULL
      || imageB==NULL
      || imageA->data==NULL
      || imageB->data==NULL
      || imageA->fileName==NULL
      || imageB->fileName==NULL )
    return;

#ifdef DEBUG
    Notice( "Testing whether %s/%s and %s differ",
            cam->nickName, imageA->fileName, imageB->fileName );
#endif

  int motion = HasImageChanged( 0,
                                cam->nickName, imageA, imageB,
                                cam->color_dark,
                                cam->dark_brightness_boost,
                                cam->color_diff_threshold,
                                cam->despeckle_dark_threshold,
                                cam->despeckle_nondark_min,
                                cam->despeckle_bright_threshold,
                                cam->despeckle_nonbright_max,
                                cam->checkerboard_square_size,
                                cam->checkerboard_min_white,
                                cam->checkerboard_num_white,
                                cam->checkerboard_percent,
                                &motionPercent,
                                NULL,
                                NULL );

#ifdef DEBUG
    Notice( "Compared %s / %s to %s - %8.4lf - %s",
            cam->nickName, imageA->fileName, imageB->fileName, motionPercent,
            motion==0 ? "MOTION" : "STATIC" );
#endif

  if( motion==0 )
    {
    if( cam->storePreMotion )
      {
      /* Notice("Storing pre-motion image %s/%s", cam->nickName, imageB->fileName ); */
      StoreImage( preLocked, config, cam, imageB->fileName, FileSize2( cam->folderPath, imageB->fileName), -1 );
      }
#ifdef DEBUG
    Notice("Storing image due to motion (%s/%s - %8.2lf)", cam->nickName, imageA->fileName, motionPercent );
#endif
    StoreImage( preLocked, config, cam, imageA->fileName, FileSize2( cam->folderPath, imageA->fileName), 0 );
    cam->motionCountdown = cam->motionFrames;
    return;
    }
  /* printf("... motion detection = %d\n", motion);*/
  }

_IMAGE* FreePrevImage( _IMAGE* prevImage, _CAMERA* cam )
  {
  if( prevImage == cam->recentImage )
    { /* do not double-free! */
    FreeImage( &prevImage );
    cam->recentImage = NULL;
    }
  else
    {
    FreeImage( &prevImage );
    FreeImage( &(cam->recentImage) );
    }

  return NULL;
  }

typedef struct _NewImageData
  {
  _CONFIG* config;
  _CAMERA* cam;
  char* fileName;
  char* prevFile;
  } _NEWIMAGEDATA;

void* ProcessNewImageInThread( void* params )
  {
  if( params==NULL )
    return NULL;

  _NEWIMAGEDATA* tparams = (_NEWIMAGEDATA*)params;

  _CONFIG* config = tparams->config;
  _CAMERA* cam = tparams->cam;
  char* fileName = tparams->fileName;
  char* prevFile = tparams->prevFile;

  FREE( tparams );

  if( config==NULL
      || cam==NULL
      || EMPTY( cam->nickName )
      || EMPTY( fileName ) )
    {
    FREEIFNOTNULL( fileName );
    FREEIFNOTNULL( prevFile );
    return NULL;
    }

#ifdef DEBUG
  Notice( "ProcessNewImageInThread(%s)", cam->nickName);
#endif

  pthread_mutex_lock( &(cam->lock) );

  /* don't process the same file twice */
  if( NOTEMPTY( cam->lastImageSourceName )
      && strcmp( cam->lastImageSourceName, fileName )==0 )
    {
#ifdef DEBUG
  Notice( "Skip reprocessing image %s in camera %s", fileName, cam->nickName );
#endif
    ++ cam->lastImageCount;
    if( cam->lastImageCount >= IMAGE_TOO_OLD )
      {
      Notice( "Second-last image in %s folder (%s) keeps coming up (%d times).",
              cam->nickName, fileName, cam->lastImageCount );
      KillCameraProcess( cam );
      }

    goto end;
    }

  /* remember what file we were working on, to avoid dups (as above) */
  FreeIfAllocated( &(cam->lastImageSourceName) );
  cam->lastImageSourceName = strdup( fileName );
  cam->lastImageCount = 0;

  int size = (int)FileSize2( cam->folderPath, fileName );
  if( size < TINY_IMAGE )
    {
    Warning( "Image %s for camera %s too small (%d versus previous %d)",
             fileName, cam->nickName, size, cam->largestFile );
    goto end;
    }

  /* first image */
  if( EMPTY( cam->lastStoredImage ) )
    {
    Notice( "Captured first image for %s - %s", cam->nickName, fileName );
    StoreImage( 1, config, cam, fileName, size, 0 );
    goto end;
    }

  /* recent motion, storing due to countdown rule */
  if( cam->motionCountdown )
    {
    -- cam->motionCountdown;
#ifdef DEBUG
    Notice("Storing image due to countdown (%s/%s - %d)", cam->nickName, fileName, cam->motionCountdown );
#endif
    StoreImage( 1, config, cam, fileName, size, 0 );
    goto end;
    }

  /* too much time since we last stored an image? */
  int age = time(NULL) - cam->lastImageTime;
  if( age >= cam->minimumIntervalSeconds )
    {
#ifdef DEBUG
    Notice("Storing image due to timeout (%s/%s - %d)", cam->nickName, fileName, age );
#endif
    StoreImage( 1, config, cam, fileName, size, 0 );
    goto end;
    }

  /* read image, check if it's too dark? */
#ifdef DEBUG
  Notice( "Reading JPEG file for %s - %s / %s", cam->nickName, cam->folderPath, fileName );
#endif
  _IMAGE* image = ImageFromJPEGFile2( cam->nickName, cam->folderPath, fileName );

  if( image==NULL )
    {
    Warning( "Failed to get JPEG file for %s - %s / %s", cam->nickName, cam->folderPath, fileName );
    goto end;
    }

  int luminosity = AverageImageLuminosity( image );
  if( luminosity <= BLACK_IMAGE_MAX_LUMINOSITY )
    {
#ifdef DEBUG
  Notice( "Image from camera %s is too dark.", cam->nickName );
#endif
    /* keep the image around though - we might want to
       compare against it later */
    FreePrevImage( NULL, cam );
    cam->recentImage = image;
    goto end;
    }

  /* look for motion: */
#ifdef DEBUG
  Notice( "Motion detection on camera %s required", cam->nickName );
#endif

  _IMAGE* prevImage = NULL;
  if( NOTEMPTY( prevFile )
      && cam->recentImage!=NULL
      && NOTEMPTY( cam->recentImage->fileName )
      && strcmp( cam->recentImage->fileName, prevFile )==0 )
    { /* use the image file we've already got from last time */
#ifdef DEBUG
    Notice( "Using previously parsed JPEG (%s/%s)", cam->nickName, cam->recentImage->fileName );
#endif
    prevImage = cam->recentImage;
    }
  else
    { /* free the cached image file - it's no good */
    FreeImage( &(cam->recentImage) );
#ifdef DEBUG
    Notice( "Parsing JPEG for previous image from %s - %s / %s", cam->nickName, cam->folderPath, prevFile );
#endif
    prevImage = ImageFromJPEGFile2( cam->nickName, cam->folderPath, prevFile );
    }

  if( prevImage==NULL || prevImage->data==NULL )
    { /* for some reason we have a bad previous image */
    Warning( "Previous image NULL or empty.  Storing current image %s.");
    prevImage = FreePrevImage( prevImage, cam );
    StoreImage( 1, config, cam, fileName, size, 0 );
    }
  else
    {
    StoreImageIfDifferent( 1, config, cam, image, prevImage );
    prevImage = FreePrevImage( prevImage, cam );
    }

  /* keep the image we just looked at for future reference */
  /* Notice( "Keeping image (%s/%s)", cam->nickName, image->fileName ); */
  cam->recentImage = image;

  end:
  FREEIFNOTNULL( fileName );
  FREEIFNOTNULL( prevFile );

  pthread_mutex_unlock( &(cam->lock) );
  return NULL;
  }

void ProcessNewImage( _CONFIG* config, _CAMERA* cam,
                      char* fileName, char* prevFile )
  {
  if( config==NULL )
    return;
  if( cam==NULL || EMPTY( cam->nickName ) )
    return;
  if( fileName==NULL )
    return;

#ifdef DEBUG
  Notice( "ProcessNewImage( <conf> , %s, %s, %s, HMDT=%d )",
          cam->nickName, NULLPROTECT( fileName ),
          NULLPROTECT( prevFile ), cam->haveMotionDetectThread );
#endif

  if( cam->haveMotionDetectThread )
    {
    pthread_join( cam->motionDetectThread, NULL );
    if( cam->threadParams!=NULL )
      {
      FREEIFNOTNULL( cam->threadParams->fileName );
      FREEIFNOTNULL( cam->threadParams->prevFile );
      }
    FREEIFNOTNULL( cam->threadParams );
    cam->haveMotionDetectThread = 0;
    }

  _NEWIMAGEDATA* tparams = (_NEWIMAGEDATA*)calloc( 1, sizeof(_NEWIMAGEDATA) );
  tparams->config = config;
  tparams->cam = cam;
  tparams->fileName = SAFESTRDUP( fileName );
  tparams->prevFile = SAFESTRDUP( prevFile );

  int err = pthread_create( &(cam->motionDetectThread),
                            NULL,
                            ProcessNewImageInThread,
                            (void*)tparams
                          );
  /* QQQ
  (void)ProcessNewImageInThread( (void*)tparams );
  int err = 0;
  */

  if( err==0 )
    {
#ifdef DEBUG
    Warning( "Created motion detection thread for camera %s", cam->nickName );
#endif
    cam->haveMotionDetectThread = 1;
    }
  else
    {
    Warning( "Failed to create worker thread - %d", err );
    if( cam->threadParams!=NULL )
      {
      FREEIFNOTNULL( cam->threadParams->fileName );
      FREEIFNOTNULL( cam->threadParams->prevFile );
      }
    FREEIFNOTNULL( cam->threadParams );
    }
  }

void ScanFolderForNewFiles( _CONFIG* config, _CAMERA* cam )
  {
  if( EMPTY( cam->folderPath ) )
    {
    Warning("Empty folder path for camera [%s]", NULLPROTECT( cam->nickName ) );
    return;
    }

  char** folder = NULL;
  int nFiles = GetOrderedDirectoryEntries( cam->folderPath, "test-image-", ".jpg", &folder, 1 );
#ifdef DEBUG
  Notice( "Folder %s has %d files", cam->folderPath, nFiles );
#endif
  if( nFiles<0 )
    {
    return;
    }
  else if( nFiles==0 )
    {
    int nSeconds = time(NULL) - cam->launchTime;
    if( nSeconds < SETTLING_TIME )
      {
      /* pretty noisy */
      /* Notice( "Camera %s still has no image, will wait a bit longer.", cam->nickName ); */
      }
    else
      {
      Warning( "Camera %s not generating images.", cam->nickName );
      KillCameraProcess( cam );
      }
    }
  else if( nFiles==1 )
    {
    time_t t = FileDate2( cam->folderPath, NULLPROTECT(folder[0]) );
    if( t<=0 )
      {
      Warning( "Camera %s file %s has weird date.",
               cam->nickName, NULLPROTECT(folder[0]));
      KillCameraProcess( cam );
      }
    else /* file has a believable timestamp */
      {
      int fileAge = time(NULL) - t;
      if( fileAge > SETTLING_TIME )
        { /* file is too old.  process died? */
        Warning( "Single file in %s folder is too old (file %s age %d).", cam->nickName, folder[0], fileAge );
        KillCameraProcess( cam );
        }
      else
        { /* file age is reasonable. wait for more files. */
        Notice( "Single file in %s folder - waiting for more.", cam->nickName );
        }
      }
    }
  else if( nFiles>=2 )
    {
    char* fileName = folder[nFiles-2];

    /* how old is this image? */
    time_t t = FileDate2( cam->folderPath, NULLPROTECT(fileName) );
    if( t>0 )
      {
      int fileAge = time(NULL) - t;
      if( fileAge > SETTLING_TIME )
        { /* file is too old.  process died? */
        Notice( "Second-last image in %s folder (%s) is too old (%d).",
                cam->nickName, fileName, fileAge );
        /* the test above seems to return bad data, thus causing
           process churn for no good reason */
        /* KillCameraProcess( cam ); */
        }
      }

    if( nFiles>TOO_MANY_FILES  )
      {
      if( cam!=NULL && cam->debug )
        {
        Notice( "Removing %d old images under %s", nFiles - FILES_TO_KEEP, cam->nickName );
        }
      for( int j=0; j < (nFiles-FILES_TO_KEEP); ++j )
        {
        (void)FileUnlink2( cam->folderPath, folder[j] );
        }
      }

    if( (nFiles-2)>0 ) /* this image was not the first */
      {
      /* printf("Processing %s; prev image is %s\n", fileName, folder[nFiles-3]); */
      ProcessNewImage( config, cam, fileName, folder[nFiles-3] );
      }
    else
      {
      /* printf("Processing %s; no prev image\n", fileName); */
      ProcessNewImage( config, cam, fileName, NULL );
      }
    }
  else /* wtf? */
    {
    Warning( "Unexpected number of files in folder for %s - %d",
             cam->nickName, nFiles );
    }

  FreeArrayOfStrings( folder, nFiles );
  }

int MonitorSingleCamera( _CONFIG* config, _CAMERA* cam )
  {
  if( EMPTY( cam->nickName ) )
    {
    Warning("Cannot monitor a camera with no nickname");
    return -1;
    }

  if( cam->childProcess<=0 )
    {
    if( cam->launchAttempts < MAX_LAUNCH_ATTEMPTS )
      {
      Warning("Camera %s has no child process.  Attempting (re)launch.", cam->nickName);
      pid_t p = LaunchCapture( config, cam );
      if( p<=0 )
        {
        /* try again */
        Warning( "Did not manage to launch process for camera %s on attempt %d",
                 NULLPROTECT( cam->nickName ),
                 cam->launchAttempts );
        ++ cam->launchAttempts;
        }
      /* launched new child */
      }
    else
      {
      /* Warning("No child process for %s, not attempting again.", cam->nickName ); */
      return -2;
      }
    }

  int procExists = (cam->childProcess <= 0) ?
                   -999 : ProcessExistsAndIsMine( cam->childProcess );

  if( cam->childProcess>0
      && procExists==0
      && NOTEMPTY( cam->folderPath ) )
    {
    ScanFolderForNewFiles( config, cam );
    }
  else
    {
    Warning( "MonitorSingleCamera() problem.  pid=%d, proc=%d, path=%s",
             cam->childProcess, procExists,
             NULLPROTECT(cam->folderPath) );
    KillCameraProcess( cam );
    }

  return 0;
  }

/* should not really return */
void MonitorCameras( _CONFIG* config )
  {
  time_t timeLastCamerasCheck = 0;

  int err = CheckConfigForValidCameras( config );
  if( err<0 )
    {
    Error("No config or no valid camera.  Aborting.");
    }

  if( EMPTY( config->backupDir ) )
    {
    Error("Must specify BACKUPDIR in config.  Aborting.");
    }

  int iteration = 0;
  for(;;)
    {
    time_t tStart = time(NULL);

    if( (iteration % 60)==0 )
      {
      Notice("Camera scan loop (%d)", iteration);
      }
    ++iteration;

    int nGood = 0;
    for( _CAMERA* cam=config->cameras; cam!=NULL; cam=cam->next )
      {
      err = MonitorSingleCamera( config, cam );
      if( err==0 )
        {
        ++nGood;
        }
      }

    if( nGood==0 )
      {
      Error("No cameras are in a healthy state.  Aborting.");
      }

    /* avoid busy loop */
    time_t tnow = time(NULL);
    if( tnow - timeLastCamerasCheck > config->hup_interval )
      {
      timeLastCamerasCheck = tnow;
      PingCameras( SIGHUP );
      }
    else
      {
      if( tnow - tStart < 1 )
        {
        sleep(1);
        }
      }
    }
  }

_CAMERA* FindCamera( _CONFIG* conf, char* id )
  {
  if( EMPTY( id ) )
    {
    Warning( "Cannot find camera without specifying an ID" );
    return NULL;
    }

  if( conf==NULL )
    {
    Warning( "Cannot find camera without config" );
    return NULL;
    }

  if( conf->cameras==NULL )
    {
    Warning( "Cannot find camera when there are none" );
    return NULL;
    }

  for( _CAMERA* cam=conf->cameras; cam!=NULL; cam=cam->next )
    {
    if( NOTEMPTY( cam->nickName )
        && strcasecmp( cam->nickName, id )==0 )
      {
      return cam;
      }
    }

  Warning( "Camera %s not found.", id );
  return NULL;
  }

/* 0 = yes */
int ImageFilenameInDateRange( char* name,
                              char* fromDate,
                              char* fromTime,
                              char* toDate,
                              char* toTime )
  {
  if( strncmp( name, "image-", 6 )!=0 )
    return -1;
  if( !isdigit( name[6] )
      || !isdigit( name[7] )
      || !isdigit( name[8] )
      || !isdigit( name[9] ) )
    return -2; /* year is wrong */
  if( !isdigit( name[11] )
      || !isdigit( name[12] ) )
    return -3; /* month is wrong */
  if( !isdigit( name[14] )
      || !isdigit( name[15] ) )
    return -4; /* DoM is wrong */
  if( !isdigit( name[17] )
      || !isdigit( name[18] ) )
    return -5; /* hour is wrong */
  if( !isdigit( name[20] )
      || !isdigit( name[21] ) )
    return -6; /* minute is wrong */
  if( !isdigit( name[23] )
      || !isdigit( name[24] ) )
    return -7; /* second is wrong */

  char fileDate[20];
  fileDate[0] = name[6];
  fileDate[1] = name[7];
  fileDate[2] = name[8];
  fileDate[3] = name[9];
  fileDate[4] = '-';
  fileDate[5] = name[11];
  fileDate[6] = name[12];
  fileDate[7] = '-';
  fileDate[8] = name[14];
  fileDate[9] = name[15];
  fileDate[10] = 0;

  char fileTime[20];
  fileTime[0] = name[17];
  fileTime[1] = name[18];
  fileTime[2] = ':';
  fileTime[3] = name[20];
  fileTime[4] = name[21];
  fileTime[5] = ':';
  fileTime[6] = name[23];
  fileTime[7] = name[24];
  fileTime[8] = 0;

  /* image-2020-10-18_15-17-28.jpg */
  /* 01234567890123456789012345678 */
  /* 0         1         2         */

  if( strcmp( fromDate, fileDate )>0 ) /* fromDate > fileDate - too old */
    return -1;

  if( strcmp( fileDate, toDate )>0 ) /* fileDate > toDate - too new */
    return -2;

  if( strcmp( fromDate, fileDate )==0
      && strcmp( fromTime, fileTime )>0 ) /* too old */
    return -3;

  if( strcmp( fileDate, toDate )==0
      && strcmp( fileTime, toTime )>0 ) /* too late */
    return -4;

  return 0;
  }

/* ensure that cam->backupFolderPath is defined and exists first! */
int FindMatchingImages( _CAMERA* cam,
                        char* fromDate, char* fromTime,
                        char* toDate, char* toTime,
                        char*** fileNames )
  {
  if( cam==NULL ) Error("FindMatchingImages - must specify camera");
  if( EMPTY( fromDate ) ) Error("FindMatchingImages - must specify fromDate");
  if( EMPTY( fromTime ) ) Error("FindMatchingImages - must specify fromTime");
  if( EMPTY( toDate ) ) Error("FindMatchingImages - must specify toDate");
  if( EMPTY( toTime ) ) Error("FindMatchingImages - must specify toTime");
  if( fileNames==NULL ) Error("FindMatchingImages - must specify output array");
  if( EMPTY( cam->backupFolderPath ) )
    {
    Error( "Camera %s has no backup folder path", NULLPROTECT( cam->nickName ) );
    }

  DIR* d = opendir( cam->backupFolderPath );
  if( d==NULL )
    Error( "Cannot list contents of %s", cam->backupFolderPath );

  int nEntries = 0;
  struct dirent * de;
  while( (de=readdir( d ))!=NULL )
    {
    if( NOTEMPTY( de->d_name )
        && StringEndsWith( de->d_name, ".jpg", 0 )==0
        && ImageFilenameInDateRange( de->d_name, fromDate, fromTime, toDate, toTime )==0 )
      {
      ++nEntries;
      }
    }
  closedir( d );

  char** array = (char**)calloc( nEntries, sizeof(char*) );
  if( fileNames!=NULL )
    {
    *fileNames = array;
    }
  int entryNum = 0;
  d = opendir( cam->backupFolderPath );
  if( d==NULL )
    Error( "Cannot list contents of %s", cam->backupFolderPath );
  while( entryNum<nEntries
         && (de=readdir( d ))!=NULL )
    {
    if( NOTEMPTY( de->d_name )
        && StringEndsWith( de->d_name, ".jpg", 0 )==0
        && ImageFilenameInDateRange( de->d_name, fromDate, fromTime, toDate, toTime )==0 )
      {
      array[entryNum] = strdup( de->d_name );
      ++entryNum;
      }
    }
  closedir( d );

  qsort( array, nEntries, sizeof( char* ), CompareStrings );

  return nEntries;
  }
