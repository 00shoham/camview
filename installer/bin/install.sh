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
  binSrc=../src/$bin
  if [ -f $binSrc ]; then
    echo 'we have something ready to install' > /dev/null
  else
    echo "$binSrc not found.  Aborting."
    exit 3
  fi

  if [ -f $binTarget ]; then
    echo "$binSrc is already in $binTarget" > /dev/null
    zz=`diff "$binSrc" "$binTarget"`
    if [ -z "$zz" ]; then
      echo "$binTarget already up to date" > /dev/null
    else
      sudo install -o root -g root -m 755 "$binSrc" "$binTarget"
    fi
  else
    sudo install -o root -g root -m 755 "$binSrc" "$binTarget"
  fi
done

