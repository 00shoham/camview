#include "base.h"

#define SEARCH "/usr/bin/ffmpeg -y -i rtsp://admin:555hotel@192.168.1.85:10554/live/ch0 -an -r 1 -qscale 1 -f image2 -vframes 99999999 \"test-image-%08d.jpg\""

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

  char* result = NULL;
  int err = POpenAndSearch( "/bin/ps -efww", SEARCH, &result );
  printf( "Original search term: [%s]\n", SEARCH );
  printf( "Search returned %d (%s)\n", err, NULLPROTECT( result ) );
  char* tmp = RemoveExtraSpaces( strdup( SEARCH ), 1 );
  printf( "Simplifed search term: [%s]\n", tmp );
  err = POpenAndSearch( "/bin/ps -efww", tmp, &result );
  printf( "Search - no quotes returned %d (%s)\n", err, NULLPROTECT( result ) );
  free( tmp );

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
