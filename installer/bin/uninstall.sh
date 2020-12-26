#!/bin/sh

# run this from its parent directory!

MYPATH=`echo $0 | sed 's/[a-zA-Z0-9\._-]*$//'`

. $MYPATH/local-variables.sh

if [ -z "$BINDIR" ]; then
  echo "BINDIR must be set"
  exit 1
fi
if [ -z "$BINARIES" ]; then
  echo "BINARIES must be set"
  exit 2
fi

for bin in $BINARIES; do
  binTarget=$BINDIR/$bin
  if [ -f "$binTarget" ]; then
    sudo rm -f "$binTarget"
  else
    echo "$binTarget is not actually there." > /dev/null
  fi
done
