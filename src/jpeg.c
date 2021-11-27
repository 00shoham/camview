#include "base.h"

int DecompressJPEGFromBytes(
      unsigned char* compressedData, long jpegSize, _IMAGE* img )
  {
  if( compressedData==NULL )
    {
    Warning("DecompressJPEGFromBytes requires a compressed data stream");
    return -1;
    }

  if( jpegSize<=0 )
    {
    Warning("DecompressJPEGFromBytes requires a non-zero data set to operate on");
    return -2;
    }

  if( img==NULL )
    {
    Warning("DecompressJPEGFromBytes requires pointers to an image struct");
    return -3;
    }

  /* jpeg library initialization */
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;

  cinfo.err = jpeg_std_error( &jerr );
  jpeg_create_decompress( &cinfo );
  jpeg_mem_src( &cinfo, compressedData, jpegSize );
  if( jpeg_read_header(&cinfo, TRUE)!=1 )
    {
    Warning("Invalid JPEG data stream");
    return -4;
    }
  if( jpeg_start_decompress( &cinfo )!=1 )
    {
    Warning("Failed to initialize JPEG decompress");
    return -5;
    }

  img->width = cinfo.output_width;
  img->height = cinfo.output_height;
  img->bpp = cinfo.output_components;

  long rawDataSize = img->width
                   * img->height
                   * img->bpp * sizeof(JSAMPLE);

  if( img->data!=NULL )
    {
    free( img->data );
    img->data = NULL;
    }

  img->data = (JSAMPLE*)malloc( rawDataSize );
  if( img->data==NULL )
    {
    Warning( "Failed to allocate %ld bytes when decompressing a JPEG file",
             rawDataSize );
    return -6;
    }

  int samplesPerRow = img->bpp * img->width;
  while( cinfo.output_scanline < img->height )
    {
    JSAMPLE *oneLine[1];
    oneLine[0] = img->data + cinfo.output_scanline * samplesPerRow;

    if( jpeg_read_scanlines( &cinfo, oneLine, 1 ) != 1 )
      {
      Warning( "Failed to decompress line %d of JPEG",
               (int)(cinfo.output_scanline) );
      return -7;
      }
    }

  jpeg_finish_decompress( &cinfo );
  jpeg_destroy_decompress( &cinfo );

  return 0;
  }

_IMAGE* ImageFromJPEGFile( char* nickName, char* fileName )
  {
  if( EMPTY( nickName ) )
    Error( "ImageFromJPEGFile - must specify camera ID" );

  if( EMPTY( fileName ) )
    Error( "ImageFromJPEGFile - must specify fileName" );

  unsigned char* fileData = NULL;
  long fileBytes = FileRead( fileName, &fileData );
  if( fileBytes<=0 )
    {
    if( fileData!=NULL )
      free( fileData );
    Warning( "ImageFromJPEGFile: %s empty", fileName );
    return NULL;
    }
  _IMAGE* image = NewImage( nickName, GetFilenameFromPath( fileName ), 0, 0, 0 );
  int err = DecompressJPEGFromBytes( fileData, fileBytes, image );
  FREE( fileData );
  if( err )
    {
    FreeImage( &image );
    return NULL;
    }
  return image;
  }

_IMAGE* ImageFromJPEGFile2( char* nickName, char* path, char* fileName )
  {
  char* fullPath = MakeFullPath( path, fileName );
  _IMAGE* ptr = ImageFromJPEGFile( nickName, fullPath );
  FREE( fullPath );
  return ptr;
  }

int CompressJPEG( _IMAGE* img, char* fileName, int quality )
  {
  if( img==NULL || img->data==NULL )
    {
    Warning( "Cannot compress JPEG of NULL image" );
    return -1;
    }

  if( img->width<1
      || img->height<1
      || img->bpp<1
      || img->bpp>4 )
    {
    Warning( "Cannot compress JPEG with invalid dimensions (%d x %d x %d)",
             img->width, img->height, img->bpp );
    return -2;
    }

  if( EMPTY( fileName ) )
    {
    Warning( "Cannot compress JPEG into empty filename" );
    return -3;
    }

  if( quality<0 || quality>100 )
    {
    Warning( "Cannot compress JPEG with invalid quality (%d - must be 0:100)",
             quality );
    return -4;
    }

  FILE* f = NULL;
  if( strcmp( fileName, "-" )==0 )
    {
    f = stdout;
    }
  else
    {
    f = fopen( fileName, "w" );
    if( f==NULL )
      {
      Warning( "Cannot compress JPEG - cannot write to %s", fileName );
      return -5;
      }
    }

  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;
  cinfo.err = jpeg_std_error( &jerr );
  jpeg_create_compress( &cinfo );

  jpeg_stdio_dest( &cinfo, f );

  cinfo.image_width = img->width;
  cinfo.image_height = img->height;
  cinfo.input_components = img->bpp;
  if( img->bpp==3 )
    cinfo.in_color_space = JCS_RGB;
  else if( img->bpp==4 ) /* I guess? */
    cinfo.in_color_space = JCS_RGB;
  else if( img->bpp==1 )
    cinfo.in_color_space = JCS_GRAYSCALE;
  else
    Error( "What color space should I for %d bytes/pixel when encoding JPEG?",
           img->bpp );

  jpeg_set_defaults( &cinfo );
  jpeg_set_quality( &cinfo, quality, TRUE );
  jpeg_start_compress( &cinfo, TRUE );

  int samplesPerRow = img->width * img->bpp;

  while( cinfo.next_scanline < img->height )
    {
    JSAMPROW rowPtr[1];
    rowPtr[0] = img->data + cinfo.next_scanline * samplesPerRow;
    if( jpeg_write_scanlines( &cinfo, rowPtr, 1 )!=1 )
      {
      Warning( "Failed to compress line %d of JPEG",
               (int)(cinfo.next_scanline) );
      return -7;
      }
    }

  jpeg_finish_compress( &cinfo );
  if( strcmp( fileName, "-" )==0 )
    { /* no need to close stdout */ }
  else
    {
    fclose( f );
    }

  jpeg_destroy_compress( &cinfo );

  return 0;
  }

