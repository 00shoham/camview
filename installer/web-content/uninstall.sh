#!/bin/sh

# run this from its parent directory!

MYPATH=`echo $0 | sed 's/[a-zA-Z0-9\._-]*$//'`

. $MYPATH/local-variables.sh

if [ -z "$WEBDIR" ]; then
  echo "WEBDIR must be set"
  exit 6
fi
if [ -z "$CGIDIR" ]; then
  echo "CGIDIR must be set"
  exit 7
fi
if [ -z "$WEBLOG" ]; then
  echo "WEBLOG must be set"
  exit 8
fi

if [ -f "$WEBLOG" ] ; then
  sudo /bin/rm -f "$WEBLOG"
fi

if [ -d "$WEBDIR" ] ; then
  sudo /bin/rm -rf "$WEBDIR"
fi

if [ -d "$CGIDIR" ] ; then
  sudo /bin/rm -rf "$CGIDIR"
fi

