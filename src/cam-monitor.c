#include "base.h"

extern int glob_argc;
extern char** glob_argv;
extern _CONFIG* glob_conf;
extern int useMutexInErrorReporting;

void SendPeriodicHangup( pid_t monitorPid, int interval )
  {
  if( fork()==0 )
    {
    close( STDIN_FILENO );
    close( STDOUT_FILENO );
    close( STDERR_FILENO );
    for(;;)
      {
      sleep( interval );
      kill( monitorPid, SIGHUP );
      }
    }
  }

int main( int argc, char** argv )
  {
  useMutexInErrorReporting = 1;

  glob_argc = argc;
  glob_argv = argv;

  int consoleOnly = 0;

  _CONFIG* config = NULL;

  config = (_CONFIG*)calloc(1, sizeof(_CONFIG) );
  glob_conf = config;

  char* confName = CONFIGNAME;
  for( int i=1; i<argc; ++i )
    {
    if( strcmp( argv[i], "-c" )==0 && i+1<argc )
      {
      ++i;
      confName = argv[i];
      }
    else if( strcmp( argv[i], "-console")==0 )
      {
      consoleOnly = 1;
      }
    else
      {
      Error("Invalid argument: %s (use -c conffile or -console)", argv[i] );
      }
    }

  SetDefaults( config );
  ReadConfig( config, confName );
  ValidateConfig( config );

  if( config!=NULL
      && NOTEMPTY( config->logFile )
      && FileExists( config->logFile )==0 )
    {
    if( RotateFile( config->logFile, LOG_FILES_TO_KEEP )!=0 )
      {
      Warning( "Failed to rotate-out old log file %s", config->logFile );
      int err = unlink( config->logFile );
      if( err )
        Warning( "Failed to unlink old log file %s - %d:%d:%s",
                 config->logFile, err, errno, strerror( errno ) );
      }
    }

  if( config!=NULL && NOTEMPTY( config->baseDir ) )
    {
    EnsureDirExists( config->baseDir );
    if( chdir( config->baseDir )!=0 )
      {
      Error( "Failed to chdir to %s (%d:%s)", config->baseDir,
             errno, strerror( errno ) );
      }
    }

  pid_t childProcess = 0;
  if( ! consoleOnly )
    {
    childProcess = LaunchDaemon( 0 );
    if( childProcess==0 ) /* in child */
      {
      /* try to connect to the cameras again */
      (void)signal( SIGHUP, PingCameras );

      /* shut down child processes if we get killed */
      (void)signal( SIGINT, TerminateMonitor );
      (void)signal( SIGQUIT, TerminateMonitor );
      (void)signal( SIGKILL, TerminateMonitor );
      (void)signal( SIGTERM, TerminateMonitor );

      /* for debugging */
      (void)signal( SIGSEGV, MySegFaultHandler );
      }
    }

  if( consoleOnly || childProcess==0 ) /* worker process */
    {
    if( NOTEMPTY( config->logFile ) )
      {
      Notice( "Opening log file %s", config->logFile );
      logFileHandle = fopen( config->logFile, "a" );
      Notice( "Log file handle is %p", logFileHandle );
      }

    Notice( "***************" );
    Notice( "**  STARTUP  **" );
    Notice( "***************" );

    pid_t mypid = getpid();
    Notice( "Launched service.  In child process. My PID is %ld",
            (long)mypid );

    int n = LaunchAllCameras( config );
    Notice("Launched %d cameras.  Monitoring...", n );

    MonitorCameras( config );
    }
  else
    {
    Notice("Child process is %d.", (int)childProcess );
    }

  FreeConfig( config );
  return 0;
  }
