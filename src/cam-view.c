#include "base.h"

extern int glob_argc;
extern char** glob_argv;
extern _CONFIG* glob_conf;
extern int inCGI;

void CGIBody()
  {
  /* CGIHeader( "text/html", 0, CAMVIEW_TITLE, 0, NULL, 0, NULL); */
  char cameras[BIGBUF];
  char* ptr = cameras;
  char* end = ptr + sizeof( cameras ) - 1;
  strcpy( ptr, "var cameras = [];\n" );
  ptr += strlen( ptr );

  char* whoAmI = ExtractUserIDOrDie( cm_api, glob_conf->userEnvVar );

  int cameraNum = 0;
  for( _CAMERA* cam=glob_conf->cameras; cam!=NULL; cam=cam->next )
    {
    if( IsUserInGroups( whoAmI, cam->access )!=0 )
      continue;

    if( EMPTY( cam->nickName ) )
      {
      Error( "Camera %d has no nickname", cameraNum );
      }
    snprintf( ptr, end-ptr,
              "cameras[%d] = { name:\"%s\", imageFilename:\"\" };\n",
              cameraNum, cam->nickName );
    ptr += strlen( ptr );
    ++cameraNum;
    }

  if( cameraNum==0 )
    {
    Error( "No cameras found in configuration file and/or accessible to %s", whoAmI );
    }

  snprintf( ptr, end-ptr, "var nCams = %d;\n", cameraNum );

  char* template = NULL;
  long templateLen = FileRead( CAMVIEW_TEMPLATE, (unsigned char**)&template );
  if( templateLen<=0 )
    {
    Error( "Cannot open template page %s", CAMVIEW_TEMPLATE );
    }

  char* confName = DEFAULT_CONF_NAME;
  if( NOTEMPTY( glob_conf->configName ) )
    {
    confName = glob_conf->configName;
    }

  char page[BIGBUF];
  int err = ExpandMacrosVA( template, page, sizeof(page)-1,
                            "CAMERAS", cameras,
                            "TITLE", confName,
                            NULL, NULL );

  if( err<1 )
    {
    Error( "Failed to expand CAMERAS macro in %s", CAMVIEW_TEMPLATE );
    }
  free( template );

  fputs( page, stdout );
  fputs( "\r\n\r\n", stdout );
  }

int main( int argc, char** argv )
  {
  glob_argc = argc;
  glob_argv = argv;

  CGIHeader( "text/html", 0, CAMVIEW_TITLE, 0, NULL, 0, NULL);

  _CONFIG* config = NULL;
  config = (_CONFIG*)calloc(1, sizeof(_CONFIG) );
  glob_conf = config;

  char* confName = CONFIGNAME;
  if( FileExists( confName )!=0 )
    {
    confName = ALTCONFIGNAME;
    }
  if( FileExists( confName )!=0 )
    {
    CGIHeader( NULL, 0, NULL, 0, NULL, 0, NULL);

    Error( "Cannot locate %s or %s", CONFIGNAME, ALTCONFIGNAME );
    }
  SetDefaults( config );
  ReadConfig( config, confName );

  if( EMPTY( config->cgiLogFile ) )
    {
    CGIHeader( NULL, 0, NULL, 0, NULL, 0, NULL);

    Error( "You must specify CGILOGFILE in the configuration file %s",
           CONFIGNAME );
    }

  logFileHandle = fopen( config->cgiLogFile, "a" );
  if( logFileHandle==NULL )
    {
    CGIHeader( NULL, 0, NULL, 0, NULL, 0, NULL);
    Error( "Failed to open %s", config->cgiLogFile );
    }

  CGIBody();

  fclose( logFileHandle );

  return 0;
  }
