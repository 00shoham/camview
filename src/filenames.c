#include "base.h"

_FILENAME* NewFilename( char* name, _FILENAME* list )
  {
  _FILENAME* f = (_FILENAME*)malloc( sizeof( _FILENAME ) );
  f->name = strdup( name );
  f->next = list;
  return f;
  }

void FreeFilenames( _FILENAME* list )
  {
  while( list!=NULL )
    {
    _FILENAME* next = list->next;
    FREEIFNOTNULL( list->name );
    FREEIFNOTNULL( list );
    list = next;
    }
  }

void BackupFiles( char* parentFolder, _FILENAME* list, char* cmd )
  {
  char buf[BIGBUF];
  char expandedbuf[2*BIGBUF];
  char* ptr;
  int size = sizeof(buf)-1;

  if( EMPTY( parentFolder ) )
    {
    Warning( "Cannot backup files without specifying a parent folder");
    return;
    }

  if( list==NULL )
    { /* nothing to backup */
    return;
    }

  if( EMPTY( cmd ) )
    {
    Warning( "Cannot backup files without specifying a backup command");
    return;
    }

  ptr = buf;
  while( list && size>0 )
    {
    snprintf( ptr, size, " %s", list->name );
    int n = strlen( ptr );
    size -= n;
    ptr += n;
    list = list->next;
    }

  if( size<=0 )
    {
    Warning("Trying to dump too many filenames into the (backup) command buffer");
    return;
    }

  /* expand buf into the command and perhaps run it */
  _TAG_VALUE* tv = NewTagValue( "FILES", buf, NULL, 0 );
  int nReplacements = ExpandMacros( cmd, expandedbuf, sizeof( expandedbuf)-1, tv );
  FreeTagValue( tv );

  if( nReplacements!=1 )
    {
    Warning("Backup command did not include %%FILES%% - aborting.");
    return;
    }
  /* Notice("Command generation returned %d - will run [%s]", err, expandedbuf ); */

  pid_t childProc = fork();
  if( childProc<0 )
    {
    Warning( "No backup because cannot fork(%d) - %d/%s", (int)childProc,
             errno, strerror(errno) );
    return;
    }

  if( childProc==0 )
    {
    /* in child */
    if( chdir( parentFolder )!=0 )
      {
      Warning( "No backup because cannot chdir(%s) - %d:%s ", parentFolder, errno, strerror( errno ) );
      exit( 0 );
      }
    else
      {
      /* Notice( "Changed directory to %s prior to backup", parentFolder ); */
      }

    int err = 0;
    if( (err=system( expandedbuf ))<0 )
      {
      Error( "Tried to run backup command [%s] - failed with error %d - %d - %s",
             expandedbuf, err, errno, strerror( errno ) );
      }

    /* in case the OS needs some help cleaning up.  probably useless.
       definitely harmless as it's in the child process. */
    sleep(1);

    /* this function was the sole purpose of the child process, so exit. */
    exit(0);
    }
  else
    {
    /* in parent - return to caller*/
    return;
    }
  }
