#include "base.h"

int IsFileOpen( char* path )
  {
  int fd = open( path, O_RDONLY );
  if( fd<0 )
    return -1;

  int err = fcntl( fd, F_SETLEASE, F_RDLCK );
  close( fd );
  return err;
  }

int main( int argc, char** argv )
  {

  /*
  char* line;
  int err = POpenAndSearch( "/bin/ps -ef", "firefox", &line );
  printf( "Search for 'firefox' returned %d (%s)\n", err, NULLPROTECT( line ) );
  FREE( line );
  err = POpenAndSearch( "/bin/ps -ef", "firehen", &line );
  printf( "Search for 'firehen' returned %d (%s)\n", err, NULLPROTECT( line ) );
  */

  for( int i=1; i<argc; ++i )
    {
    /*
    int err = LockFile( argv[i] );
    printf( "LockFile --> %d\n", err );
    */

    int err = IsFileOpen( argv[i] );
    printf( "IsFileOpen --> %d\n", err );
    }

  return 0;
  }
