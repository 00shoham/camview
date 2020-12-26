#!/bin/sh

# run this from its parent directory!

MYPATH=`echo $0 | sed 's/[a-zA-Z0-9\._-]*$//'`

. $MYPATH/local-variables.sh

if [ -z "$BINDIR" ]; then
  echo "BINDIR must be set"
  exit 1
fi
if [ -z "$SCRIPTS" ]; then
  echo "SCRIPTS must be set"
  exit 2
fi

for scr in $SCRIPTS; do
  txt=$scr.txt
  sh=$BINDIR/$scr.sh

  if [ -f "$sh" ]; then
    sudo rm -f "$sh"
  fi
done

zz=`cat /etc/passwd | grep "$CVUSER"`
if [ -z "$zz" ] ; then
  echo 'no such user so no such crontab' > /dev/null
else
  sudo crontab -u "$CVUSER" -r
fi
