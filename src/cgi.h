#ifndef _INCLUDE_CGI
#define _INCLUDE_CGI

#define LF 10
#define CR 13

typedef struct _cgi_header
  {
  char* separatorString;
  _TAG_VALUE* files;
  _TAG_VALUE* headers;
  } _CGI_HEADER;

enum postState
  {
  ps_FIRSTHEADER,
  ps_SEPARATOR,
  ps_HEADERLINE,
  ps_VARIABLE,
  ps_DATA
  };

int (*funcPtr)( _CONFIG* , _CGI_HEADER * );

int ParsePostData( _CONFIG* conf,
                    FILE* stream,
                    _CGI_HEADER *header,
                    int (*funcPtr)( _CONFIG* , _CGI_HEADER * ) );

void CGIHeader( char* contentType,
                long contentLength,
                char* pageTitle,
                int nCSS, char** css,
                int nJS, char** js
                );
void CGIFooter();
void DumpJPEGToBrowser( char* nickName, long nBytes, char* fileName );

_TAG_VALUE* ParseHeaderSubVariables( _TAG_VALUE* list, char* buf );
_TAG_VALUE* ParseHeaderLine( _TAG_VALUE* workingHeaders, char* buf );
_TAG_VALUE* HeadersContainTag( _TAG_VALUE* list, char* tag );
int HeadersContainTagAndSubTag( _TAG_VALUE* list, char* tag, char* subTag );
_TAG_VALUE* ParseValue( char* buf, _TAG_VALUE* workingHeaders );
_TAG_VALUE* AppendValue( char* buf, _TAG_VALUE* workingHeaders );
_TAG_VALUE* ParseQueryString( _TAG_VALUE* list, char* string );
void DownloadFile( long filesize, char* path, char* fileName );

#endif
