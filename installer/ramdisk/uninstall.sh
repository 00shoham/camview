#!/bin/sh

# run this from its parent directory!

MYPATH=`echo $0 | sed 's/[a-zA-Z0-9\._-]*$//'`

. $MYPATH/local-variables.sh

if [ -z "$RAMPATH" ]; then
  echo "RAMPATH must be set"
  exit 1
fi
if [ -z "$SECPATH" ]; then
  echo "SECPATH must be set"
  exit 2
fi
if [ -z "$CVUSER" ]; then
  echo "CVUSER must be set"
  exit 3
fi
if [ -z "$CVGROUP" ]; then
  echo "CVGROUP must be set"
  exit 4
fi

sudo killall -1 ffmpeg
sudo killall -9 cam-monitor

zz=`df -h "$RAMPATH"` 2> /dev/null
if [ -z "$zz" ] ; then
  echo "not there" > /dev/null
else
  sudo umount "$RAMPATH"
fi

if [ -d "$RAMPATH" ]; then
  sudo rmdir "$RAMPATH"
fi

zz=`cat /etc/fstab | grep "$RAMPATH"`
if [ -z "$zz" ]; then
  echo "already gone" > /dev/null
else
  sudo sh -c "cat /etc/fstab | grep -v \"$RAMPATH\" > /etc/fstab.bak && mv /etc/fstab.bak /etc/fstab"
fi

