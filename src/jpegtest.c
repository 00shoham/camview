#include "base.h"

extern int glob_argc;
extern char** glob_argv;
extern _CONFIG* glob_conf;

#define START_CLOCK(S) { segment=S; t1 = TimeInMicroSeconds(); }
#define STOP_CLOCK() { t2 = TimeInMicroSeconds(); d=t2-t1; printf("%s ran in %lld microseconds\n", segment, d);}

int main( int argc, char** argv )
  {
  char* segment;
  long long t1, t2, d;
  glob_argc = argc;
  glob_argv = argv;
  /*
  _CONFIG* config = NULL;
  config = (_CONFIG*)calloc(1, sizeof(_CONFIG) );
  */

  if( argc<=1 )
    Error("Specify input filename.");

  START_CLOCK("file I/O")
  char* inputName = argv[1];
  unsigned char* data;
  long n = FileRead( inputName, &data );
  if( n<=0 )
    Error("No data in %s", inputName );
  STOP_CLOCK()

  printf("%s has %ld bytes\n", inputName, n );
  char baseName[BUFLEN-100];
  (void)GetBaseFilenameFromPath( inputName, baseName, sizeof(baseName) );
  printf("Basename is %s\n", baseName );

  START_CLOCK("Uncompress JPEG")
  _IMAGE* input = NewImage( "testcam", inputName, 0, 0, 0 );
  int err = DecompressJPEGFromBytes( data, n, input );
  printf("Decompressing returned %d\n", err );
  if( err!=0 )
    Error("Decompression problem");
  STOP_CLOCK()
  FREE( data );

  printf("Width = %d, height = %d, bpp = %d\n", input->width, input->height, input->bpp );
  printf("Average input image luminosity is %d\n", AverageImageLuminosity( input ) );

  START_CLOCK("Boost brightness")
  char boostName[BUFLEN];
  snprintf(boostName, sizeof(boostName), "%s-boost.jpg", baseName );
  _IMAGE* boost = NewImage( "testcam", boostName, input->width, input->height, input->bpp ); 
  CopyImage( input, boost );
  BoostLuminosity( boost, 1.2 );
  STOP_CLOCK()
  START_CLOCK("Write JPEG")
  err = CompressJPEG( boost, boostName, 90 );
  STOP_CLOCK()
  printf("Compressing %s returned %d\n", boostName, err );

  START_CLOCK("Smooth image")
  char fuzzyName[BUFLEN];
  snprintf(fuzzyName, sizeof(fuzzyName), "%s-fuzzy.jpg", baseName );
  _IMAGE* fuzzy = NewImage( "testcam", fuzzyName, input->width, input->height, input->bpp );
  ImageAveragePixelsWithNeighbours( input, fuzzy );
  STOP_CLOCK()
  START_CLOCK("Write JPEG")
  err = CompressJPEG( fuzzy, fuzzyName, 90 );
  STOP_CLOCK()
  printf("Compressing %s returned %d\n", fuzzyName, err );

  START_CLOCK("Despeckle image")
  char despeckleName[BUFLEN];
  snprintf(despeckleName, sizeof(despeckleName), "%s-despeckle.jpg", baseName );
  _IMAGE* despeckle = NewImage( "testcam", despeckleName, input->width, input->height, input->bpp );
  Despeckle( fuzzy, despeckle,
              DEFAULT_DESPECKLE_DARK_THRESHOLD,
              DEFAULT_DESPECKLE_NONDARK_MIN,
              DEFAULT_DESPECKLE_BRIGHT_THRESHOLD,
              DEFAULT_DESPECKLE_NONBRIGHT_MAX );
  STOP_CLOCK()
  START_CLOCK("Write JPEG")
  err = CompressJPEG( despeckle, despeckleName, 90 );
  STOP_CLOCK()
  printf("Compressing %s returned %d\n", despeckleName, err );

  START_CLOCK("DIFF two images in colour")
  char diffName[BUFLEN];
  snprintf(diffName, sizeof(diffName), "%s-diff.jpg", baseName );
  _IMAGE* diff = NewImage( "testcam", diffName, input->width, input->height, input->bpp );
  ImageSubtractWithThresholdVector( input, despeckle, diff, 50 );
  STOP_CLOCK()
  START_CLOCK("Write JPEG")
  err = CompressJPEG( diff, diffName, 90 );
  STOP_CLOCK()
  printf("Compressing %s returned %d\n", diffName, err );

  START_CLOCK("Calculate scalar image diff")
  long distance = TotalImageDifference( input, despeckle, 50 );
  STOP_CLOCK()
  printf("Total difference is %ld\n", distance );

  START_CLOCK("Image diff to grayscale")
  char monoName[BUFLEN];
  snprintf(monoName, sizeof(monoName), "%s-mono.jpg", baseName );
  _IMAGE* mono = NewImage( "testcam", monoName, input->width, input->height, 1 );
  ImageSubtractAbsoluteWithThresholdAndNeighbours( input, despeckle,
    mono, DEFAULT_COLOR_DIFF_THRESHOLD );
  STOP_CLOCK()
  START_CLOCK("Write JPEG")
  err = CompressJPEG( mono, monoName, 90 );
  STOP_CLOCK()
  printf("Compressing %s returned %d\n", monoName, err );

  START_CLOCK("Reduce grayscale image to checkerboard")
  char checkerboardName[BUFLEN];
  snprintf(checkerboardName, sizeof(checkerboardName), "%s-checkerboard.jpg", baseName );
  _IMAGE* checkerboard = NewImage( "testcam", checkerboardName,
                                   input->width/DEFAULT_CHECKERBOARD_SQUARE_SIZE,
                                   input->height/DEFAULT_CHECKERBOARD_SQUARE_SIZE, 1 );
  GrayScaleToCheckerboard( mono, checkerboard,
                            DEFAULT_CHECKERBOARD_SQUARE_SIZE,
                            DEFAULT_CHECKERBOARD_MIN_WHITE,
                            DEFAULT_CHECKERBOARD_NUM_WHITE );
  STOP_CLOCK()
  START_CLOCK("Write JPEG")
  err = CompressJPEG( checkerboard, checkerboardName, 90 );
  STOP_CLOCK()
  printf("Compressing %s returned %d\n", checkerboardName, err );

  FreeImage( &input );
  FreeImage( &boost );
  FreeImage( &diff );
  FreeImage( &fuzzy );
  FreeImage( &despeckle );
  FreeImage( &mono );
  FreeImage( &checkerboard );

  return 0;
  }
