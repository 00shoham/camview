#include "base.h"

int glob_argc = 0;
char** glob_argv = NULL;
int inCGI = 0;
pthread_mutex_t errLock = PTHREAD_MUTEX_INITIALIZER;
int useMutex = 0;

#define LOCK_MUTEX\
  if( useMutex )\
    {\
    pthread_mutex_lock( &errLock );\
    }

#define UNLOCK_MUTEX\
  if( useMutex )\
    {\
    pthread_mutex_unlock( &errLock );\
    }

extern _CONFIG* glob_conf;

void mysyslog( char* prefix, char* msg )
  {
  time_t tnow = time(NULL);
  struct tm *tmp = localtime( &tnow );
  char longtime[BUFLEN];
  snprintf( longtime, sizeof(longtime)-1,
            "%04d-%02d-%02d %02d:%02d:%02d %s ",
            tmp->tm_year + 1900,
            tmp->tm_mon + 1,
            tmp->tm_mday,
            tmp->tm_hour,
            tmp->tm_min,
            tmp->tm_sec,
            prefix );

  char parsingLocation[BUFLEN];
  if( glob_conf!=NULL
      && glob_conf->currentlyParsing>0
      && glob_conf->parserLocation!=NULL
      && NOTEMPTY( glob_conf->parserLocation->tag ) )
    {
    snprintf( parsingLocation, sizeof(parsingLocation)-1, "%s::%d ",
              glob_conf->parserLocation->tag,
              glob_conf->parserLocation->iValue );
    }
  else
    {
    parsingLocation[0] = 0;
    }

  if( inCGI==0 && (glob_conf==NULL || glob_conf->logFileHandle==NULL ) )
    {
    LOCK_MUTEX
    fputs( longtime, stdout );
    fputs( parsingLocation, stdout );
    fputs( msg, stdout );
    fputs( "\n", stdout );
    fflush( stdout );
    UNLOCK_MUTEX
    }

  if( glob_conf!=NULL && glob_conf->logFileHandle!=NULL )
    {
    if( inCGI==1 && glob_conf->logFileHandle==stdout )
      { /* don't send logs to CGI output, which is stdout! */
      }
    else
      {
      LOCK_MUTEX
      fputs( longtime, glob_conf->logFileHandle );
      fputs( parsingLocation, glob_conf->logFileHandle );
      fputs( msg, glob_conf->logFileHandle );
      fputs( "\n", glob_conf->logFileHandle );
      fflush( glob_conf->logFileHandle );
      UNLOCK_MUTEX
      }
    }
  }

void Error( char* fmt, ... )
  {
  va_list arglist;
  char buf[BIGBUF];

  va_start( arglist, fmt );
  vsnprintf( buf, sizeof(buf), fmt, arglist );
  va_end( arglist );

  mysyslog( "ERROR", buf );
  if( inCGI )
    {
    fputs( "<p>ERROR: ", stdout );
    fputs( buf, stdout );
    fputs( "</p>\n", stdout );

    CGIFooter();
    }

  exit(EXIT_FAILURE);
  }

void Warning( char* fmt, ... )
  {
  va_list arglist;
  char buf[BIGBUF];

  va_start( arglist, fmt );
  vsnprintf( buf, sizeof(buf), fmt, arglist );
  va_end( arglist );

  mysyslog( "WARNING", buf );
  return;
  }

void Notice( char* fmt, ... )
  {
  va_list arglist;
  char buf[BIGBUF];

  va_start( arglist, fmt );
  vsnprintf( buf, sizeof(buf), fmt, arglist );
  va_end( arglist );

  mysyslog( "NOTICE", buf );
  return;
  }
