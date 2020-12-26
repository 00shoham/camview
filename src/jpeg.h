#ifndef _INCLUDE_JPEG
#define _INCLUDE_JPEG

int DecompressJPEGFromBytes(
      unsigned char* compressedData, long jpegSize, _IMAGE* img );

_IMAGE* ImageFromJPEGFile( char* nickName, char* fileName );

_IMAGE* ImageFromJPEGFile2( char* nickName, char* path, char* fileName );

int CompressJPEG( _IMAGE* img, char* fileName, int quality );

#endif
