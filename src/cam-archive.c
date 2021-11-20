#include "base.h"

extern int glob_argc;
extern char** glob_argv;
extern _CONFIG* glob_conf;

char* DateString( int deltaDays )
  {
  time_t t = time(NULL);
  t += deltaDays * SECONDS_PER_DAY;
  char buf[100];
  return strdup( DateStr( t, buf, sizeof(buf)-1 ) );
  }

int CountImagesSingleCamera( _CAMERA* cam, char* when )
  {
  if( cam==NULL )
    Error( "Cannot count images for NULL camera");
  if( EMPTY( cam->nickName ) )
    Error( "Cannot count images for camera without name");
  if( EMPTY( when ) )
    Error( "Cannot count images without date string");
  if( EMPTY( cam->backupFolderPath ) )
    Error( "Cannot count images for camera %s without backup folder", cam->nickName);
  if( DirExists( cam->backupFolderPath )!=0 )
    Error( "Cannot count images in missing folder %s", cam->backupFolderPath );
  if( chdir( cam->backupFolderPath )!=0 )
    Error( "Cannot chdir to %s", cam->backupFolderPath );
  DIR* d = opendir( "." );
  if( d==NULL )
    Error( "Cannot list contents of %s", cam->backupFolderPath );

  int nEntries = 0;
  struct dirent * de;
  while( (de=readdir( d ))!=NULL )
    {
    if( NOTEMPTY( de->d_name )
        && strstr( de->d_name, when )!=NULL
        && StringEndsWith( de->d_name, ".jpg", 0 )==0 )
      {
      ++nEntries;
      }
    }
  closedir( d );

  return nEntries;
  }

void CountImages( _CONFIG* config, int days, char* camera )
  {
  if( config==NULL )
    Error( "Cannot count images without config");

  char* when = DateString( days );

  for( _CAMERA* cam = config->cameras; cam!=NULL; cam=cam->next )
    {
    CameraBackupFolder( config, cam );
    if( EMPTY( camera )
        || strcasecmp( camera, cam->nickName )==0 )
      {
      int nEntries = CountImagesSingleCamera( cam, when );
      printf( "Camera %s has %8d images from %s (%d days ago)\n",
              cam->nickName, nEntries, when, -1 * days );
      }
    }

  free( when );
  }

int RemoveImages( _CAMERA* cam, char* when )
  {
  if( cam==NULL )
    Error( "Cannot count images for NULL camera");
  if( EMPTY( cam->nickName ) )
    Error( "Cannot count images for camera without name");
  if( EMPTY( when ) )
    Error( "Cannot count images without date string");
  if( EMPTY( cam->backupFolderPath ) )
    Error( "Cannot count images for camera %s without backup folder", cam->nickName);
  if( DirExists( cam->backupFolderPath )!=0 )
    Error( "Cannot count images in missing folder %s", cam->backupFolderPath );
  if( chdir( cam->backupFolderPath )!=0 )
    Error( "Cannot chdir to %s", cam->backupFolderPath );
  DIR* d = opendir( "." );
  if( d==NULL )
    Error( "Cannot list contents of %s", cam->backupFolderPath );

  int nEntries = 0;
  struct dirent * de;
  while( (de=readdir( d ))!=NULL )
    {
    if( NOTEMPTY( de->d_name )
        && strstr( de->d_name, when )!=NULL
        && StringEndsWith( de->d_name, ".jpg", 0 )==0 )
      {
      if( unlink( de->d_name )==0 )
        {
        ++nEntries;
        }
      }
    }
  closedir( d );

  return nEntries;
  }

void MakeTarBall( int doRemove, _CAMERA* cam, char* when )
  {
  if( cam==NULL )
    Error( "Cannot count images for NULL camera");
  if( EMPTY( cam->nickName ) )
    Error( "Cannot count images for camera without name");
  if( EMPTY( when ) )
    Error( "Cannot count images without date string");
  if( EMPTY( cam->backupFolderPath ) )
    Error( "Cannot count images for camera %s without backup folder", cam->nickName);
  if( DirExists( cam->backupFolderPath )!=0 )
    Error( "Cannot count images in missing folder %s", cam->backupFolderPath );
  if( chdir( cam->backupFolderPath )!=0 )
    Error( "Cannot chdir to %s", cam->backupFolderPath );

  char cmd[BUFLEN];

  snprintf( cmd, sizeof(cmd)-1, "/usr/bin/find . -type f -name \"*%s*jpg\" -print0 > .list", when );
  printf("Running [%s]\n", cmd );
  int err = system( cmd );
  if( err==-1 && errno==ECHILD )
    { /* child exited, no return code */
    err = 0;
    }

  if( err!=0 )
    {
    printf("Failed to generate list file!  No archive and keeping images\n");
    return;
    }

  if( FileExists( ".list" )!=0 )
    {
    printf("Command run but cannot find .list file!  No archive and keeping images\n");
    return;
    }

  snprintf( cmd, sizeof(cmd)-1, "/bin/tar cf backup-%s.tar --null --files-from=.list", when );
  printf("Running [%s]\n", cmd );

  err = system( cmd );
  if( err==-1 && errno==ECHILD )
    { /* child exited, no return code */
    err = 0;
    }

  if( err==0 )
    {
    printf("Success!\n");
    if( doRemove )
      {
      printf( "Removing old files.\n" );
      int n = RemoveImages( cam, when );
      printf( "Removed %d image files from %s\n", n, cam->backupFolderPath );
      }

    unlink( ".list" );
    }
  else
    {
    printf("Tar command returned error code %d.  Not deleting files.\n", err );
    }
  }

void TarImages( int doRemove, _CONFIG* config, int days, char* camera )
  {
  if( config==NULL )
    Error( "Cannot count images without config");

  char* when = DateString( days );

  for( _CAMERA* cam = config->cameras; cam!=NULL; cam=cam->next )
    {
    CameraBackupFolder( config, cam );
    if( EMPTY( camera )
        || strcasecmp( camera, cam->nickName )==0 )
      {
      int nEntries = CountImagesSingleCamera( cam, when );
      if( nEntries>0 )
        {
        printf( "Create archive matching %s in %s\n", 
                when, cam->backupFolderPath );
        MakeTarBall( doRemove, cam, when );
        }
      }
    }

  free( when );
  }

void Usage( char* cmd, int exitCode )
  {
  printf("USAGE: %s -config CONFIGFILE ...args...\n", cmd );
  printf("where args may include:\n");
  printf("   -count       ==> count image files\n");
  printf("   -tar         ==> create tarball for each matching date\n");
  printf("   -remove      ==> remove image files after tar completes\n");
  printf("   -days        ==> number of days in the past\n");
  printf("   -range N1 N2 ==> range of days to test\n");
  printf("   -camera      ==> operate on the specified camera (default=all).\n");

  exit( exitCode);
  }

int main( int argc, char** argv )
  {
  int i=0;
  int days=0;
  int d1=0;
  int d2=0;
  int doCount = 0;
  int doTar = 0;
  int doRemove = 0;
  char* camera = NULL;

  glob_argc = argc;
  glob_argv = argv;

  _CONFIG* config = NULL;
  config = (_CONFIG*)calloc(1, sizeof(_CONFIG) );
  glob_conf = config;

  if( argc<=1 )
    {
    Usage( argv[0], 0);
    }

  for( i=1; i<argc; ++i )
    {
    if( ( strcmp( argv[i], "-config" )==0
        || strcmp( argv[i], "-c" )==0 )
        && i+1<argc
        && FileExists( argv[i+1] )==0 )
      {
      ++i;
      SetDefaults( config );
      ReadConfig( config, argv[i] );
      }
    else if( strcmp( argv[i], "-days" )==0
        && i+1<argc
        && atoi( argv[i+1] )>=0 )
      {
      ++i;
      days = atoi( argv[i] );
      }
    else if( strcmp( argv[i], "-range" )==0
        && i+2<argc
        && atoi( argv[i+1] )>=0
        && atoi( argv[i+2] )>=0 )
      {
      ++i;
      d1 = atoi( argv[i] );
      ++i;
      d2 = atoi( argv[i] );
      }
    else if( strcmp( argv[i], "-tar" )==0 )
      {
      doTar = 1;
      }
    else if( strcmp( argv[i], "-remove" )==0 )
      {
      doRemove = 1;
      }
    else if( strcmp( argv[i], "-count" )==0 )
      {
      doCount = 1;
      }
    else if( strcmp( argv[i], "-camera" )==0
        && i+1<argc
        && ValidCamera( config, argv[i+1] )==0 )
      {
      ++i;
      camera = argv[i];
      }
    else if( strcmp( argv[i], "-h" )==0
             || strcmp( argv[i], "-help" )==0 )
      {
      Usage( argv[0], 0 );
      }
    else
      {
      Warning("Unknown argument: [%s]", argv[i] );
      Usage( argv[0], -1 );
      }
    }

  if( config==NULL )
    {
    Warning("No configuration specified. Use -c FILENAME");
    }

  if( doCount )
    {
    if( d1!=0 || d2!=0 )
      {
      if( d1<d2 )
        {
        for( int d=d1; d<=d2; ++d )
          {
          CountImages( config, -1 * d, camera );
          }
        }
      else
        {
        for( int d=d1; d>=d2; --d )
          {
          CountImages( config, -1 * d, camera );
          }
        }
      }
    else
      {
      CountImages( config, -1 * days, camera );
      }
    }

  if( doTar )
    {
    if( d1!=0 || d2!=0 )
      {
      if( d1<d2 )
        {
        for( int d=d1; d<=d2; ++d )
          {
          TarImages( doRemove, config, -1 * d, camera );
          }
        }
      else
        {
        for( int d=d1; d>=d2; --d )
          {
          TarImages( doRemove, config, -1 * d, camera );
          }
        }
      }
    else
      {
      TarImages( doRemove, config, -1 * days, camera );
      }
    }

  return 0;
  }
