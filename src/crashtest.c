#include "base.h"

void SetupSignalHandler()
  {
  (void)signal( SIGSEGV, SegFaultHandler );
  }

void bar()
  {
  char* ptr = NULL;
  *ptr = 15;
  }

void foo()
  {
  bar();
  }

int main( int argc, char** argv )
  {
  SetupSignalHandler();

  foo();

  return 0;
  }
