#include "base.h"

/*
#define DEBUG 1
*/

void DumpJPEGToBrowser( char* nickName, long nBytes, char* fileName )
  {
  FILE* f = fopen( fileName, "r" );
  if( f==NULL )
    {
    CGIHeader( NULL, 0, NULL, 0, NULL, 0, NULL);
    Error("Cannot open file %s in camera %s", fileName, nickName );
    }
  CGIHeader( "image/jpeg", nBytes, NULL, 0, NULL, 0, NULL );
  int c = -1;
  int nChars = 0;
  while( (c=fgetc(f))!=EOF )
    {
    fputc( c, stdout );
    ++nChars;
    }
  fclose( f );
  if( nBytes != nChars )
    {
    Warning( "Expected to send %d bytes (%s/%s) - but actually sent %d",
             nBytes, nickName, fileName, nChars );
    }
  }

