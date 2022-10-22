#include "base.h"

_GROUP* NewGroup( char* id, _GROUP* list )
  {
  if( EMPTY( id ) )
    return NULL;

  _GROUP* newg = (_GROUP*)SafeCalloc( 1, sizeof(_GROUP), "GROUP" );
  newg->id = strdup( id );
  newg->next = list;
  return newg;
  }

_GROUP* FindGroup( char* id, _GROUP* list )
  {
  if( EMPTY( id ) )
    return NULL;
  if( list==NULL )
    return NULL;
  for( _GROUP* g = list; g!=NULL; g=g->next )
    if( NOTEMPTY( g->id ) && strcmp( id, g->id )==0 )
      return g;
  return NULL;
  }

void FreeGroups( _GROUP* group )
  {
  if( group==NULL )
    return;
  if( group->next!=NULL )
    FreeGroups( group->next );
  if( NOTEMPTY( group->id ) )
    free( group->id );
  if( group->members!=NULL )
    FreeMembers( group->members );
  free( group );
  }

_MEMBER* NewMember( char* id, _MEMBER* list )
  {
  if( EMPTY( id ) )
    return NULL;

  _MEMBER* newm = (_MEMBER*)SafeCalloc( 1, sizeof(_MEMBER), "MEMBER" );
  newm->id = strdup( id );
  newm->next = list;
  return newm;
  }

_MEMBER* FindMember( char* id, _MEMBER* list )
  {
  if( EMPTY( id ) )
    return NULL;
  if( list == NULL )
    return NULL;
  for( _MEMBER* m = list; m!=NULL; m=m->next )
    if( NOTEMPTY( m->id ) && strcmp( id, m->id )==0 )
      return m;
  return NULL;
  }
  
void FreeMembers( _MEMBER* member )
  {
  if( member==NULL )
    return;
  if( member->next!=NULL )
    FreeMembers( member->next );
  if( NOTEMPTY( member->id ) )
    free( member->id );
  free( member );
  }

_GROUP_POINTER* NewGroupPointer( char* id, _GROUP* allGroups, _GROUP_POINTER* list )
  {
  if( EMPTY( id ) )
    return NULL;
  if( allGroups == NULL )
    return NULL;

  _GROUP* group = FindGroup( id, allGroups );
  if( group==NULL )
    return NULL;

  _GROUP_POINTER* newPtr = (_GROUP_POINTER*)SafeCalloc( 1, sizeof(_GROUP_POINTER), "GROUP_POINTER" );
  newPtr->group = group;
  newPtr->next = list;
  return newPtr;
  }

int IsUserInGroups( char* id, _GROUP_POINTER* groups )
  {
  if( EMPTY( id ) )
    return -1;

  for( _GROUP_POINTER* g = groups; g!=NULL; g=g->next )
    {
    if( g->group!=NULL
        && g->group->members!=NULL
        && FindMember( id, g->group->members )!=NULL )
      return 0;
    }

  return -2;
  }

void FreeGroupPointers( _GROUP_POINTER* list )
  {
  if( list==NULL )
    return;
  if( list->next!=NULL )
    FreeGroupPointers( list->next );
  free( list );
  }

