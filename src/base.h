#ifndef _INCLUDE_BASE
#define _INCLUDE_BASE

#define _GNU_SOURCE

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <time.h>
#include <sys/time.h>
#include <regex.h>
#include <ctype.h>
#include <locale.h>
#include <pwd.h>
#include <grp.h>
#include <signal.h>
#include <sys/stat.h>
#include <syslog.h>
#include <errno.h>
#include <jpeglib.h>
#include <math.h>
#include <pthread.h>
#include <execinfo.h>

#define LOG_FILES_TO_KEEP 10

#define INVALID_INT -999999
#define INVALID_DOUBLE -99999.0

enum callMethod { cm_invalid, cm_ui, cm_api };

#define DEFAULT_USER_ENV_VAR "REMOTE_USER"

void Error( char* fmt, ... );
void Warning( char* fmt, ... );
void Notice( char* fmt, ... );
void SegFaultHandler( int signo );

#define HEXDIGITS "0123456789abcdefABCDEF"

#define MAX_LAUNCH_ATTEMPTS 3

#define BIGBUF 500000
#define BUFLEN 2000
#define CONFIGDIR "."
#define CONFIGNAME "config.ini"
#define ALTCONFIGNAME "/usr/local/etc/cam-view/config.ini"
#define CONFIGLOCAL "config.local"
#define DF_COMMAND "/bin/df %PATH% | /bin/grep -v '^Filesystem' | /usr/bin/awk '{print $4}'"

#define RE_EMAIL_ADDRESS "[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\\.[A-Za-z]{2,4}"
#define RE_EMAIL_DOMAIN "[A-Za-z0-9.-]+\\.[A-Za-z]{2,4}"
#define RE_INTEGER "[-]?[0-9]+"
#define RE_IDENTIFIER "[0-9a-zA-Z_.-]+"
#define RE_WORDS "[a-zA-Z0-9 ]+"
#define RE_SENTENCE "[a-zA-Z0-9 ,.?:/()-]+"
#define RE_TEXTBLOCK "[a-zA-Z0-9 ,.?:/=<>()\n-]+"
#define RE_IPADDR "[0-9][0-9]?[0-9]?\\.[0-9][0-9]?[0-9]?\\.[0-9][0-9]?[0-9]?\\.[0-9][0-9]?[0-9]?"
#define RE_DATE "[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]"
#define RE_TIME "[0-9][0-9]:[0-9][0-9]:[0-9][0-9]"
#define RE_TIME_NOSEC "[0-9][0-9]:[0-9][0-9]"

#define VALIDFILECHARS "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ-_."
#define VALIDPATHCHARS "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ-_./"
#define REPLACEMENTCHAR '-'
#define SETTLING_TIME 90 /* max seconds between images */
#define IMAGE_TOO_OLD 60*5 /* we keep getting the same file */
#define MINIMUM_CAPTURE_INTERVAL 5*60 /* seconds between mandatory image captures */
#define STORE_PRE_MOTION 0 /* do we store the frame before we detected motion? */
#define MOTION_FRAMES 1 /* frames to capture after detecting motion */
#define TOO_MANY_FILES 100 /* when to cleanup */
#define FILES_TO_KEEP 50 /* how many files to keep */
#define IMAGE_DIFF_THRESHOLD 2500 /* the sensitivity knob for motion capture */
#define TINY_IMAGE 10000 /* bytes  - presumably a bad file */
#define TOO_SMALL_THRESHOLD 0.40 /* images this much smaller than last, largest are discarded */
#define SECONDS_PER_DAY 24 * 60 * 60
#define FILE_CACHE_LENGTH 10

#define DEFAULT_COLOR_DIFF_THRESHOLD 40           /* out of 255 */
#define DEFAULT_COLOR_DARK 50                     /* out of 255+255+255 (RGB) */
#define DEFAULT_DARK_BRIGHTNESS_BOOST 1.5         /* if image is dark, color=exp(boost*log(color)) */
#define DEFAULT_DESPECKLE_DARK_THRESHOLD    30    /* when removing spots, R+G+B<this = dark */
#define DEFAULT_DESPECKLE_NONDARK_MIN       200   /* and R+G+B>=this = not dark */
#define DEFAULT_DESPECKLE_BRIGHT_THRESHOLD  140   /* and R+G+B>=this = bright */
#define DEFAULT_DESPECKLE_NONBRIGHT_MAX     60    /* while R+G+B<=this = not bright */
#define DEFAULT_CHECKERBOARD_SQUARE_SIZE    8     /* reduce diff image to squares this many pixels across */
#define DEFAULT_CHECKERBOARD_MIN_WHITE      100   /* out of 255 - this constitutes "white" in the diff grayscale */
#define DEFAULT_CHECKERBOARD_NUM_WHITE      33    /* out of 8x8=64 pixels in the diff grayscale must be 'white' for the checkerboard to be white */
#define DEFAULT_CHECKERBOARD_PERCENT        0.02  /* fraction of checkerboard that should be lit up to indicate motion*/

#define JPEG_WRITE_IMAGEQUALITY 90

#define MAX_WIDTH  10000 /* for image scaling */
#define MAX_HEIGHT 10000 /* for image scaling */

#define BLACK_IMAGE_MAX_LUMINOSITY 5

#define DEFAULT_HUP_INTERVAL   10*60  /* send main process HUP this often */

#define CAMVIEW_TITLE "Camera Preview"
#define CAMVIEW_TEMPLATE "cam-view.template"

#define DOWNLOAD_TITLE "Download captured images"
#define DOWNLOAD_TEMPLATE "download.template"

#define DEFAULT_CONF_NAME "Recent Images"
#define OLDEST_DOWNLOAD 14 /* days old */

#define FILENAME_DOWNLOADS "downloads.txt"
#define DOWNLOAD_EXPIRY_SECONDS 5*60*60

#define MAX_BACK 20
#define MAX_FORWARD 20

#include "util.h"
#include "nargv.h"
#include "image.h"
#include "camera.h"
#include "config.h"
#include "daemon.h"
#include "ping.h"
#include "cgi.h"
#include "filenames.h"
#include "jpeg.h"

#endif
