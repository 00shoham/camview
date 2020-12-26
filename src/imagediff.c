#include "base.h"

extern int glob_argc;
extern char** glob_argv;
extern _CONFIG* glob_conf;

void Usage( char* cmd )
  {
  printf("USAGE: %s ...args...\n", cmd );
  printf("  Where args include:\n");
  printf("  Mandatory: -image1 inputFilename.jpg -image2 inputFilename.jpg\n");
  printf("  Optional: -diff outputFilename.jpg\n");
  printf("  Optional: -checkerboard outputFilename.jpg\n");
  printf("  Optional: -width NNN\n");
  printf("  Optional: -html\n");
  printf("  Optional: -h\n");
  exit(0);
  }

int main( int argc, char** argv )
  {
  glob_argc = argc;
  glob_argv = argv;

  _CONFIG* config = NULL;
  config = (_CONFIG*)calloc(1, sizeof(_CONFIG) );
  glob_conf = config;

  glob_conf->logFileHandle = stdout;

  int width = 500;
  char* image1 = NULL;
  char* image2 = NULL;
  char* diffImage = NULL;
  char* cbImage = NULL;
  int doHTML = 0;

  for( int i=1; i<argc; ++i )
    {
    if( strcmp( argv[i], "-image1" )==0 && i+1<argc )
      {
      ++i;
      image1 = argv[i];
      }
    else if( strcmp( argv[i], "-image2" )==0 && i+1<argc )
      {
      ++i;
      image2 = argv[i];
      }
    else if( strcmp( argv[i], "-diff" )==0 && i+1<argc )
      {
      ++i;
      diffImage = argv[i];
      }
    else if( strcmp( argv[i], "-checkerboard" )==0 && i+1<argc )
      {
      ++i;
      cbImage = argv[i];
      }
    else if( strcmp( argv[i], "-width" )==0 && i+1<argc && atoi( argv[i+1] )>0 )
      {
      ++i;
      width = atoi(argv[i]);
      }
    else if( strcmp( argv[i], "-html" )==0 )
      {
      doHTML = 1;
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

  if( image1 == NULL ) Error( "You must specify -image1" );
  if( image2 == NULL ) Error( "You must specify -image2" );

  double cbPercent = 0;

  if( doHTML==0 && diffImage!=NULL )
    fprintf( stderr, "Generating diff image %s\n", diffImage );

  if( doHTML==0 && cbImage!=NULL )
    fprintf( stderr, "Generating checkerboard image %s\n", cbImage );

  _IMAGE* img1 = ImageFromJPEGFile( "imagediff", image1 );
  _IMAGE* img2 = ImageFromJPEGFile( "imagediff", image2 );

  int motion = HasImageChanged(
                 0,
                 "imagediff", img1, img2,
                 DEFAULT_COLOR_DARK,
                 DEFAULT_DARK_BRIGHTNESS_BOOST,
                 DEFAULT_COLOR_DIFF_THRESHOLD,
                 DEFAULT_DESPECKLE_DARK_THRESHOLD,
                 DEFAULT_DESPECKLE_NONDARK_MIN,
                 DEFAULT_DESPECKLE_BRIGHT_THRESHOLD,
                 DEFAULT_DESPECKLE_NONBRIGHT_MAX,
                 DEFAULT_CHECKERBOARD_SQUARE_SIZE,
                 DEFAULT_CHECKERBOARD_MIN_WHITE,
                 DEFAULT_CHECKERBOARD_NUM_WHITE,
                 DEFAULT_CHECKERBOARD_PERCENT,
                 &cbPercent,
                 diffImage, cbImage );

  if( doHTML )
    {
    printf("    <tr>\n");
    printf("      <td>%s</td>\n", motion==0?"<b>Motion</b>":"Static" );
    printf("      <td>%8.2lf</td>\n", cbPercent );
    printf("      <td><img width=\"%dpx\" src=\"%s\"/></td>\n", width, image1 );
    printf("      <td><img width=\"%dpx\" src=\"%s\"/></td>\n", width, image2 );
    if( diffImage!=NULL )
      {
      printf("      <td><img width=\"%dpx\" src=\"%s\"/></td>\n", width, diffImage );
      }
    else
      {
      printf("      <td>&nbsp;</td>\n" );
      }
    if( cbImage!=NULL )
      {
      printf("      <td><img width=\"%dpx\" src=\"%s\"/></td>\n",
             width, cbImage );
      }
    else
      {
      printf("      <td>&nbsp;</td>\n" );
      }
    printf("      </td>\n");
    printf("    </tr>\n");
    }
  else
    {
    printf("DIFF: %s|%s - %8.2lf - %s\n", image1, image2,
                                          cbPercent,
                                          motion==0?"Motion":"Static" );
    }

  return 0;
  }
