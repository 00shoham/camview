#include "base.h"

pid_t LaunchDaemon( int closeSTDIO )
  {
  pid_t pid;

  /* Fork off the parent process */
  pid = fork();

  /* An error occurred */
  if( pid < 0 )
    {
    Error("Failed to fork child process");
    }

  /* Success: return first pid to the parent */
  if( pid > 0 )
    {
    /* printf("Forked child process %d\n", pid ); */
    return pid;
    }

  /* On success: The child process becomes session leader */
  if( setsid() < 0 )
    {
    Error("Failed to setsid in child process");
    }

  /* Catch, ignore and handle signals */
  if( signal(SIGCHLD, SIG_IGN )==SIG_ERR )
    {
    Error("Failed to trap (ignore) SIGCHLD");
    }

  /* Fork off for the second time*/
  pid = fork();

  /* An error occurred */
  if( pid < 0 )
    {
    Error("Failed to fork second child process");
    }

  /* Success: Let the original parent's child terminate */
  if( pid > 0 )
    {
    exit(EXIT_SUCCESS);
    }

  /* Set new file permissions */
  umask(0);

  /* Change the working directory to the root directory */
  /* or another appropriated directory */
  if( chdir("/tmp/")!=0 )
    {
    Error("Failed to chdir to /tmp/");
    }

  /* Close all open file descriptors */
  if( closeSTDIO )
    {
    int x;
    for( x = sysconf(_SC_OPEN_MAX); x>=0; x-- )
      {
      close (x);
      }
    }

  /* shut down child processes if we get killed */
  (void)signal( SIGHUP, PingCameras );
  (void)signal( SIGINT, TerminateMonitor );
  (void)signal( SIGQUIT, TerminateMonitor );
  (void)signal( SIGKILL, TerminateMonitor );
  (void)signal( SIGSEGV, TerminateMonitor );
  (void)signal( SIGTERM, TerminateMonitor );

  /* Open the log file */
  /* openlog( "monitor", LOG_PID, LOG_DAEMON ); */

  return 0;
  }
