#include "base.h"

int IsFileOpen( char* path )
  {
  int fd = open( path, O_RDONLY );
  if( fd<0 )
    return -1;

  int err = fcntl( fd, F_SETLEASE, F_RDLCK );
  printf( "fcntl returned %d\n", err );
  close( fd );
  return 0;
  }

int main( int argc, char** argv )
  {
  for( int i=1; i<argc; ++i )
    {
    int err = IsFileOpen( argv[i] );
    printf( "IsFileOpen() --> %d\n", err );
    }

  return 0;
  }
