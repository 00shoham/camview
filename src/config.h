#ifndef _INCLUDE_CONFIG
#define _INCLUDE_CONFIG

typedef struct _config
  {
  int currentlyParsing;
  _TAG_VALUE* parserLocation;

  char* configName;

  _GROUP* groups;
  int nCameras;
  _CAMERA* cameras;
  char* baseDir;
  char* backupDir;
  char* downloadDir;
  char* logFile;
  char* cgiLogFile;
  FILE* logFileHandle;
  int minimumIntervalSeconds;
  int storePreMotion;
  int motionFrames;
  int fileCacheLength;
  int nFilesToBackup;
  _FILENAME* filesToBackup;
  char* backupCommand;

  /* image diff algorithm parameters */

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

  /* health checking - HUP main process this often: */
  int hup_interval;

  /* used in the parse process - variables that have been defined*/
  _TAG_VALUE *list;

  /* to avoid duplicate includes */
  _TAG_VALUE *includes;

  /* used in installers and diagnostics */
  int listIncludes;
  int includeCounter;
  } _CONFIG;

void SetDefaults( _CONFIG* config );
void ReadConfig( _CONFIG* config, char* filePath );
void PrintConfig( FILE* f, _CONFIG* config );
void FreeConfig( _CONFIG* config );
int ValidCamera( _CONFIG* config, char* name );
int CountCameras( _CONFIG* conf );
void PrintVariable( _CONFIG* config, char* varName );
void ValidateConfig( _CONFIG* config );

#endif
