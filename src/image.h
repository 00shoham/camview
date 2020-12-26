#ifndef _INCLUDE_IMAGE
#define _INCLUDE_IMAGE

typedef struct _image
  {
  char* cameraID;
  char* fileName;
  int width;
  int height;
  int bpp;
  JSAMPLE* data;

  /* This is a cached value and may be -1.  Use AverageImageLuminosity() function */
  int _averageLuminosity;
  } _IMAGE;

_IMAGE* NewImage( char* cameraID, char* fileName,
                  int width, int height, int bpp );
void SizeImage( _IMAGE* i, int w, int h, int bpp );
void CopyImage( _IMAGE* src, _IMAGE* dst );
void FreeImage( _IMAGE** p2 );
void CopyImage( _IMAGE* src, _IMAGE* dst );


char* TimeStampFilename( int deltaSeconds );
int IsImageBlack( char* nickName, char* fileName );
int HasImageChanged( int debug,
                     char* nickName,
                     _IMAGE* inputA,
                     _IMAGE* inputB,
                     int color_dark,
                     double dark_brightness_boost,
                     int color_diff_threshold,
                     int despeckle_dark_threshold,
                     int despeckle_nondark_min,
                     int despeckle_bright_threshold,
                     int despeckle_nonbright_max,
                     int checkerboard_square_size,
                     int checkerboard_minwhite,
                     int checkerboard_numwhite,
                     double checkerboard_percent,
                     double* calculatedScore,
                     char* diffImageName,
                     char* cbImageName );

void ImageAveragePixelsWithNeighbours( _IMAGE* in, _IMAGE* out );

void ImageSubtractWithThresholdPerColor( _IMAGE* a, _IMAGE* b,
                                          _IMAGE* diff, int threshold );

void ImageSubtractWithThresholdVector( _IMAGE* a, _IMAGE* b, _IMAGE* diff,
                                        int threshold );

void ImageSubtractAbsoluteWithThreshold( _IMAGE* a, _IMAGE* b,
                                          _IMAGE* diff, int threshold );

void ImageSubtractAbsoluteWithThresholdAndNeighbours( _IMAGE* a, _IMAGE* b,
                                                       _IMAGE* diff, int threshold );

long TotalImageDifference( _IMAGE* a, _IMAGE* b, int threshold );

void ImageGrayScale( _IMAGE* input, _IMAGE* output );

void ImageInvertGrayScale( _IMAGE* input, _IMAGE* output );

void ImageHalveResolution( _IMAGE* input, _IMAGE* output );

int AverageImageLuminosity( _IMAGE* image );

long TotalPixels( _IMAGE* image );

void BoostLuminosity( _IMAGE* image, double factor );

void CopyBorders( _IMAGE* input, _IMAGE* output );

void Despeckle( _IMAGE* input, _IMAGE* output,
                int darkThreshold, int nonDarkMin,
                int brightThreshold, int nonBrightMax );

void GrayScaleToCheckerboard( _IMAGE* input, _IMAGE* output,
                               int pixelsPerSquare,
                               int minValueWhite,
                               int minWhitesPerSquare );

int ScaleImage( _IMAGE* input, _IMAGE* output );

int CalculateScaledHeight( int iWidth, int iHeight, int maxWidth,
                           int* oWidth, int* oHeight );

#endif
