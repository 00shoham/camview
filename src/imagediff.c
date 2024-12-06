#include "base.h"

extern int glob_argc;
extern char** glob_argv;
extern _CONFIG* glob_conf;

void Usage( char* cmd )
  {
  printf( "USAGE: %s ...args...\n", cmd );
  printf( "  Where args include:\n");
  printf( "  Mandatory: -image1 inputFilename.jpg -image2 inputFilename.jpg\n");
  printf( "  Optional: -diff outputFilename.jpg\n");
  printf( "  Optional: -checkerboard outputFilename.jpg\n");
  printf( "  Optional: -width NNN\n");
  printf( "  Optional: -html\n");
  printf( "  Optional: -defaults\n");
  printf( "  Optional: -color_diff_threshold INT\n" );
  printf( "  Optional: -color_dark INT\n" );
  printf( "  Optional: -dark_brightness_boost FLOAT\n" );
  printf( "  Optional: -despeckle_dark_threshold INT\n" );
  printf( "  Optional: -despeckle_nondark_min INT\n" );
  printf( "  Optional: -despeckle_bright_threshold INT\n" );
  printf( "  Optional: -despeckle_nonbright_max INT\n" );
  printf( "  Optional: -checkerboard_square_size INT\n" );
  printf( "  Optional: -checkerboard_min_white INT\n" );
  printf( "  Optional: -checkerboard_num_white INT\n" );
  printf( "  Optional: -checkerboard_percent FLOAT\n" );
  printf( "  Optional: -debug\n" );
  printf( "  Optional: -h\n");
  exit(0);
  }

void PrintDefaults()
  {
  printf( "Default parameters:\n" );
  printf( "color_diff_threshold = %d (/255)\n", DEFAULT_COLOR_DIFF_THRESHOLD );
  printf( "color_dark = %d\n (/765)", DEFAULT_COLOR_DARK );
  printf( "dark_brightness_boost = %lf (color=exp(boost*log(color)))\n", DEFAULT_DARK_BRIGHTNESS_BOOST );
  printf( "despeckle_dark_threshold = %d/765\n", DEFAULT_DESPECKLE_DARK_THRESHOLD );
  printf( "despeckle_nondark_min = %d/765\n", DEFAULT_DESPECKLE_NONDARK_MIN );
  printf( "despeckle_bright_threshold = %d/765\n", DEFAULT_DESPECKLE_BRIGHT_THRESHOLD );
  printf( "despeckle_nonbright_max = %d/765\n", DEFAULT_DESPECKLE_NONBRIGHT_MAX );
  printf( "checkerboard_square_size = %d pixels\n", DEFAULT_CHECKERBOARD_SQUARE_SIZE );
  printf( "checkerboard_min_white = %d/255\n", DEFAULT_CHECKERBOARD_MIN_WHITE );
  printf( "checkerboard_num_white = %d/(square size pixels^2)\n", DEFAULT_CHECKERBOARD_NUM_WHITE );
  printf( "checkerboard_percent = %lf (%%)\n", DEFAULT_CHECKERBOARD_PERCENT );
  }

int main( int argc, char** argv )
  {
  glob_argc = argc;
  glob_argv = argv;

  int doDebug = 0;

  _CONFIG* config = NULL;
  config = (_CONFIG*)calloc(1, sizeof(_CONFIG) );
  glob_conf = config;

  int width = 500;
  char* image1 = NULL;
  char* image2 = NULL;
  char* diffImage = NULL;
  char* cbImage = NULL;
  int doHTML = 0;

  int color_diff_threshold = DEFAULT_COLOR_DIFF_THRESHOLD;
  int color_dark = DEFAULT_COLOR_DARK;
  double dark_brightness_boost = DEFAULT_DARK_BRIGHTNESS_BOOST;
  int despeckle_dark_threshold = DEFAULT_DESPECKLE_DARK_THRESHOLD;;
  int despeckle_nondark_min = DEFAULT_DESPECKLE_NONDARK_MIN;
  int despeckle_bright_threshold = DEFAULT_DESPECKLE_BRIGHT_THRESHOLD;
  int despeckle_nonbright_max = DEFAULT_DESPECKLE_NONBRIGHT_MAX;
  int checkerboard_square_size = DEFAULT_CHECKERBOARD_SQUARE_SIZE;
  int checkerboard_min_white = DEFAULT_CHECKERBOARD_MIN_WHITE;
  int checkerboard_num_white = DEFAULT_CHECKERBOARD_NUM_WHITE;
  double checkerboard_percent = DEFAULT_CHECKERBOARD_PERCENT;

  for( int i=1; i<argc; ++i )
    {
    if( strcmp( argv[i], "-debug" )==0 )
      {
      ++ doDebug;
      }
    else if( strcmp( argv[i], "-image1" )==0 && i+1<argc )
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
    else if( strcmp( argv[i], "-defaults" )==0 )
      {
      PrintDefaults();
      exit(0);
      }
    else if(  strcmp( argv[i], "-color_diff_threshold" )==0 && i+1<argc )
      {
      int iVal = atoi( argv[++i] );
      color_diff_threshold = iVal;
      }
    else if(  strcmp( argv[i], "-color_dark" )==0 && i+1<argc )
      {
      int iVal = atoi( argv[++i] );
      color_dark = iVal;
      }
    else if(  strcmp( argv[i], "-dark_brightness_boost" )==0 && i+1<argc )
      {
      double dVal = atoi( argv[++i] );
      dark_brightness_boost = dVal;
      }
    else if(  strcmp( argv[i], "-despeckle_dark_threshold" )==0 && i+1<argc )
      {
      int iVal = atoi( argv[++i] );
      despeckle_dark_threshold = iVal;
      }
    else if(  strcmp( argv[i], "-despeckle_nondark_min" )==0 && i+1<argc )
      {
      int iVal = atoi( argv[++i] );
      despeckle_nondark_min = iVal;
      }
    else if(  strcmp( argv[i], "-despeckle_bright_threshold" )==0 && i+1<argc )
      {
      int iVal = atoi( argv[++i] );
      despeckle_bright_threshold = iVal;
      }
    else if(  strcmp( argv[i], "-despeckle_nonbright_max" )==0 && i+1<argc )
      {
      int iVal = atoi( argv[++i] );
      despeckle_nonbright_max = iVal;
      }
    else if(  strcmp( argv[i], "-checkerboard_square_size" )==0 && i+1<argc )
      {
      int iVal = atoi( argv[++i] );
      checkerboard_square_size = iVal;
      }
    else if(  strcmp( argv[i], "-checkerboard_min_white" )==0 && i+1<argc )
      {
      int iVal = atoi( argv[++i] );
      checkerboard_min_white = iVal;
      }
    else if(  strcmp( argv[i], "-checkerboard_num_white" )==0 && i+1<argc )
      {
      int iVal = atoi( argv[++i] );
      checkerboard_num_white = iVal;
      }
    else if(  strcmp( argv[i], "-checkerboard_percent" )==0 && i+1<argc )
      {
      double dVal = atoi( argv[++i] );
      checkerboard_percent = dVal;
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

  if( doDebug )
    logFileHandle = stdout;

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
                 doDebug,
                 "imagediff", img1, img2,
                 color_dark,
                 dark_brightness_boost,
                 color_diff_threshold,
                 despeckle_dark_threshold,
                 despeckle_nondark_min,
                 despeckle_bright_threshold,
                 despeckle_nonbright_max,
                 checkerboard_square_size,
                 checkerboard_min_white,
                 checkerboard_num_white,
                 checkerboard_percent,
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
