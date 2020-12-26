#!/bin/bash

MYID=""
WHOAMI=`/sbin/ifconfig | /bin/grep 'inet 192.168.1.100'`
if [ -z "$WHOAMI" ] ; then
  echo "Not Canmore" > /dev/null
else
  export MYID=canmore
fi

WHOAMI=`/sbin/ifconfig | /bin/grep 'inet 10.1.0.101'`
if [ -z "$WHOAMI" ] ; then
  echo "Not Calgary" > /dev/null
else
  export MYID=calgary
fi

WHOAMI=`/sbin/ifconfig | /bin/grep 'inet 10.200.0.1'`
if [ -z "$WHOAMI" ] ; then
  echo "Not Office" > /dev/null
else
  export MYID="mtp hitachi effectiv"
fi

if [ -z "$MYID" ]; then
  echo "I don't know where I'm running."
  exit 1
fi

for f in $MYID; do
  echo "$f"
done
