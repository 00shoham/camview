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

if [ -d "$RAMPATH" ]; then
  echo "good" > /dev/null
else
  sudo mkdir -p $RAMPATH
  sudo chown "$CVUSER:$CVGROUP" $RAMPATH
fi

FSLINE="tmpfs $RAMPATH tmpfs rw,size=1G,uid=$CVUSER,gid=$CVGROUP 0 0"
zz=`cat /etc/fstab | grep "$RAMPATH"`
if [ -z "$zz" ]; then
  echo "Adding $FSLINE"
  sudo sh -c "echo \"$FSLINE\" >> /etc/fstab"
  sudo mount "$RAMPATH"
fi

if [ -d "$SECPATH" ]; then
  echo "good" > /dev/null
else
  sudo mkdir -p "$SECPATH"
  sudo chown "$CVUSER:$CVGROUP" "$SECPATH"
fi

