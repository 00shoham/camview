#!/bin/sh

# run this from its parent directory!

MYPATH=`echo $0 | sed 's/[a-zA-Z0-9\._-]*$//'`

. $MYPATH/local-variables.sh

if [ -z "$CVUSER" ]; then
  echo "CVUSER must be set"
  exit 1
fi

zz=`cat /etc/passwd | grep "$CVUSER"`
if [ -z "$zz" ]; then
  echo "$CVUSER does not exist" > /dev/null
else
  sudo /usr/sbin/deluser --remove-home "$CVUSER"
fi

