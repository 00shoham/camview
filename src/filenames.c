#include "base.h"

_FILENAME* NewFilename( char* name, _FILENAME* list )
  {
  if( EMPTY( name ) )
    return list;

  _FILENAME* f = (_FILENAME*)SafeCalloc( 1, sizeof( _FILENAME ), "_FILENAME" );
  f->name = SAFESTRDUP( name );
  f->next = list;
  return f;
  }

void FreeFilenames( _FILENAME* list )
  {
  while( list!=NULL )
    {
    _FILENAME* next = list->next;
    if( list->name != NULL )
      {
      free( list->name );
      list->name = NULL;
      }
    free( list );
    list = next;
    }
  }

void BackupFiles( char* parentFolder, _FILENAME* list, char* cmd )
  {
  char buf[BIGBUF];
  char cmdWithFilenames[2*BIGBUF];
  char* ptr = NULL;
  int spaceRemaining = sizeof(buf)-1;
  int err = 0;

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
  while( list!=NULL && spaceRemaining>0 )
    {
    if( NOTEMPTY( list->name ) )
      {
      int n = strlen( list->name );
      if( spaceRemaining > (n+2) )
        {
        strcpy( ptr, " " );
        ++ptr;
        strcpy( ptr, list->name );
        ptr += n;
        spaceRemaining -= (n + 1);
        *ptr = 0;
        }
      }
    list = list->next;
    }

  if( spaceRemaining<1 )
    {
    Warning( "Trying to dump too many filenames into the (backup) command buffer" );
    return;
    }

  /* expand buf into the command and perhaps run it */
  _TAG_VALUE* tv = NewTagValue( "FILES", buf, NULL, 0 );
  int nReplacements = ExpandMacros( cmd, cmdWithFilenames, sizeof( cmdWithFilenames )-1, tv );
  FreeTagValue( tv );

  if( nReplacements!=1 )
    {
    Warning( "Backup command did not include %%FILES%% - aborting." );
    return;
    }
  /* Notice("Command generation returned %d - will run [%s]", err, cmdWithFilenames ); */

  char oldLocation[BUFLEN];
  ptr = getcwd( oldLocation, sizeof( oldLocation )-1 );
  if( EMPTY( ptr ) )
    {
    Warning( "Backup command not run because could not getcwd() - %d:%s",
             errno, strerror( errno ) );
    return;
    }

  err = chdir( parentFolder );
  if( err )
    {
    Warning( "Backup command failed to chdir to [%s] - %d:%d:%s",
             parentFolder, err, errno, strerror( errno ) );
    return;
    }

#ifdef DEBUG
  Notice( "Backup using [%s] in [%s]", cmdWithFilenames, parentFolder );
#endif
  err = AsyncRunCommandNoIO( cmdWithFilenames );
  if( err )
    Warning( "AsyncRunCommandNoIO() --> %d", err );

  if( NOTEMPTY( ptr ) )
    {
    /* in parent - return to caller*/
    err = chdir( ptr );
    if( err )
      Warning( "Backup command failed to return to [%s] - %d:%d:%s",
               ptr, err, errno, strerror( errno ) );
    Notice( "Forked off backup in %s.", parentFolder );
    return;
    }
  }
