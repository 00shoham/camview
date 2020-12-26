#!/bin/sh

.  ./global-variables.sh

while getopts c:h flag; do
  case "${flag}" in
    c) CONFDIR=${OPTARG};;
    h) echo "USAGE: $0 [-c confdir]"; exit 0;;
  esac
done

if [ -z "$CONFDIR" ]; then
  echo "You must specify a CONFDIR in global-variables.sh or via -c argument"
  exit 1
fi

if [ -d "$CONFDIR" ]; then
  echo "ok" > /dev/null
else
  echo "$CONFDIR must actually exist..." > /dev/null
  exit 2
fi

# Stop everything first:
sudo su -c '/usr/local/bin/cam-view-stop.sh' - "$CVUSER"

for dir in $UNINSTDIRS; do
  inst=$dir/uninstall.sh
  if [ -f "$inst" ]; then
    $inst
    if [ $? -eq 0 ] ; then
      echo 'that worked' > /dev/null
    else
      echo "$inst returned an error code - $?"
    fi
  fi
done
