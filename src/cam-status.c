#include "base.h"

extern int glob_argc;
extern char** glob_argv;
extern _CONFIG* glob_conf;
extern int inCGI;

#define MONITOR_BINARY_NAME "cam-monitor"
#define MIN_LIVE_SPACE   512l * 1024l * 1024l /* should have 1/2 GB free on capture drive */
#define MIN_BACKUP_SPACE 30l * 1024l * 1024l * 1024l /* should have 30 GB free on capture drive */
#define CAM_STATUS_PAGE_TITLE "System Status"
#define STATUS_HEAD_TEMPLATE "cam-status-head.template"
#define STATUS_FOOT_TEMPLATE "cam-status-foot.template"

long GetMonitorPID()
  {
  FILE* f = popen( "/bin/ps -ef", "r" );
  if( f==NULL )
    {
    Error( "Cannot run /bin/ps -ef" );
    }

  long mainPid = -1;
  char buf[BUFLEN];
  while( fgets( buf, sizeof(buf)-1, f )==buf )
    {
    if( strstr( buf, MONITOR_BINARY_NAME )!=NULL )
      {
      long pid=-1, ppid=-1;
      char uid[BUFLEN], c[BUFLEN], stime[BUFLEN], tty[BUFLEN],
           time[BUFLEN], cmd[BUFLEN];
      if( sscanf( buf, "%s %ld %ld %s %s %s %s %s",
                  uid, &pid, &ppid, c, stime, tty, time, cmd )==8
          && strstr( cmd, MONITOR_BINARY_NAME )!=NULL )
        {
        if( ppid==1 ) /* main job has a parent PID of 1 */
          {
          mainPid = pid;
          }
        }
      }

    if( mainPid>0 )
      {
      break;
      }
    }
  pclose( f );

  return mainPid;
  }

int DoesProcessExist( long pid )
  {
  char cmd[BUFLEN];
  snprintf( cmd, sizeof(cmd)-1, "ps -p '%ld'", pid );
  FILE* f = popen( cmd, "r" );
  if( f==NULL )
    return -1;
  char buf[BUFLEN];
  char pids[BUFLEN];
  snprintf( pids, sizeof(pids)-1, "%ld", pid );

  int gotIt = 0;
  while( fgets( buf, sizeof(buf)-1, f )==buf )
    {
    if( strstr( buf, pids )!=NULL )
      {
      gotIt = 1;
      break;
      }
    }
  pclose( f );

  if( gotIt )
    return 0;

  return -2;
  }

int main( int argc, char** argv )
  {
  int nProblems = 0;
  int nCamerasWithProblems = 0;
  glob_argc = argc;
  glob_argv = argv;

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

  for( int i=1; i<argc; ++i )
    {
    if( strcmp( argv[i], "-c" )==0 && i+1<argc )
      {
      ++i;
      confName = argv[i];
      }
    else
      {
      CGIHeader( NULL, 0, NULL, 0, NULL, 0, NULL);
      Error("Invalid argument: %s (use -c conffile)", argv[i] );
      }
    }

  SetDefaults( config );
  ReadConfig( config, confName );

  char cameras[BIGBUF];
  char* ptr = cameras;
  char* end = ptr + sizeof( cameras ) - 1;
  strcpy( ptr, "var cameras = [];\n" );
  ptr += strlen( ptr );

  int cameraNum = 0;
  for( _CAMERA* cam=glob_conf->cameras; cam!=NULL; cam=cam->next )
    {
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
    Error( "No cameras found in configuration file" );
    }

  snprintf( ptr, end-ptr, "var nCams = %d;\n", cameraNum );

  char pageHead[BIGBUF];
  char* templateHead = NULL;
  long templateLenHead = FileRead( STATUS_HEAD_TEMPLATE, (unsigned char**)&templateHead );
  if( templateLenHead<=0 )
    {
    FREE( templateHead );
    CGIHeader( NULL, 0, NULL, 0, NULL, 0, NULL);
    Error( "Cannot open template page %s (%ld)", STATUS_HEAD_TEMPLATE, templateLenHead );
    }
  int err = ExpandMacrosVA( templateHead, pageHead, sizeof(pageHead)-1,
                            "TITLE", CAM_STATUS_PAGE_TITLE,
                            "CAMERAS", cameras,
                            NULL, NULL );
  FREE( templateHead );
  if( err<0 )
    {
    CGIHeader( NULL, 0, NULL, 0, NULL, 0, NULL);
    Error( "Cannot expand page header template" );
    }

  char* templateFoot = NULL;
  long templateLenFoot = FileRead( STATUS_FOOT_TEMPLATE, (unsigned char**)&templateFoot );
  if( templateLenFoot<=0 )
    {
    Error( "Cannot open template page %s (%d)", STATUS_FOOT_TEMPLATE, templateLenFoot );
    }

  CGIHeader( "text/html", 0, NULL, 0, NULL, 0, NULL);
  fputs( pageHead, stdout );

  long pid = GetMonitorPID();
  if( pid>0 )
    {
    printf( "<p class=\"status-good\">Monitor process is running (%ld).</p>\n", pid);
    }
  else
    {
    printf( "<p class=\"status-bad\">Monitor process not found.</p>\n" );
    ++nProblems;
    } 

  long spaceLive = GetAvailableSpaceOnVolumeBytes( config->baseDir );
  printf( "<p class=\"status-%s\">Space available on capture drive: %ld MBytes</p>\n",
          spaceLive>MIN_LIVE_SPACE ? "good" : "bad",
          spaceLive/1024/1024 );

  if( spaceLive<=MIN_LIVE_SPACE ) ++nProblems;

  long spaceBackup = GetAvailableSpaceOnVolumeBytes( config->backupDir );
  printf( "<p class=\"status-%s\">Space available on backup drive: %ld MBytes</p>\n",
          spaceBackup>MIN_BACKUP_SPACE ? "good" : "bad",
          spaceBackup/1024/1024 );

  if( spaceBackup<=MIN_BACKUP_SPACE ) ++nProblems;

  for( _CAMERA* cam=config->cameras; cam!=NULL; cam=cam->next )
    {
    int problemsThisCamera = 0;
    printf( "<div class=\"camera\">Camera: %s\n", NULLPROTECT( cam->nickName ) );
    if( EMPTY( cam->nickName ) )
      {
      continue;
      }
    CameraFolder( config, cam ); /* does the chdir too */
    char pidFile[BUFLEN];
    snprintf( pidFile, sizeof(pidFile)-1, "%s.pid", cam->nickName );
    FILE* f = fopen( pidFile, "r" );
    if( f==NULL )
      {
      printf( "<p class=\"status-bad in-camera\">Cannot open PID file %s</p>\n", pidFile );
      ++nProblems;
      ++problemsThisCamera;
      continue;
      }
    long pid = -1;
    if( fscanf( f, "%ld", &pid )!=1 )
      {
      printf( "<p class=\"status-bad in-camera\">PID file %s does not contain a PID</p>\n", pidFile );
      ++nProblems;
      ++problemsThisCamera;
      }
    fclose( f );
    f = NULL;

    if( pid<0 )
      {
      printf( "<p class=\"status-bad in-camera\">PID file %s does not contain a PID</p>\n", pidFile );
      ++nProblems;
      ++problemsThisCamera;
      }
    else
      {
      if( DoesProcessExist( pid )==0 )
        {
        printf( "<p class=\"status-good in-camera\">Capture process is running (%ld).</p>\n", pid);
        }
      else
        {
        printf( "<p class=\"status-bad in-camera\">Capture process %ld has stopped.</p>\n", pid );
        ++nProblems;
      ++problemsThisCamera;
        }
      }

    char** folder = NULL;
    int nFiles = GetOrderedDirectoryEntries( cam->folderPath, NULL, ".jpg", &folder, 1 );

    if( nFiles>0 )
      {
      time_t tLast = FileDate( folder[nFiles-1] );
      int age = time(NULL) - tLast;
      if( age > IMAGE_TOO_OLD )
        {
        char date[20], time[20];
        printf( "<p class=\"status-bad in-camera\">Last image (%s) too old (%d seconds since %s %s).</p>\n",
                folder[nFiles-1], age, DateStr( tLast, date, sizeof(date)-1 ),
                TimeStr( tLast, time, sizeof(time)-1, 1 ) );
        ++nProblems;
        ++problemsThisCamera;
        }
      else
        {
        printf( "<p class=\"status-good in-camera\">Most recent image is good (%d seconds ago).</p>\n", age);
        }

      if( nFiles > TOO_MANY_FILES*2 )
        {
        printf( "<p class=\"status-bad in-camera\">There are %d images -- too many -- in the capture folder.</p>\n", nFiles);
        ++nProblems;
        ++problemsThisCamera;
        }
      else
        {
        printf( "<p class=\"status-good in-camera\">There are %d images in the capture folder.</p>\n", nFiles);
        }

      FreeArrayOfStrings( folder, nFiles );
      }
    else
      {
      printf( "<p class=\"status-bad in-camera\">There are no images in the capture folder.</p>\n" );
      ++nProblems;
      ++problemsThisCamera;
      }

    CameraBackupFolder( config, cam );
    if( chdir( cam->backupFolderPath )==0 )
      {
      int nBackupImages = CountFilesInFolder( ".", NULL, ".jpg", NULL, NULL );
      if( nBackupImages>0 )
        {
        printf( "<p class=\"status-good in-camera\">There are %d images in the backup folder.</p>\n", nBackupImages);
        }
      else
        {
        printf( "<p class=\"status-bad in-camera\">There are no images in the backup folder.</p>\n" );
        ++nProblems;
        ++problemsThisCamera;
        }
      }
    else
      {
      printf( "<p class=\"status-bad in-camera\">Cannot chdir to %s</p>\n", cam->backupFolderPath );
      ++nProblems;
      }

    if( problemsThisCamera==0 )
      {
      printf( "<a href=\"#\" title=\"Click to zoom in to camera %s\" onclick=\"ZoomImage('%s');return false;\"><img class=\"float-right\" src=\"single-image?cam=%s&maxwidth=200\"/></a>\n",
              cam->nickName, cam->nickName, cam->nickName );
      }
    else
      {
      ++nCamerasWithProblems;
      }

    printf( "</div>\n");
    }

  char nProblemsStr[BUFLEN];
  snprintf( nProblemsStr, sizeof(nProblemsStr)-1, "%d", nProblems );

  char nProblemCamerasStr[BUFLEN];
  snprintf( nProblemCamerasStr, sizeof(nProblemCamerasStr)-1, "%d", nCamerasWithProblems );

  char pageFoot[BIGBUF];
  err = ExpandMacrosVA( templateFoot, pageFoot, sizeof(pageFoot)-1,
                        "PROBLEMS", nProblemsStr,
                        "BADCAMERAS", nProblemCamerasStr,
                        NULL, NULL );
  FREE( templateFoot );

  if( err<0 )
    {
    Error( "Cannot expand page footer template" );
    }
  fputs( pageFoot, stdout );

  return 0;
  }
