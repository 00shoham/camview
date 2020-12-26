#include "base.h"

extern int glob_argc;
extern char** glob_argv;
extern _CONFIG* glob_conf;

void Usage( char* cmd )
  {
  printf("USAGE: %s ...args...\n", cmd );
  printf("  Where args include:\n");
  printf("  Mandatory: -input inputFilename.jpg -output outputFilename.jpg -boost FACTOR\n");
  printf("  Optional: -h\n");
  exit(0);
  }

int main( int argc, char** argv )
  {
  _CONFIG* config = NULL;
  config = (_CONFIG*)calloc(1, sizeof(_CONFIG) );
  glob_conf = config;

  glob_conf->logFileHandle = stdout;

  char* input = NULL;
  char* output = NULL;
  double factor = 0;

  for( int i=1; i<argc; ++i )
    {
    if( strcmp( argv[i], "-input" )==0 && i+1<argc )
      {
      ++i;
      input = argv[i];
      }
    else if( strcmp( argv[i], "-output" )==0 && i+1<argc )
      {
      ++i;
      output = argv[i];
      }
    else if( strcmp( argv[i], "-factor" )==0 && i+1<argc && atof( argv[i+1] )>0 )
      {
      ++i;
      factor = atof(argv[i]);
      }
    else if( strcmp( argv[i], "-h" )==0 )
      {
      Usage( argv[0] );
      }
    else
      {
      Error( "Unrecognized argument: %s", argv[i] );
      }
    }

  if( input == NULL ) Error( "You must specify -input" );
  if( output == NULL ) Error( "You must specify -output" );
  if( factor<=0.01 ) Error( "You must specify -factor" );

  _IMAGE* in = ImageFromJPEGFile( "brightenimage", input );
  BoostLuminosity( in, factor );
  CompressJPEG( in, output, JPEG_WRITE_IMAGEQUALITY );

  return 0;
  }
