#ifndef _INCLUDE_CAMERA
#define _INCLUDE_CAMERA

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
