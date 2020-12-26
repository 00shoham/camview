#include "base.h"

extern int glob_argc;
extern char** glob_argv;
extern _CONFIG* glob_conf;


void Usage( char* cmd, int exitCode )
  {
  printf("USAGE: %s [-listincludes] -c CONFIGFILE [-printvar VARNAME]\n", cmd );

  exit( exitCode);
  }

int main( int argc, char** argv )
  {
  int i=0;

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
    else if( strcmp( argv[i], "-printvar" )==0
        && i+1<argc )
      {
      ++i;
      char* varName = argv[i];
      if( glob_conf!=NULL )
        {
        PrintVariable( glob_conf, varName );
        }
      }
    else if( strcmp( argv[i], "-listincludes" )==0 )
      {
      config->listIncludes = 1;
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

  if( config->listIncludes )
    {
    fputs( "\n", stdout );
    }

  return 0;
  }
