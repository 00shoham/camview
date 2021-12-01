#ifndef _INCLUDE_CAMERA
#define _INCLUDE_CAMERA

typedef struct _filename _FILENAME;

typedef struct _NewImageData _NEWIMAGEDATA;

typedef struct _camera
  {
  char* nickName;
  char* ipAddress;
  char* captureCommand;
  char* folderPath;
  char* backupFolderPath;
  char* lastStoredImage;
  int launchAttempts;
  pid_t childProcess;
  time_t launchTime;
  char* lastImageSourceName;
  int lastImageCount;
  time_t lastImageTime;
  int largestFile;
  int motionCountdown;
  struct _camera *next;
  int minimumIntervalSeconds;
  int storePreMotion;
  int motionFrames;
  _IMAGE* recentImage;
  int haveMotionDetectThread;
  pthread_t motionDetectThread;
  int debug;

  /* brighten the image first, if it's too dark */
  int color_dark;
  double dark_brightness_boost;

  /* how far apart must colors be to count as a diff? */
  int color_diff_threshold;

  /* used to remove white dots on a non-white background
     and black dots on a non-black background */
  int despeckle_dark_threshold;
  int despeckle_nondark_min;
  int despeckle_bright_threshold;
  int despeckle_nonbright_max;

  /* reduce the diff grayscale into blocks of white/black */
  int checkerboard_square_size;
  int checkerboard_min_white;
  int checkerboard_num_white;

  /* what percentage of the blocks must be 'lit up' to count as motion? */
  double checkerboard_percent;

  /* prevent thread problems */
  pthread_mutex_t lock;
  } _CAMERA;

void SetDefaultsSingleCamera( _CONFIG* config, _CAMERA* cam );
void FreeCamera( _CAMERA* cam );
pid_t LaunchCapture( _CONFIG* config, _CAMERA* cam );
int LaunchAllCameras( _CONFIG* config );
void MonitorCameras( _CONFIG* config );
void TerminateMonitor( int signo );
void PingCameras( int signo );
void CameraFolder( _CONFIG* config, _CAMERA* cam );
void CameraBackupFolder( _CONFIG* config, _CAMERA* cam );
_CAMERA* FindCamera( _CONFIG* conf, char* id );
int FindMatchingImages( _CAMERA* cam,
                        char* fromDate, char* fromTime,
                        char* toDate, char* toTime,
                        char*** fileNames );

#endif
