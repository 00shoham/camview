#include "base.h"

int PingAddress( char* netAddress )
  {
  char cmd[BUFLEN];
  snprintf( cmd, BUFLEN-1, "/bin/ping -t 2 -c 2 %s", netAddress );
  FILE* h = popen( cmd, "r" );
  if( h==NULL )
    {
    Warning( "Cannot run test-ping command against %s (%d:%s)",
             netAddress, errno, strerror( errno ) );
    return -1;
    }

  char buf[BUFLEN];
  while( fgets( buf, sizeof(buf)-1, h )==buf )
    {
    (void)StripEOL(buf);
    /* printf("Ping output: [%s]\n", buf ); */
    if( StringMatchesRegex( "^PING ", buf )==0 )
      {
      /* header */
      }
    else if( StringMatchesRegex( "bytes from.*icmp_seq.*ttl.*time", buf )==0 )
      {
      fclose( h );
      return 0;
      }
    else if( StringMatchesRegex( "Unreachable", buf )==0 )
      {
      fclose( h );
      Warning("Host %s not reachable (a)", netAddress );
      return -2;
      }
    else if( StringMatchesRegex( "100% packet loss", buf )==0 )
      {
      fclose( h );
      Warning("Host %s not reachable (b)", netAddress );
      return -3;
      }
    else if( StringMatchesRegex( " 0 received", buf )==0 )
      {
      fclose( h );
      Warning("Host %s not reachable (c)", netAddress );
      return -4;
      }
    }

  fclose( h );
  Warning("Host %s not reachable (d)", netAddress );

  return -5;
  }

int IPAddressFromCommand( char* buf, int bufLen, char* command )
  {
  if( buf==NULL )
    {
    Warning("Cannot store IP address in NULL buffer");
    return -1;
    }

  if( bufLen<16 )
    {
    Warning("Cannot store IP address in tiny buffer");
    return -2;
    }

  if( command==NULL )
    {
    Warning("Cannot extract IP address from NULL command");
    return -3;
    }

  char* tmpBuf = strdup( command );

  char* ptr = ExtractRegexFromString( RE_IPADDR, tmpBuf );
  if( ptr!=NULL )
    {
    strncpy( buf, ptr, bufLen-1 );
    FREE( tmpBuf );
    return 0;
    }

  FREE( tmpBuf );
  buf[0] = 0;

  return -1;
  }
