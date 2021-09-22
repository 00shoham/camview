#include "base.h"

_IMAGE* NewImage( char* cameraID, char* fileName,
                  int width, int height, int bpp )
  {
  _IMAGE* p = (_IMAGE*)calloc( 1, sizeof(_IMAGE) );
  if( p==NULL )
    {
    Error("Cannot allocate _IMAGE");
    }
  if( NOTEMPTY( cameraID ) )
    p->cameraID = strdup( cameraID );
  if( NOTEMPTY( fileName ) )
    p->fileName = strdup( fileName );
  p->width = width;
  p->height = height;
  p->bpp = bpp;
  p->_averageLuminosity = -1;
  p->data = NULL;
  if( p->width > 0
      && p->height > 0
      && p->bpp > 0 )
    {
    p->data = (JSAMPLE*)malloc( p->width
                                * p->height
                                * p->bpp * sizeof(JSAMPLE) );
    if( p->data==NULL )
      {
      Error( "Failed to allocate %d x %d x %d image buffer",
             p->width, p->height, p->bpp );
      }
    }

  return p;
  }

void SizeImage( _IMAGE* i, int w, int h, int bpp )
  {
  if( i->data!=NULL ) free( i->data );
  i->data = (JSAMPLE*)malloc( w * h * bpp * sizeof(JSAMPLE) );
  if( i->data==NULL ) Error("Cannot allocate image %d x %d x %d", w, h, bpp );
  i->width = w;
  i->height = h;
  i->bpp = bpp;
  }

void CopyImage( _IMAGE* src, _IMAGE* dst )
  {
  if( src==NULL || dst==NULL )
    return;
  if( dst->data!=NULL )
    {
    if( dst->width * dst->height * dst->bpp != 
        src->width * src->height * src->bpp )
      {
      free( dst->data );
      dst->data = NULL;
      }
    }
  dst->width = src->width;
  dst->height = src->height;
  dst->bpp = src->bpp;
  dst->_averageLuminosity = src->_averageLuminosity;
  long nSamples = src->width * src->height * src->bpp;
  if( dst->data==NULL )
    {
    dst->data = (JSAMPLE*)malloc( sizeof(JSAMPLE) * nSamples );
    if( dst->data==NULL ) Error("Cannot allocate image %ld bytes", nSamples );
    }
  memcpy( dst->data, src->data, nSamples );
  }

void FreeImage( _IMAGE** p2 )
  {
  if( p2==NULL )
    return;

  _IMAGE* p = *p2;
  if( p==NULL )
    return;

  if( p->cameraID != NULL )
    {
    free( p->cameraID );
    p->cameraID = NULL;
    }
  if( p->fileName != NULL )
    {
    free( p->fileName );
    p->fileName = NULL;
    }
  if( p->data != NULL )
    {
    free( p->data );
    p->data = NULL;
    }
  free( p );

  *p2 = NULL;
  }

char* TimeStampFilename( int deltaSeconds )
  {
  char name[BUFLEN];

  strcpy( name, "image-" );
  time_t tnow = time(NULL) + deltaSeconds;

  struct tm *tmp = localtime( &tnow );
  if( tmp==NULL )
    {
    Error("Cannot work out localtime in struct tm format");
    }

  snprintf( name, sizeof(name)-1,
            "%04d-%02d-%02d_%02d-%02d-%02d",
            tmp->tm_year + 1900,
            tmp->tm_mon + 1,
            tmp->tm_mday,
            tmp->tm_hour,
            tmp->tm_min,
            tmp->tm_sec );

  return strdup( name );
  }

int nDebugImages = 100;
void StoreImagesForDebug( char* nickName, char* imageA, char* imageB, double threshold )
  {
  char copyA[BUFLEN];
  snprintf( copyA, sizeof(copyA)-1, "/tmp/%s-%d-A-%s-%d.jpg",
            nickName, nDebugImages, imageA, (int)threshold );
  FileCopy( imageA, copyA );

  char copyB[BUFLEN];
  snprintf( copyB, sizeof(copyA)-1, "/tmp/%s-%d-B-%s-%d.jpg",
            nickName, nDebugImages, imageB, (int)threshold );
  FileCopy( imageB, copyB );
  }

/* 0 mean true */
int IsImageBlack( char* nickName, char* fileName )
  {
  if( EMPTY( nickName ) )
    {
    Warning( "Must specify camera nickname to check if image is black" );
    return -1;
    }

  if( EMPTY( fileName ) )
    {
    Warning( "Cannot check if empty file is black" );
    return -2;
    }

  if( FileExists( fileName )!=0 )
    {
    Warning( "Cannot check if non-existent file %s/%s is black", nickName, fileName );
    return -3;
    }

  unsigned char* fileData = NULL;
  long fileBytes = FileRead( fileName, &fileData );
  if( fileBytes<=0 )
    {
    if( fileData!=NULL )
      free( fileData );
    Warning( "Cannot get luminosity of empty file (%s/%s)",
             nickName, fileName );
    return -4;
    }

  _IMAGE* image = NewImage( nickName, fileName, 0, 0, 0);
  int err = DecompressJPEGFromBytes( fileData, fileBytes, image );
  if( err!=0 || image->data==NULL )
    {
    Warning( "Could not parse image %s/%s", nickName, fileName );
    FreeImage( &image );
    FREE( fileData );
    return -5;
    }
  FREE( fileData );

  int luminosity = AverageImageLuminosity( image );
  FreeImage( &image );

  if( luminosity > BLACK_IMAGE_MAX_LUMINOSITY )
    {
    return 1;
    }
  else
    {
    return 0;
    }
  }

/* 0 mean changes happened */
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
                     char* cbImageName )
  {
  if( EMPTY( nickName ) )
    {
    Warning( "Came nickname required when comparing images" );
    return -1;
    }

  if( inputA==NULL || inputA->data==NULL )
    {
    Warning( "Cannot test for image changes if input A is NULL" );
    return -1;
    }

  if( inputB==NULL || inputB->data==NULL )
    {
    Warning( "Cannot test for image changes if input B is NULL" );
    return -2;
    }

  if( color_dark<0 || color_dark>255 )
    {
    Warning( "The color level for 'dark' is invalid (%d)", color_dark );
    return -5;
    }

  if( dark_brightness_boost<1 || dark_brightness_boost>5 )
    {
    Warning( "The amount to boost darkness images must be between 1 and 5 (log scale)" );
    return -6;
    }

  if( color_diff_threshold<0 || color_diff_threshold>200 )
    {
    Warning( "The amount by which colors must differ to appear in the diff image must be between 1 and 200" );
    return -7;
    }

  if( despeckle_dark_threshold<0 || despeckle_dark_threshold>100 )
    {
    Warning( "The threshold for dark pixels when despeckling must be between 0 and 100" );
    return -8;
    }

  if( despeckle_nondark_min<=despeckle_dark_threshold || despeckle_nondark_min>(255*2) )
    {
    Warning( "The threshold for non-dark pixels when despeckling must be between %d and %d",
             despeckle_dark_threshold, 255*2  );
    return -9;
    }

  if( despeckle_bright_threshold<50 || despeckle_bright_threshold>3*255 )
    {
    Warning( "The threshold for bright pixels when despeckling must be between 50 and %d", 3*255 );
    return -10;
    }

  if( despeckle_nonbright_max>=despeckle_bright_threshold || despeckle_nonbright_max<0 )
    {
    Warning( "The threshold for non-bright pixels when despeckling must be between 0 and %d", despeckle_bright_threshold );
    return -11;
    }

  if( checkerboard_square_size<2 || checkerboard_square_size>64 )
    {
    Warning( "The motion detection checkerboard square size must be between 2 and 64 pixels" );
    return -12;
    }

  if( checkerboard_minwhite<1 || checkerboard_minwhite>255 )
    {
    Warning( "The minimum 'white level' for motion detection must be between 1 and 255" );
    return -13;
    }

  if( checkerboard_numwhite<1 || checkerboard_numwhite>(checkerboard_square_size*checkerboard_square_size) )
    {
    Warning( "The minimum number of white pixels in the checkerboard must be between 1 and %d", (checkerboard_square_size*checkerboard_square_size) );
    return -14;
    }

  if( checkerboard_percent<0.001 || checkerboard_percent > 50 )
    {
    Warning( "The checkerboard 'percent of board white' must be between 0.001%% and 50%%" );
    return -15;
    }

  if( debug )
    {
    Notice( "HasImageChanged( %s, %s, %s, "
            "... %d, %8.2lf, "
            "... %d, "
            "... %d, %d, %d, %d, "
            "... %d, %d, %d, %8.2lf, "
            "... %p )",
            nickName, NULLPROTECT(inputA->fileName), NULLPROTECT(inputB->fileName),
            color_dark, dark_brightness_boost,
            color_diff_threshold,
            despeckle_dark_threshold, despeckle_nondark_min, despeckle_bright_threshold, despeckle_nonbright_max,
            checkerboard_square_size, checkerboard_minwhite, checkerboard_numwhite, checkerboard_percent,
            calculatedScore );
    }

  int err = 0;

  if( inputA->width!=inputB->width )
    {
    Warning( "Inconsistent width between %s/%s and %s", inputA->cameraID, NULLPROTECT(inputA->fileName), NULLPROTECT(inputB->fileName ));
    return -30;
    }

  if( inputA->height!=inputB->height )
    {
    Warning( "Inconsistent height between %s/%s and %s", inputA->cameraID, NULLPROTECT(inputA->fileName), NULLPROTECT(inputB->fileName ));
    return -31;
    }

  if( inputA->bpp!=inputB->bpp )
    {
    Warning( "Inconsistent bpp between %s/%s and %s", inputA->cameraID, NULLPROTECT(inputA->fileName), NULLPROTECT(inputB->fileName ));
    return -32;
    }

  int luminosityA = AverageImageLuminosity( inputA );
  int luminosityB = AverageImageLuminosity( inputB );

  if( luminosityA < color_dark || luminosityB < color_dark )
    {
    BoostLuminosity( inputA, dark_brightness_boost );
    BoostLuminosity( inputB, dark_brightness_boost );
    if( debug )
      {
      luminosityA = AverageImageLuminosity( inputA );
      luminosityB = AverageImageLuminosity( inputB );
      Notice( "Boosted input images luminosity %s/%s and %s now have luminosity %d, %d",
              inputA->cameraID, NULLPROTECT(inputA->fileName), NULLPROTECT(inputB->fileName), luminosityA, luminosityB );
      }
    }
  else
    {
    /*
    if( debug )
      {
      Notice( "No need to brighten images");
      }
    */
    }

  /* calculate diff image at full resolution but gray scale */
  _IMAGE* diffImage = NewImage( nickName, "diff",
                                inputA->width, inputA->height, 1 );

  ImageSubtractAbsoluteWithThresholdAndNeighbours( inputA, inputB,
                                                   diffImage, color_diff_threshold );
  /*
  if( debug )
    {
    Notice( "Calculated diff between %s/%s and %s", nickName, imageA, imageB );
    }
  */

  /* remove white dots on dark background and dark dots on light background
     from the grayscale diff image - that's just noise */
  _IMAGE* cleanDiffImage = NewImage( nickName, "despeckled",
                                     inputA->width, inputA->height, 1 );

  Despeckle( diffImage, cleanDiffImage,
             despeckle_dark_threshold,
             despeckle_nondark_min,
             despeckle_bright_threshold,
             despeckle_nonbright_max );

  /*
  if( debug )
    {
    int luminosityD = AverageImageLuminosity( cleanDiffImage, widthA, heightA, 1 );
    Notice( "%s/%s and %s yield a 'spotless' diff with %3d average luminosity",
            nickName, imageA, imageB, luminosityD );
    }
  */

  if( NOTEMPTY( diffImageName ) )
    {
    err = CompressJPEG( cleanDiffImage, diffImageName, JPEG_WRITE_IMAGEQUALITY );
    if( err==0 )
      {
      if( debug )
        {
        printf("Wrote %s\n", diffImageName );
        }
      }
    else
      {
      Error("Failed to write %s", diffImageName );
      }
    }

  int cbWidth = inputA->width / checkerboard_square_size;
  int cbHeight = inputA->height / checkerboard_square_size;
  /*
  if( debug )
    {
    Notice( "Allocating CB of %d x %d", cbWidth, cbHeight );
    }
  */

  _IMAGE* checkerBoard = NewImage( nickName, "checkerboard", cbWidth, cbHeight, 1);

  GrayScaleToCheckerboard( cleanDiffImage, checkerBoard,
                           checkerboard_square_size,
                           checkerboard_minwhite,
                           checkerboard_numwhite );
  /*
  if( debug )
    {
    Notice( "Generated %d/%d checkerboard of diff (%s/%s - %s)",
            cbWidth, cbHeight, nickName, imageA, imageB );
    }
  */

  if( NOTEMPTY( cbImageName ) )
    {
    err = CompressJPEG( checkerBoard, cbImageName, JPEG_WRITE_IMAGEQUALITY );
    if( err==0 )
      {
      if( debug )
        {
        printf("Wrote %s\n", cbImageName );
        }
      }
    else
      {
      Error("Failed to write %s", cbImageName );
      }
    }

  long totalCB = TotalPixels( checkerBoard ) / 255;
  long cbArea = checkerBoard->width * checkerBoard->height;
  double cbPercent = (double)totalCB / (double)cbArea * 100.0;
  if( debug )
    {
    Notice( "%s/%s and %s yield a checkerboard with %2.8lf%% white (T=%2.8lf%%)",
            nickName, NULLPROTECT(inputA->fileName), NULLPROTECT(inputB->fileName), cbPercent, checkerboard_percent );
    }

  if( calculatedScore!=NULL )
    {
    *calculatedScore = cbPercent;
    /*
    if( debug )
      {
      Notice( "Returned calculated score to caller" );
      }
    */
    }

  FreeImage( &checkerBoard );
  FreeImage( &cleanDiffImage );
  FreeImage( &diffImage );

  if( cbPercent >= checkerboard_percent )
    {
    return 0; /* motion! */
    }
  else
    {
    return 1; /* no motion */
    }
  }

void RemoveLinesFromCheckerboard( _IMAGE* checkerBoard,
                                   int rowWhitePixelsThreshold )
  {
  if( checkerBoard==NULL
      || checkerBoard->data==NULL
      || checkerBoard->width<=0
      || checkerBoard->height<=0
      || checkerBoard->bpp!=1 )
    return;

  int* rowBrightness = (int*)malloc( checkerBoard->height * sizeof(int) );
  JSAMPLE* ptr = checkerBoard->data;
  for( int rowNum=0; rowNum<checkerBoard->height; ++rowNum )
    {
    int numWhite = 0;
    for( int colNum=0; colNum<checkerBoard->width; ++colNum )
      {
      if( *ptr )
        {
        ++ numWhite;
        }
      ++ ptr;
      }
    rowBrightness[ rowNum ] = (numWhite >= rowWhitePixelsThreshold) ? 1 : 0;
    if( rowBrightness[ rowNum ] )
      {
      printf("Bright row!\n");
      }
    }

  ptr = checkerBoard->data + checkerBoard->width;
  for( int rowNum=1; rowNum<checkerBoard->height; ++rowNum )
    {
    if( rowBrightness[rowNum-1]==0
        && rowBrightness[rowNum]==1 )
      {
      rowBrightness[rowNum] = 0;
      for( int colNum=0; colNum<checkerBoard->width; ++colNum )
        {
        *ptr = 0;
        ++ptr;
        }
      }
    else
      {
      ptr += checkerBoard->width;
      }
    }

  free( rowBrightness );
  }

#define SAMPLE(IMG,X,Y,BPP,COLOR) IMG[((Y)*in->width+(X))*(BPP)+COLOR]
#define INTSAMPLE(IMG,X,Y,BPP,COLOR) (int)IMG[((Y)*in->width+(X))*(BPP)+COLOR]
void ImageAveragePixelsWithNeighbours( _IMAGE* in, _IMAGE* out )
  {
  if( in==NULL || in->data==NULL )
    {
    Warning("Cannot smooth NULL input");
    return;
    }
  if( out==NULL || out->data==NULL )
    {
    Warning("Cannot smooth NULL output");
    return;
    }
  if( in->width<2 || in->height<2 )
    {
    Warning("Cannot smooth image of fewer than 2x2 pixels");
    return;
    }
  if( in->width!=out->width
      || in->height!=out->height
      || in->bpp!=out->bpp )
    {
    Warning("Cannot smooth image where input/output images are of different sizes" );
    return;
    }

  out->_averageLuminosity = -1;
  /* do the corners */
  for( int color=0; color<in->bpp; color++ )
    {
    SAMPLE(out->data,0,0,in->bpp,color)
      = (INTSAMPLE(in->data,0,0,in->bpp,color)
         + INTSAMPLE(in->data,1,0,in->bpp,color)
         + INTSAMPLE(in->data,0,1,in->bpp,color)
         + INTSAMPLE(in->data,1,1,in->bpp,color))/4;
    SAMPLE(out->data,in->width-1,0,in->bpp,color)
      = (INTSAMPLE(in->data,in->width-1,0,in->bpp,color)
         + INTSAMPLE(in->data,in->width-2,0,in->bpp,color)
         + INTSAMPLE(in->data,in->width-1,1,in->bpp,color)
         + INTSAMPLE(in->data,in->width-2,1,in->bpp,color))/4;
    SAMPLE(out->data,in->width-1,in->height-1,in->bpp,color)
      = (INTSAMPLE(in->data,in->width-1,in->height-1,in->bpp,color)
         + INTSAMPLE(in->data,in->width-2,in->height-1,in->bpp,color)
         + INTSAMPLE(in->data,in->width-1,in->height-2,in->bpp,color)
         + INTSAMPLE(in->data,in->width-2,in->height-2,in->bpp,color))/4;
    SAMPLE(out->data,0,in->height-1,in->bpp,color)
      = (INTSAMPLE(in->data,0,in->height-1,in->bpp,color)
         + INTSAMPLE(in->data,1,in->height-1,in->bpp,color)
         + INTSAMPLE(in->data,1,in->height-2,in->bpp,color)
         + INTSAMPLE(in->data,0,in->height-2,in->bpp,color))/4;
    /* do the top and bottom edges */
    for( int x=1; x<in->width-1; ++x )
      {
      SAMPLE(out->data,x,0,in->bpp,color)
        = (INTSAMPLE(in->data,x-1,0,in->bpp,color)
           + INTSAMPLE(in->data,x,0,in->bpp,color)
           + INTSAMPLE(in->data,x+1,0,in->bpp,color)
           + INTSAMPLE(in->data,x-1,1,in->bpp,color)
           + INTSAMPLE(in->data,x,1,in->bpp,color)
           + INTSAMPLE(in->data,x+1,1,in->bpp,color))/6;

      SAMPLE(out->data,x,in->height-1,in->bpp,color)
        = (INTSAMPLE(in->data,x-1,in->height-1,in->bpp,color)
           + INTSAMPLE(in->data,x,in->height-1,in->bpp,color)
           + INTSAMPLE(in->data,x+1,in->height-1,in->bpp,color)
           + INTSAMPLE(in->data,x-1,in->height-2,in->bpp,color)
           + INTSAMPLE(in->data,x,in->height-2,in->bpp,color)
           + INTSAMPLE(in->data,x+1,in->height-2,in->bpp,color))/6;
      }

    /* do the left and right edges */
    for( int y=1; y<in->height-1; ++y )
      {
      SAMPLE(out->data,0,y,in->bpp,color)
        = (INTSAMPLE(in->data,0,y-1,in->bpp,color)
           + INTSAMPLE(in->data,0,y,in->bpp,color)
           + INTSAMPLE(in->data,0,y+1,in->bpp,color)
           + INTSAMPLE(in->data,1,y-1,in->bpp,color)
           + INTSAMPLE(in->data,1,y,in->bpp,color)
           + INTSAMPLE(in->data,1,y+1,in->bpp,color))/6;
      SAMPLE(out->data,in->width-1,y,in->bpp,color)
        = (INTSAMPLE(in->data,in->width-1,y-1,in->bpp,color)
           + INTSAMPLE(in->data,in->width-1,y,in->bpp,color)
           + INTSAMPLE(in->data,in->width-1,y+1,in->bpp,color)
           + INTSAMPLE(in->data,in->width-2,y-1,in->bpp,color)
           + INTSAMPLE(in->data,in->width-2,y,in->bpp,color)
           + INTSAMPLE(in->data,in->width-2,y+1,in->bpp,color))/6;
      }

    /* do the body of the image */
    for( int y=1; y<in->height-1; ++y )
      {
      for( int x=1; x<in->width-1; ++x )
        {
        SAMPLE(out->data,x,y,in->bpp,color)
          = (INTSAMPLE(in->data,x-1,y-1,in->bpp,color)
             + INTSAMPLE(in->data,x,y-1,in->bpp,color)
             + INTSAMPLE(in->data,x+1,y-1,in->bpp,color)
             + INTSAMPLE(in->data,x-1,y,in->bpp,color)
             + INTSAMPLE(in->data,x,y,in->bpp,color)
             + INTSAMPLE(in->data,x+1,y,in->bpp,color)
             + INTSAMPLE(in->data,x-1,y+1,in->bpp,color)
             + INTSAMPLE(in->data,x,y+1,in->bpp,color)
             + INTSAMPLE(in->data,x+1,y+1,in->bpp,color))/9;
        }
      }
    }
  }

void ImageSubtractWithThresholdPerColor( _IMAGE* a, _IMAGE* b,
                                          _IMAGE* diff, int threshold )
  {
  if( a==NULL || b==NULL || diff==NULL
      || a->data==NULL || b->data==NULL || diff->data==NULL )
    {
    Warning("Cannot diff without input, output and diff images");
    return;
    }
  if( a->width<1 || a->height<1 || a->bpp<1 || a->bpp>4 )
    {
    Warning("Image diff requires width>0, height>0, bpp between 1 and 4");
    return;
    }
  if( a->width!=b->width
      || a->height!=b->height
      || a->bpp!=b->bpp
      || a->width!=diff->width
      || a->height!=diff->height
      || a->bpp!=diff->bpp )

    {
    Warning("Image substraction requires a, b and diff images to have the same dimensions.");
    return;
    }
  if( threshold<0 )
    {
    Warning("Image diff does not work with negative threshold");
    return;
    }
  if( threshold > a->bpp * 256 )
    {
    Warning("Image diff - threshold makes no sense (%d)", threshold);
    return;
    }

  diff->_averageLuminosity = -1;
  for( int y=0; y < a->height; ++y )
    {
    for( int x=0; x < a->width; ++x )
      {
      for( int color=0; color < a->bpp; ++color )
        {
        int offset = (y*a->width + x)*a->bpp + color;
        int va = a->data[ offset ];
        int vb = b->data[ offset ];
        int d = abs( va-vb );
        if( d < threshold )
          {
          d = 0;
          }
        diff->data[ offset ] = d;
        }
      }
    }
  }

void ImageSubtractWithThresholdVector( _IMAGE* a, _IMAGE* b, _IMAGE* diff,
                                        int threshold )
  {
  if( a==NULL || b==NULL || diff==NULL
      || a->data==NULL || b->data==NULL || diff->data==NULL )
    {
    Warning("Cannot diff without input, output and diff images");
    return;
    }
  if( a->width<1 || a->height<1 || a->bpp<1 || a->bpp>4 )
    {
    Warning("Image diff requires width>0, height>0, bpp between 1 and 4");
    return;
    }
  if( a->width!=b->width
      || a->height!=b->height
      || a->bpp!=b->bpp
      || a->width!=diff->width
      || a->height!=diff->height
      || a->bpp!=diff->bpp )

    {
    Warning("Image subtraction (vector) requires a, b and diff images to have the same dimensions.");
    return;
    }
  if( threshold<0 )
    {
    Warning("Image diff does not work with negative threshold");
    return;
    }
  if( threshold>a->bpp*256 )
    {
    Warning("Image diff - threshold makes no sense (%d)", threshold);
    return;
    }

  diff->_averageLuminosity = -1;
  for( int y=0; y<a->height; ++y )
    {
    for( int x=0; x<a->width; ++x )
      {
      int offset = (y*a->width + x)*a->bpp;
      long value = 0;
      for( int color=0; color<a->bpp; ++color )
        {
        int va = a->data[ offset + color ];
        int vb = b->data[ offset + color ];
        long d = (long)abs( va-vb );
        value += d*d;
        }
      double d = (double)value;
      d = sqrt( d );
      value = (int)(d + 0.5);
      for( int color=0; color<a->bpp; ++color )
        {
        diff->data[ offset + color ] = (JSAMPLE)value;
        }
      }
    }
  }

/* inputs are colour; diff is grayscale */
void ImageSubtractAbsoluteWithThreshold( _IMAGE* a, _IMAGE* b,
                                          _IMAGE* diff, int threshold )
  {
  if( a==NULL || b==NULL || diff==NULL
      || a->data==NULL || b->data==NULL || diff->data==NULL )
    {
    Warning("Cannot diff without input, output and diff images");
    return;
    }
  if( a->width!=b->width
      || a->height!=b->height
      || a->bpp!=b->bpp
      || a->width!=diff->width
      || a->height!=diff->height
      || a->bpp!=diff->bpp )

    {
    Warning("Image subtraction(abs) requires a, b and diff images to have the same dimensions.");
    return;
    }
  if( a->width<1 || a->height<1 || a->bpp<1 || a->bpp>4 )
    {
    Warning("Image diff requires width>0, height>0, bpp between 1 and 4");
    return;
    }
  if( threshold<0 || threshold>255 )
    {
    Warning("Image diff - threshold makes no sense (%d)", threshold);
    return;
    }

  diff->_averageLuminosity = -1;
  for( int y=0; y<a->height; ++y )
    {
    for( int x=0; x<a->width; ++x )
      {
      /* QQQ offset can start as 0 and increment by bpp rather than
         this whole calc */
      int offset = (y*a->width + x)*a->bpp;
      long value = 0;
      for( int color=0; color<a->bpp; ++color )
        {
        int va = a->data[ offset + color ];
        int vb = a->data[ offset + color ];
        int d = abs( va-vb );
        if( d>threshold )
          {
          value += abs(d);
          }
        }
      if( value > 255 )
        {
        value = 255;
        }
      diff->data[ y * a->width + x ] = value;
      }
    }
  }

/* inputs are colour; output diff image is grayscale */

/*************************************************************
 This creates a grayscale image, where each pixel is a measure
 of how far the colours of the corresponding pixels in the
 two images are from one another.  There are caveats to this
 comparison that make it more useful in noisy images, however:

 - Colour distance = delta(red) + delta(green) + delta(blue)
   (we could have done sqrt( D(R)^2 + D(G)^2 + D(B)^2 ) but
   that's more computationally expensive and linear seems to
   be good enough for our purposes.

 - If the difference between any of the red, blue or green
   components of two pixels is less than a threshold amount, it is
   assumed to be "near enough" to zero and ignored.

 - Colour distance is compared to each neighbouring pixel as
   well.  The most similar neighbour is selected, on the theory that in
   a noisy image the same bit of the image may have shifted by a pixel
   or so between two successive frames, but without indicating any real
   change in scene.

 - For expediency, the 1-pixel edges of the input images
   are assumed to be identical and their differences are
   set to "0."  This is not actually true, but we don't
   care enough to incur the extra computational expense of
   calculating those.
 *************************************************************/
void ImageSubtractAbsoluteWithThresholdAndNeighbours( _IMAGE* a, _IMAGE* b,
                                                      _IMAGE* diff, int threshold )
  {
  if( a==NULL || b==NULL || diff==NULL
      || a->data==NULL || b->data==NULL || diff->data==NULL )
    {
    Warning("Cannot diff without input, output and diff images");
    return;
    }
  if( a->width!=b->width
      || a->height!=b->height
      || a->bpp!=b->bpp
      || a->width!=diff->width
      || a->height!=diff->height
      || diff->bpp != 1 )
    {
    Warning("Image subtraction requires a, b and diff images to have the same dimensions.");
    return;
    }
  if( a->width<1 || a->height<1 || a->bpp<1 || a->bpp>4 )
    {
    Warning("Image diff requires width>0, height>0, bpp between 1 and 4");
    return;
    }
  if( threshold<0 || threshold>255 )
    {
    Warning("Image diff - threshold makes no sense (%d)", threshold);
    return;
    }

  diff->_averageLuminosity = -1;

  /* zero out the top and bottom edge rows of pixels: */
  JSAMPLE* dPtr = diff->data;
  JSAMPLE* d2Ptr = diff->data + ((a->height-1)*a->width);
  for( int x=0; x<a->width; ++x )
    {
    *dPtr = 0;
    ++dPtr;
    *d2Ptr = 0;
    ++d2Ptr;
    }

  /* zero out the left and right edge rows of pixels: */
  dPtr = diff->data;
  d2Ptr = diff->data + (a->width-1);
  for( int y=0; y<a->height; ++y )
    {
    *dPtr = 0;
    dPtr += a->width;
    *d2Ptr = 0;
    d2Ptr += a->width;
    }

  /* these are the relative locations of 9 neighbour pixels */
  int offset[9];
  offset[0] = (-1 * a->width + -1) * a->bpp;
  offset[1] = (-1 * a->width + 0) * a->bpp;
  offset[2] = (-1 * a->width + 1) * a->bpp;
  offset[3] = (                -1) * a->bpp;
  offset[4] = (                0) * a->bpp;
  offset[5] = (                1) * a->bpp;
  offset[6] = (     a->width + -1) * a->bpp;
  offset[7] = (     a->width + 0) * a->bpp;
  offset[8] = (     a->width + 1) * a->bpp;

  /* dPtr is the diff pixel (mono) we'll write to */
  dPtr = diff->data + a->width + 1;

  /* aPtr and bPtr are the source pixels we'll read from */
  JSAMPLE* aPtr = a->data + (a->width + 1)*a->bpp;
  JSAMPLE* bPtr = b->data + (a->width + 1)*a->bpp;

  /* scan the inside of the source and destination images: */
  for( int y=1; y<a->height-1; ++y )
    {
    for( int x=1; x<a->width-1; ++x )
      {
      /* Calculate the color distance between the pixel in question
         and each neighbour, ignoring cases where they are sub-threshold
         apart.

         The diff image is assigned the smallest color distance to
         neighbours.
      */
      int colorDistanceToNeighbours = MAXJSAMPLE;
      for( int position=0; position<9; ++position )
        {
        int d=0;
        JSAMPLE* ap = aPtr;
        JSAMPLE* bp = bPtr + offset[position];
        for( int c=0; c<a->bpp; c++ )
          {
          int delta = abs( *ap - *bp );
          if( delta>threshold ) { d += delta; }
          ++ap;
          ++bp;
          }
        if( d < colorDistanceToNeighbours )
          {
          colorDistanceToNeighbours = d;
          }
        }

      *dPtr = colorDistanceToNeighbours;
      aPtr += a->bpp;
      bPtr += a->bpp;
      ++ dPtr;
      } /* end of row */

    /* skip last pixel on this line and first pixel on next line */
    aPtr += a->bpp * 2;
    bPtr += a->bpp * 2;
    dPtr += 2;
    } /* all interior rows */
  }

long TotalImageDifference( _IMAGE* a, _IMAGE* b, int threshold )
  {
  if( a==NULL || b==NULL
      || a->data==NULL || b->data==NULL )
    {
    Warning("Cannot diff without input and output images");
    return -1;
    }
  if( a->width<1 || a->height<1 || a->bpp<1 || a->bpp>4 )
    {
    Warning("Image diff requires width>0, height>0, bpp between 1 and 4");
    return -2;
    }
  if( a->width!=b->width
      || a->height!=b->height
      || a->bpp!=b->bpp )
    {
    Warning("TotalImageDifference requires a, b images to have the same dimensions.");
    return -3;
    }
  if( threshold<0 )
    {
    Warning("Image diff does not work with negative threshold");
    return -4;
    }
  if( threshold>a->bpp*256 )
    {
    Warning("Image diff - threshold makes no sense (%d)", threshold);
    return -5;
    }

  long total = 0;
  for( int y=0; y<a->height; ++y )
    {
    for( int x=0; x<a->width; ++x )
      {
      int offset = (y*a->width + x)*a->bpp;
      long value = 0;
      for( int color=0; color<a->bpp; ++color )
        {
        int va = a->data[ offset + color ];
        int vb = a->data[ offset + color ];
        long d = (long)abs( va-vb );
        value += d*d;
        }
      double d = (double)value;
      d = sqrt( d );
      value = (int)(d + 0.5);
      total += value;
      }
    }

  return total;
  }

void ImageGrayScale( _IMAGE* input, _IMAGE* output )
  {
  if( input==NULL || input->data==NULL )
    {
    Warning("Cannot smooth NULL input");
    return;
    }
  if( output==NULL || output->data==NULL )
    {
    Warning("Cannot smooth NULL output");
    return;
    }
  if( input->width<1 || input->height<1 )
    {
    Warning("Cannot smooth image of fewer than 1x1 pixels");
    return;
    }
  if( input->width!=output->width
      || input->height!=output->height
      || input->bpp!=output->bpp )
    {
    Warning("Image grayscale requires input, output images to have the same dimensions.");
    return;
    }

  output->_averageLuminosity = -1;
  for( int y=0; y<input->height; ++y )
    {
    for( int x=0; x<input->width; ++x )
      {
      double value = 0;
      for( int color=0; color<input->bpp; ++color )
        {
        int i = input->data[ (y*input->width + x)*input->bpp + color ];
        value += i*i;
        }
      value = sqrt(value) + 0.5;
      int ival = (int)value;
      output->data[ (y*output->width + x) ] = ival;
      }
    }
  }

void ImageInvertGrayScale( _IMAGE* input, _IMAGE* output )
  {
  if( input==NULL || input->data==NULL )
    {
    Warning("Cannot smooth NULL input");
    return;
    }
  if( output==NULL || output->data==NULL )
    {
    Warning("Cannot smooth NULL output");
    return;
    }
  if( input->width<1 || input->height<1 )
    {
    Warning("Cannot smooth image of fewer than 1x1 pixels");
    return;
    }
  if( input->width!=output->width
      || input->height!=output->height
      || input->bpp!=output->bpp )
    {
    Warning("Image invert grayscale requires input, output images to have the same dimensions.");
    return;
    }

  output->_averageLuminosity = -1;

  int maxSampleValue = (1 << BITS_IN_JSAMPLE)-1;

  for( int y=0; y<input->height; ++y )
    {
    for( int x=0; x<input->width; ++x )
      {
      int i = (int)(input->data[ (y*input->width + x) ]);
      i = maxSampleValue - i;
      output->data[ (y*input->width + x) ] = i;
      }
    }
  }

void ImageHalveResolution( _IMAGE* input, _IMAGE* output )
  {
  if( input==NULL || input->data==NULL )
    {
    Warning("Cannot smooth NULL input");
    return;
    }
  if( output==NULL || output->data==NULL )
    {
    Warning("Cannot smooth NULL output");
    return;
    }
  if( input->width<2 || input->height<2 )
    {
    Warning("Cannot smooth image of fewer than 1x1 pixels");
    return;
    }
  if( input->width%2 || input->height%2 )
    {
    Warning("Source width and height must be even numbers");
    return;
    }
  if( output->height*2 != input->height || output->width*2 != input->width )
    {
    Warning("Source width and height must be double destionation size");
    return;
    }
  if( output->bpp != input->bpp )
    {
    Warning("Source and destination bpp must be the same when halving resolution" );
    return;
    }

  output->_averageLuminosity = -1;

  for( int y=0; y<input->height; y+=2 )
    {
    for( int x=0; x<input->width; x+=2 )
      {
      for( int color=0; color<input->bpp; ++color )
        {
        int i = (int)(input->data[ (y*input->width + x) * input->bpp + color ])
              + (int)(input->data[ (y*input->width + x + 1) * input->bpp + color ])
              + (int)(input->data[ ((y+1)*input->width + x) * input->bpp + color ])
              + (int)(input->data[ ((y+1)*input->width + x + 1) * input->bpp + color ]);
        i = i / 4;
        output->data[ ((y/2)*output->width + (x/2)) * output->bpp + color ] = i;
        }
      }
    }
  }

int AverageImageLuminosity( _IMAGE* image )
  {
  if( image==NULL || image->data==NULL )
    {
    Warning("Cannot calculate luminosity of NULL image");
    return -1;
    }

  if( image->_averageLuminosity>=0 )
    {
    return image->_averageLuminosity;
    }

  if( image->width<1 || image->width>MAX_WIDTH )
    {
    Warning( "Cannot calculate luminosity of image with weird width (%d)",
             image->width );
    return -2;
    }

  if( image->height<1 || image->height>MAX_HEIGHT )
    {
    Warning( "Cannot calculate luminosity of image with weird height (%d)",
             image->height);
    return -3;
    }

  if( image->bpp<1 || image->bpp>4 )
    {
    Warning( "Cannot calculate luminosity of image with weird bpp (%d)",
             image->bpp);
    return -4;
    }

  long nPixels = 0;
  long pixelValues = 0;

  JSAMPLE* ptr = image->data;
  for( int y=0; y<image->height; y++ )
    {
    for( int x=0; x<image->width; x++ )
      {
      ++nPixels;
      for( int color=0; color<image->bpp; ++color )
        {
        pixelValues += *ptr;
        ++ptr;
        }
      }
    }

  pixelValues /= image->bpp;  /* average over colours */

  double n = (double)pixelValues / (double)nPixels;

  image->_averageLuminosity = (int)(n + 0.5);
  return image->_averageLuminosity;
  }

long TotalPixels( _IMAGE* image )
  {
  if( image==NULL || image->data==NULL )
    {
    Warning("Cannot calculate luminosity of NULL image");
    return -1;
    }

  if( image->width<1 || image->width>MAX_WIDTH )
    {
    Warning( "Cannot calculate luminosity of image with weird width (%d)",
             image->width );
    return -2;
    }

  if( image->height<1 || image->height>MAX_HEIGHT )
    {
    Warning( "Cannot calculate luminosity of image with weird height (%d)",
             image->height);
    return -3;
    }

  if( image->bpp<1 || image->bpp>4 )
    {
    Warning( "Cannot calculate luminosity of image with weird bpp (%d)",
             image->bpp);
    return -4;
    }

  long pixelValues = 0;

  for( int y=0; y<image->height; y++ )
    {
    for( int x=0; x<image->width; x++ )
      {
      for( int color=0; color<image->bpp; ++color )
        {
        pixelValues += image->data[ (y*image->width+x)*image->bpp+color ];
        }
      }
    }

  return pixelValues;
  }

double logTable[256];
int gotLogTable = 0;
int expLogTable[256];
double tableFactor = -1;

void BoostLuminosity( _IMAGE* image, double factor )
  {
  if( image==NULL || image->data==NULL )
    {
    Warning("Cannot calculate luminosity of NULL image");
    return;
    }

  if( image->width<1 || image->width>MAX_WIDTH )
    {
    Warning( "Cannot calculate luminosity of image with weird width (%d)",
             image->width );
    return;
    }

  if( image->height<1 || image->height>MAX_HEIGHT )
    {
    Warning( "Cannot calculate luminosity of image with weird height (%d)",
             image->height);
    return;
    }

  if( image->bpp<1 || image->bpp>4 )
    {
    Warning( "Cannot calculate luminosity of image with weird bpp (%d)",
             image->bpp);
    return;
    }

  image->_averageLuminosity = -1;

  if( gotLogTable==0 )
    {
    for( int i=0; i<256; ++i )
      {
      double d = (double)i;
      double l = log(d);
      logTable[i] = l;
      }
    gotLogTable = 1;
    }

  if( tableFactor==-1 )
    {
    for( int i=0; i<256; ++i )
      {
      double v =  exp( logTable[i] * factor ) + 0.5;
      if( v>255.0 ) v=255.0;
      expLogTable[i] = (int)v;
      tableFactor = factor;
      }
    }

  JSAMPLE* ptr = image->data;

  long luminosity = 0;
  int nSubPixels = image->height * image->width * image->bpp;

  int* expTable = NULL;
  if( tableFactor==factor )
    {
    expTable = expLogTable;
    }
  else
    {
    expTable = (int*)calloc( 256, sizeof(int) );
    for( int i=0; i<256; ++i )
      {
      double v =  exp( logTable[i] * factor ) + 0.5;
      if( v>255.0 ) v=255.0;
      expTable[i] = (int)v;
      }
    }

  for( int i=0; i<nSubPixels; ++i )
    {
    *ptr = expTable[ *ptr ];
    luminosity += *ptr;
    ++ptr;
    }

  if( expTable!=expLogTable )
    free( expTable );
  
  double average = luminosity;
  average /= (double)nSubPixels;
  average += 0.5;
  if( average>255.0 )
    {
    average = 255.0;
    }

  image->_averageLuminosity = (int)average;
  }

void CopyBorders( _IMAGE* input, _IMAGE* output )
  {
  if( input==NULL || input->data==NULL )
    {
    Warning("Cannot smooth NULL input");
    return;
    }
  if( output==NULL || output->data==NULL )
    {
    Warning("Cannot smooth NULL output");
    return;
    }
  if( output->bpp != input->bpp
      || output->width != input->width
      || output->height != input->height )
    {
    Warning("Source and destination bpp,image,width must be the same when copying borders" );
    return;
    }

  for( int x=0; x<input->width; x++ )
    {
    for( int color=0; color<input->bpp; ++color )
      {
      output->data[ x*input->bpp + color ] = input->data[ x*input->bpp + color ];
      output->data[ ((input->height-1)*input->width + x)*input->bpp + color ]
        = input->data[ ((input->height-1)*input->width + x)*input->bpp + color ];
      }
    }

  for( int y=0; y<input->height; y++ )
    {
    for( int color=0; color<input->bpp; ++color )
      {
      output->data[ y*input->width*input->bpp + color ]
        = input->data[ y*input->width*input->bpp + color ];
      output->data[ (y*input->width+input->width-1)*input->bpp + color ]
        = input->data[ (y*input->width+input->width-1)*input->bpp + color ];
      }
    }
  }

#define AddUpColors()\
  ({\
  int thisC = 0;\
  for( int color=0; color<input->bpp; ++color )\
    {\
    thisC += *pos;\
    ++pos;\
    }\
  thisC;\
  })


#define CopyPixel(input,output,x,y)\
  {\
  JSAMPLE* out = outputPosition;\
  JSAMPLE* in = inputPosition;\
  for( int color=0; color<input->bpp; color++ )\
    {\
    *(out++) = *(in++);\
    }\
  }

#define AverageBorderToPixel()\
  for( int color=0; color<input->bpp; color++ )\
    {\
    int val\
      = *(inputPosition + borderOffset[0] + color )\
      + *(inputPosition + borderOffset[1] + color )\
      + *(inputPosition + borderOffset[2] + color )\
      + *(inputPosition + borderOffset[3] + color )\
      /* 4 is center box - skip */\
      + *(inputPosition + borderOffset[5] + color )\
      + *(inputPosition + borderOffset[6] + color )\
      + *(inputPosition + borderOffset[7] + color )\
      + *(inputPosition + borderOffset[8] + color )\
      ;\
    *(outputPosition + color) = val / 8;\
    }\

void Despeckle( _IMAGE* input, _IMAGE* output,
                 int darkThreshold, int nonDarkMin,
                 int brightThreshold, int nonBrightMax )
  {
  if( input==NULL || output==NULL
      || input->data==NULL || output->data==NULL )
    Error("Despeckle with at least one missing image");
  if( input->width!=output->width
      || input->height!=output->height
      || input->bpp!=output->bpp )
    {
    Warning("Despeckle requires same-dimension input/output"
            "(%d,%d,%d)-->(%d,%d,%d)",
            input->width, input->height, input->bpp,
            output->width, output->height, output->bpp);
    return;
    }
  if( darkThreshold<0 || darkThreshold>(255*input->bpp) )
    Error("Despeckle with invalid dark threshold");
  if( brightThreshold<0 || brightThreshold>(255*input->bpp) )
    Error("Despeckle with invalid bright threshold");
  if( brightThreshold<darkThreshold )
    Error("Despeckle with inconsistent thresholds");
  if( input->width<3 || input->height<3 )
    Error("Despeckle on image that is too small");

  output->_averageLuminosity = -1;

  CopyBorders( input, output );

  int borderOffset[9];
  borderOffset[0] = (-1 * input->width - 1) * input->bpp;
  borderOffset[1] = (-1 * input->width    ) * input->bpp;
  borderOffset[2] = (-1 * input->width + 1) * input->bpp;
  borderOffset[3] = (                   -1) * input->bpp;
  borderOffset[4] = 0;
  borderOffset[5] = (                    1) * input->bpp;
  borderOffset[6] = (     input->width - 1) * input->bpp;
  borderOffset[7] = (     input->width    ) * input->bpp;
  borderOffset[8] = (     input->width + 1) * input->bpp;

  /* copy top and bottom rows over */
  memcpy( output->data, input->data, input->width * input->bpp );
  memcpy( output->data + (input->height-1)*input->width*input->bpp,
          input->data + (input->height-1)*input->width*input->bpp,
          input->width * input->bpp );

  /* copy left and right edges over */
  JSAMPLE* leftDestPtr = output->data + input->width * input->bpp;
  JSAMPLE* rightDestPtr = leftDestPtr + (input->width-1) * input->bpp;
  JSAMPLE* leftSrcPtr = input->data + input->width * input->bpp;
  JSAMPLE* rightSrcPtr = leftSrcPtr + (input->width-1) * input->bpp;
  for( int y=1; y<input->height-1; y++ )
    {
    for( int color=0; color<input->bpp; ++color )
      {
      *(leftDestPtr++) = *(leftSrcPtr++);
      *(rightDestPtr++) = *(rightSrcPtr++);
      }
    }

  JSAMPLE* inputPosition = input->data + (input->width+1) * input->bpp;
  JSAMPLE* outputPosition = output->data + (input->width+1) * input->bpp;
  JSAMPLE* pos = NULL;
  for( int y=1; y<input->height-1; y++ )
    {
    for( int x=1; x<input->width-1; x++ )
      {
      pos = inputPosition;
      int thisC = AddUpColors();
      pos = inputPosition + borderOffset[0];
      int borderC = AddUpColors();
      borderC += AddUpColors();
      borderC += AddUpColors();
      pos = inputPosition + borderOffset[3];
      borderC += AddUpColors();
      pos = inputPosition + borderOffset[5];
      borderC += AddUpColors();
      pos = inputPosition + borderOffset[6];
      borderC += AddUpColors();
      borderC += AddUpColors();
      borderC += AddUpColors();
      borderC /= 8;

      if( ( thisC <= darkThreshold && borderC >= nonDarkMin )
          || ( thisC >= brightThreshold && borderC <= nonBrightMax ) )
        {
        AverageBorderToPixel();
        }
      else
        {
        CopyPixel( input, output, x, y );
        }

      inputPosition += input->bpp;
      outputPosition += input->bpp;
      }

    inputPosition += 2*input->bpp;
    outputPosition += 2*input->bpp;
    }
  }

void GrayScaleToCheckerboard( _IMAGE* input, _IMAGE* output,
                               int pixelsPerSquare,
                               int minValueWhite,
                               int minWhitesPerSquare )
  {
  if( input==NULL || output==NULL
      || input->data==NULL || output->data==NULL )
    Error( "GrayScaleToCheckerboard - NULL input or output" );
  if( input->bpp!=1 || output->bpp!=1 )
    Error("GrayScaleToCheckerboard requires grayscale input/output");
  if( input->width!=(output->width * pixelsPerSquare)
      || input->height!=(output->height * pixelsPerSquare) )
    Error("GrayScaleToCheckerboard requires correlated input/output"
          " (%d,%d) --> (%d,%d)",
          input->width, input->height,
          output->width, output->height );
  if( input->width < pixelsPerSquare || input->width%pixelsPerSquare!=0 )
    Error( "GrayScaleToCheckerboard - width %d not compatible with pps %d",
           input->width, pixelsPerSquare );
  if( input->height < pixelsPerSquare || input->height%pixelsPerSquare!=0 )
    Error( "GrayScaleToCheckerboard - height %d not compatible with pps %d",
           input->height, pixelsPerSquare );
  if( minValueWhite<1 || minValueWhite>255 )
    Error( "GrayScaleToCheckerboard - minValueWhite %d invalid", minValueWhite );
  if( minWhitesPerSquare<1 || minWhitesPerSquare>(pixelsPerSquare*pixelsPerSquare) )
    Error( "GrayScaleToCheckerboard - minWhitesPerSquare %d invalid", minWhitesPerSquare );

  output->_averageLuminosity = -1;

  int outputPos = 0;
  int maxOutputPos = (input->width/pixelsPerSquare)
                   * (input->height/pixelsPerSquare);
  for( int y=0; y<input->height; y+=pixelsPerSquare )
    {
    for( int x=0; x<input->width; x+=pixelsPerSquare )
      {
      if( outputPos > maxOutputPos )
        {
        Warning( "Checkerboard generator - out of bounds (%d:%d)",
                 outputPos, maxOutputPos );
        break;
        }
      int nWhiteInSquare = 0;

      int pos = y * input->width + x;
      for( int y1=0; y1<pixelsPerSquare; y1++ )
        {
        int pos0 = pos;
        for( int x1=0; x1<pixelsPerSquare; x1++ )
          {
          int v = input->data[ pos ];
          if( v >= minValueWhite )
            {
            ++nWhiteInSquare;
            }
          pos++;
          }
        pos = pos0 + input->width;
        }

      output->data[ outputPos ] = (nWhiteInSquare>=minWhitesPerSquare) ? 255 : 0;
      ++outputPos;
      }
    }
  }

int ScaleImage( _IMAGE* input, _IMAGE* output )
  {
  if( input==NULL || output==NULL
      || input->data==NULL || output->data==NULL )
    {
    Warning( "Cannot scale with a NULL input or output (%p, %p)", input, output );
    return -1;
    }
  if( input->width < 1 || input->width > MAX_WIDTH )
    {
    Warning( "Cannot scale with unreasonable input width (%d)", input->width );
    return -2;
    }
  if( input->height < 1 || input->height > MAX_HEIGHT )
    {
    Warning( "Cannot scale with unreasonable input height (%d)", input->height );
    return -3;
    }
  if( output->width < 1 || output->width > MAX_WIDTH )
    {
    Warning( "Cannot scale with unreasonable output width (%d)", output->width );
    return -4;
    }
  if( output->height < 1 || output->height > MAX_HEIGHT )
    {
    Warning( "Cannot scale with unreasonable output height (%d)", output->height );
    return -5;
    }
  if( output->width > input->width || output->height > input->height )
    {
    Warning( "Only down-scaling is supported.  %d:%d --> %d:%d",
             input->width, input->height, output->width, output->height );
    return -6;
    }
  if( input->bpp<1 || input->bpp>4 )
    {
    Warning( "Non-credible BPP in ScaleImage (%d)", input->bpp );
    return -7;
    }
  if( input->bpp != output->bpp )
    {
    Warning( "BPP must be same in input/output when scaling");
    return -8;
    }

  output->_averageLuminosity = -1;

  double xScale = (double)input->width / (double)output->width;
  double yScale = (double)input->height / (double)output->height;

  /* int nPixels = 0; */
  for( int y=0; y<output->height; y++ )
    {
    int iY = (int)(y * yScale + 0.5);
    if( iY>=input->height ) iY = input->height-1;

    for( int x=0; x<output->width; x++ )
      {
      int iX = (int)(x * xScale + 0.5);
      if( iX>=input->width ) iX = input->width-1;

      JSAMPLE* out = output->data + (y*output->width + x)*input->bpp;
      JSAMPLE* in = input->data + (iY*input->width + iX)*input->bpp;

      /* primitive copy algorithm.  average pixels in later version */
      for( int c=0; c<input->bpp; c++ )
        {
        *(out++) = *(in++);
        }
      /* ++nPixels; */
      }
    }

  /*
  Notice("Scaled image from %dx%d to %dx%d (%d)", input->width, input->height,
          output->width, output->height, nPixels );
  */

  return 0;
  }

int CalculateScaledHeight( int iWidth, int iHeight, int maxWidth,
                           int* oWidth, int* oHeight )
  {
  if( iWidth < 1 || iWidth > MAX_WIDTH )
    {
    Warning("Unreasonable width to scale (%d)", iWidth );
    return -1;
    }

  if( iHeight < 1 || iHeight > MAX_HEIGHT )
    {
    Warning("Unreasonable height to scale (%d)", iHeight );
    return -2;
    }

  if( maxWidth < 1 || maxWidth > MAX_WIDTH )
    {
    Warning("Unreasonable width to scale to (%d)", maxWidth );
    return -3;
    }

  if( oWidth==NULL || oHeight==NULL )
    {
    Warning("No way to return results from scale calculation" );
    return -4;
    }

  if( iWidth <= maxWidth )
    {
    *oWidth = iWidth;
    *oHeight = iHeight;
    return 0;
    }

  *oWidth = maxWidth;
  double scale = (double)maxWidth / (double)iWidth;
  double height = (double)iHeight * scale + 0.5;
  int intHeight = (int)height;
  *oHeight = intHeight;

  return 0;
  }
