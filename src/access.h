#ifndef _INCLUDE_ACCESS
#define _INCLUDE_ACCESS

typedef struct _member
  {
  char* id;
  struct _member* next;
  } _MEMBER;

typedef struct _group
  {
  char* id;
  _MEMBER* members;
  struct _group* next;
  } _GROUP;

typedef struct _groupPointer
  {
  _GROUP* group;
  struct _groupPointer* next;
  } _GROUP_POINTER;

_GROUP* NewGroup( char* id, _GROUP* list );
_GROUP* FindGroup( char* id, _GROUP* list );
void FreeGroups( _GROUP* list );

_MEMBER* NewMember( char* id, _MEMBER* list );
_MEMBER* FindMember( char* id, _MEMBER* list );
void FreeMembers( _MEMBER* list );

_GROUP_POINTER* NewGroupPointer( char* id, _GROUP* allGroups, _GROUP_POINTER* list );
int IsUserInGroups( char* id, _GROUP_POINTER* groups );
void FreeGroupPointers( _GROUP_POINTER* list );

#endif
