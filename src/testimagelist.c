#include "base.h"

#define MAXFILENAMES 10000

int main()
  {
  char* files[MAXFILENAMES];
  int nFiles = 0;
  char buf[BUFLEN];

  while( fgets( buf, sizeof(buf)-1, stdin )==buf )
    {
    char* ptr = strtok( buf, "\r\n\t" );
    files[nFiles] = strdup( ptr );
    ++nFiles;
    if( nFiles==MAXFILENAMES )
      {
      Warning( "Reached max number of files: %s", nFiles );
      }
    }

  for( int i=0; i<nFiles-1; ++i )
    {
    char* i1 = files[i];
    char* i2 = files[i+1];
    if( EMPTY( i1 ) )
      {
      Warning("File %d is empty", i );
      } 
    else if( EMPTY( i2 ) )
      {
      Warning("File %d is empty", i+1 );
      } 
    else
      {
      printf( "./imagediff -html -image1 '%s' -image2 '%s' -diff 'diff-%s' -checkerboard 'cb-%s'\n",
              i1, i2, i1, i1 );
      }
    }

  return 0;
  }
